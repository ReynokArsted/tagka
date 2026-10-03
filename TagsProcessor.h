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

class TagsProcessor : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(QString connectionName READ connectionName WRITE setConnectionName NOTIFY connectionNameChanged)

public:
    explicit TagsProcessor(QObject *parent = nullptr);

    QString connectionName() const { return m_connectionName; }
    void setConnectionName(const QString &name);

    std::optional<QVector<FileEntry>> entries(const QString &expression) const;

    Q_INVOKABLE QSet<int> tagIdsForFile(const QList<QString> &paths) const;

signals:
    void connectionNameChanged();

private:
    QSqlDatabase database() const;

    QSet<QString> filesForTag(const QString &tagName) const;

    QString m_connectionName = QStringLiteral("app_connection");
};