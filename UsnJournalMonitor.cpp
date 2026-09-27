#include "UsnJournalMonitor.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QFileInfo>
#include <QDebug>
#include <cstring>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>

QByteArray UsnJournalMonitor::fileId128ToBytes(const FILE_ID_128 &id)
{
    return QByteArray(reinterpret_cast<const char *>(id.Identifier), sizeof(id.Identifier));
}

QByteArray UsnJournalMonitor::fileId128ToBytes(DWORDLONG fileReference)
{
    FILE_ID_128 id{};

    memcpy(
        id.Identifier,
        &fileReference,
        sizeof(fileReference)
    );

    return QByteArray(
        reinterpret_cast<const char*>(id.Identifier),
        sizeof(id.Identifier)
    );
}

FILE_ID_128 UsnJournalMonitor::bytesToFileId128(const QByteArray &bytes)
{
    FILE_ID_128 id{};
    if (bytes.size() == static_cast<int>(sizeof(id.Identifier)))
        memcpy(id.Identifier, bytes.constData(), sizeof(id.Identifier));
    return id;
}

QByteArray UsnJournalMonitor::currentSystemIdForPath(const QString &path)
{
    HANDLE h = CreateFileW(
        reinterpret_cast<LPCWSTR>(path.utf16()),
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, 
        OPEN_EXISTING, 
        FILE_FLAG_BACKUP_SEMANTICS, 
        nullptr
    );

    if (h == INVALID_HANDLE_VALUE) return {};

    FILE_ID_INFO info{};
    BOOL ok = GetFileInformationByHandleEx(h, FileIdInfo, &info, sizeof(info));
    CloseHandle(h);
    if (!ok) return {};

    return fileId128ToBytes(info.FileId);
}

QString UsnJournalMonitor::getPathByHandle(HANDLE handle)
{
    DWORD length = GetFinalPathNameByHandleW
    (
        handle, 
        nullptr, 
        0,
        FILE_NAME_NORMALIZED | VOLUME_NAME_DOS
    );
    if (length == 0) return {};

    std::wstring buffer(length + 1, L'\0');
    DWORD result = GetFinalPathNameByHandleW
    (
        handle, 
        buffer.data(),
        static_cast<DWORD>(buffer.size()),
        FILE_NAME_NORMALIZED | VOLUME_NAME_DOS
    );
    if (result == 0) return {};

    return QString::fromWCharArray(buffer.data(), static_cast<int>(result));
}

HANDLE UsnJournalMonitor::openVolumeHandle(const QString &guidPath)
{
    QString path = guidPath.trimmed();
    if (path.startsWith('"') && path.endsWith('"'))
        path = path.mid(1, path.size() - 2);

    while (path.endsWith('\\'))
        path.chop(1);

    qDebug().noquote() << "-> Opening volume:" << path;

    return CreateFileW(
        reinterpret_cast<LPCWSTR>(path.utf16()),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, 0, nullptr);
}

UsnJournalMonitor::UsnJournalMonitor(QObject *parent) : QThread(parent)
{
    loadVolumes();
    reconcileStartup();
}

UsnJournalMonitor::~UsnJournalMonitor()
{
    requestInterruption();
    wait();

    for (auto &wf : watched_)
        if (wf.handle != INVALID_HANDLE_VALUE) CloseHandle(wf.handle);

    for (auto &vol : volumes_)
        if (vol.handle != INVALID_HANDLE_VALUE)
        {
            USN_JOURNAL_DATA journal{};
            DWORD br = 0;
            if (DeviceIoControl
            (
                vol.handle, 
                FSCTL_QUERY_USN_JOURNAL, 
                nullptr, 
                0,
                &journal,
                sizeof(journal),
                &br, 
                nullptr
            ))
            vol.lastUsn = journal.NextUsn;
        }

    saveJournalState();

    for (auto &vol : volumes_)
        if (vol.handle != INVALID_HANDLE_VALUE) CloseHandle(vol.handle);
}

