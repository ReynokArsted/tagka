#pragma once

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QMetaType>
#include <QPair>
#include <QRunnable>
#include <QString>
#include <QThread>
#include <QThreadPool>
#include <atomic>

#include "FileHasher.h"
#include "MoveResolver.h"
#include "Storage.h"
#include "System.h"
#include "VolumeRegistry.h"
#include "WatchList.h"

using JournalPositions = QList<QPair<int, qint64>>;
Q_DECLARE_METATYPE(JournalPositions)

class FileMonitor : public QThread
{
    Q_OBJECT
public:
    explicit FileMonitor(Storage *db, QObject *parent = nullptr);
    ~FileMonitor() override;

    void checkFilesState(int onlyVolumeId = 0);
    bool addFile(const QString &path);
    int  volumeIdForPath(const QString &path);

    MoveResolver *moveChoice() { return &resolver_; }

public slots:
    void onNewFileAdded(const QString &path);

signals:
    void fileDeleted(const QString &path, const QByteArray &systemId);
    void filePathChanged(const QString &oldPath, const QString &newPath);

protected:
    void run() override;
    void timerEvent(QTimerEvent *event) override;

private slots:
    void storePathChange(int volumeId, const QByteArray &systemId, const QString &shown, const QString &newPath);
    void storeJournalPositions(const JournalPositions &positions);

private:
    void processEvent(int volumeId, const System::UsnEvent &e);
    void onRenamed(const WatchedFile &wf);
    void onModified(const WatchedFile &wf, bool closed);
    void pollVolume(VolumeInfo &vol);

    void checkFileState(const Storage::FileRecord &row);
    void restoreWatched(const Storage::FileRecord &row, const QString &actualPath);
    void refreshVolumes();

    Storage *db_;
    VolumeRegistry volumes_;
    WatchList watched_;
    QThreadPool pool_;
    std::atomic<bool> stopping_{false};
    FileHasher hasher_;
    MoveResolver resolver_;
};
