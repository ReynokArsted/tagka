#include "UsnJournalMonitor.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QFileInfo>
#include <QDebug>
#include <cstring>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>

#include "System.h"

QByteArray UsnJournalMonitor::fileId128ToBytes(const FILE_ID_128 &id)
{
    return QByteArray(reinterpret_cast<const char *>(id.Identifier), sizeof(id.Identifier));
}

QByteArray UsnJournalMonitor::fileId128ToBytes(DWORDLONG fileReference)
{
    FILE_ID_128 id{};
    memcpy
    (
        id.Identifier,
        &fileReference,
        sizeof(fileReference)
    );
    return QByteArray
    (
        reinterpret_cast<const char*>(id.Identifier),
        sizeof(id.Identifier)
    );
}

FILE_ID_128 UsnJournalMonitor::bytesToFileId128(const QByteArray &bytes)
{
    FILE_ID_128 id{};
    if (bytes.size() == static_cast<int>(sizeof(id.Identifier)))
        memcpy(id.Identifier, bytes.constData(), sizeof(id.Identifier));
    return id;
}

QByteArray UsnJournalMonitor::currentSystemIdForPath(const QString &path)
{
    return System::fileId128ForPath(path);
}

QString UsnJournalMonitor::getPathByHandle(HANDLE handle) 
{ 
    return System::pathByHandle(handle); 
}

HANDLE  UsnJournalMonitor::openVolumeHandle(const QString &p) 
{ 
    return System::openVolumeHandle(p); 
}

UsnJournalMonitor::UsnJournalMonitor(QObject *parent) : QThread(parent)
{
    loadVolumes();
    reconcileStartup();
    qDebug() << "UsnJournalMonitor created" << this;
}

UsnJournalMonitor::~UsnJournalMonitor()
{
    requestInterruption();
    wait();

    for (auto &wf : watched_)
        System::closeHandle(wf.handle);

    for (auto &vol : volumes_)
        if (const auto usn = System::queryNextUsn(vol.handle))
            vol.lastUsn = *usn;

    saveJournalState();

    for (auto &vol : volumes_)
        System::closeHandle(vol.handle);
}

void UsnJournalMonitor::loadVolumes()
{
    QSqlDatabase db = QSqlDatabase::database("app_connection");
    if (!db.isOpen()) { qWarning() << "ERROR: database is not open"; return; }

    QSqlQuery vq(db);
    vq.prepare("SELECT id, guid FROM volume");
    if (!vq.exec()) { qWarning() << vq.lastError().text(); return; }

    while (vq.next())
    {
        VolumeInfo info;
        info.id = vq.value(0).toInt();
        info.guidPath = vq.value(1).toString();
        info.handle = System::openVolumeHandle(info.guidPath);

        QSqlQuery jq(db);
        jq.prepare("SELECT last_record FROM journal_state WHERE volume = :vid");
        jq.bindValue(":vid", info.id);
        info.lastUsn = 0;
        if (jq.exec() && jq.next())
            info.lastUsn = static_cast<USN>(jq.value(0).toLongLong());

        volumes_.insert(info.id, info);
    }
}

// void UsnJournalMonitor::saveJournalState()
// {
//     QSqlDatabase db = QSqlDatabase::database("app_connection");
//     if (!db.isOpen()) { qWarning() << "ERROR: database is not open"; return; }

//         QSqlQuery q(db);
//         q.prepare("UPDATE journal_state SET last_record = :usn WHERE volume = :vid");
//         q.bindValue(":vid", 1);
//         q.bindValue(":usn", static_cast<qlonglong>(volumes_[1].lastUsn));
//         if (!q.exec())
//             qWarning() << "ERROR: save journal_state failed:" << q.lastError().text();
//         qDebug() << "volume" << QString(volumes_[1].guidPath) << "last record" << static_cast<qlonglong>(volumes_[1].lastUsn) << "saved";
//     // for (const auto &vol : volumes_)
//     // {
//     //     QSqlQuery q(db);
//     //     q.prepare("INSERT INTO journal_state (volume, last_record) VALUES (:vid, :usn) "
//     //               "ON CONFLICT(volume) DO UPDATE SET last_record = :usn");
//     //     q.bindValue(":vid", vol.id);
//     //     q.bindValue(":usn", static_cast<qlonglong>(vol.lastUsn));
//     //     if (!q.exec())
//     //         qWarning() << "ERROR: save journal_state failed:" << q.lastError().text();
//     // }
// }