void UsnJournalMonitor::loadVolumes()
{
    QSqlDatabase db = QSqlDatabase::database("app_connection");
    if (!db.isOpen()) { qWarning() << "ERROR: database is not open"; return; }

    QSqlQuery vq(db);
    vq.prepare("SELECT id, guid FROM volume");
    if (!vq.exec()) { qWarning() << vq.lastError().text(); return; }

    while (vq.next())
    {
        VolumeInfo info;
        info.id = vq.value(0).toInt();
        info.guidPath = vq.value(1).toString();
        info.handle = openVolumeHandle(info.guidPath);

        if (info.handle == INVALID_HANDLE_VALUE)
        {
            qDebug() << "ERROR: cannot open volume" << info.guidPath
                      << "error" << GetLastError();
        }

        QSqlQuery jq(db);
        jq.prepare("SELECT last_record FROM journal_state WHERE volume = :vid");
        jq.bindValue(":vid", info.id);
        info.lastUsn = 0;
        if (jq.exec() && jq.next())
            info.lastUsn = static_cast<USN>(jq.value(0).toLongLong());

        volumes_.insert(info.id, info);
    }
}

void UsnJournalMonitor::saveJournalState()
{
    QSqlDatabase db = QSqlDatabase::database("app_connection");
    if (!db.isOpen()) { qWarning() << "ERROR: database is not open"; return; }

        QSqlQuery q(db);
        q.prepare("UPDATE journal_state SET last_record = :usn WHERE volume = :vid");
        q.bindValue(":vid", 1);
        q.bindValue(":usn", static_cast<qlonglong>(volumes_[1].lastUsn));
        if (!q.exec())
            qWarning() << "ERROR: save journal_state failed:" << q.lastError().text();
        qDebug() << "volume" << QString(volumes_[1].guidPath) << "last record" << static_cast<qlonglong>(volumes_[1].lastUsn) << "saved";
    // for (const auto &vol : volumes_)
    // {
    //     QSqlQuery q(db);
    //     q.prepare("INSERT INTO journal_state (volume, last_record) VALUES (:vid, :usn) "
    //               "ON CONFLICT(volume) DO UPDATE SET last_record = :usn");
    //     q.bindValue(":vid", vol.id);
    //     q.bindValue(":usn", static_cast<qlonglong>(vol.lastUsn));
    //     if (!q.exec())
    //         qWarning() << "ERROR: save journal_state failed:" << q.lastError().text();
    // }
}

UsnJournalMonitor::VolumeInfo *UsnJournalMonitor::volumeById(int id)
{
    auto it = volumes_.find(id);
    return it == volumes_.end() ? nullptr : &it.value();
}

bool UsnJournalMonitor::reopenById
(
    const VolumeInfo &vol, 
    const QByteArray &systemId,          
    QString &currentPath
) const
{
    if (vol.handle == INVALID_HANDLE_VALUE) return false;
    if (systemId.size() != sizeof(FILE_ID_128)) return false;

    FILE_ID_128 raw = bytesToFileId128(systemId);

    FILE_ID_DESCRIPTOR desc{};
    desc.dwSize = sizeof(desc);
    desc.Type = ExtendedFileIdType;
    desc.ExtendedFileId = raw;

    HANDLE h = OpenFileById(
        vol.handle, &desc,
        FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        FILE_FLAG_BACKUP_SEMANTICS);

    if (h == INVALID_HANDLE_VALUE)
        return false;

    currentPath = getPathByHandle(h);
    CloseHandle(h);
    return !currentPath.isEmpty();
}

