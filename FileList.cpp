#include "filelist.h"

namespace
{
    // Монитор отдаёт пути со слэшами '/', приводим обе стороны к одному виду
    bool samePath(const QString &a, const QString &b)
    {
        return QDir::cleanPath(QDir::fromNativeSeparators(a))
                   .compare(QDir::cleanPath(QDir::fromNativeSeparators(b)),
                            Qt::CaseInsensitive) == 0;
    }
}

void FileList::renamePath(const QString &oldPath, const QString &newPath)
{
    for (int i = 0; i < m_entries.size(); ++i)
    {
        if (!samePath(m_entries[i].path, oldPath)) continue;

        m_entries[i].path = newPath;
        m_entries[i].name = QFileInfo(newPath).fileName();

        const QModelIndex idx = index(i);
        emit dataChanged(idx, idx, { PathRole, NameRole });
        return;
    }
}

void FileList::removePath(const QString &path)
{
    for (int i = 0; i < m_entries.size(); ++i)
    {
        if (!samePath(m_entries[i].path, path)) continue;

        beginRemoveRows(QModelIndex(), i, i);
        m_entries.removeAt(i);
        endRemoveRows();
        emit countChanged();
        return;
    }
}

FileList::FileList(QObject *parent)
    : QAbstractListModel(parent)
{
}

int FileList::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant FileList::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};

    const FileEntry &e = m_entries.at(index.row());
    switch (role)
    {
    case PathRole:  return e.path;
    case NameRole:  return e.name;
    case IsDirRole: return e.isDir;
    default:        return {};
    }
}

QHash<int, QByteArray> FileList::roleNames() const
{
    return {
        { PathRole,  "path"  },
        { NameRole,  "name"  },
        { IsDirRole, "isDir" }
    };
}

void FileList::setEntries(QVector<FileEntry> entries)
{
    qDebug() << "FileList::setEntries" << entries.size();
    const int oldCount = m_entries.size();

    beginResetModel();
    m_entries = std::move(entries);
    endResetModel();

    if (oldCount != m_entries.size())
        emit countChanged();
}