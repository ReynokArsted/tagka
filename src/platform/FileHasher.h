#pragma once

#include <QByteArray>
#include <QObject>
#include <QThreadPool>
#include <atomic>

#include "Storage.h"
#include "VolumeRegistry.h"
#include "WatchList.h"

class FileHasher : public QObject
{
    Q_OBJECT
public:
    FileHasher(Storage *db, VolumeRegistry &volumes, WatchList &watched,
               QThreadPool &pool, const std::atomic<bool> &stopping);

    void schedule(int volumeId, const QByteArray &fileId);
    void invalidate(int volumeId, const QByteArray &fileId);

private slots:
    void storeHash(int volumeId, const QByteArray &fileId, qint64 size, const QByteArray &sha256);

private:
    void hashFile(int volumeId, const QByteArray &fileId, HANDLE volumeHandle);

    Storage *db_;
    VolumeRegistry &volumes_;
    WatchList &watched_;
    QThreadPool &pool_;
    const std::atomic<bool> &stopping_;
};
