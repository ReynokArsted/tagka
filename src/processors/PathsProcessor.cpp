#include "PathsProcessor.h"

#include <QDir>
#include <QStringList>
#include <QVariantMap>

namespace {

struct InputParts { QString dir; QString prefix; };

QString normalizeDirPath(QString s)
{
    s.replace(QLatin1Char('\\'), QLatin1Char('/'));
    return s;
}

InputParts splitInput(const QString &input)
{
    const QString norm = normalizeDirPath(input);
    const int slash = norm.lastIndexOf(QLatin1Char('/'));
    if (slash == -1)
        return { QString(), norm };
    return { norm.left(slash + 1), norm.mid(slash + 1) };
}

QString withUserSeparators(QString path, const QString &input)
{
    if (input.contains(QLatin1Char('\\')))
        path.replace(QLatin1Char('/'), QLatin1Char('\\'));
    return path;
}

QString commonPrefixOf(const QStringList &names, const QString &prefix)
{
    if (names.isEmpty())
        return prefix;

    QString common = names.first();
    for (int i = 1; i < names.size(); ++i)
    {
        while (!common.isEmpty() && !names.at(i).startsWith(common, Qt::CaseSensitive))
            common.chop(1);
        if (common.isEmpty())
            break;
    }
    return common.size() < prefix.size() ? prefix : common;
}

struct Lookup
{
    InputParts parts;
    QVector<FileEntry> found;
};

Lookup lookup(const QString &input, bool showHidden)
{
    Lookup l{ splitInput(input), {} };
    if (!l.parts.dir.isEmpty())
        l.found = FileEntry::listDir(l.parts.dir, QDir::Name, l.parts.prefix, showHidden);
    return l;
}

}

PathsProcessor::PathsProcessor(QObject *parent): QObject(parent) {}

void PathsProcessor::setShowHidden(bool show)
{
    if (show == m_showHidden) return;
    m_showHidden = show;
    emit showHiddenChanged();
}

std::optional<QVector<FileEntry>> PathsProcessor::getFilesByPath(const QString &path)
{
    const QDir dir(path);
    if (!dir.exists())
        return std::nullopt;

    m_folder = dir.absolutePath();
    emit folderChanged();
    return FileEntry::listDir(m_folder, QDir::DirsFirst | QDir::Name, QString(), m_showHidden);
}

QString PathsProcessor::parent_folder() const
{
    QDir dir(m_folder);
    if (dir.cdUp())
        return dir.absolutePath();
    return {};
}

QString PathsProcessor::parentOf(const QString &path) const
{
    const QString p = QDir::fromNativeSeparators(path.trimmed());
    if (p.isEmpty() || p.contains(QLatin1Char('#')) || !QDir::isAbsolutePath(p))
        return {};

    QDir dir(p);
    return dir.cdUp() ? dir.path() : QString();
}

QVariantList PathsProcessor::candidates() const
{
    QVariantList out;
    out.reserve(m_candidates.size());
    for (const FileEntry &e : m_candidates)
        out.append(QVariantMap
        {
            { QStringLiteral("name"),  e.name  },
            { QStringLiteral("isDir"), e.isDir },
            { QStringLiteral("path"),  e.path  }
        });
    return out;
}

void PathsProcessor::setCandidates(QVector<FileEntry> c)
{
    if (m_candidates.isEmpty() && c.isEmpty())
        return;
    m_candidates = std::move(c);
    emit candidatesChanged();
}

void PathsProcessor::updateCandidates(const QString &input)
{
    setCandidates(lookup(input, m_showHidden).found);
}

QString PathsProcessor::completePrefix(const QString &input) const
{
    const auto [parts, found] = lookup(input, m_showHidden);
    if (found.isEmpty())
        return input;

    QStringList names;
    names.reserve(found.size());
    for (const FileEntry &e : found)
        names << e.name;

    const QString common = commonPrefixOf(names, parts.prefix);
    if (common.size() <= parts.prefix.size())
        return input;

    QString result = parts.dir + common;
    if (found.size() == 1 && found.first().isDir)
        result += QLatin1Char('/');
    return withUserSeparators(result, input);
}

QString PathsProcessor::acceptFirstCandidate(const QString &input) const
{
    const auto [parts, found] = lookup(input, m_showHidden);
    if (found.isEmpty())
        return input;

    QString result = parts.dir + found.first().name;
    if (found.first().isDir)
        result += QLatin1Char('/');
    return withUserSeparators(result, input);
}