void UsnJournalMonitor::scanJournal
(
    const VolumeInfo &vol, 
    USN startUsn, 
    DWORD reasonMask,
    const std::function<bool(const USN_RECORD *)> &callback
) const
{
    if (vol.handle == INVALID_HANDLE_VALUE) return;

    USN_JOURNAL_DATA journal{};
    DWORD br = 0;
    if (!DeviceIoControl(vol.handle, FSCTL_QUERY_USN_JOURNAL, nullptr, 0,
        &journal, sizeof(journal), &br, nullptr))
        return;

    USN cursor = startUsn;
    if (cursor < journal.FirstUsn) cursor = journal.FirstUsn;

    READ_USN_JOURNAL_DATA readData{};
    readData.StartUsn = cursor;
    readData.ReasonMask = reasonMask;
    readData.ReturnOnlyOnClose = FALSE;
    readData.Timeout = 0;
    readData.BytesToWaitFor = 0;
    readData.UsnJournalID = journal.UsnJournalID;

    QByteArray buffer(1024 * 1024, Qt::Uninitialized);

    while (readData.StartUsn < journal.NextUsn)
    {
        DWORD bytesRead = 0;
        BOOL ok = DeviceIoControl(
            vol.handle, FSCTL_READ_USN_JOURNAL, &readData, sizeof(readData),
            buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr);

        if (!ok || bytesRead < sizeof(USN)) break;

        USN next = *reinterpret_cast<const USN *>(buffer.constData());
        readData.StartUsn = next;

        DWORD offset = sizeof(USN);
        bool stop = false;
        while (offset + sizeof(USN_RECORD) <= bytesRead)
        {
            const auto *rec = reinterpret_cast<const USN_RECORD *>(buffer.constData() + offset);
            if (rec->RecordLength == 0) break;

            if (rec->MajorVersion == 3 && !callback(rec)) { stop = true; break; }

            offset += rec->RecordLength;
        }
        if (stop) break;
    }
}

UsnJournalMonitor::Fate UsnJournalMonitor::traceFateFromJournal
(
    const QByteArray &systemId, 
    const QString &fileName, 
    int originVolumeId,
    QByteArray &newSystemId, 
    QString &newPath, 
    int &newVolumeId
)
{
    VolumeInfo *origin = volumeById(originVolumeId);
    if (!origin) return Fate::Unchanged;

    bool deleteFound = false;
    LARGE_INTEGER deleteTime{};

    scanJournal(*origin, origin->lastUsn, USN_REASON_FILE_DELETE,
        [&](const USN_RECORD *rec) -> bool
        {
            if (fileId128ToBytes(static_cast<DWORDLONG>(
                    rec->FileReferenceNumber)) != systemId)
                return true;

            deleteFound = true;
            deleteTime = rec->TimeStamp;
            return false;
        });

    if (!deleteFound)
    {
        qDebug() << "WARN: no delete record found for" << systemId.toHex()
                  << "on volume" << originVolumeId;
        return Fate::Unchanged;
    }

    for (auto &kv : volumes_)
    {
        VolumeInfo &vol = kv;
        if (vol.id == originVolumeId) continue;

        bool created = false;
        FILE_ID_128 createdId{};

        scanJournal(vol, vol.lastUsn, USN_REASON_FILE_CREATE,
            [&](const USN_RECORD *rec) -> bool
            {
                if (rec->TimeStamp.QuadPart < deleteTime.QuadPart)
                    return true;

                const auto *base = reinterpret_cast<const char *>(rec);
                QString name = QString::fromWCharArray(
                    reinterpret_cast<const wchar_t *>(base + rec->FileNameOffset),
                    rec->FileNameLength / sizeof(WCHAR));

                if (name.compare(fileName, Qt::CaseInsensitive) != 0)
                    return true;

                created = true;
                // ?!?
                createdId = bytesToFileId128(fileId128ToBytes(static_cast<DWORDLONG>(rec->FileReferenceNumber)));
                return false;
            });

        if (created)
        {
            newSystemId = fileId128ToBytes(createdId);
            newVolumeId = vol.id;

            QString path;
            if (reopenById(vol, newSystemId, path))
                newPath = path;

            return Fate::Moved;
        }
    }
    return Fate::Deleted;
}

