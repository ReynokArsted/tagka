#include "FileMonitor.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QFileInfo>
#include <optional>

#include "Storage.h"

namespace
{
    constexpr DWORD kJournalReasonMask =
        USN_REASON_RENAME_OLD_NAME |
        USN_REASON_RENAME_NEW_NAME |
        USN_REASON_FILE_DELETE |
        USN_REASON_DATA_OVERWRITE |
        USN_REASON_DATA_EXTEND |
        USN_REASON_DATA_TRUNCATION |
        USN_REASON_CLOSE;

    bool isTrashPath(const QString &p)
    {
        return p.contains(QStringLiteral("/$Recycle.Bin/"), Qt::CaseInsensitive)
            || p.contains(QStringLiteral("/RECYCLER/"), Qt::CaseInsensitive);
    }
}

FileMonitor::FileMonitor(Storage *db, QObject *parent)
    : QThread(parent), db_(db), volumes_(db),
      hasher_(db, volumes_, watched_, pool_, stopping_),
      resolver_(db, volumes_, watched_, hasher_, pool_, stopping_)
{
    qRegisterMetaType<JournalPositions>("JournalPositions");

    connect(&resolver_, &MoveResolver::fileDeleted, this, &FileMonitor::fileDeleted);
    connect(&resolver_, &MoveResolver::filePathChanged, this, &FileMonitor::filePathChanged);

    pool_.setMaxThreadCount(2);
    volumes_.refresh();
    checkFilesState();
    startTimer(3000);
}

FileMonitor::~FileMonitor()
{
    stopping_ = true;
    requestInterruption();
    wait();
    pool_.waitForDone();
}

int FileMonitor::volumeIdForPath(const QString &path)
{
    const QString guid = System::volumeGuidForPath(path);
    int id = volumes_.idByGuid(guid);
    if (id == 0)
    {
        refreshVolumes();
        id = volumes_.idByGuid(guid);
    }
    return id;
}

void FileMonitor::timerEvent(QTimerEvent *)
{
    refreshVolumes();
}

void FileMonitor::refreshVolumes()
{
    for (const int id : volumes_.refresh())
    {
        qDebug() << "-> volume available:" << id;
        checkFilesState(id);
    }
}

bool FileMonitor::addFile(const QString &path)
{
    const int volumeId = volumeIdForPath(path);
    const QByteArray id = System::fileId128ForPath(path);
    if (volumeId == 0 || id.isEmpty()) return false;
    if (!watched_.add(volumeId, id, path)) return false;

    hasher_.schedule(volumeId, id);
    return true;
}

void FileMonitor::onNewFileAdded(const QString &path)
{
    if (!addFile(path))
        qWarning() << "WARN: cannot watch" << path;
}

void FileMonitor::checkFilesState(int onlyVolumeId)
{
    const bool all = onlyVolumeId == 0;

    for (const auto &row : db_->files())
    {
        const bool wanted = all || (row.state != Storage::AwaitingChoice
                                    && (row.volumeId == onlyVolumeId || row.state == Storage::AwaitingVolume));
        if (wanted)
            checkFileState(row);
    }
}

void FileMonitor::checkFileState(const Storage::FileRecord &row)
{
    const HANDLE volumeHandle = volumes_.handleOf(row.volumeId);
    if (!System::isValidHandle(volumeHandle))
    {
        qDebug() << "WARN: volume" << row.volumeId << "unavailable for" << row.path;
    }
    else
    {
        const QString actualPath = System::pathByFileId(volumeHandle, row.systemId);
        if (!actualPath.isEmpty())
        {
            restoreWatched(row, actualPath);
        }
        else
        {
            WatchedFile lost;
            lost.systemId = row.systemId;
            lost.path = row.path;
            lost.userPath = row.path;
            lost.volumeId = row.volumeId;
            lost.size = row.size;
            lost.sha256 = row.sha256;
            resolver_.resolvePath(lost);
        }
    }
}

