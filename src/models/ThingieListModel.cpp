#include <QDebug>

#include "Storage.h"
#include "System.h"
#include "ThingieListModel.h"

ThingieListModel::ThingieListModel(Storage *db, QObject *parent)
    : QAbstractListModel(parent), _db(db) { reload(); }

void ThingieListModel::reload()
{
    beginResetModel();
    qDeleteAll(_thingies);
    _thingies.clear();
    for (const auto &tag : _db->tags())
        _thingies.append(new Thingie(tag.id, tag.name, this));
    endResetModel();
}

QHash<int, QByteArray> ThingieListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[ColorRole] = "color";
    roles[ModelIndexRole] = "modelIndex";
    roles[IdRole] = "id";
    return roles;
}

void ThingieListModel::addThing(const QString &name)
{
    const int newId = _db->addTag(name);
    if (newId < 0) return;

    const int row = _thingies.size();
    beginInsertRows(QModelIndex(), row, row);
    _thingies.append(new Thingie(newId, name, this));
    endInsertRows();
}

bool ThingieListModel::removeThing(int tagId)
{
    const auto it = std::find_if(_thingies.begin(), _thingies.end(),
        [tagId](Thingie *t) { return t->id() == tagId; });
    if (it == _thingies.end()) return false;

    if (!_db->removeTag(tagId)) return false;

    const int row = static_cast<int>(it - _thingies.begin());
    beginRemoveRows(QModelIndex(), row, row);
    delete _thingies.at(row);
    _thingies.removeAt(row);
    endRemoveRows();

    return true;
}

bool ThingieListModel::assignTagsToFile(const QList<QString> &paths, const QVariantList &tagIds)
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
        paths, desired, &System::fileId128ForPath,
        [this](const QString &p) { return _db->volumeIdByGuid(System::volumeGuidForPath(p)); }
    );
    if (!created) return false;

    for (const QString &path : *created)
        emit newFileAdded(path);
    emit tagsAssigned(paths);
    return true;
}

QVariant ThingieListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();

    const Thingie *thingie = _thingies[index.row()];
    switch (role)
    {
        case NameRole:
            return thingie->name();
        case ColorRole:
            return thingie->color();
        case IdRole:
            return thingie->id();
        case ModelIndexRole:
            return index.row();
        default:
            return QVariant();
    }
}

int ThingieListModel::rowCount(const QModelIndex &) const { return _thingies.count(); }

QColor ThingieListModel::colorForId(int tagId) const
{
    auto it = std::find_if(_thingies.begin(), _thingies.end(),
        [tagId](Thingie *t) { return t->id() == tagId; });

    if (it != _thingies.end())
        return (*it)->color();

    return QColor(Qt::gray);
}

void ThingieListModel::move(int from, int to)
{
    if (from < 0 || from >= rowCount() || to < 0 || to >= rowCount() || from == to)
        return;
    int destRow = (to > from) ? to + 1 : to;
    beginMoveRows(QModelIndex(), from, from, QModelIndex(), destRow);
    _thingies.move(from, to);
    endMoveRows();
}