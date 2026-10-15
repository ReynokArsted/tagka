#include "FileMonitor.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <cstring>

#include "Storage.h"
#include "System.h"

QByteArray FileMonitor::fileId128ToBytes(const FILE_ID_128 &id)
{
    return QByteArray(reinterpret_cast<const char *>(id.Identifier), sizeof(id.Identifier));
}

QByteArray FileMonitor::fileId128ToBytes(DWORDLONG fileReference)
{
    FILE_ID_128 id{};
    memcpy
    (
        id.Identifier,
        &fileReference,
        sizeof(fileReference)
    );
    return QByteArray
    (
        reinterpret_cast<const char*>(id.Identifier),
        sizeof(id.Identifier)
    );
}

FILE_ID_128 FileMonitor::bytesToFileId128(const QByteArray &bytes)
{
    FILE_ID_128 id{};
    if (bytes.size() == static_cast<int>(sizeof(id.Identifier)))
        memcpy(id.Identifier, bytes.constData(), sizeof(id.Identifier));
    return id;
}

QByteArray FileMonitor::currentSystemIdForPath(const QString &path)
{
    return System::fileId128ForPath(path);
}

QString FileMonitor::getPathByHandle(HANDLE handle)
{
    return System::pathByHandle(handle);
}

HANDLE  FileMonitor::openVolumeHandle(const QString &p)
{
    return System::openVolumeHandle(p);
}

FileMonitor::FileMonitor(Storage *db, QObject *parent)
    : QThread(parent), db_(db)
{
    loadVolumes();
    reconcileStartup();
}

FileMonitor::~FileMonitor()
{
    requestInterruption();
    wait();

    for (auto &wf : watched_)
        System::closeHandle(wf.handle);

    for (auto &vol : volumes_)
        if (const auto usn = System::queryNextUsn(vol.handle))
            vol.lastUsn = *usn;

    saveJournalState();

    for (auto &vol : volumes_)
        System::closeHandle(vol.handle);
}

void FileMonitor::loadVolumes()
{
    for (const auto &v : db_->volumes())
    {
        VolumeInfo info;
        info.id = v.id;
        info.guidPath = v.guid;
        info.handle = System::openVolumeHandle(v.guid);
        info.lastUsn = static_cast<USN>(v.lastUsn);
        volumes_.insert(info.id, info);
    }
}

void FileMonitor::saveJournalState()
{
    for (const VolumeInfo &vol : std::as_const(volumes_))
        if (db_->saveJournalState(vol.id, static_cast<qlonglong>(vol.lastUsn)))
            qDebug() << "volume" << vol.guidPath << "last record"
                     << static_cast<qlonglong>(vol.lastUsn) << "saved";
}

FileMonitor::VolumeInfo *FileMonitor::volumeById(int id)
{
    auto it = volumes_.find(id);
    return it == volumes_.end() ? nullptr : &it.value();
}

bool FileMonitor::reopenById
(
    const VolumeInfo &vol,
    const QByteArray &systemId,
    QString &currentPath
) const
{
    currentPath = System::pathByFileId(vol.handle, systemId);
    return !currentPath.isEmpty();
}

FileMonitor::Fate FileMonitor::traceFateFromJournal
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
    qint64 deleteTime = 0;

    System::scanUsnJournal
    (
        origin->handle, origin->lastUsn, USN_REASON_FILE_DELETE,
        [&](const System::UsnEvent &e)
        {
            if (e.fileId != systemId) return true;

            deleteFound = true;
            deleteTime = e.timestamp;
            return false;
        }
    );

    if (!deleteFound)
    {
        qDebug() << "WARN: no delete record found for" << systemId.toHex()
                 << "on volume" << originVolumeId;
        return Fate::Unchanged;
    }

    for (auto &vol : volumes_)
    {
        if (vol.id == originVolumeId) continue;

        bool created = false;
        QByteArray createdId;

        System::scanUsnJournal
        (
            vol.handle, vol.lastUsn, USN_REASON_FILE_CREATE,
            [&](const System::UsnEvent &e)
            {
                if (e.timestamp < deleteTime) return true;
                if (e.name.compare(fileName, Qt::CaseInsensitive) != 0) return true;

                created = true;
                createdId = e.fileId;
                return false;
            }
        );

        if (created)
        {
            newSystemId = createdId;
            newVolumeId = vol.id;

            const QString path = System::pathByFileId(vol.handle, newSystemId);
            if (!path.isEmpty()) newPath = path;

            return Fate::Moved;
        }
    }
    return Fate::Deleted;
}

