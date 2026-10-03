// #include "System.h"

// namespace
// {
//     System::UsnEvent toEvent(const USN_RECORD *rec)
//     {
//         FILE_ID_128 id{};
//         const DWORDLONG ref = rec->FileReferenceNumber;
//         std::memcpy(id.Identifier, &ref, sizeof(ref));

//         const auto *base = reinterpret_cast<const char *>(rec);
//         return
//         {
//             QByteArray(reinterpret_cast<const char *>(id.Identifier), sizeof(id.Identifier)),
//             QString::fromWCharArray
//             (
//                 reinterpret_cast<const wchar_t *>(base + rec->FileNameOffset),
//                 rec->FileNameLength / sizeof(WCHAR)
//             ),
//             rec->TimeStamp.QuadPart,
//             rec->Reason
//         };
//     }
// }

// namespace
// {
//     HANDLE openPath(const QString &path, DWORD access, DWORD flags)
//     {
//         const QString native = QDir::toNativeSeparators(path);
//         return CreateFileW
//         (
//             reinterpret_cast<LPCWSTR>(native.utf16()),
//             access,
//             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
//             nullptr, OPEN_EXISTING, flags, nullptr
//         );
//     }
// }

// HANDLE System::openFileForWatch(const QString &path)
// {
//     return openPath(path, FILE_READ_ATTRIBUTES, FILE_FLAG_BACKUP_SEMANTICS);
// }


// std::optional<USN> System::scanUsnJournal
// (
//     HANDLE volumeHandle, USN startUsn, DWORD reasonMask,
//     const std::function<bool(const UsnEvent &)> &callback
// )
// {
//     const auto journal = queryUsnJournal(volumeHandle);
//     if (!journal) return std::nullopt;

//     READ_USN_JOURNAL_DATA readData{};
//     readData.StartUsn = std::max(startUsn, journal->FirstUsn);
//     readData.ReasonMask = reasonMask;
//     readData.UsnJournalID = journal->UsnJournalID;   // остальные поля уже нули

//     QByteArray buffer(1024 * 1024, Qt::Uninitialized);

//     while (readData.StartUsn < journal->NextUsn)
//     {
//         DWORD bytesRead = 0;
//         if (!DeviceIoControl(volumeHandle, FSCTL_READ_USN_JOURNAL, &readData, sizeof(readData),
//                              buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr)
//             || bytesRead < sizeof(USN))
//             break;

//         readData.StartUsn = *reinterpret_cast<const USN *>(buffer.constData());

//         DWORD offset = sizeof(USN);
//         while (offset + sizeof(USN_RECORD) <= bytesRead)
//         {
//             const auto *rec = reinterpret_cast<const USN_RECORD *>(buffer.constData() + offset);
//             if (rec->RecordLength == 0 || rec->RecordLength > bytesRead - offset) break;

//             if (!callback(toEvent(rec))) return readData.StartUsn;
//             offset += rec->RecordLength;
//         }
//     }
//     return readData.StartUsn;
// }

// std::wstring resolveExistingPath(const QString &filePath)
// {
//         QString localPath = filePath;
//         const QUrl url(filePath);
//         if (url.isLocalFile()) localPath = url.toLocalFile();

//         const QFileInfo fi(QDir::toNativeSeparators(localPath));
//         if (!fi.exists()) return {};

//         return QDir::toNativeSeparators(fi.absoluteFilePath()).toStdWString();
// }

// HWND hwndOf(QQuickWindow *window)
// {
//     return window ? reinterpret_cast<HWND>(window->winId()) : nullptr;
// }

// DWORD shellExecute(const wchar_t *verb, const std::wstring &path, HWND parent = nullptr)
// {
//     SHELLEXECUTEINFOW sei = {};
//     sei.cbSize = sizeof(sei);
//     sei.fMask  = SEE_MASK_INVOKEIDLIST | SEE_MASK_FLAG_NO_UI;
//     sei.hwnd   = parent;
//     sei.lpVerb = verb;
//     sei.lpFile = path.c_str();
//     sei.nShow  = SW_SHOWNORMAL;

//     return ShellExecuteExW(&sei) ? 0 : GetLastError();
// }

