#include <windows.h>
#include <winioctl.h>

#include <QThread>
#include <QString>
#include <QDir>
#include <QByteArray>
#include <QHash>
#include <QSqlDatabase>
#include <functional>

#include "System.h"

class UsnJournalMonitor : public QThread
{
    Q_OBJECT

public:
    explicit UsnJournalMonitor(QObject *parent = nullptr);
    ~UsnJournalMonitor() override;

    void reconcileStartup();
    bool addFile(const QString &path, int volumeId);

signals:
    void fileDeleted(const QString &path, const QByteArray &systemId);
    void filePathChanged(const QString &oldPath, const QString &newPath);

public slots:     
    void onNewFileAdded(const QString &path);

protected:
    void run() override;

private:
    void movefile
    (
        const QString &oldPath, 
        const QString &newPath, 
        const QByteArray &oldSystemId, 
        const QByteArray &newSystemId
    );

    struct WatchedFile
    {
        QByteArray systemId;
        HANDLE     handle = INVALID_HANDLE_VALUE;
        QString    path;
        int        volumeId = 0;
    };

    struct VolumeInfo
    {
        int      id = 0;
        QString  guidPath;                    
        HANDLE   handle = INVALID_HANDLE_VALUE; 
        USN      lastUsn = 0;                
    };

    struct PendingRename
    {
        QString oldName;
        QByteArray oldParentId;
    };

    QHash<QByteArray, WatchedFile> watched_; 
    QHash<int, VolumeInfo> volumes_;       
    QHash<QByteArray, PendingRename> pendingRenames_;  

    bool reopenById
    (
        const VolumeInfo &vol, 
        const QByteArray &systemId, 
        QString &currentPath
    ) const;

    enum class Fate { Unchanged, Moved, Deleted };

    Fate traceFateFromJournal
    (
        const QByteArray &systemId,
        const QString &fileName,
        int originVolumeId,
        QByteArray &newSystemId,
        QString &newPath,
        int &newVolumeId
    );

    // void scanJournal
    // (
    //     const VolumeInfo &vol, 
    //     USN startUsn, 
    //     DWORD reasonMask,
    //     const std::function<bool(const USN_RECORD *)> &callback
    // ) const;

    //void processRecord(const USN_RECORD *record, int volumeId);
    void processEvent(const System::UsnEvent &e);
    bool addWatch(const QByteArray &systemId, const QString &path, int volumeId);

    void loadVolumes();
    void updateFilePath(const QByteArray &systemId, const QString &newPath);
    void updateFileIdentity
    (
        const QByteArray &oldSystemId, 
        const QByteArray &newSystemId,
        const QString &newPath, 
        int newVolumeId
    );
    void deleteFileRecord(const QByteArray &systemId);
    void saveJournalState();

    VolumeInfo *volumeById(int id);
    static HANDLE openVolumeHandle(const QString &guidPath);
    static QString getPathByHandle(HANDLE handle);
    static QByteArray fileId128ToBytes(const FILE_ID_128 &id);
    static FILE_ID_128 bytesToFileId128(const QByteArray &bytes);
    static QByteArray fileId128ToBytes(DWORDLONG fileReference);
    static QByteArray currentSystemIdForPath(const QString &path);
};