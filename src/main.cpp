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
#include "TagsProcessor.h"
#include "FileList.h"
#include "Storage.h"
#include "FileMonitor.h"
#include "ThingieListModel.h"
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
 
    Storage db;
    if (!db.open()) return -1;

    ThingieListModel thingieModel(&db);
    FileMonitor monitor(&db);
    FileList fileList;
    System system;

    QObject::connect
    (
        &thingieModel,
        &ThingieListModel::newFileAdded,
        &monitor,
        &FileMonitor::onNewFileAdded
    );
    QObject::connect
    (
        &monitor, 
        &FileMonitor::filePathChanged,
        &fileList, 
        &FileList::renamePath
    );
    QObject::connect
    (
        &monitor, 
        &FileMonitor::fileDeleted,
        &fileList, 
        [&fileList](const QString &path, const QByteArray &) {fileList.removePath(path);}
    );
    
    monitor.start();

    QQmlApplicationEngine engine;
    Translator translator(&engine);
    engine.rootContext()->setContextProperty("Translator", &translator);
    engine.rootContext()->setContextProperty("Storage", &db);
    engine.rootContext()->setContextProperty("ThingieListModel", &thingieModel);
    engine.rootContext()->setContextProperty("FileMonitor", &monitor);
    engine.rootContext()->setContextProperty("System", &system);
    engine.rootContext()->setContextProperty("MoveChoice", monitor.moveChoice());

    qmlRegisterType<FileList>("untitled.files", 1, 0, "FileList");

    QObject::connect
    (
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection
    );
    engine.loadFromModule("untitled", "Main");

    return app.exec();
}