// System::System(QObject* parent): QObject(parent) {}

// void System::openWith(const QString &filePath, QQuickWindow *window)
// {
//     const std::wstring wpath = resolveExistingPath(filePath);
//     if (wpath.empty()) return;

//     if (const DWORD err = shellExecute(L"openas", wpath, hwndOf(window)))
//         qWarning() << "ERROR: ShellExecuteExW (openas) failed with error code" << err;
// }

// void System::openFile(const QString &filePath, QQuickWindow *window)
// {
//     const std::wstring wpath = resolveExistingPath(filePath);
//     if (wpath.empty()) return;

//     const DWORD err = shellExecute(L"open", wpath);
//     if (err == 0) return;

//     if (err == ERROR_NO_ASSOCIATION) // 1155
//     {
//         if (const DWORD err2 = shellExecute(L"openas", wpath, hwndOf(window)))
//             qWarning() << "ERROR: ShellExecuteExW (openas fallback) failed with error code" << err2;
//     }
//     else qWarning() << "ERROR: ShellExecuteExW (open) failed with error code" << err;
// }

// QByteArray System::fileId128ForPath(const QString &path)
// {
//     const QString native = QDir::toNativeSeparators(path);

//     HANDLE h = openPath(path, 0, FILE_FLAG_BACKUP_SEMANTICS);
//     // HANDLE h = CreateFileW
//     // (
//     //     reinterpret_cast<LPCWSTR>(native.utf16()),
//     //     0,
//     //     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
//     //     nullptr,
//     //     OPEN_EXISTING,
//     //     FILE_FLAG_BACKUP_SEMANTICS,
//     //     nullptr
//     // );

//     if (h == INVALID_HANDLE_VALUE) return {};

//     FILE_ID_INFO info{};
//     const BOOL ok = GetFileInformationByHandleEx(h, FileIdInfo, &info, sizeof(info));
//     CloseHandle(h);
//     if (!ok) return {};

//     return QByteArray
//     (
//         reinterpret_cast<const char*>(info.FileId.Identifier),
//         sizeof(info.FileId.Identifier)
//     );
// }

// QString System::pathByHandle(HANDLE handle)
// {
//     const HANDLE h = static_cast<HANDLE>(handle);
//     constexpr DWORD flags = FILE_NAME_NORMALIZED | VOLUME_NAME_DOS;

//     const DWORD length = GetFinalPathNameByHandleW(h, nullptr, 0, flags);
//     if (length == 0) return {};

//     std::wstring buffer(length + 1, L'\0');
//     const DWORD result = GetFinalPathNameByHandleW
//     (
//         h,
//         buffer.data(),
//         static_cast<DWORD>(buffer.size()),
//         flags
//     );
//     if (result == 0) return {};

//     QString p = QString::fromWCharArray(buffer.data(), static_cast<int>(result));
//     if (p.startsWith(QStringLiteral("\\\\?\\UNC\\")))
//         p = QStringLiteral("\\\\") + p.mid(8);
//     else if (p.startsWith(QStringLiteral("\\\\?\\")))
//         p = p.mid(4);

//     return QDir::fromNativeSeparators(p);
// }

// // HANDLE System::openVolumeHandle(const QString &guidPath)
// // {
// //     QString path = guidPath.trimmed();
// //     if (path.startsWith('"') && path.endsWith('"'))
// //         path = path.mid(1, path.size() - 2);

// //     while (path.endsWith('\\'))
// //         path.chop(1);

// //     qDebug().noquote() << "-> Opening volume:" << path;

// //     return CreateFileW
// //     (
// //         reinterpret_cast<LPCWSTR>(path.utf16()),
// //         GENERIC_READ,
// //         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
// //         nullptr,
// //         OPEN_EXISTING,
// //         0,
// //         nullptr
// //     );
// // }

// HANDLE System::openVolumeHandle(const QString &guidPath)
// {
//     QString path = guidPath.trimmed();
//     if (path.startsWith('"') && path.endsWith('"'))
//         path = path.mid(1, path.size() - 2);

//     while (path.endsWith('\\'))
//         path.chop(1);

//     qDebug().noquote() << "-> Opening volume:" << path;

