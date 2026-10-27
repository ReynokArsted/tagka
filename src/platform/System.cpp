#include "System.h"

#include <algorithm>
#include <cstring>

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUrl>

#include <winioctl.h>

namespace
{
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

    bool queryUsnJournal(HANDLE volumeHandle, USN_JOURNAL_DATA &journal)
    {
        if (!System::isValidHandle(volumeHandle)) return false;

        DWORD br = 0;
        return DeviceIoControl(volumeHandle, FSCTL_QUERY_USN_JOURNAL, nullptr, 0,
                               &journal, sizeof(journal), &br, nullptr) != 0;
    }

    struct MftEnumData
    {
        DWORDLONG StartFileReferenceNumber;
        USN LowUsn;
        USN HighUsn;
    };

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

    if (err == ERROR_NO_ASSOCIATION)
    {
        if (const DWORD err2 = shellExecute(L"openas", wpath, hwndOf(window)))
            qWarning() << "ERROR: ShellExecuteExW (openas fallback) failed with error code" << err2;
    }
    else qWarning() << "ERROR: ShellExecuteExW (open) failed with error code" << err;
}

QString System::renameFile(const QString &filePath, const QString &newName)
{
    static const QRegularExpression forbidden(QStringLiteral(R"([\\/:*?"<>|])"));

    const QString name = newName.trimmed();
    if (name.isEmpty() || name == "." || name == ".." || name.contains(forbidden))
    {
        qWarning() << "ERROR: invalid file name" << newName;
        return {};
    }

    const QFileInfo fi(filePath);
    if (!fi.exists())
    {
        qWarning() << "ERROR: cannot rename, file not found" << filePath;
        return {};
    }

    const QString oldPath = fi.absoluteFilePath();
    const QString newPath = QDir(fi.absolutePath()).filePath(name);
    if (newPath == oldPath) return newPath;

    const QString nativeOld = QDir::toNativeSeparators(oldPath);
    const QString nativeNew = QDir::toNativeSeparators(newPath);
    if (!MoveFileExW(reinterpret_cast<LPCWSTR>(nativeOld.utf16()),
                     reinterpret_cast<LPCWSTR>(nativeNew.utf16()), 0))
    {
        qWarning() << "ERROR: MoveFileExW failed with error code" << GetLastError()
                   << oldPath << "->" << newPath;
        return {};
    }
    return newPath;
}

QStringList System::moveToTrash(const QStringList &paths)
{
    QStringList removed;
    for (const QString &path : paths)
    {
        if (QFile::moveToTrash(path)) removed << path;
        else qWarning() << "ERROR: cannot move to trash" << path;
    }
    return removed;
}

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

static bool isMountedNtfs(const wchar_t *name)
{
    const UINT type = GetDriveTypeW(name);
    bool ok = type == DRIVE_FIXED || type == DRIVE_REMOVABLE;

    DWORD needed = 0;
    if (ok)
    {
        GetVolumePathNamesForVolumeNameW(name, nullptr, 0, &needed);
        ok = needed > 1;
    }

    wchar_t fs[MAX_PATH]{};
    ok = ok && GetVolumeInformationW(name, nullptr, 0, nullptr, nullptr, nullptr, fs, MAX_PATH);
    return ok && QString::fromWCharArray(fs).compare(QStringLiteral("NTFS"), Qt::CaseInsensitive) == 0;
}

QStringList System::mountedNtfsVolumes()
{
    QStringList result;

    DWORD oldMode = 0;
    SetThreadErrorMode(SEM_FAILCRITICALERRORS, &oldMode);

    wchar_t name[MAX_PATH]{};
    HANDLE find = FindFirstVolumeW(name, MAX_PATH);
    if (find != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (isMountedNtfs(name))
                result.append(QString::fromWCharArray(name));
        }
        while (FindNextVolumeW(find, name, MAX_PATH));
        FindVolumeClose(find);
    }

    SetThreadErrorMode(oldMode, nullptr);
    return result;
}

bool System::ensureUsnJournal(HANDLE volumeHandle)
{
    USN_JOURNAL_DATA journal{};
    if (queryUsnJournal(volumeHandle, journal)) return true;

    CREATE_USN_JOURNAL_DATA create{};
    create.MaximumSize = 32ull * 1024 * 1024;
    create.AllocationDelta = 4ull * 1024 * 1024;

    DWORD br = 0;
    if (!DeviceIoControl(volumeHandle, FSCTL_CREATE_USN_JOURNAL, &create, sizeof(create),
                         nullptr, 0, &br, nullptr))
    {
        qWarning() << "ERROR: cannot create USN journal, error" << GetLastError();
        return false;
    }
    return queryUsnJournal(volumeHandle, journal);
}

HANDLE System::openFileForWatch(const QString &path)
{
    return openPath(path, FILE_READ_ATTRIBUTES, FILE_FLAG_BACKUP_SEMANTICS);
}

HANDLE System::openFileById(HANDLE volumeHandle, const QByteArray &fileId128, DWORD access)
{
    if (!isValidHandle(volumeHandle) || fileId128.size() != sizeof(FILE_ID_128))
        return INVALID_HANDLE_VALUE;

    FILE_ID_DESCRIPTOR desc{};
    desc.dwSize = sizeof(desc);
    desc.Type = ExtendedFileIdType;
    std::memcpy
    (
        desc.ExtendedFileId.Identifier,
        fileId128.constData(),
        sizeof(desc.ExtendedFileId.Identifier)
    );

    return OpenFileById
    (
        volumeHandle, &desc, access,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN
    );
}

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
    HANDLE h = openFileById(volumeHandle, fileId128, FILE_READ_ATTRIBUTES);
    if (h == INVALID_HANDLE_VALUE) return {};

    const QString path = pathByHandle(h);
    closeHandle(h);
    return path;
}

