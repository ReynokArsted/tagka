#ifndef ThingieLISTMODEL_H
#define ThingieLISTMODEL_H

#include <QAbstractListModel>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QVariantList>

#include "Thingie.h"

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

    ThingieListModel(QObject *parent = nullptr);
    void updateFromVector(std::vector<Thingie*> newThingies);
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    Q_INVOKABLE void move(int from, int to);
    Q_INVOKABLE void addThing(const QString &name);
    Q_INVOKABLE bool removeThing(int tagId);
    Q_INVOKABLE bool assignTagsToFile(const QList<QString> &paths, const QVariantList &tagIds);
    Q_INVOKABLE QColor colorForId(int tagId) const; 

private:
    QList<Thingie*> _thingies;

signals:
    void newFileAdded(const QString &path);
    void tagsAssigned(const QStringList &paths);
};

#endif