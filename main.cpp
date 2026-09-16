#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QFile>
#include <QTextStream>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QFileSystemWatcher>
#include <objbase.h>

#include "Translator.h"
#include "FileListModel.h"
#include "DataBaseModule.h"
#include "UsnJournalMonitor.h"
#include "ThingModel.h"

void logHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QFile file("log.txt");
    if (!file.open(QIODevice::Append | QIODevice::Text)) return;
    QTextStream out(&file);
    out << msg << "\n";
}

int main(int argc, char *argv[])
{
    QFile::remove("log.txt");
    qInstallMessageHandler(logHandler);

    QGuiApplication app(argc, argv);
    
    DataBaseModule db_module;
    if (!db_module.open()) return -1;
    QSqlDatabase db = QSqlDatabase::database("app_connection");
 
    UsnJournalMonitor monitor;
    ThingModel thingModel;

    QObject::connect
    (
        &monitor,
        &UsnJournalMonitor::fileMoved,
        &app,
        [db](const QString &oldPath,
           const QString &newPath,
           quint64 fileId)
        {
            qDebug() << "id:" << fileId << "from" << oldPath << "to" << newPath;

            QString o_p = oldPath;
            QString n_p = newPath;

            if (o_p.startsWith(QStringLiteral("\\\\?\\")))
                o_p.remove(0, 4);
            o_p.replace('\\', '/');

            if (n_p.startsWith(QStringLiteral("\\\\?\\")))
                n_p.remove(0, 4);
            n_p.replace('\\', '/');

            qDebug() << "o_p = " << o_p;
            qDebug() << "n_p = " << n_p;

            QSqlQuery query(db);
            query.prepare("UPDATE file SET path = :new_path WHERE path = :old_path");
            query.bindValue(":new_path", n_p);
            query.bindValue(":old_path", o_p);

            if (!query.exec()) 
            {
                qWarning() << "ERROR: update file path failed:" << query.lastError().text();
            }
        }
    );

    QObject::connect
    (
        &monitor,
        &UsnJournalMonitor::fileDeleted,
        &app,
        [](const QString &path,
           quint64 fileId)
        {
            qDebug() << "id:" << fileId << "deleted by" << path;
        }
    );

    monitor.start();

    //query.exec("create table file(id integer, path text, PRIMARY KEY(id AUTOINCREMENT))");
    //query.exec("create table tag_file(id integer, tag_id integer, file_id integer, PRIMARY KEY(id AUTOINCREMENT))");
    // query.exec("create table tag(id integer, tag_name varchar(20), PRIMARY KEY(id AUTOINCREMENT))");
    // query.exec("insert into tag(tag_name) values('spring')");
    // query.exec("insert into tag(tag_name) values('cat')");
    // query.exec("insert into tag(tag_name) values('electronics')");

    QQmlApplicationEngine engine;
    Translator translator(&engine);
    engine.rootContext()->setContextProperty("Translator", &translator);
    engine.rootContext()->setContextProperty("DataBaseModule", &db_module);
    engine.rootContext()->setContextProperty("ThingModel", &thingModel);

    qmlRegisterType<FileListModel>("untitled.files", 1, 0, "FileListModel");

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("untitled", "Main");

    return app.exec();
}
