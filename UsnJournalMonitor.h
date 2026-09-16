#include <QHash>
#include <QThread>
#include <QString>
#include <QFileInfo>
#include <windows.h>
#include <winioctl.h>


class UsnJournalMonitor : public QThread
{
    Q_OBJECT

public:
    explicit UsnJournalMonitor(QObject *parent = nullptr): QThread(parent) 
    {
        wchar_t volumeGuid[MAX_PATH]{};
        if (!GetVolumeNameForVolumeMountPointW
            (
                L"C:\\",
                volumeGuid,
                MAX_PATH
            )) 
        {
            qDebug() << "ERROR: get guid failed";
            return;
        }
        qDebug() << "GUID тома:" << QString(volumeGuid);

        QSqlDatabase db = QSqlDatabase::database("app_connection");
        if (!db.isOpen()) 
        {
            qWarning() << "ERROR: database is not open";
            return;
        }

        QSqlQuery query(db);
        query.prepare("SELECT path FROM file");
        if (!query.exec()) return;

        while(query.next())
        {
            QString q_path = query.value(0).toString();
            this->addFile(q_path);

            struct FileIdentity
            {
                ULONGLONG volumeSerialNumber;
                FILE_ID_128 fileId;
            };
    
            HANDLE hFile = CreateFileW
            (
                reinterpret_cast<LPCWSTR>(q_path.utf16()),
                0,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                nullptr,
                OPEN_EXISTING,
                0,
                nullptr
            );

            if (hFile == INVALID_HANDLE_VALUE) 
            {
                qDebug() << "ERROR: CreateFileW failed";
                return;
            }

            FILE_ID_INFO info{};
            BOOL ok = GetFileInformationByHandleEx
            (
                hFile,
                FileIdInfo,
                &info,
                sizeof(info)
            );

            DWORD error = GetLastError();
            CloseHandle(hFile);

            if (!ok) 
            {
                SetLastError(error);
                qDebug() << "ERROR: GetFileInformationByHandleEx failed";
                return;
            }

            QByteArray idBytes
            (
                reinterpret_cast<const char*>(info.FileId.Identifier),
                sizeof(info.FileId.Identifier)
            );

            qDebug() << "File ID:" << idBytes.toHex();

            QSqlQuery query(db);
            query.prepare("UPDATE file SET system_id = :sid WHERE path = :path");
            query.bindValue(":sid", idBytes.toHex());
            query.bindValue(":path", q_path);
            if (!query.exec()) 
                qWarning() << "ERROR: update file system_id failed:" << query.lastError().text();
        
            int volume_id = 0;
            query.prepare("SELECT id FROM volume WHERE guid = :volume_guid");
            query.bindValue(":volume_guid", QString(volumeGuid));
            if (query.next()) volume_id = query.value(0).toInt();

            if (volume_id != 0)
            {
                query.prepare("UPDATE file SET volume_id = :volume_id WHERE path = :path");
                query.bindValue(":volume_id", volume_id);
                query.bindValue(":path", q_path);
                if (!query.exec()) 
                    qWarning() << "ERROR: update file volume guid failed:" << query.lastError().text();
            }
        }
    }

    ~UsnJournalMonitor() override
    {
        requestInterruption();
        wait();

        for (auto &file : files) 
        {
            if (file.handle != INVALID_HANDLE_VALUE)
                CloseHandle(file.handle);
        }
    }

    bool addFile(const QString &path)
    {
        HANDLE handle = CreateFileW(
            reinterpret_cast<LPCWSTR>(path.utf16()),
            FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ |
            FILE_SHARE_WRITE |
            FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            nullptr
        );

        if (handle == INVALID_HANDLE_VALUE)
            return false;

        BY_HANDLE_FILE_INFORMATION info{};

        if (!GetFileInformationByHandle(handle, &info)) {
            CloseHandle(handle);
            return false;
        }

        quint64 fileId =
            (static_cast<quint64>(info.nFileIndexHigh) << 32) |
             static_cast<quint64>(info.nFileIndexLow);

        WatchedFile file;
        file.id = fileId;
        file.handle = handle;
        file.path = QFileInfo(path).absoluteFilePath();

        files.insert(fileId, file);
        return true;
    }

signals:
    void fileMoved
    (
        const QString &oldPath,
        const QString &newPath,
        quint64 fileId
    );

    void fileDeleted
    (
        const QString &path,
        quint64 fileId
    );

