#ifndef THINGMODEL_H
#define THINGMODEL_H

#include <QObject>
#include <QQmlEngine>
#include <ThingieListModel.h>

class DataBaseModule;   // forward declaration, выше class ThingModel

class Thingie;
class ThingModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ThingieListModel* listOfThingies READ listOfThingies CONSTANT)
public:


// было:  ThingModel(QObject* parent = nullptr);
explicit ThingModel(DataBaseModule *db, QObject *parent = nullptr);
    //ThingModel(QObject* parent = nullptr);
    ThingieListModel* listOfThingies() { return &_listOfThingies; }

private:
    ThingieListModel _listOfThingies;
};
#endif