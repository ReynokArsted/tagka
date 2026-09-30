#pragma once
#include <QObject>

#include "TagsProcessor.h"
#include "PathsProcessor.h"

class RequestProcessor : public QObject
{
    Q_OBJECT
    // Q_PROPERTY(PathsProcessor *paths READ paths CONSTANT)
    // Q_PROPERTY(TagsProcessor  *tags  READ tags  CONSTANT)
public:
    explicit RequestProcessor(QObject *parent = nullptr);

    void process_request(const QString &request);

private:
    TagsProcessor *tp;
    PathsProcessor *pp;
};