    void journalPositionChanged
    (
        quint64 journalId,
        qint64 lastUsn
    );

protected:
    void run() override
    {
        HANDLE volume = CreateFileW
        (
            L"\\\\.\\C:",
            GENERIC_READ,
            FILE_SHARE_READ |
            FILE_SHARE_WRITE |
            FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

        if (volume == INVALID_HANDLE_VALUE) 
        {
            qDebug() << "ERROR: CreateFile volume failed" << GetLastError();
            return;
        }

        USN_JOURNAL_DATA journal{};
        DWORD bytesReturned = 0;

        if (!DeviceIoControl(
                volume,
                FSCTL_QUERY_USN_JOURNAL,
                nullptr,
                0,
                &journal,
                sizeof(journal),
                &bytesReturned,
                nullptr
            )) 
        {
            qDebug() << "ERROR: FSCTL_QUERY_USN_JOURNAL failed " << GetLastError();
            CloseHandle(volume);
            return;
        }

        qDebug() << "USN journal found:"
            << "FirstUsn =" << journal.FirstUsn
            << "NextUsn =" << journal.NextUsn
            <<"JournalId =" << journal.UsnJournalID;

        READ_USN_JOURNAL_DATA readData{};

        readData.StartUsn = journal.NextUsn;

        readData.ReasonMask =
            USN_REASON_RENAME_OLD_NAME |
            USN_REASON_RENAME_NEW_NAME |
            USN_REASON_FILE_DELETE;
    // USN_REASON_DATA_EXTEND |
    // USN_REASON_DATA_OVERWRITE |
    // USN_REASON_DATA_TRUNCATION |
    // USN_REASON_BASIC_INFO_CHANGE |
    // USN_REASON_SECURITY_CHANGE |
    // USN_REASON_CLOSE |
    // USN_REASON_FILE_CREATE;

        readData.ReturnOnlyOnClose = FALSE;
        readData.Timeout = 1000;
        readData.BytesToWaitFor = 1;
        readData.UsnJournalID = journal.UsnJournalID;

        QByteArray buffer(1024 * 1024, Qt::Uninitialized);

        while (!isInterruptionRequested()) 
        {
            DWORD bytesRead = 0;
            BOOL ok = DeviceIoControl
            (
                volume,
                FSCTL_READ_USN_JOURNAL,
                &readData,
                sizeof(readData),
                buffer.data(),
                static_cast<DWORD>(buffer.size()),
                &bytesRead,
                nullptr
            );

            if (!ok) 
            {
                DWORD error = GetLastError();

                if (error == ERROR_HANDLE_EOF) 
                {
                    msleep(100);
                    continue;
                }
                break;
            }
            if (bytesRead < sizeof(USN)) continue;

            USN nextUsn = *reinterpret_cast<const USN *>(buffer.constData());
            readData.StartUsn = nextUsn;

            DWORD offset = sizeof(USN);

            while (offset + sizeof(USN_RECORD) <= bytesRead) 
            {
                auto *record = reinterpret_cast<const USN_RECORD *>(buffer.constData() + offset);
                if (record->RecordLength == 0) break;
                processRecord(record);
                offset += record->RecordLength;
            }
        }
        CloseHandle(volume);
    }

private:
    struct WatchedFile
    {
        quint64 id = 0;
        HANDLE handle = INVALID_HANDLE_VALUE;
        QString path;
    };

    struct PendingRename
    {
        QString oldName;
        quint64 oldParentId = 0;
    };

    QString volumeName;
    quint64 savedJournalId = 0;
    USN savedLastUsn = 0;

    QHash<quint64, WatchedFile> files;
    QHash<quint64, PendingRename> pendingRenames;

    QString getPathByHandle(HANDLE handle)
    {
        DWORD length = GetFinalPathNameByHandleW
        (
            handle,
            nullptr,
            0,
            FILE_NAME_NORMALIZED | VOLUME_NAME_DOS
        );

        if (length == 0)
            return {};

        std::wstring buffer(length + 1, L'\0');

        DWORD result = GetFinalPathNameByHandleW
        (
            handle,
            buffer.data(),
            static_cast<DWORD>(buffer.size()),
            FILE_NAME_NORMALIZED | VOLUME_NAME_DOS
        );

        if (result == 0) return {};

        return QString::fromWCharArray
        (
            buffer.data(),
            static_cast<int>(result)
        );
    }

    void processRecord(const USN_RECORD *record)
    {
        const quint64 fileId = static_cast<quint64>(record->FileReferenceNumber);

        auto fileIt = files.find(fileId);

        if (fileIt == files.end()) return;

        const QString name = QString::fromWCharArray
        (
            record->FileName,
            record->FileNameLength / sizeof(WCHAR)
        );

        qDebug() << "USN record:"
            << "fileId =" << fileId
            << "reason =" << Qt::hex << record->Reason
            << "name =" << name;

        if (record->Reason & USN_REASON_RENAME_OLD_NAME) 
        {
            const QString name = QString::fromWCharArray
            (
                record->FileName,
                record->FileNameLength / sizeof(WCHAR)
            );

            PendingRename rename;
            rename.oldName = name;
            rename.oldParentId =
            static_cast<quint64>(record->ParentFileReferenceNumber);

            pendingRenames.insert(fileId, rename);
            return;
        }


        if (record->Reason & USN_REASON_RENAME_NEW_NAME) 
        {
            auto renameIt = pendingRenames.find(fileId);

            if (renameIt == pendingRenames.end()) 
            {
                qDebug() << "ERROR: RENAME_NEW_NAME without old name";
                return;
            }   

            const QString oldPath = fileIt->path;
            const QString newPath = getPathByHandle(fileIt->handle);

            if (!newPath.isEmpty()) 
            {
                emit fileMoved
                (
                    oldPath,
                    newPath,
                    fileId
                );
                fileIt->path = newPath;
            }
            pendingRenames.erase(renameIt);

            return;
        }
        
        if (record->Reason & USN_REASON_FILE_DELETE) 
        {
            const QString path = fileIt->path;

            emit fileDeleted(path, fileId);

            files.erase(fileIt);
            pendingRenames.remove(fileId);
        }
    }
};