void UsnJournalMonitor::reconcileStartup()
{
    QSqlDatabase db = QSqlDatabase::database("app_connection");
    if (!db.isOpen()) { qWarning() << "ERROR: database is not open"; return; }

    QSqlQuery query(db);
    query.prepare("SELECT path, system_id, volume_id FROM file");
    if (!query.exec()) { qWarning() << query.lastError().text(); return; }

    struct Row { QString path; QByteArray systemId; int volumeId; };
    QVector<Row> rows;
    while (query.next())
    {
        rows.push_back({
            query.value(0).toString(),
            QByteArray::fromHex(query.value(1).toByteArray()),
            query.value(2).toInt()
        });
    }

    for (const Row &row : rows)
    {
        VolumeInfo *vol = volumeById(row.volumeId);
        if (!vol)
        {
            qDebug() << "WARN: unknown volume_id" << row.volumeId << "for" << row.path;
            continue;
        }

        QString actualPath;
        bool stillOnVolume = reopenById(*vol, row.systemId, actualPath);

        if (stillOnVolume)
        {
            if (actualPath.compare(row.path, Qt::CaseInsensitive) != 0)
            {
                qDebug() << "-> path changed:" << row.path << "->" << actualPath;
                updateFilePath(row.systemId, actualPath);
            }

            addWatch(row.systemId, actualPath, row.volumeId);
            continue;
        }

        QByteArray newSystemId;
        QString newPath;
        int newVolumeId = 0;

        Fate fate = traceFateFromJournal
        (
            row.systemId, 
            QFileInfo(row.path).fileName(),
            row.volumeId, 
            newSystemId, 
            newPath, 
            newVolumeId
        );

        switch (fate)
        {
            case Fate::Moved:
                qDebug() << "-> file moved to another volume:" << row.path << "->" << newPath;
                updateFileIdentity(row.systemId, newSystemId, newPath, newVolumeId);
                addWatch(newSystemId, newPath, newVolumeId);
                break;
            case Fate::Deleted:
                qDebug() << "-> file deleted from system:" << row.path;
                deleteFileRecord(row.systemId);
                emit fileDeleted(row.path, row.systemId);
                break;
            case Fate::Unchanged:
                break;
        }
    }
}

void UsnJournalMonitor::updateFilePath(const QByteArray &systemId, const QString &newPath)
{
    QSqlDatabase db = QSqlDatabase::database("app_connection");
    QSqlQuery q(db);
    q.prepare("UPDATE file SET path = :path WHERE system_id = :sid");
    q.bindValue(":path", newPath);
    q.bindValue(":sid", QString(systemId.toHex()));
    if (!q.exec())
        qWarning() << "ERROR: update file path failed:" << q.lastError().text();
}

void UsnJournalMonitor::updateFileIdentity
(
    const QByteArray &oldSystemId,
    const QByteArray &newSystemId,
    const QString &newPath, 
    int newVolumeId
)
{
    QSqlDatabase db = QSqlDatabase::database("app_connection");
    QSqlQuery q(db);
    q.prepare("UPDATE file SET system_id = :nsid, path = :path, volume_id = :vid "
              "WHERE system_id = :osid");
    q.bindValue(":nsid", QString(newSystemId.toHex()));
    q.bindValue(":path", newPath);
    q.bindValue(":vid", newVolumeId);
    q.bindValue(":osid", QString(oldSystemId.toHex()));
    if (!q.exec())
        qWarning() << "ERROR: update file identity failed:" << q.lastError().text();
}

void UsnJournalMonitor::deleteFileRecord(const QByteArray &systemId)
{
    QSqlDatabase db = QSqlDatabase::database("app_connection");
    QSqlQuery q(db);
    q.prepare("DELETE FROM file WHERE system_id = :sid");
    q.bindValue(":sid", QString(systemId.toHex()));
    if (!q.exec())
        qWarning() << "ERROR: delete file record failed:" << q.lastError().text();
}

bool UsnJournalMonitor::addFile(const QString &path, int volumeId)
{
    QByteArray systemId = currentSystemIdForPath(path);
    if (systemId.isEmpty()) return false;
    return addWatch(systemId, path, volumeId);
}

