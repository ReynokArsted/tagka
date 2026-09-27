#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QSqlError>

#include "DataBaseModule.h"

DataBaseModule::DataBaseModule(QObject *parent)
    : QObject(parent) {}

bool DataBaseModule::open()
{
    if (QSqlDatabase::contains(m_connectionName)) 
    {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName);
        if (db.isOpen()) return true;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);

    const QString dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("test.db");
    db.setDatabaseName(dbPath);

    if (!db.open()) 
    {
        qWarning() << "ERROR: DB open failed with error" << db.lastError().text();
        return false;
    }
    return true;
}

QSqlDatabase DataBaseModule::database() const
{
    return QSqlDatabase::database(m_connectionName);
}