void UsnJournalMonitor::saveJournalState()
{
    QSqlDatabase db = QSqlDatabase::database("app_connection");
    if (!db.isOpen()) { qWarning() << "ERROR: database is not open"; return; }

    for (const VolumeInfo &vol : std::as_const(volumes_))
    {
        QSqlQuery q(db);
        q.prepare("UPDATE journal_state SET last_record = :usn WHERE volume = :vid");
        q.bindValue(":vid", vol.id);
        q.bindValue(":usn", static_cast<qlonglong>(vol.lastUsn));
        if (!q.exec())
            qWarning() << "ERROR: save journal_state failed:" << q.lastError().text();
        else
            qDebug() << "volume" << vol.guidPath << "last record"
                     << static_cast<qlonglong>(vol.lastUsn) << "saved";
    }
}

UsnJournalMonitor::VolumeInfo *UsnJournalMonitor::volumeById(int id)
{
    auto it = volumes_.find(id);
    return it == volumes_.end() ? nullptr : &it.value();
}

bool UsnJournalMonitor::reopenById
(
    const VolumeInfo &vol,
    const QByteArray &systemId,
    QString &currentPath
) const
{
    currentPath = System::pathByFileId(vol.handle, systemId);
    return !currentPath.isEmpty();
}

// void UsnJournalMonitor::scanJournal
// (
//     const VolumeInfo &vol,
//     USN startUsn,
//     DWORD reasonMask,
//     const std::function<bool(const USN_RECORD *)> &callback
// ) const
// {
//     System::scanUsnJournal(vol.handle, startUsn, reasonMask, callback);
// }

UsnJournalMonitor::Fate UsnJournalMonitor::traceFateFromJournal
(
    const QByteArray &systemId,
    const QString &fileName,
    int originVolumeId,
    QByteArray &newSystemId,
    QString &newPath,
    int &newVolumeId
)
{
    VolumeInfo *origin = volumeById(originVolumeId);
    if (!origin) return Fate::Unchanged;

    bool deleteFound = false;
    qint64 deleteTime = 0;

    // System::scanUsnJournal
    // (
    //     origin->handle, origin->lastUsn, USN_REASON_FILE_DELETE,
    //     [&](const USN_RECORD *rec) -> bool
    //     {
    //         if (System::usnRecordFileId(rec) != systemId)
    //             return true;

    //         deleteFound = true;
    //         deleteTime = System::usnRecordTimestamp(rec);
    //         return false;
    //     }
    // );

    System::scanUsnJournal
    (
        origin->handle, origin->lastUsn, USN_REASON_FILE_DELETE,
        [&](const System::UsnEvent &e)
        {
            if (e.fileId != systemId) return true;

            deleteFound = true;
            deleteTime = e.timestamp;
            return false;
        }
    );

    if (!deleteFound)
    {
        qDebug() << "WARN: no delete record found for" << systemId.toHex()
                 << "on volume" << originVolumeId;
        return Fate::Unchanged;
    }

    for (auto &vol : volumes_)
    {
        if (vol.id == originVolumeId) continue;

        bool created = false;
        QByteArray createdId;

        // System::scanUsnJournal
        // (
        //     vol.handle, vol.lastUsn, USN_REASON_FILE_CREATE,
        //     [&](const USN_RECORD *rec) -> bool
        //     {
        //         if (System::usnRecordTimestamp(rec) < deleteTime)
        //             return true;

        //         if (System::usnRecordFileName(rec).compare(fileName, Qt::CaseInsensitive) != 0)
        //             return true;

        //         created = true;
        //         createdId = System::usnRecordFileId(rec);
        //         return false;
        //     }
        // );
        // в цикле по томам
        System::scanUsnJournal
        (
            vol.handle, vol.lastUsn, USN_REASON_FILE_CREATE,
            [&](const System::UsnEvent &e)
            {
                if (e.timestamp < deleteTime) return true;
                if (e.name.compare(fileName, Qt::CaseInsensitive) != 0) return true;

                created = true;
                createdId = e.fileId;
                return false;
            }
        );

        if (created)
        {
            newSystemId = createdId;
            newVolumeId = vol.id;

            const QString path = System::pathByFileId(vol.handle, newSystemId);
            if (!path.isEmpty()) newPath = path;

            return Fate::Moved;
        }
    }
    return Fate::Deleted;
}

