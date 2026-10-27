#include "MoveResolver.h"

#include <QDebug>
#include <QFileInfo>
#include <QSet>
#include <QtConcurrent>
#include <optional>

#include "System.h"

MoveResolver::MoveResolver(Storage *db, VolumeRegistry &volumes, WatchList &watched, FileHasher &hasher,
                           QThreadPool &pool, const std::atomic<bool> &stopping)
    : db_(db), volumes_(volumes), watched_(watched), hasher_(hasher), pool_(pool), stopping_(stopping)
{
    qRegisterMetaType<Candidate>();
    qRegisterMetaType<QList<Candidate>>();
    qRegisterMetaType<SearchResult>();
    qRegisterMetaType<WatchedFile>();

    flushTimer_.setSingleShot(true);
    connect(&flushTimer_, &QTimer::timeout, this, &MoveResolver::flush);
}

namespace
{
    constexpr int kQuietMs = 200;
    constexpr int kMaxWaitMs = 5000;

    QString lowerName(const WatchedFile &wf)
    {
        return QFileInfo(wf.shownPath()).fileName().toLower();
    }
}

void MoveResolver::resolvePath(const WatchedFile &wf)
{
    QMetaObject::invokeMethod(this, "enqueue", Qt::QueuedConnection, Q_ARG(WatchedFile, wf));
}

void MoveResolver::enqueue(const WatchedFile &wf)
{
    bool known = false;
    for (const WatchedFile &p : std::as_const(pending_))
        known = known || (p.volumeId == wf.volumeId && p.systemId == wf.systemId);

    if (pending_.isEmpty()) waitStarted_.start();
    if (!known) pending_.append(wf);

    const int left = kMaxWaitMs - int(waitStarted_.elapsed());
    flushTimer_.start(qMax(0, qMin(kQuietMs, left)));
}

void MoveResolver::flush()
{
    const QList<WatchedFile> batch = pending_;
    pending_.clear();

    if (!batch.isEmpty())
        QtConcurrent::run(&pool_, &MoveResolver::resolveBatch, this, batch);
}

void MoveResolver::resolveBatch(const QList<WatchedFile> &batch)
{
    QSet<QString> names;
    for (const WatchedFile &wf : batch)
        names.insert(lowerName(wf));

    const QList<VolumeRef> vols = volumes_.refs();
    NameIndex index;
    for (const VolumeRef &vol : vols)
    {
        if (System::isValidHandle(vol.handle))
        {
            index.insert(vol.id, System::enumByNames(vol.handle, names, &stopping_));
            qDebug() << "   volume" << vol.id << ": scanned for" << names.size() << "name(s)";
        }
    }

    HashCache cache;
    for (auto it = batch.cbegin(); !stopping_ && it != batch.cend(); ++it)
    {
        const WatchedFile &wf = *it;
        const QString shown = wf.shownPath();
        qDebug() << "-> file gone, searching other volumes:" << shown
                 << "size" << wf.size << "etalon hash" << (wf.sha256.isEmpty() ? "NONE" : "yes");

        const SearchResult r = findCandidates(wf, vols, index, cache);
        if (!stopping_)
        {
            qDebug() << "-> search result for" << shown << ": matches" << r.matches.size()
                     << "strong" << r.strong << "complete" << r.complete;

            QMetaObject::invokeMethod
            (
                this, 
                "applyResolution", 
                Qt::QueuedConnection,
                Q_ARG(WatchedFile, wf), 
                Q_ARG(QString, shown), 
                Q_ARG(SearchResult, r)
            );
        }
    }
}

SearchResult MoveResolver::findCandidates
(
    const WatchedFile &wf, 
    const QList<VolumeRef> &vols,
    const NameIndex &index, 
    HashCache &cache
)
{
    SearchResult r;
    QList<Candidate> strong, weak;
    const QString name = lowerName(wf);

    for (const VolumeRef &vol : vols)
    {
        const bool otherVolume = vol.id != wf.volumeId;
        const bool available = System::isValidHandle(vol.handle);

        if (otherVolume && !available)
            r.complete = false;
        else if (otherVolume && !scanVolume(vol, wf, index.value(vol.id).value(name), strong, weak, cache))
            return r;
    }

    r.strong = !strong.isEmpty();
    r.matches = r.strong ? strong : weak;
    return r;
}

