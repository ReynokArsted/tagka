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
    struct Tag
    { 
        int id; 
        QString name; 
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
    };

    explicit Storage(QObject *parent = nullptr);

    bool open();

    QList<Tag> tags() const;
    int  addTag(const QString &name);           
    bool removeTag(int tagId);                  

    std::optional<QSet<QString>> filesForTag(const QString &tagName) const;
    QSet<int> commonTagIds(const QList<QString> &paths) const;
    std::optional<QStringList> assignTags
    (
        const QList<QString> &paths, 
        const QSet<int> &desired,
        const std::function<QByteArray(const QString &)> &systemId,
        int volumeId
    );

    QList<FileRecord> files() const;
    bool replaceFile(const QByteArray &oldSystemId, const std::optional<FileRecord> &state);

    QList<Volume> volumes() const;
    bool saveJournalState(int volumeId, qint64 usn);

private:
    QSqlDatabase database() const;
    bool run(QSqlQuery &q, const QString &sql, const QVariantMap &binds = {}) const;
    bool inTransaction(const std::function<bool(QSqlQuery &)> &body) const;

    QString m_connectionName = "app_connection";
};