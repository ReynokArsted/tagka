#include "FileHasher.h"

#include <QMetaObject>
#include <QtConcurrent>

#include "System.h"

FileHasher::FileHasher(Storage *db, VolumeRegistry &volumes, WatchList &watched,
                       QThreadPool &pool, const std::atomic<bool> &stopping)
    : db_(db), volumes_(volumes), watched_(watched), pool_(pool), stopping_(stopping)
{
}

void FileHasher::schedule(int volumeId, const QByteArray &fileId)
{
    const HANDLE volumeHandle = volumes_.handleOf(volumeId);
    if (System::isValidHandle(volumeHandle))
        QtConcurrent::run(&pool_, &FileHasher::hashFile, this, volumeId, fileId, volumeHandle);
}

void FileHasher::invalidate(int volumeId, const QByteArray &fileId)
{
    watched_.setHash(volumeId, fileId, 0, {});
    QMetaObject::invokeMethod(this, "storeHash", Qt::QueuedConnection,
                              Q_ARG(int, volumeId), Q_ARG(QByteArray, fileId),
                              Q_ARG(qint64, qint64(0)), Q_ARG(QByteArray, QByteArray()));
}

void FileHasher::storeHash(int volumeId, const QByteArray &fileId, qint64 size, const QByteArray &sha256)
{
    db_->setHash(volumeId, fileId, size, sha256);
}

void FileHasher::hashFile(int volumeId, const QByteArray &fileId, HANDLE volumeHandle)
{
    HANDLE h = System::openFileById(volumeHandle, fileId, FILE_GENERIC_READ);
    if (!System::isValidHandle(h)) return;

    BY_HANDLE_FILE_INFORMATION before{}, after{};
    GetFileInformationByHandle(h, &before);

    qint64 size = 0;
    const QByteArray hash = System::sha256OfHandle(h, size, &stopping_);

    GetFileInformationByHandle(h, &after);
    System::closeHandle(h);

    const bool changed =
        before.nFileSizeLow  != after.nFileSizeLow  ||
        before.nFileSizeHigh != after.nFileSizeHigh ||
        before.ftLastWriteTime.dwLowDateTime  != after.ftLastWriteTime.dwLowDateTime ||
        before.ftLastWriteTime.dwHighDateTime != after.ftLastWriteTime.dwHighDateTime;

    if (!hash.isEmpty() && !changed)
    {
        watched_.setHash(volumeId, fileId, size, hash);
        QMetaObject::invokeMethod(this, "storeHash", Qt::QueuedConnection,
                                  Q_ARG(int, volumeId), Q_ARG(QByteArray, fileId),
                                  Q_ARG(qint64, size), Q_ARG(QByteArray, hash));
    }
}