QString System::volumeGuidForPath(const QString &path)
{
    const QString native = QDir::toNativeSeparators(QFileInfo(path).absoluteFilePath());

    wchar_t mount[MAX_PATH]{};
    wchar_t guid[64]{};
    if (!GetVolumePathNameW(reinterpret_cast<LPCWSTR>(native.utf16()), mount, MAX_PATH)) return {};
    if (!GetVolumeNameForVolumeMountPointW(mount, guid, 64)) return {};

    return QString::fromWCharArray(guid);
}

qint64 System::fileSize(HANDLE handle)
{
    LARGE_INTEGER li{};
    return GetFileSizeEx(handle, &li) ? li.QuadPart : -1;
}

bool System::isDeletePending(HANDLE handle)
{
    FILE_STANDARD_INFO info{};
    return GetFileInformationByHandleEx(handle, FileStandardInfo, &info, sizeof(info))
        && info.DeletePending;
}

QByteArray System::sha256OfHandle(HANDLE handle, qint64 &size, const std::atomic<bool> *cancel)
{
    size = fileSize(handle);
    if (size < 0) return {};

    LARGE_INTEGER zero{};
    if (!SetFilePointerEx(handle, zero, nullptr, FILE_BEGIN)) return {};

    QCryptographicHash hash(QCryptographicHash::Sha256);
    QByteArray buffer(1 << 20, Qt::Uninitialized);
    DWORD n = 0;
    qint64 total = 0;

    while (ReadFile(handle, buffer.data(), static_cast<DWORD>(buffer.size()), &n, nullptr) && n > 0)
    {
        if (cancel && cancel->load()) return {};
        hash.addData(QByteArrayView(buffer.constData(), n));
        total += n;
    }

    if (total != size) return {};
    return hash.result();
}

QHash<QString, QList<QByteArray>> System::enumByNames
(
    HANDLE volumeHandle, 
    const QSet<QString> &namesLower, 
    const std::atomic<bool> *cancel
)
{
    QHash<QString, QList<QByteArray>> result;

    USN_JOURNAL_DATA journal{};
    if (!queryUsnJournal(volumeHandle, journal)) return result;

    MftEnumData med{};
    med.StartFileReferenceNumber = 0;
    med.LowUsn = 0;
    med.HighUsn = journal.NextUsn;

    QByteArray buffer(1 << 20, Qt::Uninitialized);
    DWORD br = 0;

    while (!(cancel && cancel->load())
           && DeviceIoControl
                (   
                    volumeHandle, 
                    FSCTL_ENUM_USN_DATA, 
                    &med, 
                    sizeof(med),
                    buffer.data(), 
                    static_cast<DWORD>(buffer.size()), 
                    &br, 
                    nullptr
                )
           && br >= sizeof(DWORDLONG))
    {
        med.StartFileReferenceNumber = *reinterpret_cast<const DWORDLONG *>(buffer.constData());

        DWORD offset = sizeof(DWORDLONG);
        bool recordOk = true;
        while (recordOk && offset + sizeof(USN_RECORD) <= br)
        {
            const auto *rec = reinterpret_cast<const USN_RECORD *>(buffer.constData() + offset);
            recordOk = rec->RecordLength != 0 && rec->RecordLength <= br - offset;

            if (recordOk)
            {
                if (!(rec->FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    const UsnEvent e = toEvent(rec);
                    const QString key = e.name.toLower();
                    if (namesLower.contains(key))
                        result[key].append(e.fileId);
                }
                offset += rec->RecordLength;
            }
        }
    }
    return result;
}

USN System::queryNextUsn(HANDLE volumeHandle)
{
    USN_JOURNAL_DATA journal{};
    return queryUsnJournal(volumeHandle, journal) ? journal.NextUsn : InvalidUsn;
}

System::UsnReader::UsnReader(HANDLE volumeHandle, USN startUsn, DWORD reasonMask)
    : volume_(volumeHandle), buffer_(1024 * 1024, Qt::Uninitialized)
{
    available_ = queryUsnJournal(volume_, journal_);
    if (available_)
    {
        request_.StartUsn = std::max(startUsn, journal_.FirstUsn);
        request_.ReasonMask = reasonMask;
        request_.UsnJournalID = journal_.UsnJournalID;
    }
}

USN System::UsnReader::position() const
{
    return available_ ? request_.StartUsn : InvalidUsn;
}

bool System::UsnReader::next(UsnEvent &event)
{
    bool found = false;
    bool more = available_ && !finished_;

    while (more && !found)
    {
        if (offset_ + sizeof(USN_RECORD) <= bytesRead_)
        {
            const auto *rec = reinterpret_cast<const USN_RECORD *>(buffer_.constData() + offset_);
            if (rec->RecordLength == 0 || rec->RecordLength > bytesRead_ - offset_)
            {
                offset_ = bytesRead_;
            }
            else
            {
                event = toEvent(rec);
                offset_ += rec->RecordLength;
                found = true;
            }
        }
        else
        {
            more = readBlock();
        }
    }
    return found;
}

bool System::UsnReader::readBlock()
{
    bool ok = request_.StartUsn < journal_.NextUsn;
    if (ok)
    {
        bytesRead_ = 0;
        ok = DeviceIoControl
            (
                volume_, FSCTL_READ_USN_JOURNAL,
                &request_, sizeof(request_),
                buffer_.data(), static_cast<DWORD>(buffer_.size()),
                &bytesRead_, nullptr
            ) && bytesRead_ >= sizeof(USN);
    }

    if (ok)
    {
        request_.StartUsn = *reinterpret_cast<const USN *>(buffer_.constData());
        offset_ = sizeof(USN);
    }
    else
    {
        finished_ = true;
    }
    return ok;
}