void UsnJournalMonitor::reconcileStartup()
{
    QSqlDatabase db = QSqlDatabase::database("app_connection");
    if (!db.isOpen()) { qWarning() << "ERROR: database is not open"; return; }

    QSqlQuery query(db);
    query.prepare("SELECT path, system_id, volume_id FROM file");
    if (!query.exec()) { qWarning() << query.lastError().text(); return; }

    struct Row { QString path; QByteArray systemId; int volumeId; };
    QVector<Row> rows;
    while (query.next())
    {
        rows.push_back({
            query.value(0).toString(),
            QByteArray::fromHex(query.value(1).toByteArray()),
            query.value(2).toInt()
        });
    }

    for (const Row &row : rows)
    {
        VolumeInfo *vol = volumeById(row.volumeId);
        if (!vol)
        {
            qDebug() << "WARN: unknown volume_id" << row.volumeId << "for" << row.path;
            continue;
        }

        const QString actualPath = System::pathByFileId(vol->handle, row.systemId);
        const bool stillOnVolume = !actualPath.isEmpty();

        if (stillOnVolume)
        {
            if (actualPath.compare(row.path, Qt::CaseInsensitive) != 0)
            {
                qDebug() << "-> path changed:" << row.path << "->" << actualPath;
                updateFilePath(row.systemId, actualPath);
            }

            addWatch(row.systemId, actualPath, row.volumeId);
            continue;
        }

        QByteArray newSystemId;
        QString newPath;
        int newVolumeId = 0;

        Fate fate = traceFateFromJournal
        (
            row.systemId, 
            QFileInfo(row.path).fileName(),
            row.volumeId, 
            newSystemId, 
            newPath, 
            newVolumeId
        );

        switch (fate)
        {
            case Fate::Moved:
                qDebug() << "-> file moved to another volume:" << row.path << "->" << newPath;
                updateFileIdentity(row.systemId, newSystemId, newPath, newVolumeId);
                addWatch(newSystemId, newPath, newVolumeId);
                break;
            case Fate::Deleted:
                qDebug() << "-> file deleted from system:" << row.path;
                deleteFileRecord(row.systemId);
                emit fileDeleted(row.path, row.systemId);
                break;
            case Fate::Unchanged:
                break;
        }
    }
}

void UsnJournalMonitor::updateFilePath(const QByteArray &systemId, const QString &newPath)
{
    QSqlDatabase db = QSqlDatabase::database("app_connection");
    QSqlQuery q(db);
    q.prepare("UPDATE file SET path = :path WHERE system_id = :sid");
    q.bindValue(":path", newPath);
    q.bindValue(":sid", QString(systemId.toHex()));
    if (!q.exec())
        qWarning() << "ERROR: update file path failed:" << q.lastError().text();
    qDebug() << "-> update rows:" << q.numRowsAffected();
}

void UsnJournalMonitor::updateFileIdentity
(
    const QByteArray &oldSystemId,
    const QByteArray &newSystemId,
    const QString &newPath, 
    int newVolumeId
)
{
    QSqlDatabase db = QSqlDatabase::database("app_connection");
    QSqlQuery q(db);
    q.prepare("UPDATE file SET system_id = :nsid, path = :path, volume_id = :vid "
              "WHERE system_id = :osid");
    q.bindValue(":nsid", QString(newSystemId.toHex()));
    q.bindValue(":path", newPath);
    q.bindValue(":vid", newVolumeId);
    q.bindValue(":osid", QString(oldSystemId.toHex()));
    if (!q.exec())
        qWarning() << "ERROR: update file identity failed:" << q.lastError().text();
}

void UsnJournalMonitor::deleteFileRecord(const QByteArray &systemId)
{
    QSqlDatabase db = QSqlDatabase::database("app_connection");
    QSqlQuery q(db);
    q.prepare("DELETE FROM file WHERE system_id = :sid");
    q.bindValue(":sid", QString(systemId.toHex()));
    if (!q.exec())
        qWarning() << "ERROR: delete file record failed:" << q.lastError().text();
}

