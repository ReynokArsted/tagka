#include <QDebug>
#include <QDir>

#include "ThingieListModel.h"

ThingieListModel::ThingieListModel(QObject *parent) :
    QAbstractListModel(parent) {}

void ThingieListModel::updateFromVector(std::vector<Thingie*> newThingies)
{
    beginResetModel();
    _thingies.clear();
    for (const auto &item : newThingies)
        _thingies << item;
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
    const int row = _thingies.size();

    QSqlDatabase db = QSqlDatabase::database("app_connection");
    if (!db.isOpen()) 
    {
        qWarning() << "ERROR: database is not open";
        return;
    }
    QSqlQuery query(db);

    query.prepare("INSERT INTO tag(tag_name) VALUES(:name)");
    query.bindValue(":name", name);

    if (!query.exec()) {
        qWarning() << "INSERT tag failed:" << query.lastError().text();
        endInsertRows();
        return;
    }

    const int newId = query.lastInsertId().toInt();
    beginInsertRows(QModelIndex(), row, row);
    _thingies.append(new Thingie(newId, name, this));
    endInsertRows();
}

bool ThingieListModel::removeThing(int tagId)
{
    const int row = std::find_if(_thingies.begin(), _thingies.end(),
        [tagId](Thingie *t) { return t->id() == tagId; }) - _thingies.begin();

    if (row < 0 || row >= _thingies.size()) return false;

    QSqlDatabase db = QSqlDatabase::database("app_connection");
    if (!db.isOpen()) 
    {
        qWarning() << "ERROR: database is not open";
        return false;
    }
    QSqlQuery deleteLinks(db);

    deleteLinks.prepare("DELETE FROM tag_file WHERE tag_id = :tagId");
    deleteLinks.bindValue(":tagId", tagId);
    if (!deleteLinks.exec())
        qWarning() << "ERROR: delete tag_file failed: " << deleteLinks.lastError().text();

    QSqlQuery deleteTag(db);
    deleteTag.prepare("DELETE FROM tag WHERE id = :tagId");
    deleteTag.bindValue(":tagId", tagId);
    if (!deleteTag.exec()) {
        qWarning() << "ERROR: delete tag failed: " << deleteTag.lastError().text();
        return false;
    }

    beginRemoveRows(QModelIndex(), row, row);
    delete _thingies.at(row);
    _thingies.removeAt(row);
    endRemoveRows();

    return true;
}

