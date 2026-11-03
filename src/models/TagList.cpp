#include <QDebug>

#include "Storage.h"
#include "System.h"
#include "TagList.h"

TagList::TagList(Storage *db, QObject *parent)
    : QAbstractListModel(parent), _db(db) { reload(); }

void TagList::reload()
{
    beginResetModel();
    qDeleteAll(_Tags);
    _Tags.clear();
    for (const auto &tag : _db->tags())
    {
        auto *tagItem = new Tag(tag.id, tag.name, this);

        const QColor stored(tag.color);
        if (stored.isValid())
            tagItem->setColor(stored);
        else
            _db->setTagColor(tag.id, tagItem->color().name());

        _Tags.append(tagItem);
    }
    endResetModel();
}

QHash<int, QByteArray> TagList::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[ColorRole] = "color";
    roles[ModelIndexRole] = "modelIndex";
    roles[IdRole] = "id";
    return roles;
}

void TagList::addThing(const QString &name)
{
    const QColor color = Tag::randomColor();
    const int newId = _db->addTag(name, color.name());
    if (newId < 0) return;

    auto *tagItem = new Tag(newId, name, this);
    tagItem->setColor(color);

    const int row = _Tags.size();
    beginInsertRows(QModelIndex(), row, row);
    _Tags.append(tagItem);
    endInsertRows();
}

bool TagList::removeThing(int tagId)
{
    const auto it = std::find_if(_Tags.begin(), _Tags.end(),
        [tagId](Tag *t) { return t->id() == tagId; });
    if (it == _Tags.end()) return false;

    if (!_db->removeTag(tagId)) return false;

    const int row = static_cast<int>(it - _Tags.begin());
    beginRemoveRows(QModelIndex(), row, row);
    delete _Tags.at(row);
    _Tags.removeAt(row);
    endRemoveRows();

    return true;
}

bool TagList::assignTagsToFile(const QList<QString> &paths, const QVariantList &tagIds)
{
    if (paths.isEmpty()) return false;

    QSet<int> desired;
    for (const QVariant &tagIdVar : tagIds)
    {
        const int tagId = tagIdVar.toInt();
        if (tagId >= 0) desired.insert(tagId);
    }
    const auto created = _db->assignTags
    (
        paths, 
        desired, 
        &System::fileId128ForPath,
        [this](const QString &p) { return _db->volumeIdByGuid(System::volumeGuidForPath(p)); }
    );
    if (!created) return false;

    for (const QString &path : *created)
        emit newFileAdded(path);
    emit tagsAssigned(paths);
    return true;
}

bool TagList::removeAllTags(const QList<QString> &paths)
{
    if (paths.isEmpty() || !_db->removeFileTags(paths)) return false;

    emit tagsAssigned(paths);
    return true;
}

QVariant TagList::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();

    const Tag *tagItem = _Tags[index.row()];
    switch (role)
    {
        case NameRole:
            return tagItem->name();
        case ColorRole:
            return tagItem->color();
        case IdRole:
            return tagItem->id();
        case ModelIndexRole:
            return index.row();
        default:
            return QVariant();
    }
}

int TagList::rowCount(const QModelIndex &) const { return _Tags.count(); }

QColor TagList::colorForId(int tagId) const
{
    auto it = std::find_if(_Tags.begin(), _Tags.end(),
        [tagId](Tag *t) { return t->id() == tagId; });

    if (it != _Tags.end())
        return (*it)->color();

    return QColor(Qt::gray);
}

bool TagList::setThingColor(int tagId, const QColor &color)
{
    const auto it = std::find_if(_Tags.begin(), _Tags.end(),
        [tagId](Tag *t) { return t->id() == tagId; });
    if (it == _Tags.end() || !color.isValid()) return false;

    if (!_db->setTagColor(tagId, color.name())) return false;

    (*it)->setColor(color);
    const QModelIndex idx = index(static_cast<int>(it - _Tags.begin()));
    emit dataChanged(idx, idx, { ColorRole });
    emit tagColorChanged(tagId);
    return true;
}

void TagList::move(int from, int to)
{
    if (from < 0 || from >= rowCount() || to < 0 || to >= rowCount() || from == to)
        return;
    int destRow = (to > from) ? to + 1 : to;
    beginMoveRows(QModelIndex(), from, from, QModelIndex(), destRow);
    _Tags.move(from, to);
    endMoveRows();
}