bool UsnJournalMonitor::addFile(const QString &path, int volumeId)
{
    const QByteArray systemId = System::fileId128ForPath(path);
    if (systemId.isEmpty()) return false;
    return addWatch(systemId, path, volumeId);
}

bool UsnJournalMonitor::addWatch(const QByteArray &systemId, const QString &path, int volumeId)
{
    HANDLE handle = System::openFileForWatch(path);
    if (!System::isValidHandle(handle)) return false;

    WatchedFile wf;
    wf.systemId = systemId;
    wf.handle = handle;
    wf.path = QFileInfo(path).absoluteFilePath();
    wf.volumeId = volumeId;

    watched_.insert(systemId, wf);
    return true;
}

void UsnJournalMonitor::onNewFileAdded(const QString &path)
{
    if (!addFile(path, 1))
        qWarning() << "WARN: cannot watch" << path;
}

//void UsnJournalMonitor::processRecord(const USN_RECORD *record, int volumeId)
void UsnJournalMonitor::processEvent(const System::UsnEvent &e)
{
    const QByteArray &fileId = e.fileId;

    auto it = watched_.find(fileId);
    if (it == watched_.end()) return;

    qDebug() << "-> USN record: fileId =" << fileId.toHex()
             << "reason =" << Qt::hex << e.reason << "name =" << e.name;

    if (e.reason & USN_REASON_RENAME_OLD_NAME)
        return;

    if (e.reason & USN_REASON_RENAME_NEW_NAME)
    {
        const QString oldPath = it->path;
        const QString newPath = System::pathByHandle(it->handle);
        qDebug() << "-> rename new:" << oldPath << "->" << newPath;
        if (newPath.isEmpty() || newPath == oldPath) return;

        it->path = newPath;
        QMetaObject::invokeMethod(this, [this, fileId, oldPath, newPath]()
        {
            qDebug() << "-> apply rename" << newPath;
            updateFilePath(fileId, newPath);
            emit filePathChanged(oldPath, newPath);
        }, Qt::QueuedConnection);
        return;
    }

    if (e.reason & USN_REASON_FILE_DELETE)
    {
        const QString path = it->path;
        System::closeHandle(it->handle);
        watched_.erase(it);
        pendingRenames_.remove(fileId);

        QMetaObject::invokeMethod(this, [this, fileId, path]()
        {
            deleteFileRecord(fileId);
            emit fileDeleted(path, fileId);
        }, Qt::QueuedConnection);
    }
}

// void UsnJournalMonitor::run()
// {
//     constexpr DWORD reasonMask =
//         USN_REASON_RENAME_OLD_NAME |
//         USN_REASON_RENAME_NEW_NAME |
//         USN_REASON_FILE_DELETE;

//     while (!isInterruptionRequested())
//     {
//         bool anyVolume = false;

//         for (auto &vol : volumes_)
//         {
//             if (!System::isValidHandle(vol.handle)) continue;
//             anyVolume = true;

//             const auto next = System::readUsnJournalOnce
//             (
//                 vol.handle, vol.lastUsn, reasonMask,
//                 [&](const USN_RECORD *rec) { processRecord(rec, vol.id); }
//             );
//             if (next) vol.lastUsn = *next;
//         }

//         if (!anyVolume) break;
//         msleep(200);
//     }
// }

void UsnJournalMonitor::run()
{
    constexpr DWORD reasonMask =
        USN_REASON_RENAME_OLD_NAME |
        USN_REASON_RENAME_NEW_NAME |
        USN_REASON_FILE_DELETE;

    while (!isInterruptionRequested())
    {
        bool anyVolume = false;

        for (auto &vol : volumes_)
        {
            if (!System::isValidHandle(vol.handle)) continue;
            anyVolume = true;

            if (!vol.lastUsn)
            {
                const auto n = System::queryNextUsn(vol.handle);
                if (!n) continue;
                vol.lastUsn = *n;
            }

            if (const auto next = System::scanUsnJournal
            (
                vol.handle, vol.lastUsn, reasonMask,
                [this](const System::UsnEvent &e) { processEvent(e); return true; }
            ))
                vol.lastUsn = *next;
        }

        if (!anyVolume) break;
        msleep(200);
    }
}