//     HANDLE h = openPath(path, GENERIC_READ, 0);
//     // HANDLE h = CreateFileW
//     // (
//     //     reinterpret_cast<LPCWSTR>(path.utf16()),
//     //     GENERIC_READ,
//     //     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
//     //     nullptr,
//     //     OPEN_EXISTING,
//     //     0,
//     //     nullptr
//     // );

//     const DWORD err = GetLastError();   // сразу, пока ничто не успело её затереть
//     if (h == INVALID_HANDLE_VALUE)
//         qWarning() << "ERROR: CreateFileW failed for" << path << "error" << err;

//     SetLastError(err);                  // чтобы System::lastError() у вызывающего тоже работал
//     return h;
// }

// void System::closeHandle(HANDLE handle)
// {
//     if (handle && handle != INVALID_HANDLE_VALUE)
//         CloseHandle(handle);
// }

// DWORD System::lastError()
// {
//     return GetLastError();
// }

// std::optional<USN_JOURNAL_DATA> System::queryUsnJournal(HANDLE volumeHandle)
// {
//     if (!volumeHandle || volumeHandle == INVALID_HANDLE_VALUE) return std::nullopt;

//     USN_JOURNAL_DATA journal{};
//     DWORD br = 0;
//     if (!DeviceIoControl(volumeHandle, FSCTL_QUERY_USN_JOURNAL, nullptr, 0,
//                          &journal, sizeof(journal), &br, nullptr))
//         return std::nullopt;

//     return journal;
// }

// std::optional<USN> System::queryNextUsn(HANDLE volumeHandle)
// {
//     if (const auto journal = queryUsnJournal(volumeHandle))
//         return journal->NextUsn;
//     return std::nullopt;
// }

// QString System::pathByFileId(HANDLE volumeHandle, const QByteArray &fileId128)
// {
//     if (!volumeHandle || volumeHandle == INVALID_HANDLE_VALUE) return {};
//     if (fileId128.size() != sizeof(FILE_ID_128)) return {};

//     FILE_ID_DESCRIPTOR desc{};
//     desc.dwSize = sizeof(desc);
//     desc.Type = ExtendedFileIdType;
//     std::memcpy(desc.ExtendedFileId.Identifier, fileId128.constData(),
//                 sizeof(desc.ExtendedFileId.Identifier));

//     HANDLE h = OpenFileById
//     (
//         volumeHandle, &desc,
//         FILE_READ_ATTRIBUTES,
//         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
//         nullptr,
//         FILE_FLAG_BACKUP_SEMANTICS
//     );
//     if (h == INVALID_HANDLE_VALUE) return {};

//     const QString path = pathByHandle(h);
//     CloseHandle(h);
//     return path;
// }

// // void System::scanUsnJournal
// // (
// //     HANDLE volumeHandle,
// //     USN startUsn,
// //     DWORD reasonMask,
// //     const std::function<bool(const USN_RECORD *)> &callback
// // )
// // {
// //     const auto journal = queryUsnJournal(volumeHandle);
// //     if (!journal) return;

// //     READ_USN_JOURNAL_DATA readData{};
// //     readData.StartUsn = (startUsn < journal->FirstUsn) ? journal->FirstUsn : startUsn;
// //     readData.ReasonMask = reasonMask;
// //     readData.ReturnOnlyOnClose = FALSE;
// //     readData.Timeout = 0;
// //     readData.BytesToWaitFor = 0;
// //     readData.UsnJournalID = journal->UsnJournalID;

// //     QByteArray buffer(1024 * 1024, Qt::Uninitialized);

// //     while (readData.StartUsn < journal->NextUsn)
// //     {
// //         DWORD bytesRead = 0;
// //         const BOOL ok = DeviceIoControl
// //         (
// //             volumeHandle, FSCTL_READ_USN_JOURNAL, &readData, sizeof(readData),
// //             buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr
// //         );
// //         if (!ok || bytesRead < sizeof(USN)) break;

// //         readData.StartUsn = *reinterpret_cast<const USN *>(buffer.constData());

