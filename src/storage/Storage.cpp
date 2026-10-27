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
    return migrate();
}

bool Storage::migrate()
{
    const auto addColumns = [this](const QString &table, const QList<QPair<QString, QString>> &additions)
    {
        QSqlQuery q;
        if (!run(q, QStringLiteral("PRAGMA table_info(%1)").arg(table)))
            return false;

        QSet<QString> columns;
        while (q.next())
            columns.insert(q.value(1).toString());

        for (const auto &[name, ddl] : additions)
        {
            if (!columns.contains(name))
            {
                QSqlQuery alter;
                if (!run(alter, ddl)) return false;
            }
        }
        return true;
    };

    QSqlQuery q;
    return addColumns("file",
           {
               { "size",   "ALTER TABLE file ADD COLUMN size INTEGER" },
               { "sha256", "ALTER TABLE file ADD COLUMN sha256 TEXT" },
               { "state",  "ALTER TABLE file ADD COLUMN state INTEGER NOT NULL DEFAULT 0" },
           })
        && addColumns("tag",
           {
               { "color",  "ALTER TABLE tag ADD COLUMN color TEXT" },
           })
        && run(q, "CREATE TABLE IF NOT EXISTS user_set("
                  "id INTEGER PRIMARY KEY CHECK(id = 1), "
                  "flags INTEGER NOT NULL)");
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

namespace
{
    QString normalizeVolumeGuid(QString s)
    {
        s = s.trimmed();
        if (s.startsWith('"') && s.endsWith('"')) s = s.mid(1, s.size() - 2);
        while (s.endsWith('\\')) s.chop(1);
        s.replace(QStringLiteral("\\\\.\\"), QStringLiteral("\\\\?\\"));
        return s.toLower();
    }
}

bool Storage::beginTransaction() const
{
    QSqlDatabase db = database();
    const bool ok = db.transaction();
    if (!ok)
        qWarning() << "ERROR: transaction failed:" << db.lastError().text();
    return ok;
}

bool Storage::endTransaction(bool commit) const
{
    QSqlDatabase db = database();
    bool ok = false;
    if (commit)
        ok = db.commit();
    else
        db.rollback();
    return ok;
}

QList<Storage::Tag> Storage::tags() const
{
    QList<Tag> result;
    QSqlQuery q;
    if (!run(q, "SELECT id, tag_name, COALESCE(color, '') FROM tag"))
        return result;

    while (q.next())
        result.push_back({ q.value(0).toInt(), q.value(1).toString(), q.value(2).toString() });
    return result;
}

int Storage::addTag(const QString &name, const QString &color)
{
    const QVariant colorValue = color.isEmpty() ? QVariant(QMetaType(QMetaType::QString))
                                                : QVariant(color);
    QSqlQuery q;
    if (!run(q, "INSERT INTO tag(tag_name, color) VALUES(:name, :color)",
             { {":name", name}, {":color", colorValue} }))
        return -1;
    return q.lastInsertId().toInt();
}

bool Storage::setTagColor(int tagId, const QString &color)
{
    QSqlQuery q;
    return run(q, "UPDATE tag SET color = :color WHERE id = :id",
               { {":color", color}, {":id", tagId} });
}

namespace
{
    const QList<QPair<QString, Storage::UserSetBit>> &userSetKeys()
    {
        static const QList<QPair<QString, Storage::UserSetBit>> keys
        {
            { "darkMode",           Storage::DarkMode },
            { "english",            Storage::English },
            { "showDeleteWarnings", Storage::DeleteWarnings },
            { "showHiddenFiles",    Storage::ShowHiddenFiles },
        };
        return keys;
    }
}

qint64 Storage::userSet() const
{
    QSqlQuery q;
    if (!run(q, "SELECT flags FROM user_set WHERE id = 1") || !q.next())
        return DefaultUserSet;
    return q.value(0).toLongLong();
}

bool Storage::saveUserSet(qint64 flags)
{
    QSqlQuery q;
    return run(q, "INSERT OR REPLACE INTO user_set(id, flags) VALUES(1, :flags)",
               { {":flags", flags} });
}

QVariantMap Storage::userSettings() const
{
    const qint64 flags = userSet();
    QVariantMap result;
    for (const auto &[key, bit] : userSetKeys())
        result.insert(key, (flags & (qint64(1) << bit)) != 0);
    return result;
}

bool Storage::setUserSettings(const QVariantMap &settings)
{
    qint64 flags = userSet();
    for (const auto &[key, bit] : userSetKeys())
    {
        if (!settings.contains(key)) continue;

        const qint64 mask = qint64(1) << bit;
        if (settings.value(key).toBool()) flags |= mask;
        else                              flags &= ~mask;
    }
    return saveUserSet(flags);
}

bool Storage::removeTag(int tagId)
{
    bool ok = beginTransaction();
    if (ok)
    {
        QSqlQuery q(database());
        const QVariantMap bind{ {":id", tagId} };
        const bool deleted = run(q, "DELETE FROM tag_file WHERE tag_id = :id", bind)
                          && run(q, "DELETE FROM tag WHERE id = :id", bind);
        ok = endTransaction(deleted);
    }
    return ok;
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

    for (auto it = paths.cbegin(); it != paths.cend() && (first || !result.isEmpty()); ++it)
    {
        const QString &path = *it;
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
    }
    return result;
}

std::optional<QStringList> Storage::assignTags
(
    const QList<QString> &paths,
    const QSet<int> &desired,
    const std::function<QByteArray(const QString &)> &systemId,
    const std::function<int(const QString &)> &volumeOf
)
{
    QStringList created;
    bool ok = beginTransaction();
    if (ok)
    {
        QSqlQuery q(database());
        const bool done = insertMissingFiles(q, paths, created, systemId, volumeOf)
                       && syncTags(q, paths, desired);
        ok = endTransaction(done);
    }

    std::optional<QStringList> result;
    if (ok)
        result = created;
    return result;
}

bool Storage::removeFileTags(const QList<QString> &paths)
{
    bool ok = beginTransaction();
    if (ok)
    {
        QSqlQuery q(database());
        bool done = true;
        for (auto it = paths.cbegin(); done && it != paths.cend(); ++it)
            done = run(q, "DELETE FROM tag_file "
                          "WHERE file_id IN (SELECT id FROM file WHERE path = :path)",
                       { {":path", *it} });
        ok = endTransaction(done);
    }
    return ok;
}

bool Storage::insertMissingFiles
(
    QSqlQuery &q, const QList<QString> &paths, QStringList &created,
    const std::function<QByteArray(const QString &)> &systemId,
    const std::function<int(const QString &)> &volumeOf
) const
{
    bool ok = true;
    for (auto it = paths.cbegin(); ok && it != paths.cend(); ++it)
    {
        const QString &path = *it;
        ok = run(q, "SELECT 1 FROM file WHERE path = :path", { {":path", path} });
        if (ok && !q.next())
        {
            const QByteArray sid = systemId(path);
            const int vid = volumeOf(path);
            ok = !sid.isEmpty() && vid != 0;
            if (!ok)
                qWarning() << "ERROR: cannot get system id or volume for" << path;
            else
                ok = run(q, "INSERT INTO file(path, system_id, volume_id) VALUES(:path, :sid, :vid)",
                         { {":path", path}, {":sid", QString::fromLatin1(sid.toHex())}, {":vid", vid} });
            if (ok)
                created << path;
        }
    }
    return ok;
}

bool Storage::syncTags(QSqlQuery &q, const QList<QString> &paths, const QSet<int> &desired) const
{
    const QSet<int> common = commonTagIds(paths);
    bool ok = true;

    for (auto it = paths.cbegin(); ok && it != paths.cend(); ++it)
    {
        const QString &path = *it;

        for (int tagId : common - desired)
            ok = ok && run(q, "DELETE FROM tag_file WHERE tag_id = :tag "
                              "AND file_id = (SELECT id FROM file WHERE path = :path)",
                           { {":tag", tagId}, {":path", path} });

        for (int tagId : desired - common)
            ok = ok && run(q, "INSERT INTO tag_file(tag_id, file_id) "
                              "SELECT :tag, file.id FROM file WHERE file.path = :path "
                              "AND NOT EXISTS (SELECT 1 FROM tag_file "
                              "                WHERE tag_id = :tag AND file_id = file.id)",
                           { {":tag", tagId}, {":path", path} });
    }
    return ok;
}

QList<Storage::FileRecord> Storage::files() const
{
    QList<FileRecord> result;
    QSqlQuery q;
    if (!run(q, "SELECT path, system_id, volume_id, COALESCE(size, 0), sha256, COALESCE(state, 0) FROM file"))
        return result;

    while (q.next())
        result.push_back({ q.value(0).toString(),
                           QByteArray::fromHex(q.value(1).toByteArray()),
                           q.value(2).toInt(),
                           q.value(3).toLongLong(),
                           QByteArray::fromHex(q.value(4).toByteArray()),
                           q.value(5).toInt() });
    return result;
}

bool Storage::replaceFile(int oldVolumeId, const QByteArray &oldSystemId,
                          const std::optional<FileRecord> &state)
{
    QSqlQuery q;
    const QString osid = QString::fromLatin1(oldSystemId.toHex());

    if (!state)
        return run(q, "DELETE FROM file WHERE system_id = :osid AND volume_id = :ovid",
                   { {":osid", osid}, {":ovid", oldVolumeId} });

    return run(q,
        "UPDATE file SET path = :path, system_id = :nsid, volume_id = :vid, state = 0 "
        "WHERE system_id = :osid AND volume_id = :ovid",
        {
            {":path", state->path},
            {":nsid", QString::fromLatin1(state->systemId.toHex())},
            {":vid",  state->volumeId},
            {":osid", osid},
            {":ovid", oldVolumeId}
        });
}

bool Storage::setHash(int volumeId, const QByteArray &systemId, qint64 size, const QByteArray &sha256)
{
    QSqlQuery q;
    const QVariant hash = sha256.isEmpty() ? QVariant(QMetaType(QMetaType::QString))
                                           : QVariant(QString::fromLatin1(sha256.toHex()));
    return run(q, "UPDATE file SET size = :size, sha256 = :hash "
                  "WHERE system_id = :sid AND volume_id = :vid",
               { {":size", size}, {":hash", hash},
                 {":sid", QString::fromLatin1(systemId.toHex())}, {":vid", volumeId} });
}

bool Storage::setState(int volumeId, const QByteArray &systemId, int state)
{
    QSqlQuery q;
    return run(q, "UPDATE file SET state = :state WHERE system_id = :sid AND volume_id = :vid",
               { {":state", state},
                 {":sid", QString::fromLatin1(systemId.toHex())}, {":vid", volumeId} });
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

int Storage::volumeIdByGuid(const QString &guidPath) const
{
    const QString want = normalizeVolumeGuid(guidPath);
    if (want.isEmpty()) return 0;
    for (const Volume &v : volumes())
        if (normalizeVolumeGuid(v.guid) == want) return v.id;
    return 0;
}

int Storage::addVolume(const QString &guidPath)
{
    if (const int existing = volumeIdByGuid(guidPath)) return existing;

    QSqlQuery q;
    if (!run(q, "INSERT INTO volume(guid) VALUES(:guid)", { {":guid", guidPath} }))
        return 0;
    return q.lastInsertId().toInt();
}

bool Storage::saveJournalState(int volumeId, qint64 usn)
{
    QSqlQuery q;
    if (!run(q, "UPDATE journal_state SET last_record = :usn WHERE volume = :vid",
             { {":usn", usn}, {":vid", volumeId} }))
        return false;

    if (q.numRowsAffected() > 0)
        return true;

    return run(q, "INSERT INTO journal_state(volume, system_id, last_record) "
                  "SELECT id, guid, :usn FROM volume WHERE id = :vid",
               { {":usn", usn}, {":vid", volumeId} });
}