bool ThingieListModel::assignTagsToFile(const QList<QString> &paths, const QVariantList &tagIds)
{
    if (paths.isEmpty()) return false;

    QSqlDatabase db = QSqlDatabase::database("app_connection");
    if (!db.isOpen()) 
    {
        qWarning() << "ERROR: database is not open";
        return false;
    }

    QSet<int> desiredTagIds;
    for (const QVariant &tagIdVar : tagIds) 
    {
        const int tagId = tagIdVar.toInt();
        if (tagId >= 0) desiredTagIds.insert(tagId);
    }
    QVector<QSet<int>> existingTagIds;
    QList<int> existingFileIds;   

    for (int i = 0; i < paths.length(); i++)
    {
        QSqlQuery selectFile(db);
        selectFile.prepare("SELECT id FROM file WHERE path = :path");
        selectFile.bindValue(":path", paths[i]);
        if (!selectFile.exec()) 
        {
            qWarning() << "ERROR: select file failed: " << selectFile.lastError().text();
            return false;
        }

        int fileId = -1;
        if (selectFile.next()) fileId = selectFile.value(0).toInt();
        else 
        {
            QSqlQuery insertFile(db);
            insertFile.prepare("INSERT INTO file(path) VALUES(:path)");
            insertFile.bindValue(":path", paths[i]);
            if (!insertFile.exec()) 
            {
                qWarning() << "ERROR: insert file failed: " << insertFile.lastError().text();
                return false;
            }
            fileId = insertFile.lastInsertId().toInt();
        }
        existingFileIds.push_back(fileId);

        QSqlQuery selectExisting(db);
        selectExisting.prepare("SELECT tag_id FROM tag_file WHERE file_id = :fileId");
        selectExisting.bindValue(":fileId", fileId);
        if (!selectExisting.exec()) 
        {
            qWarning() << "ERROR: select existing tag_file failed: " << selectExisting.lastError().text();
            return false;
        }

        int set_ind = existingTagIds.isEmpty()? 0 : existingTagIds.length();
        existingTagIds.push_back(QSet<int>{});
        while (selectExisting.next())
            existingTagIds[set_ind].insert(selectExisting.value(0).toInt());
    }

    if (existingTagIds.isEmpty()) return false;
    int min_length = existingTagIds[0].count();
    for (int i = 0; i < existingTagIds.length(); i++)
    {
        if (existingTagIds[i].count() < min_length)
        {
            min_length = existingTagIds[i].count();
            existingTagIds.swapItemsAt(i, 0);
        }
    }
    for (int i = 1; i < existingTagIds.count(); ++i)
        existingTagIds[0] &= existingTagIds[i]; 

    const QSet<int> toRemove = existingTagIds[0] - desiredTagIds;
    const QSet<int> toAdd = desiredTagIds - existingTagIds[0];
    for (int i = 0; i < existingFileIds.length(); i++)
    {
        for (int tagId : toRemove) 
        {
            QSqlQuery deleteLink(db);
            deleteLink.prepare("DELETE FROM tag_file WHERE tag_id = :tagId AND file_id = :fileId");
            deleteLink.bindValue(":tagId", tagId);
            deleteLink.bindValue(":fileId", existingFileIds[i]);
            if (!deleteLink.exec())
                qWarning() << "ERROR: delete tag_file failed: " << deleteLink.lastError().text();
        }
        for (int tagId : toAdd) 
        {
            QSqlQuery insertTagFile(db);
            insertTagFile.prepare("INSERT INTO tag_file(tag_id, file_id) VALUES(:tagId, :fileId)");
            insertTagFile.bindValue(":tagId", tagId);
            insertTagFile.bindValue(":fileId", existingFileIds[i]);
            if (!insertTagFile.exec())
                qWarning() << "ERROR: insert tag_file failed: " << insertTagFile.lastError().text();
        }
    }

    return true;
}

QSet<int> ThingieListModel::tagIdsForFile(const QList<QString> &paths) const
{
    QSet<int> result;
    if (paths.isEmpty()) return result;

    QSqlDatabase db = QSqlDatabase::database("app_connection");
    if (!db.isOpen()) 
    {
        qWarning() << "ERROR: database is not open";
        return result;
    }
    QSqlQuery query(db);

    QVector<QSet<int>> tags_ids_sets;
    for (int i = 0; i < paths.length(); i++)
    {
        QSqlQuery query(db);
        query.prepare
        (
            "SELECT tag_file.tag_id FROM tag_file "
            "JOIN file ON file.id = tag_file.file_id "
            "WHERE file.path = :paths"
        );
        query.bindValue(":paths", paths[i]);

        if (!query.exec()) 
        {
            qWarning() << "ERROR: select tagIdsForFile failed: " << query.lastError().text();
            return result;
        }

        int set_ind = tags_ids_sets.isEmpty()? 0 : tags_ids_sets.length();
        tags_ids_sets.push_back(QSet<int>{});
        while (query.next())
            tags_ids_sets[set_ind].insert(query.value(0).toInt());
    }

    if (tags_ids_sets.isEmpty()) return result;
    int min_length = tags_ids_sets[0].count();
    for (int i = 0; i < tags_ids_sets.length(); i++)
    {
        if (tags_ids_sets[i].count() < min_length)
        {
            min_length = tags_ids_sets[i].count();
            tags_ids_sets.swapItemsAt(i, 0);
        }
    }

    result = tags_ids_sets[0];
    for (int i = 1; i < tags_ids_sets.count(); ++i)
        result &= tags_ids_sets[i];

    return result;
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
            if (std::find(_thingies.begin(), _thingies.end(), thingie) != _thingies.end()) 
                return std::distance(_thingies.begin(), std::find(_thingies.begin(), _thingies.end(), thingie));
            else return -1;
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

QString ThingieListModel::print()
{
    QString tmp;
    for(int i = 0; i < _thingies.size(); ++i) {
        tmp.append(QString::number(i));
        tmp.append(": ");
        tmp.append(_thingies.at(i)->name());
        tmp.append("; ");
    }
    return tmp;
}