// //         DWORD offset = sizeof(USN);
// //         bool stop = false;
// //         while (offset + sizeof(USN_RECORD) <= bytesRead)
// //         {
// //             const auto *rec = reinterpret_cast<const USN_RECORD *>(buffer.constData() + offset);
// //             if (rec->RecordLength == 0) break;

// //             if (!callback(rec)) { stop = true; break; }

// //             offset += rec->RecordLength;
// //         }
// //         if (stop) break;
// //     }
// // }

// bool System::isValidHandle(HANDLE handle)
// {
//     return handle && handle != INVALID_HANDLE_VALUE;
// }

// HANDLE System::openFileForWatch(const QString &path)
// {
//     const QString native = QDir::toNativeSeparators(path);

//     return CreateFileW
//     (
//         reinterpret_cast<LPCWSTR>(native.utf16()),
//         FILE_READ_ATTRIBUTES,
//         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
//         nullptr,
//         OPEN_EXISTING,
//         FILE_FLAG_BACKUP_SEMANTICS,
//         nullptr
//     );
// }

// QByteArray System::usnRecordFileId(const USN_RECORD *rec)
// {
//     FILE_ID_128 id{};
//     const DWORDLONG ref = rec->FileReferenceNumber;
//     std::memcpy(id.Identifier, &ref, sizeof(ref));
//     return QByteArray(reinterpret_cast<const char *>(id.Identifier), sizeof(id.Identifier));
// }

// QString System::usnRecordFileName(const USN_RECORD *rec)
// {
//     const auto *base = reinterpret_cast<const char *>(rec);
//     return QString::fromWCharArray
//     (
//         reinterpret_cast<const wchar_t *>(base + rec->FileNameOffset),
//         rec->FileNameLength / sizeof(WCHAR)
//     );
// }

// qint64 System::usnRecordTimestamp(const USN_RECORD *rec)
// {
//     return rec->TimeStamp.QuadPart;
// }

// std::optional<USN> System::readUsnJournalOnce
// (
//     HANDLE volumeHandle,
//     USN startUsn,
//     DWORD reasonMask,
//     const std::function<void(const USN_RECORD *)> &onRecord
// )
// {
//     const auto journal = queryUsnJournal(volumeHandle);
//     if (!journal) return std::nullopt;

//     READ_USN_JOURNAL_DATA readData{};
//     readData.StartUsn = startUsn ? startUsn : journal->NextUsn;
//     readData.ReasonMask = reasonMask;
//     readData.ReturnOnlyOnClose = FALSE;
//     readData.Timeout = 0;
//     readData.BytesToWaitFor = 0;
//     readData.UsnJournalID = journal->UsnJournalID;

//     QByteArray buffer(1024 * 1024, Qt::Uninitialized);
//     DWORD bytesRead = 0;

//     const BOOL ok = DeviceIoControl
//     (
//         volumeHandle, FSCTL_READ_USN_JOURNAL, &readData, sizeof(readData),
//         buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr
//     );
//     if (!ok || bytesRead < sizeof(USN)) return std::nullopt;

//     const USN next = *reinterpret_cast<const USN *>(buffer.constData());

//     DWORD offset = sizeof(USN);
//     while (offset + sizeof(USN_RECORD) <= bytesRead)
//     {
//         const auto *rec = reinterpret_cast<const USN_RECORD *>(buffer.constData() + offset);
//         if (rec->RecordLength == 0) break;

//         onRecord(rec);
//         offset += rec->RecordLength;
//     }
//     return next;
// }

#include "System.h"

#include <algorithm>
#include <cstring>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QUrl>

namespace
{
    // ---------- Shell ----------

    std::wstring resolveExistingPath(const QString &filePath)
    {
        QString localPath = filePath;
        const QUrl url(filePath);
        if (url.isLocalFile()) localPath = url.toLocalFile();

        const QFileInfo fi(QDir::toNativeSeparators(localPath));
        if (!fi.exists()) return {};

        return QDir::toNativeSeparators(fi.absoluteFilePath()).toStdWString();
    }

    HWND hwndOf(QQuickWindow *window)
    {
        return window ? reinterpret_cast<HWND>(window->winId()) : nullptr;
    }

