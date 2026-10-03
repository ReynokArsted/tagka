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
//#include "FileListModel.h"
#include "FileList.h"
#include "DataBaseModule.h"
#include "UsnJournalMonitor.h"
#include "ThingModel.h"
#include "System.h"

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
    FileList fileList;
    ThingModel thingModel;
    System system;

    QObject::connect
    (
        thingModel.listOfThingies(),
        &ThingieListModel::newFileAdded,
        &monitor,
        &UsnJournalMonitor::onNewFileAdded
    );
    QObject::connect
    (
        &monitor,
        &UsnJournalMonitor::fileDeleted,
        &app,
        [](const QString &path,
           const QByteArray &fileId)
        {
            qDebug() << "-> id:" << fileId << "deleted by" << path;
        }
    );
    QObject::connect(&monitor, &UsnJournalMonitor::filePathChanged,
                 &fileList, &FileList::renamePath);

    QObject::connect(&monitor, &UsnJournalMonitor::fileDeleted,
                 &fileList, [&fileList](const QString &path, const QByteArray &)
                 { fileList.removePath(path); });
    monitor.start();

    //query.exec("create table file(id integer, path text, PRIMARY KEY(id AUTOINCREMENT))");
    //query.exec("create table tag_file(id integer, tag_id integer, file_id integer, PRIMARY KEY(id AUTOINCREMENT))");
    //query.exec("create table tag(id integer, tag_name varchar(20), PRIMARY KEY(id AUTOINCREMENT))");
    //query.exec("insert into tag(tag_name) values('spring')");
    //query.exec("insert into tag(tag_name) values('cat')");
    //query.exec("insert into tag(tag_name) values('electronics')");

    QQmlApplicationEngine engine;
    Translator translator(&engine);
    engine.rootContext()->setContextProperty("Translator", &translator);
    engine.rootContext()->setContextProperty("DataBaseModule", &db_module);
    engine.rootContext()->setContextProperty("ThingModel", &thingModel);
    //engine.rootContext()->setContextProperty("FileListModel", &fileListModel);
    engine.rootContext()->setContextProperty("UsnJournalMonitor", &monitor);
    engine.rootContext()->setContextProperty("System", &system);

    qmlRegisterType<FileList>("untitled.files", 1, 0, "FileList");

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("untitled", "Main");

    return app.exec();
}
