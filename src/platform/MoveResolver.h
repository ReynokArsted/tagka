#pragma once

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QMetaType>
#include <QObject>
#include <QElapsedTimer>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QThreadPool>
#include <atomic>

#include "FileHasher.h"
#include "Storage.h"
#include "VolumeRegistry.h"
#include "WatchList.h"

struct Candidate
{
    int volumeId = 0;
    QByteArray fileId;
    QString path;
};
Q_DECLARE_METATYPE(Candidate)

struct SearchResult
{
    QList<Candidate> matches;
    bool strong = false;
    bool complete = true;
};
Q_DECLARE_METATYPE(SearchResult)
Q_DECLARE_METATYPE(WatchedFile)

class MoveResolver : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool active READ active NOTIFY currentChanged)
    Q_PROPERTY(QString oldPath READ oldPath NOTIFY currentChanged)
    Q_PROPERTY(QStringList candidates READ candidates NOTIFY currentChanged)
    Q_PROPERTY(int waiting READ waiting NOTIFY currentChanged)
public:
    MoveResolver(Storage *db, VolumeRegistry &volumes, WatchList &watched, FileHasher &hasher,
                 QThreadPool &pool, const std::atomic<bool> &stopping);

    void resolvePath(const WatchedFile &wf);

    bool active() const { return !queue_.isEmpty(); }
    QString oldPath() const;
    QStringList candidates() const;
    int waiting() const { return qMax(0, int(queue_.size()) - 1); }

    Q_INVOKABLE void choose(int index);

signals:
    void fileDeleted(const QString &path, const QByteArray &systemId);
    void filePathChanged(const QString &oldPath, const QString &newPath);
    void currentChanged();

private slots:
    void enqueue(const WatchedFile &wf);
    void applyResolution(const WatchedFile &wf, const QString &shown, const SearchResult &r);

private:
    struct PendingChoice
    {
        WatchedFile wf;
        QString shown;
        QList<Candidate> list;
    };

    using NameIndex = QHash<int, QHash<QString, QList<QByteArray>>>;
    using HashCache = QHash<QByteArray, QByteArray>;

    void resolveBatch(const QList<WatchedFile> &batch);
    SearchResult findCandidates(const WatchedFile &wf, const QList<VolumeRef> &vols,
                                const NameIndex &index, HashCache &cache);
    bool scanVolume(const VolumeRef &vol, const WatchedFile &wf, const QList<QByteArray> &ids,
                    QList<Candidate> &strong, QList<Candidate> &weak, HashCache &cache);
    void probeCandidate(const VolumeRef &vol, const QByteArray &id, const WatchedFile &wf,
                        QList<Candidate> &strong, QList<Candidate> &weak, HashCache &cache);

    void flush();
    void enqueueChoice(const PendingChoice &pc);
    void moveRecord(const WatchedFile &wf, const QString &shown, const Candidate &c);

    Storage *db_;
    VolumeRegistry &volumes_;
    WatchList &watched_;
    FileHasher &hasher_;
    QThreadPool &pool_;
    const std::atomic<bool> &stopping_;
    QList<PendingChoice> queue_;

    QList<WatchedFile> pending_;
    QTimer flushTimer_;
    QElapsedTimer waitStarted_;
};
