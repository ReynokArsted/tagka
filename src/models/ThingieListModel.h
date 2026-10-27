#ifndef THINGIELISTMODEL_H
#define THINGIELISTMODEL_H

#include <QAbstractListModel>
#include <QColor>
#include <QList>
#include <QStringList>
#include <QVariantList>

#include "Thingie.h"

class Storage;

class ThingieListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum ThingieRoles
    {
        NameRole = Qt::UserRole + 1,
        ColorRole,
        ModelIndexRole,
        IdRole,
    };

    explicit ThingieListModel(Storage *db, QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    Q_INVOKABLE void reload();
    Q_INVOKABLE void move(int from, int to);
    Q_INVOKABLE void addThing(const QString &name);
    Q_INVOKABLE bool removeThing(int tagId);
    Q_INVOKABLE bool assignTagsToFile(const QList<QString> &paths, const QVariantList &tagIds);
    Q_INVOKABLE bool removeAllTags(const QList<QString> &paths);
    Q_INVOKABLE QColor colorForId(int tagId) const;
    Q_INVOKABLE bool setThingColor(int tagId, const QColor &color);

signals:
    void newFileAdded(const QString &path);
    void tagsAssigned(const QStringList &paths);
    void tagColorChanged(int tagId);

private:
    QList<Thingie*> _thingies;
    Storage *_db = nullptr;
};

#endif