    // 0 при успехе, иначе GetLastError()
    DWORD shellExecute(const wchar_t *verb, const std::wstring &path, HWND parent = nullptr)
    {
        SHELLEXECUTEINFOW sei = {};
        sei.cbSize = sizeof(sei);
        sei.fMask  = SEE_MASK_INVOKEIDLIST | SEE_MASK_FLAG_NO_UI;
        sei.hwnd   = parent;
        sei.lpVerb = verb;
        sei.lpFile = path.c_str();
        sei.nShow  = SW_SHOWNORMAL;

        return ShellExecuteExW(&sei) ? 0 : GetLastError();
    }

    // ---------- Файлы и журнал ----------

    HANDLE openPath(const QString &path, DWORD access, DWORD flags)
    {
        const QString native = QDir::toNativeSeparators(path);
        return CreateFileW
        (
            reinterpret_cast<LPCWSTR>(native.utf16()),
            access,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr, OPEN_EXISTING, flags, nullptr
        );
    }

    std::optional<USN_JOURNAL_DATA> queryUsnJournal(HANDLE volumeHandle)
    {
        if (!System::isValidHandle(volumeHandle)) return std::nullopt;

        USN_JOURNAL_DATA journal{};
        DWORD br = 0;
        if (!DeviceIoControl(volumeHandle, FSCTL_QUERY_USN_JOURNAL, nullptr, 0,
                             &journal, sizeof(journal), &br, nullptr))
            return std::nullopt;

        return journal;
    }

    System::UsnEvent toEvent(const USN_RECORD *rec)
    {
        FILE_ID_128 id{};
        const DWORDLONG ref = rec->FileReferenceNumber;
        std::memcpy(id.Identifier, &ref, sizeof(ref));

        const auto *base = reinterpret_cast<const char *>(rec);
        return
        {
            QByteArray(reinterpret_cast<const char *>(id.Identifier), sizeof(id.Identifier)),
            QString::fromWCharArray
            (
                reinterpret_cast<const wchar_t *>(base + rec->FileNameOffset),
                rec->FileNameLength / sizeof(WCHAR)
            ),
            rec->TimeStamp.QuadPart,
            rec->Reason
        };
    }
}

System::System(QObject *parent) : QObject(parent) {}

// ---------- Открытие файлов ----------

void System::openWith(const QString &filePath, QQuickWindow *window)
{
    const std::wstring wpath = resolveExistingPath(filePath);
    if (wpath.empty()) return;

    if (const DWORD err = shellExecute(L"openas", wpath, hwndOf(window)))
        qWarning() << "ERROR: ShellExecuteExW (openas) failed with error code" << err;
}

void System::openFile(const QString &filePath, QQuickWindow *window)
{
    const std::wstring wpath = resolveExistingPath(filePath);
    if (wpath.empty()) return;

    const DWORD err = shellExecute(L"open", wpath);
    if (err == 0) return;

    if (err == ERROR_NO_ASSOCIATION) // 1155
    {
        if (const DWORD err2 = shellExecute(L"openas", wpath, hwndOf(window)))
            qWarning() << "ERROR: ShellExecuteExW (openas fallback) failed with error code" << err2;
    }
    else qWarning() << "ERROR: ShellExecuteExW (open) failed with error code" << err;
}

// ---------- Хэндлы ----------

bool System::isValidHandle(HANDLE handle)
{
    return handle && handle != INVALID_HANDLE_VALUE;
}

void System::closeHandle(HANDLE handle)
{
    if (isValidHandle(handle))
        CloseHandle(handle);
}

HANDLE System::openVolumeHandle(const QString &guidPath)
{
    QString path = guidPath.trimmed();
    if (path.startsWith('"') && path.endsWith('"'))
        path = path.mid(1, path.size() - 2);

    while (path.endsWith('\\'))
        path.chop(1);

    qDebug().noquote() << "-> Opening volume:" << path;

    HANDLE h = openPath(path, GENERIC_READ, 0);
    if (h == INVALID_HANDLE_VALUE)
        qWarning() << "ERROR: cannot open volume" << path << "error" << GetLastError();

    return h;
}

HANDLE System::openFileForWatch(const QString &path)
{
    return openPath(path, FILE_READ_ATTRIBUTES, FILE_FLAG_BACKUP_SEMANTICS);
}

