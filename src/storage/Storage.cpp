#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QSqlError>

#include "Storage.h"

Storage::Storage(QObject *parent)
    : QObject(parent) {}

bool Storage::open()
{
    if (QSqlDatabase::contains(m_connectionName))
    {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName);
        if (db.isOpen()) return true;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);

    const QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("test.db");
    db.setDatabaseName(dbPath);

    if (!db.open())
    {
        qWarning() << "ERROR: DB open failed with error" << db.lastError().text();
        return false;
    }
    return true;
}

QSqlDatabase Storage::database() const
{
    return QSqlDatabase::database(m_connectionName);
}


bool Storage::run(QSqlQuery &q, const QString &sql, const QVariantMap &binds) const
{
    if (!database().isOpen())
    {
        qWarning() << "ERROR: database is not open";
        return false;
    }

    q = QSqlQuery(database());
    q.prepare(sql);
    for (auto it = binds.cbegin(); it != binds.cend(); ++it)
        q.bindValue(it.key(), it.value());

    if (!q.exec())
    {
        qWarning() << "ERROR: query failed:" << sql << "->" << q.lastError().text();
        return false;
    }
    return true;
}

bool Storage::inTransaction(const std::function<bool(QSqlQuery &)> &body) const
{
    QSqlDatabase db = database();
    if (!db.transaction())
    {
        qWarning() << "ERROR: transaction failed:" << db.lastError().text();
        return false;
    }

    QSqlQuery q(db);
    if (body(q))
        return db.commit();

    db.rollback();
    return false;
}

QList<Storage::Tag> Storage::tags() const
{
    QList<Tag> result;
    QSqlQuery q;
    if (!run(q, "SELECT id, tag_name FROM tag"))
        return result;

    while (q.next())
        result.push_back({ q.value(0).toInt(), q.value(1).toString() });
    return result;
}

int Storage::addTag(const QString &name)
{
    QSqlQuery q;
    if (!run(q, "INSERT INTO tag(tag_name) VALUES(:name)", { {":name", name} }))
        return -1;
    return q.lastInsertId().toInt();
}

bool Storage::removeTag(int tagId)
{
    return inTransaction([&](QSqlQuery &q)
    {
        const QVariantMap bind{ {":id", tagId} };
        return run(q, "DELETE FROM tag_file WHERE tag_id = :id", bind)
            && run(q, "DELETE FROM tag WHERE id = :id", bind);
    });
}

std::optional<QSet<QString>> Storage::filesForTag(const QString &tagName) const
{
    QSqlQuery q;
    const bool ok = tagName.isEmpty()
        ? run(q, "SELECT file.path FROM tag_file "
                 "JOIN file ON tag_file.file_id = file.id "
                 "GROUP BY file.path")
        : run(q, "SELECT file.path FROM tag_file "
                 "JOIN file ON tag_file.file_id = file.id "
                 "JOIN tag ON tag_file.tag_id = tag.id "
                 "WHERE tag_name = :tag",
                 { {":tag", tagName} });
    if (!ok)
        return std::nullopt;

    QSet<QString> files;
    while (q.next())
        files.insert(q.value(0).toString());

    qDebug() << "-> tag_set:" << files;
    return files;
}

QSet<int> Storage::commonTagIds(const QList<QString> &paths) const
{
    QSet<int> result;
    bool first = true;

    for (const QString &path : paths)
    {
        QSqlQuery q;
        if (!run(q, "SELECT tag_file.tag_id FROM tag_file "
                    "JOIN file ON file.id = tag_file.file_id "
                    "WHERE file.path = :path",
                    { {":path", path} }))
            return {};

        QSet<int> ids;
        while (q.next())
            ids.insert(q.value(0).toInt());

        if (first) { result = ids; first = false; }
        else       result &= ids;

        if (result.isEmpty())
            break;
    }
    return result;
}

std::optional<QStringList> Storage::assignTags
(
    const QList<QString> &paths, 
    const QSet<int> &desired,
    const std::function<QByteArray(const QString &)> &systemId,
    int volumeId
)
{
    QStringList created;
    const bool ok = inTransaction([&](QSqlQuery &q)
    {
        created.clear();

        for (const QString &path : paths)
        {
            if (!run(q, "SELECT 1 FROM file WHERE path = :path", { {":path", path} }))
                return false;
            if (q.next())
                continue;

            const QByteArray sid = systemId(path);
            if (sid.isEmpty())
            {
                qWarning() << "ERROR: cannot get system id for" << path;
                return false;
            }
            if (!run(q, "INSERT INTO file(path, system_id, volume_id) VALUES(:path, :sid, :vid)",
                { {":path", path}, {":sid", QString::fromLatin1(sid.toHex())}, {":vid", volumeId} }))
                return false;
            created << path;
        }

        const QSet<int> common = commonTagIds(paths);
        for (const QString &path : paths)
        {
            for (int tagId : common - desired)
                if (!run(q, "DELETE FROM tag_file WHERE tag_id = :tag "
                            "AND file_id = (SELECT id FROM file WHERE path = :path)",
                            { {":tag", tagId}, {":path", path} }))
                    return false;

            for (int tagId : desired - common)
                if (!run(q, "INSERT INTO tag_file(tag_id, file_id) "
                            "SELECT :tag, file.id FROM file WHERE file.path = :path "
                            "AND NOT EXISTS (SELECT 1 FROM tag_file "
                            "                WHERE tag_id = :tag AND file_id = file.id)",
                            { {":tag", tagId}, {":path", path} }))
                    return false;
        }
        return true;
    });

    if (!ok)
        return std::nullopt;
    return created;
}

QList<Storage::FileRecord> Storage::files() const
{
    QList<FileRecord> result;
    QSqlQuery q;
    if (!run(q, "SELECT path, system_id, volume_id FROM file"))
        return result;

    while (q.next())
        result.push_back({ q.value(0).toString(),
                           QByteArray::fromHex(q.value(1).toByteArray()),
                           q.value(2).toInt() });
    return result;
}

bool Storage::replaceFile(const QByteArray &oldSystemId, const std::optional<FileRecord> &state)
{
    QSqlQuery q;
    const QString osid = QString::fromLatin1(oldSystemId.toHex());

    if (!state)
        return run(q, "DELETE FROM file WHERE system_id = :osid", { {":osid", osid} });

    return run(q,
        "UPDATE file SET path = :path, system_id = :nsid, volume_id = :vid WHERE system_id = :osid",
        {
            {":path", state->path},
            {":nsid", QString::fromLatin1(state->systemId.toHex())},
            {":vid",  state->volumeId},
            {":osid", osid}
        });
}

QList<Storage::Volume> Storage::volumes() const
{
    QList<Volume> result;
    QSqlQuery q;
    if (!run(q, "SELECT v.id, v.guid, COALESCE(j.last_record, 0) FROM volume v "
                "LEFT JOIN journal_state j ON j.volume = v.id"))
        return result;

    while (q.next())
        result.push_back({ q.value(0).toInt(), q.value(1).toString(), q.value(2).toLongLong() });
    return result;
}

bool Storage::saveJournalState(int volumeId, qint64 usn)
{
    QSqlQuery q;
    return run(q, "UPDATE journal_state SET last_record = :usn WHERE volume = :vid",
               { {":usn", usn}, {":vid", volumeId} });
}