void FileMonitor::reconcileStartup()
{
    for (const auto &row : db_->files())
    {
        VolumeInfo *vol = volumeById(row.volumeId);
        if (!vol)
        {
            qDebug() << "WARN: unknown volume_id" << row.volumeId << "for" << row.path;
            continue;
        }

        const QString actualPath = System::pathByFileId(vol->handle, row.systemId);
        const bool stillOnVolume = !actualPath.isEmpty();

        if (stillOnVolume)
        {
            if (actualPath.compare(row.path, Qt::CaseInsensitive) != 0)
            {
                qDebug() << "-> path changed:" << row.path << "->" << actualPath;
                db_->replaceFile(row.systemId, Storage::FileRecord{ actualPath, row.systemId, row.volumeId });
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
                db_->replaceFile(row.systemId, Storage::FileRecord{ newPath, newSystemId, newVolumeId });
                addWatch(newSystemId, newPath, newVolumeId);
                break;
            case Fate::Deleted:
                qDebug() << "-> file deleted from system:" << row.path;
                db_->replaceFile(row.systemId, std::nullopt);
                emit fileDeleted(row.path, row.systemId);
                break;
            case Fate::Unchanged:
                break;
        }
    }
}

bool FileMonitor::addFile(const QString &path, int volumeId)
{
    const QByteArray systemId = System::fileId128ForPath(path);
    if (systemId.isEmpty()) return false;
    return addWatch(systemId, path, volumeId);
}

bool FileMonitor::addWatch(const QByteArray &systemId, const QString &path, int volumeId)
{
    HANDLE handle = System::openFileForWatch(path);
    if (!System::isValidHandle(handle)) return false;

    WatchedFile wf;
    wf.systemId = systemId;
    wf.handle = handle;
    wf.path = QFileInfo(path).absoluteFilePath();
    wf.volumeId = volumeId;

    watched_.insert(systemId, wf);
    return true;
}

void FileMonitor::onNewFileAdded(const QString &path)
{
    if (!addFile(path, 1))
        qWarning() << "WARN: cannot watch" << path;
}

void FileMonitor::processEvent(const System::UsnEvent &e)
{
    const QByteArray &fileId = e.fileId;

    auto it = watched_.find(fileId);
    if (it == watched_.end()) return;

    qDebug() << "-> USN record: fileId =" << fileId.toHex()
             << "reason =" << Qt::hex << e.reason << "name =" << e.name;

    if (e.reason & USN_REASON_RENAME_OLD_NAME)
        return;

    if (e.reason & USN_REASON_RENAME_NEW_NAME)
    {
        const QString oldPath = it->path;
        const QString newPath = System::pathByHandle(it->handle);
        qDebug() << "-> rename new:" << oldPath << "->" << newPath;
        if (newPath.isEmpty() || newPath == oldPath) return;

        it->path = newPath;
        const int volumeId = it->volumeId;
        QMetaObject::invokeMethod(this, [this, fileId, oldPath, newPath, volumeId]()
        {
            qDebug() << "-> apply rename" << newPath;
            db_->replaceFile(fileId, Storage::FileRecord{ newPath, fileId, volumeId });
            emit filePathChanged(oldPath, newPath);
        }, Qt::QueuedConnection);
        return;
    }

    if (e.reason & USN_REASON_FILE_DELETE)
    {
        const QString path = it->path;
        System::closeHandle(it->handle);
        watched_.erase(it);
        pendingRenames_.remove(fileId);

        QMetaObject::invokeMethod(this, [this, fileId, path]()
        {
            db_->replaceFile(fileId, std::nullopt);
            emit fileDeleted(path, fileId);
        }, Qt::QueuedConnection);
    }
}

void FileMonitor::run()
{
    constexpr DWORD reasonMask =
        USN_REASON_RENAME_OLD_NAME |
        USN_REASON_RENAME_NEW_NAME |
        USN_REASON_FILE_DELETE;

    while (!isInterruptionRequested())
    {
        bool anyVolume = false;

        for (auto &vol : volumes_)
        {
            if (!System::isValidHandle(vol.handle)) continue;
            anyVolume = true;

            if (!vol.lastUsn)
            {
                const auto n = System::queryNextUsn(vol.handle);
                if (!n) continue;
                vol.lastUsn = *n;
            }

            if (const auto next = System::scanUsnJournal
            (
                vol.handle, vol.lastUsn, reasonMask,
                [this](const System::UsnEvent &e) { processEvent(e); return true; }
            ))
                vol.lastUsn = *next;
        }

        if (!anyVolume) break;
        msleep(200);
    }
}