// ---------- Идентификаторы и пути ----------

QByteArray System::fileId128ForPath(const QString &path)
{
    HANDLE h = openPath(path, 0, FILE_FLAG_BACKUP_SEMANTICS);
    if (h == INVALID_HANDLE_VALUE) return {};

    FILE_ID_INFO info{};
    const BOOL ok = GetFileInformationByHandleEx(h, FileIdInfo, &info, sizeof(info));
    closeHandle(h);
    if (!ok) return {};

    return QByteArray
    (
        reinterpret_cast<const char *>(info.FileId.Identifier),
        sizeof(info.FileId.Identifier)
    );
}

QString System::pathByHandle(HANDLE handle)
{
    constexpr DWORD flags = FILE_NAME_NORMALIZED | VOLUME_NAME_DOS;

    const DWORD length = GetFinalPathNameByHandleW(handle, nullptr, 0, flags);
    if (length == 0) return {};

    std::wstring buffer(length + 1, L'\0');
    const DWORD result = GetFinalPathNameByHandleW
    (
        handle,
        buffer.data(),
        static_cast<DWORD>(buffer.size()),
        flags
    );
    if (result == 0) return {};

    QString p = QString::fromWCharArray(buffer.data(), static_cast<int>(result));
    if (p.startsWith(QStringLiteral("\\\\?\\UNC\\")))
        p = QStringLiteral("\\\\") + p.mid(8);
    else if (p.startsWith(QStringLiteral("\\\\?\\")))
        p = p.mid(4);

    return QDir::fromNativeSeparators(p);
}

QString System::pathByFileId(HANDLE volumeHandle, const QByteArray &fileId128)
{
    if (!isValidHandle(volumeHandle)) return {};
    if (fileId128.size() != sizeof(FILE_ID_128)) return {};

    FILE_ID_DESCRIPTOR desc{};
    desc.dwSize = sizeof(desc);
    desc.Type = ExtendedFileIdType;
    std::memcpy(desc.ExtendedFileId.Identifier, fileId128.constData(),
                sizeof(desc.ExtendedFileId.Identifier));

    HANDLE h = OpenFileById
    (
        volumeHandle, &desc,
        FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        FILE_FLAG_BACKUP_SEMANTICS
    );
    if (h == INVALID_HANDLE_VALUE) return {};

    const QString path = pathByHandle(h);
    closeHandle(h);
    return path;
}

// ---------- Журнал USN ----------

std::optional<USN> System::queryNextUsn(HANDLE volumeHandle)
{
    if (const auto journal = queryUsnJournal(volumeHandle))
        return journal->NextUsn;
    return std::nullopt;
}

std::optional<USN> System::scanUsnJournal
(
    HANDLE volumeHandle, USN startUsn, DWORD reasonMask,
    const std::function<bool(const UsnEvent &)> &callback
)
{
    const auto journal = queryUsnJournal(volumeHandle);
    if (!journal) return std::nullopt;

    READ_USN_JOURNAL_DATA readData{};
    readData.StartUsn = std::max(startUsn, journal->FirstUsn);
    readData.ReasonMask = reasonMask;
    readData.UsnJournalID = journal->UsnJournalID;   // остальные поля нулевые

    QByteArray buffer(1024 * 1024, Qt::Uninitialized);

    while (readData.StartUsn < journal->NextUsn)
    {
        DWORD bytesRead = 0;
        if (!DeviceIoControl(volumeHandle, FSCTL_READ_USN_JOURNAL, &readData, sizeof(readData),
                             buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr)
            || bytesRead < sizeof(USN))
            break;

        readData.StartUsn = *reinterpret_cast<const USN *>(buffer.constData());

        DWORD offset = sizeof(USN);
        while (offset + sizeof(USN_RECORD) <= bytesRead)
        {
            const auto *rec = reinterpret_cast<const USN_RECORD *>(buffer.constData() + offset);
            if (rec->RecordLength == 0 || rec->RecordLength > bytesRead - offset) break;

            if (!callback(toEvent(rec))) return readData.StartUsn;
            offset += rec->RecordLength;
        }
    }
    return readData.StartUsn;
}