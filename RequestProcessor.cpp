#include "RequestProcessor.h"

RequestProcessor::RequestProcessor(QObject* parent)
    : QObject(parent), pp(new PathsProcessor(this)), tp(new TagsProcessor(this)) {}


void RequestProcessor::process_request(const QString &request) {}