void FileMonitor::restoreWatched(const Storage::FileRecord &row, const QString &actualPath)
{
    if (isTrashPath(actualPath))
    {
        if (watched_.add(row.volumeId, row.systemId, actualPath))
            watched_.setTrash(row.volumeId, row.systemId, true, actualPath, row.path);
    }
    else
    {
        if (actualPath.compare(row.path, Qt::CaseInsensitive) != 0)
        {
            qDebug() << "-> path changed:" << row.path << "->" << actualPath;
            db_->replaceFile(row.volumeId, row.systemId,
                             Storage::FileRecord{ actualPath, row.systemId, row.volumeId });
        }
        watched_.add(row.volumeId, row.systemId, actualPath);
    }

    watched_.setHash(row.volumeId, row.systemId, row.size, row.sha256);
    if (row.sha256.isEmpty()) hasher_.schedule(row.volumeId, row.systemId);
    if (row.state != Storage::Tracked)
        db_->setState(row.volumeId, row.systemId, Storage::Tracked);
}

void FileMonitor::processEvent(int volumeId, const System::UsnEvent &e)
{
    const WatchedFile wf = watched_.find(volumeId, e.fileId);
    if (!wf.isValid()) return;

    constexpr DWORD dataMask =
        USN_REASON_DATA_OVERWRITE | USN_REASON_DATA_EXTEND | USN_REASON_DATA_TRUNCATION;

    if (e.reason & USN_REASON_FILE_DELETE)
    {
        watched_.remove(wf.volumeId, wf.systemId);
        resolver_.resolvePath(wf);
    }
    else if (!(e.reason & USN_REASON_RENAME_OLD_NAME))
    {
        if (e.reason & USN_REASON_RENAME_NEW_NAME) onRenamed(wf);
        if (e.reason & dataMask)                   onModified(wf, e.reason & USN_REASON_CLOSE);
    }
}

void FileMonitor::onRenamed(const WatchedFile &wf)
{
    const QString newPath = System::pathByFileId(volumes_.handleOf(wf.volumeId), wf.systemId);
    if (newPath.isEmpty() || newPath.compare(wf.path, Qt::CaseInsensitive) == 0) return;

    const QString shown = wf.shownPath();

    if (isTrashPath(newPath))
    {
        watched_.setTrash(wf.volumeId, wf.systemId, true, newPath, shown);
        return;
    }

    watched_.setTrash(wf.volumeId, wf.systemId, false, newPath, newPath);
    if (newPath.compare(shown, Qt::CaseInsensitive) == 0) return;

    QMetaObject::invokeMethod(this, "storePathChange", Qt::QueuedConnection,
                              Q_ARG(int, wf.volumeId), Q_ARG(QByteArray, wf.systemId),
                              Q_ARG(QString, shown), Q_ARG(QString, newPath));
}

void FileMonitor::storePathChange(int volumeId, const QByteArray &systemId,
                                  const QString &shown, const QString &newPath)
{
    db_->replaceFile(volumeId, systemId, Storage::FileRecord{ newPath, systemId, volumeId });
    emit filePathChanged(shown, newPath);
}

void FileMonitor::onModified(const WatchedFile &wf, bool closed)
{
    hasher_.invalidate(wf.volumeId, wf.systemId);

    if (closed) hasher_.schedule(wf.volumeId, wf.systemId);
}

void FileMonitor::storeJournalPositions(const JournalPositions &positions)
{
    for (const auto &p : positions)
        db_->saveJournalState(p.first, p.second);
}

void FileMonitor::run()
{
    QElapsedTimer saveTimer;
    saveTimer.start();

    while (!isInterruptionRequested())
    {
        {
            QMutexLocker lock(&volumes_.mutex());
            for (VolumeInfo &vol : volumes_.all())
                pollVolume(vol);
        }

        if (saveTimer.elapsed() > 5000)
        {
            QMetaObject::invokeMethod(this, "storeJournalPositions", Qt::QueuedConnection,
                                      Q_ARG(JournalPositions, volumes_.positions()));
            saveTimer.restart();
        }
        msleep(200);
    }
}

void FileMonitor::pollVolume(VolumeInfo &vol)
{
    if (!System::isValidHandle(vol.handle)) return;

    if (!vol.lastUsn)
    {
        const USN start = System::queryNextUsn(vol.handle);
        if (!System::isValidUsn(start)) return;
        vol.lastUsn = start;
    }

    System::UsnReader reader(vol.handle, vol.lastUsn, kJournalReasonMask);
    System::UsnEvent event;
    while (reader.next(event))
        processEvent(vol.id, event);

    const USN next = reader.position();
    if (System::isValidUsn(next)) vol.lastUsn = next;
}
