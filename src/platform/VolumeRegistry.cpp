#include "VolumeRegistry.h"

#include <QDateTime>
#include <QDebug>
#include <QMutexLocker>
#include <QSet>

#include "Storage.h"

namespace
{
    QString normalizeGuid(QString s)
    {
        s = s.trimmed();
        if (s.startsWith('"') && s.endsWith('"'))
            s = s.mid(1, s.size() - 2);
        while (s.endsWith('\\'))
            s.chop(1);
        s.replace(QStringLiteral("\\\\.\\"), QStringLiteral("\\\\?\\"));
        return s.toLower();
    }
}

VolumeRegistry::VolumeRegistry(Storage *db) : db_(db), notifier_(this)
{
    for (const auto &v : db_->volumes())
    {
        VolumeInfo info;
        info.id = v.id;
        info.guidPath = v.guid;
        info.handle = System::openVolumeHandle(v.guid);
        info.lastUsn = static_cast<USN>(v.lastUsn);
        attach(info);
        volumes_.insert(info.id, info);
    }
}

VolumeRegistry::~VolumeRegistry()
{
    for (auto &vol : volumes_)
    {
        if (vol.lastUsn == 0)
        {
            const USN usn = System::queryNextUsn(vol.handle);
            if (System::isValidUsn(usn)) vol.lastUsn = usn;
        }
    }

    save();

    for (auto &vol : volumes_)
    {
        detach(vol);
        System::closeHandle(vol.handle);
    }
}

void VolumeRegistry::attach(VolumeInfo &v)
{
    if (!v.notify && System::isValidHandle(v.handle))
        v.notify = notifier_.subscribe(v.handle);
}

void VolumeRegistry::detach(VolumeInfo &v)
{
    notifier_.unsubscribe(v.notify);
    v.notify = nullptr;
}

void VolumeRegistry::onReleaseRequested(HDEVNOTIFY notify)
{
    QMutexLocker lock(&mutex_);
    for (VolumeInfo &v : volumes_)
    {
        if (v.notify == notify)
        {
            qDebug() << "-> releasing volume for safe removal:" << v.guidPath;
            detach(v);
            System::closeHandle(v.handle);
            v.handle = INVALID_HANDLE_VALUE;
            v.releasedAt = QDateTime::currentMSecsSinceEpoch();
            return;
        }
    }
}

QList<int> VolumeRegistry::refresh()
{
    const QStringList mounted = System::mountedNtfsVolumes();

    QSet<QString> present;
    for (const QString &g : mounted)
        present.insert(normalizeGuid(g));

    QList<int> appeared;
    QMutexLocker lock(&mutex_);

    for (VolumeInfo &v : volumes_)
    {
        if (!present.contains(normalizeGuid(v.guidPath)))
        {
            if (System::isValidHandle(v.handle))
                qDebug() << "-> volume disconnected:" << v.guidPath;
            detach(v);
            System::closeHandle(v.handle);
            v.handle = INVALID_HANDLE_VALUE;
            v.releasedAt = 0;
        }
    }

    for (const QString &guid : mounted)
    {
        const int id = registerLocked(guid);
        if (id != 0 && reopenLocked(volumes_[id]))
            appeared.append(id);
    }
    return appeared;
}

int VolumeRegistry::registerLocked(const QString &guid)
{
    int id = idByGuid(guid);
    if (id == 0)
    {
        id = db_->addVolume(guid);
        if (id > 0)
        {
            VolumeInfo info;
            info.id = id;
            info.guidPath = guid;
            volumes_.insert(id, info);
            qDebug() << "-> new volume registered:" << guid << "id" << id;
        }
        else
        {
            qWarning() << "ERROR: cannot register volume" << guid;
            id = 0;
        }
    }
    return id;
}

bool VolumeRegistry::reopenLocked(VolumeInfo &v)
{
    const bool needsOpen = !System::isValidHandle(v.handle);
    const bool cooling = v.releasedAt && QDateTime::currentMSecsSinceEpoch() - v.releasedAt < 10'000;
    bool opened = false;

    if (needsOpen && !cooling)
    {
        v.releasedAt = 0;
        v.handle = System::openVolumeHandle(v.guidPath);
        opened = System::isValidHandle(v.handle);

        if (opened && !System::ensureUsnJournal(v.handle))
        {
            System::closeHandle(v.handle);
            v.handle = INVALID_HANDLE_VALUE;
            opened = false;
        }
        if (opened)
        {
            attach(v);
            v.lastUsn = 0;
        }
    }
    return opened;
}

int VolumeRegistry::idByGuid(const QString &guidPath) const
{
    QMutexLocker lock(&mutex_);
    const QString want = normalizeGuid(guidPath);
    int result = 0;

    for (auto it = volumes_.cbegin(); result == 0 && !want.isEmpty() && it != volumes_.cend(); ++it)
    {
        if (normalizeGuid(it->guidPath) == want)
            result = it->id;
    }
    return result;
}

HANDLE VolumeRegistry::handleOf(int id) const
{
    QMutexLocker lock(&mutex_);
    const auto it = volumes_.constFind(id);
    return it == volumes_.constEnd() ? INVALID_HANDLE_VALUE : it->handle;
}

QList<VolumeRef> VolumeRegistry::refs() const
{
    QMutexLocker lock(&mutex_);
    QList<VolumeRef> result;
    for (const VolumeInfo &v : std::as_const(volumes_))
        result.append({ v.id, v.handle });
    return result;
}

QList<QPair<int, qint64>> VolumeRegistry::positions() const
{
    QMutexLocker lock(&mutex_);
    QList<QPair<int, qint64>> result;
    for (const VolumeInfo &v : std::as_const(volumes_))
        if (v.lastUsn > 0)
            result.append({ v.id, static_cast<qint64>(v.lastUsn) });
    return result;
}

void VolumeRegistry::save()
{
    QMutexLocker lock(&mutex_);
    for (const VolumeInfo &vol : std::as_const(volumes_))
        if (db_->saveJournalState(vol.id, static_cast<qlonglong>(vol.lastUsn)))
            qDebug() << "volume" << vol.guidPath << "last record"
                     << static_cast<qlonglong>(vol.lastUsn) << "saved";
}
