#pragma once

#include <QByteArray>
#include <QFileInfo>
#include <QHash>
#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <QString>

#include "System.h"

struct WatchedFile
{
    QByteArray systemId;
    QString path;
    QString userPath;
    bool inTrash = false;
    int volumeId = 0;
    qint64 size = 0;
    QByteArray sha256;

    bool isValid() const { return !systemId.isEmpty(); }

    QString shownPath() const { return userPath.isEmpty() ? path : userPath; }
};

class WatchList
{
public:
    WatchList() = default;
    WatchList(const WatchList &) = delete;
    WatchList &operator=(const WatchList &) = delete;

    static QByteArray key(int volumeId, const QByteArray &id)
    {
        QByteArray k(reinterpret_cast<const char *>(&volumeId), sizeof(volumeId));
        k.append(id);
        return k;
    }

    bool add(int volumeId, const QByteArray &id, const QString &path)
    {
        if (id.isEmpty()) return false;

        const QString abs = QFileInfo(path).absoluteFilePath();
        const QByteArray k = key(volumeId, id);
        WatchedFile wf{ id, abs, abs, false, volumeId, 0, {} };

        QMutexLocker lock(&mutex_);
        if (auto it = files_.find(k); it != files_.end())
        {
            wf.size = it->size;
            wf.sha256 = it->sha256;
        }
        files_.insert(k, wf);
        return true;
    }

    WatchedFile find(int volumeId, const QByteArray &id) const
    {
        QMutexLocker lock(&mutex_);
        return files_.value(key(volumeId, id));
    }

    QList<WatchedFile> snapshot() const
    {
        QMutexLocker lock(&mutex_);
        return files_.values();
    }

    void setHash(int volumeId, const QByteArray &id, qint64 size, const QByteArray &sha256)
    {
        QMutexLocker lock(&mutex_);
        if (auto it = files_.find(key(volumeId, id)); it != files_.end())
        {
            it->size = size;
            it->sha256 = sha256;
        }
    }

    void setTrash(int volumeId, const QByteArray &id, bool inTrash,
                  const QString &path, const QString &userPath)
    {
        QMutexLocker lock(&mutex_);
        if (auto it = files_.find(key(volumeId, id)); it != files_.end())
        {
            it->inTrash = inTrash;
            it->path = path;
            it->userPath = userPath;
        }
    }

    void remove(int volumeId, const QByteArray &id)
    {
        QMutexLocker lock(&mutex_);
        files_.remove(key(volumeId, id));
    }

private:
    mutable QMutex mutex_;
    QHash<QByteArray, WatchedFile> files_;
};
