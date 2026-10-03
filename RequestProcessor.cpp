#include "RequestProcessor.h"

RequestProcessor::RequestProcessor(QObject *parent)
    : QObject(parent)
    , m_files(new FileList(this))
    , m_paths(new PathsProcessor(this))
    , m_tags(new TagsProcessor(this))
{
    connect(m_paths, &PathsProcessor::candidatesChanged, this, [this]
    {
        if (!m_paths->candidateEntries().isEmpty())
            m_files->setEntries(m_paths->candidateEntries());
    });
}

void RequestProcessor::setQuery(const QString &input)
{
    const bool isTagExpression = input.contains(QLatin1Char('#'));

    const std::optional<QVector<FileEntry>> entries = isTagExpression
        ? m_tags->entries(input)
        : m_paths->entries(input);

    if (entries)
        m_files->setEntries(*entries);
}