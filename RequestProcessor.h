#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

#include "FileList.h"
#include "PathsProcessor.h"
#include "TagsProcessor.h"

class RequestProcessor : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(FileList  *files READ files CONSTANT)
    Q_PROPERTY(PathsProcessor *paths READ paths CONSTANT)
    Q_PROPERTY(TagsProcessor  *tags  READ tags  CONSTANT)

public:
    explicit RequestProcessor(QObject *parent = nullptr);

    FileList  *files() const { return m_files; }
    PathsProcessor *paths() const { return m_paths; }
    TagsProcessor  *tags()  const { return m_tags; }

    Q_INVOKABLE void setQuery(const QString &input);

private:
    FileList  *m_files;
    PathsProcessor *m_paths;
    TagsProcessor  *m_tags;
};