bool MoveResolver::scanVolume
(
    const VolumeRef &vol, 
    const WatchedFile &wf, 
    const QList<QByteArray> &ids,
    QList<Candidate> &strong, 
    QList<Candidate> &weak, 
    HashCache &cache
)
{
    for (const QByteArray &id : ids)
    {
        if (stopping_) return false;
        probeCandidate(vol, id, wf, strong, weak, cache);
    }
    return true;
}

void MoveResolver::probeCandidate
(   
    const VolumeRef &vol, 
    const QByteArray &id, 
    const WatchedFile &wf,
    QList<Candidate> &strong, 
    QList<Candidate> &weak, 
    HashCache &cache
)
{
    HANDLE h = System::openFileById(vol.handle, id, FILE_GENERIC_READ);
    if (System::isValidHandle(h))
    {
        const QString path = System::pathByHandle(h);
        const qint64 size = System::fileSize(h);
        qDebug() << "   candidate" << path << "size" << size << "(wanted" << wf.size << ")";

        if (!path.isEmpty())
        {
            if (wf.sha256.isEmpty())
            {
                weak.append({ vol.id, id, path });
            }
            else if (size == wf.size)
            {
                const QByteArray key = WatchList::key(vol.id, id);
                QByteArray hash = cache.value(key);
                if (hash.isEmpty())
                {
                    qint64 hashedSize = 0;
                    hash = System::sha256OfHandle(h, hashedSize, &stopping_);
                    cache.insert(key, hash);
                }
                if (hash == wf.sha256)
                    strong.append({ vol.id, id, path });
            }
        }
        System::closeHandle(h);
    }
}

void MoveResolver::applyResolution(const WatchedFile &wf, const QString &shown, const SearchResult &r)
{
    if (r.strong && r.matches.size() == 1)
    {
        qDebug() << "-> moved automatically:" << shown << "->" << r.matches.first().path;
        moveRecord(wf, shown, r.matches.first());
        return;
    }

    if (!r.matches.isEmpty())
    {
        qDebug() << "-> user choice needed for" << shown << ":" << r.matches.size() << "candidate(s)";
        enqueueChoice({ wf, shown, r.matches });
        db_->setState(wf.volumeId, wf.systemId, Storage::AwaitingChoice);
        return;
    }

    if (r.complete)
    {
        qDebug() << "-> no candidates, record removed:" << shown;
        db_->replaceFile(wf.volumeId, wf.systemId, std::nullopt);
        emit fileDeleted(shown, wf.systemId);
        return;
    }

    db_->setState(wf.volumeId, wf.systemId, Storage::AwaitingVolume);
}

void MoveResolver::moveRecord(const WatchedFile &wf, const QString &shown, const Candidate &c)
{
    db_->replaceFile(wf.volumeId, wf.systemId,
                     Storage::FileRecord{ c.path, c.fileId, c.volumeId });

    if (watched_.add(c.volumeId, c.fileId, c.path))
    {
        watched_.setHash(c.volumeId, c.fileId, wf.size, wf.sha256);
        if (wf.sha256.isEmpty()) hasher_.schedule(c.volumeId, c.fileId);
    }
    emit filePathChanged(shown, c.path);
}

QString MoveResolver::oldPath() const
{
    return queue_.isEmpty() ? QString() : queue_.first().shown;
}

QStringList MoveResolver::candidates() const
{
    QStringList result;
    if (!queue_.isEmpty())
    {
        for (const Candidate &c : queue_.first().list)
            result << c.path;
    }
    return result;
}

void MoveResolver::enqueueChoice(const PendingChoice &pc)
{
    bool replaced = false;
    for (PendingChoice &q : queue_)
    {
        if (q.wf.volumeId == pc.wf.volumeId && q.wf.systemId == pc.wf.systemId)
        {
            q = pc;
            replaced = true;
        }
    }
    if (!replaced)
        queue_.append(pc);

    emit currentChanged();
}

void MoveResolver::choose(int index)
{
    if (!queue_.isEmpty())
    {
        const PendingChoice pc = queue_.takeFirst();
        emit currentChanged();

        if (index < 0 || index >= pc.list.size())
        {
            db_->replaceFile(pc.wf.volumeId, pc.wf.systemId, std::nullopt);
            emit fileDeleted(pc.shown, pc.wf.systemId);
        }
        else
        {
            moveRecord(pc.wf, pc.shown, pc.list.at(index));
        }
    }
}
