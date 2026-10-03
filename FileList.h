#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>
#include <QtQml/qqmlregistration.h>
#include <QDir>
#include <QFileInfo>

#include "fileentry.h"

class FileList: public QAbstractListModel
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles
    {
        PathRole = Qt::UserRole + 1,
        NameRole,
        IsDirRole
    };

    explicit FileList(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    int count() const { return m_entries.size(); }

    void setEntries(QVector<FileEntry> entries);
    ///
    Q_INVOKABLE void renamePath(const QString &oldPath, const QString &newPath);
    Q_INVOKABLE void removePath(const QString &path);
    ///

signals:
    void countChanged();

private:
    QVector<FileEntry> m_entries;
};