bool UsnJournalMonitor::addWatch(const QByteArray &systemId, const QString &path, int volumeId)
{
    HANDLE handle = CreateFileW(
        reinterpret_cast<LPCWSTR>(path.utf16()),
        FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

    if (handle == INVALID_HANDLE_VALUE) return false;

    WatchedFile wf;
    wf.systemId = systemId;
    wf.handle = handle;
    wf.path = QFileInfo(path).absoluteFilePath();
    wf.volumeId = volumeId;

    watched_.insert(systemId, wf);
    return true;
}

void UsnJournalMonitor::processRecord(const USN_RECORD *record, int volumeId)
{
    const QByteArray fileId = fileId128ToBytes(static_cast<DWORDLONG>(record->FileReferenceNumber));

    auto it = watched_.find(fileId);
    if (it == watched_.end()) return;

    const auto *base = reinterpret_cast<const char *>(record);
    const QString name = QString::fromWCharArray(
        reinterpret_cast<const wchar_t *>(base + record->FileNameOffset),
        record->FileNameLength / sizeof(WCHAR));

    qDebug() << "-> USN record: fileId =" << fileId.toHex()
              << "reason =" << Qt::hex << record->Reason << "name =" << name;

    if (record->Reason & USN_REASON_RENAME_OLD_NAME)
    {
        PendingRename rn;
        rn.oldName = name;
        rn.oldParentId = fileId128ToBytes(static_cast<DWORDLONG>(record->ParentFileReferenceNumber));
        pendingRenames_.insert(fileId, rn);
        return;
    }

    if (record->Reason & USN_REASON_RENAME_NEW_NAME)
    {
        auto rIt = pendingRenames_.find(fileId);
        if (rIt == pendingRenames_.end())
        {
            qDebug() << "ERROR: RENAME_NEW_NAME without old name";
            return;
        }

        const QString oldPath = it->path;
        const QString newPath = getPathByHandle(it->handle);

        if (!newPath.isEmpty())
        {
            updateFilePath(fileId, newPath);
            it->path = newPath;
        }
        pendingRenames_.erase(rIt);
        return;
    }

    if (record->Reason & USN_REASON_FILE_DELETE)
    {
        const QString path = it->path;
        deleteFileRecord(fileId);
        emit fileDeleted(path, fileId);

        CloseHandle(it->handle);
        watched_.erase(it);
        pendingRenames_.remove(fileId);
    }
}

void UsnJournalMonitor::run()
{
    while (!isInterruptionRequested())
    {
        bool anyVolume = false;

        for (auto &kv : volumes_)
        {
            VolumeInfo &vol = kv;
            if (vol.handle == INVALID_HANDLE_VALUE) continue;
            anyVolume = true;

            USN_JOURNAL_DATA journal{};
            DWORD br = 0;
            if (!DeviceIoControl(vol.handle, FSCTL_QUERY_USN_JOURNAL, nullptr, 0,
                &journal, sizeof(journal), &br, nullptr))
                continue;

            READ_USN_JOURNAL_DATA readData{};
            readData.StartUsn = vol.lastUsn ? vol.lastUsn : journal.NextUsn;
            readData.ReasonMask = USN_REASON_RENAME_OLD_NAME |
                USN_REASON_RENAME_NEW_NAME | USN_REASON_FILE_DELETE;
            readData.ReturnOnlyOnClose = FALSE;
            readData.Timeout = 0;
            readData.BytesToWaitFor = 0;
            readData.UsnJournalID = journal.UsnJournalID;

            QByteArray buffer(1024 * 1024, Qt::Uninitialized);
            DWORD bytesRead = 0;

            BOOL ok = DeviceIoControl(
                vol.handle, FSCTL_READ_USN_JOURNAL, &readData, sizeof(readData),
                buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr);

            if (!ok || bytesRead < sizeof(USN)) continue;

            USN next = *reinterpret_cast<const USN *>(buffer.constData());
            vol.lastUsn = next;

            DWORD offset = sizeof(USN);
            while (offset + sizeof(USN_RECORD) <= bytesRead)
            {
                const auto *rec = reinterpret_cast<const USN_RECORD *>(buffer.constData() + offset);
                if (rec->RecordLength == 0) break;
                processRecord(rec, vol.id);
                offset += rec->RecordLength;
            }
        }

        if (!anyVolume) break;
        msleep(200);
    }
}