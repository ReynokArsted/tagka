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

class Storage;  

class TagsProcessor : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
public:
    explicit TagsProcessor(Storage *db, QObject *parent = nullptr);

    std::optional<QVector<FileEntry>> getFilesByExpression(const QString &expression);
    Q_INVOKABLE QSet<int> tagIdsForFile(const QList<QString> &paths) const;

signals:
    void errorOccurred(const QString &message, bool critical = false);

private:
    Storage *m_db = nullptr;
};