#pragma once

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringList>
#include <QString>
#include <QVariantMap>
#include <functional>
#include <optional>

class Storage : public QObject
{
    Q_OBJECT
public:
    enum State { Tracked = 0, AwaitingChoice = 1, AwaitingVolume = 2 };

    enum UserSetBit
    {
        DarkMode        = 0,
        English         = 1,
        DeleteWarnings  = 2,
        ShowHiddenFiles = 3,
    };
    static constexpr qint64 DefaultUserSet = qint64(1) << DeleteWarnings;

    struct Tag
    {
        int id;
        QString name;
        QString color;
    };
    struct Volume
    {
        int id;
        QString guid;
        qint64 lastUsn;
    };
    struct FileRecord
    {
        QString path;
        QByteArray systemId;
        int volumeId;
        qint64 size = 0;
        QByteArray sha256;
        int state = Tracked;
    };

    explicit Storage(QObject *parent = nullptr);

    bool open();

    QList<Tag> tags() const;
    int  addTag(const QString &name, const QString &color = QString());
    bool setTagColor(int tagId, const QString &color);
    bool removeTag(int tagId);

    qint64 userSet() const;
    bool saveUserSet(qint64 flags);
    Q_INVOKABLE QVariantMap userSettings() const;
    Q_INVOKABLE bool setUserSettings(const QVariantMap &settings);

    std::optional<QSet<QString>> filesForTag(const QString &tagName) const;
    QSet<int> commonTagIds(const QList<QString> &paths) const;

    std::optional<QStringList> assignTags
    (
        const QList<QString> &paths,
        const QSet<int> &desired,
        const std::function<QByteArray(const QString &)> &systemId,
        const std::function<int(const QString &)> &volumeOf
    );
    bool removeFileTags(const QList<QString> &paths);

    QList<FileRecord> files() const;

    bool replaceFile(int oldVolumeId, const QByteArray &oldSystemId,
                     const std::optional<FileRecord> &state);
    bool setHash(int volumeId, const QByteArray &systemId, qint64 size, const QByteArray &sha256);
    bool setState(int volumeId, const QByteArray &systemId, int state);

    QList<Volume> volumes() const;
    int volumeIdByGuid(const QString &guidPath) const;
    int addVolume(const QString &guidPath);
    bool saveJournalState(int volumeId, qint64 usn);

private:
    QSqlDatabase database() const;
    bool migrate();
    bool run(QSqlQuery &q, const QString &sql, const QVariantMap &binds = {}) const;
    bool beginTransaction() const;
    bool endTransaction(bool commit) const;

    bool insertMissingFiles
    (
        QSqlQuery &q, const QList<QString> &paths, QStringList &created,
        const std::function<QByteArray(const QString &)> &systemId,
        const std::function<int(const QString &)> &volumeOf
    ) const;
    bool syncTags(QSqlQuery &q, const QList<QString> &paths, const QSet<int> &desired) const;

    QString m_connectionName = "app_connection";
};
