#include "RequestProcessor.h"
#include "DataBaseModule.h"

RequestProcessor::RequestProcessor(QObject *parent)
    : QObject(parent)
    , m_files(new FileList(this))
    , m_paths(new PathsProcessor(this))
    , m_tags(new TagsProcessor(new DataBaseModule(this), this))
{

    connect(m_paths, &PathsProcessor::candidatesChanged, this, [this]
    {
        if (!m_paths->candidateEntries().isEmpty())
            m_files->setEntries(m_paths->candidateEntries());
    });
    connect(m_tags, &TagsProcessor::errorOccurred, this, &RequestProcessor::errorOccurred);
}

void RequestProcessor::doRequest(const QString &input)
{
    if (input.trimmed().isEmpty())
    {
        m_files->setEntries({});
        return;
    }

    const bool isTagExpression = input.contains(QLatin1Char('#'));

    const std::optional<QVector<FileEntry>> entries = isTagExpression
        ? m_tags->getFilesByExpression(input)
        : m_paths->getFilesByPath(input);

    if (entries)
        m_files->setEntries(*entries);
}