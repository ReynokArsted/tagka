#include "PathsProcessor.h"

PathsProcessor::PathsProcessor(QObject* parent)
    : QObject(parent) {}

QVariantList PathsProcessor::getCandidates(QString dirPath, QString prefix)
{
    QVariantList out;
    dirPath = normalizeDirPath(dirPath);
    QDir dir(dirPath);
    if (!dir.exists())
        return out;

    // add hidden/system files?
    auto infos = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::Name);

    for (const auto &info : infos) {
        QString name = info.fileName();
        if (!prefix.isEmpty() && !name.startsWith(prefix, Qt::CaseSensitive))
            continue;

        QVariantMap m;
        m["name"] = name;
        m["isDir"] = info.isDir();
        m["path"] = info.absoluteFilePath();
        out.append(m);
    }

    //qDebug() << "-> cand_list:" << out;
    return out;
}

QString PathsProcessor::normalizeDirPath(const QString& s)
{
    QString t = s;
    t.replace("\\", "/");
    return t;
}

void PathsProcessor::showCandidates(const QVariantList &candidates)
{
    QVariantList newItems;
    for (int i = 0; i < candidates.length(); i++) 
    {
        QString q_path = candidates[i].toMap().value("path").toString();
        qDebug() << "-> cand:" << q_path;

        QFileInfo info(q_path);
        QVariantMap m;
        m["name"]  = info.fileName();
        m["path"]  = info.absoluteFilePath();
        m["isDir"] = info.isDir();
        newItems.append(m);
    }
    m_items = newItems;
    emit itemsChanged();
}

QChar PathsProcessor::detectSep(const QString &input)
{
    return input.contains(QLatin1Char('\\')) ? QLatin1Char('\\') : QLatin1Char('/');
}

PathsProcessor::InputParts PathsProcessor::splitInput(const QString &input)
{
    const QString norm = normalizeDirPath(input);
    const int slash = norm.lastIndexOf(QLatin1Char('/'));
    if (slash == -1)
        return { QString(), norm };
    return { norm.left(slash + 1), norm.mid(slash + 1) };
}

QString PathsProcessor::commonPrefixOf(const QStringList &names, const QString &prefix)
{
    if (names.isEmpty())
        return prefix;

    QString common = names.first();
    for (int i = 1; i < names.size(); ++i) {
        while (!common.isEmpty() && !names.at(i).startsWith(common, Qt::CaseSensitive))
            common.chop(1);
        if (common.isEmpty())
            break;
    }
    return common.size() < prefix.size() ? prefix : common;
}

QVariantList PathsProcessor::getCandidates(const QString &dirPath, const QString &prefix) const
{
    QVariantList out;
    const QDir dir(normalizeDirPath(dirPath));
    if (!dir.exists())
        return out;

    const auto infos = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::Name);
    for (const auto &info : infos) {
        const QString name = info.fileName();
        if (!prefix.isEmpty() && !name.startsWith(prefix, Qt::CaseSensitive))
            continue;

        out.append(QVariantMap{
            {QStringLiteral("name"),  name},
            {QStringLiteral("isDir"), info.isDir()},
            {QStringLiteral("path"),  info.absoluteFilePath()}
        });
    }
    return out;
}

void PathsProcessor::setCandidates(const QVariantList &c)
{
    if (m_candidates.isEmpty() && c.isEmpty())
        return;
    m_candidates = c;
    emit candidatesChanged();
}

void PathsProcessor::clearCandidates()
{
    setCandidates({});
}

void PathsProcessor::updateCandidates(const QString &input)
{
    const InputParts parts = splitInput(input);

    if (parts.dir.isEmpty()) {
        setCandidates({});
        return;
    }

    const QVariantList c = getCandidates(parts.dir, parts.prefix);
    if (!c.isEmpty())
        showCandidates(c);
    setCandidates(c);
}

QString PathsProcessor::completePrefix(const QString &input) const
{
    const InputParts parts = splitInput(input);
    if (parts.dir.isEmpty())
        return input;

    const QVariantList cands = getCandidates(parts.dir, parts.prefix);
    if (cands.isEmpty())
        return input;

    QStringList names;
    names.reserve(cands.size());
    for (const QVariant &v : cands)
        names << v.toMap().value(QStringLiteral("name")).toString();

    const QString common = commonPrefixOf(names, parts.prefix);
    if (common.size() <= parts.prefix.size())
        return input;

    QString result = parts.dir + common;
    if (cands.size() == 1
        && cands.first().toMap().value(QStringLiteral("isDir")).toBool())
        result += QLatin1Char('/');

    result.replace(QLatin1Char('/'), detectSep(input));
    return result;
}

QString PathsProcessor::acceptFirstCandidate(const QString &input) const
{
    const InputParts parts = splitInput(input);
    if (parts.dir.isEmpty())
        return input;

    const QVariantList cands = getCandidates(parts.dir, parts.prefix);
    if (cands.isEmpty())
        return input;

    const QVariantMap first = cands.first().toMap();
    QString result = parts.dir + first.value(QStringLiteral("name")).toString();
    if (first.value(QStringLiteral("isDir")).toBool())
        result += QLatin1Char('/');

    result.replace(QLatin1Char('/'), detectSep(input));
    return result;
}