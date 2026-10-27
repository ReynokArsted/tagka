#pragma once

#include <QHash>
#include <QList>
#include <QMutex>
#include <QPair>
#include <QString>
#include <functional>

#include "DeviceNotifier.h"
#include "System.h"

class Storage;

struct VolumeInfo
{
    int id = 0;
    QString guidPath;
    HANDLE handle = INVALID_HANDLE_VALUE;
    USN lastUsn = 0;
    HDEVNOTIFY notify = nullptr;
    qint64 releasedAt = 0;
};

struct VolumeRef
{
    int id = 0;
    HANDLE handle = INVALID_HANDLE_VALUE;
};

class VolumeRegistry : private DeviceNotifier::Listener
{
public:
    explicit VolumeRegistry(Storage *db);
    ~VolumeRegistry();

    VolumeRegistry(const VolumeRegistry &) = delete;
    VolumeRegistry &operator=(const VolumeRegistry &) = delete;

    QList<int> refresh();

    int idByGuid(const QString &guidPath) const;
    HANDLE handleOf(int id) const;

    QList<VolumeRef> refs() const;
    QList<QPair<int, qint64>> positions() const;
    QRecursiveMutex &mutex() { return mutex_; }
    QHash<int, VolumeInfo> &all() { return volumes_; }
    void save();

private:
    int registerLocked(const QString &guid);
    bool reopenLocked(VolumeInfo &v);

    void onReleaseRequested(HDEVNOTIFY notify) override;
    void attach(VolumeInfo &v);
    void detach(VolumeInfo &v);

    Storage *db_;
    DeviceNotifier notifier_;
    mutable QRecursiveMutex mutex_;
    QHash<int, VolumeInfo> volumes_;
};
