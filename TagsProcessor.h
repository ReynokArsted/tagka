#pragma once

#include <QList>
#include <QObject>
#include <QSet>
#include <QSqlDatabase>
#include <QString>
#include <QVector>
#include <optional>
#include <QtQml/qqmlregistration.h>

#include "FileEntry.h"

class DataBaseModule;   // выше class TagsProcessor

class TagsProcessor : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    //Q_PROPERTY(QString connectionName READ connectionName WRITE setConnectionName NOTIFY connectionNameChanged)

public:



// было:  explicit TagsProcessor(QObject *parent = nullptr);
explicit TagsProcessor(DataBaseModule *db, QObject *parent = nullptr);


    // explicit TagsProcessor(QObject *parent = nullptr);

    // QString connectionName() const { return m_connectionName; }
    // void setConnectionName(const QString &name);

    std::optional<QVector<FileEntry>> getFilesByExpression(const QString &expression);

    Q_INVOKABLE QSet<int> tagIdsForFile(const QList<QString> &paths) const;

signals:
    // void connectionNameChanged();
    void errorOccurred(const QString &message, bool critical = false);

private:
// в private:
DataBaseModule *m_db = nullptr;
    // QSqlDatabase database() const;

    // QSet<QString> filesForTag(const QString &tagName) const;

    // QString m_connectionName = QStringLiteral("app_connection");
};