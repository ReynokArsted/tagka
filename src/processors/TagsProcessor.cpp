#include "TagsProcessor.h"
#include "Storage.h"

#include <QDebug>
#include <QFileInfo>
#include <QStringList>
#include <functional>

namespace {

const QString kOps       = QStringLiteral("()*\\+");
const QString kAllTagged = QStringLiteral("##"); 
const QString kPlus      = QStringLiteral("+");
const QString kStar      = QStringLiteral("*");
const QString kMinus     = QStringLiteral("\\");
const QString kOpen      = QStringLiteral("(");
const QString kClose     = QStringLiteral(")");

using TagLoader = std::function<QSet<QString>(const QString &tagName)>;

bool isOp(const QString &s)
{
    return kOps.contains(s);
}

bool isTag(const QString &tag)
{
    if (tag.length() <= 1) return false;
    if (!tag.startsWith(QLatin1Char('#')) || tag.count(QLatin1Char('#')) >= 2) return false;
    return true;
}

// 1. Check brace balance and add spaces around operators
bool spaceOperators(QString &temp)
{
    QVector<int> brace_balances;
    int current_brace_index = 0;

    for (int i = 0; i < temp.length(); i++)
    {
        if (!kOps.contains(temp[i]))
            continue;

        if (temp[i] == QLatin1Char('('))
        {
            brace_balances.push_back(1);
            current_brace_index = brace_balances.length() - 1;
        }

        if (temp[i] == QLatin1Char(')'))
        {
            if (brace_balances.isEmpty())                      
            {
                qDebug() << "ERROR: extra closing brace";
                return false;
            }
            brace_balances[current_brace_index]--;

            if (current_brace_index > 0) current_brace_index--;

            while (brace_balances[current_brace_index] == 0 && current_brace_index > 0)
                current_brace_index--;
        }

        if (i > 0 && temp[i - 1] != QLatin1Char(' '))
        {
            temp.insert(i, QLatin1Char(' '));
            i++;
        }
        if (i + 1 < temp.length() && temp[i + 1] != QLatin1Char(' '))
            temp.insert(i + 1, QLatin1Char(' '));
    }

    bool brace_balance_was_lost = false;
    for (int i = 0; i < brace_balances.length(); i++)
        if (brace_balances[i] != 0)
        {
            brace_balance_was_lost = true;
            current_brace_index = i;
        }

    if (brace_balance_was_lost)
    {
        qDebug() << "ERROR: the " + QString::number(current_brace_index + 1) + "th brace wasn't closed";
        return false;
    }
    return true;
}

// 2. Check operands count
bool hasMissingOperand(const QStringList &tl)
{
    const int n = tl.length();
    for (int i = 0; i < n; i++)
    {
        const QString &t = tl[i];
        if (t != kPlus && t != kStar && t != kMinus) continue;

        const QString prev = i > 0     ? tl[i - 1] : QString();
        const QString next = i + 1 < n ? tl[i + 1] : QString();

        const bool next_ok = isTag(next) || next == kOpen;
        const bool prev_ok = isTag(prev) || prev == kClose;
        const bool unary   = (t == kMinus) && (i == 0 || prev == kOpen);

        if (!next_ok || (!prev_ok && !unary))
            return true;
    }
    return false;
}

// 3. Delete redundant braces
void stripRedundantBraces(QStringList &tl)
{
    for (int i = 1; i < tl.length() - 1; i++)
    {
        if (tl[i].contains(QLatin1Char('#')))   // #cat + (#iron + (#cat) ()) \ #chest ()
        {
            int k = 1;
            bool braces_is_here = true;
            while (braces_is_here && (i - k) >= 0 && (i + k) < tl.length())
            {
                if (tl[i - k] == kOpen && tl[i + k] == kClose)
                {
                    tl[i + k] = QStringLiteral(" ");
                    tl[i - k] = QStringLiteral(" ");
                }
                else braces_is_here = false;
                k++;
            }
        }
        else if (tl[i] == kOpen && tl[i + 1] == kClose)
        {
            tl[i] = QStringLiteral(" ");
            tl[i + 1] = QStringLiteral(" ");
        }
    }
    tl.removeAll(QStringLiteral(" "));
}

// 4. Insert implicit operators
bool insertImplicitOperators(QStringList &tl)
{
    auto operand_end   = [](const QString &s) { return isTag(s) || s == kClose; };
    auto operand_start = [](const QString &s) { return isTag(s) || s == kOpen; };

    bool path_indicator = false;
    bool error_was_found = false;

    for (int i = 0; i < tl.length(); i++)
    {
        if (isTag(tl[i]) || isOp(tl[i]))
        {
            if (path_indicator) error_was_found = true;

            if (tl[i] == kMinus && (i == 0 || tl[i - 1] == kOpen))
            {
                tl.insert(i, kAllTagged);
                i++;
            }

            if (i + 1 != tl.length() && operand_end(tl[i]) && operand_start(tl[i + 1]))
            {
                tl.insert(i + 1, kStar);
                i++;
            }
        }
        else path_indicator = true;
    }
    return !error_was_found;
}

// 5. Execute operations
QVector<QVector<int>> planOperations(const QStringList &tl)
{
    QVector<QVector<int>> operations_list;
    operations_list.emplace_back();

    bool in_brace = false;
    int current_brace_pair = 0;

    for (int i = 0; i < tl.length(); i++)
    {
        if (tl[i].isEmpty() || !isOp(tl[i]))
            continue;

        if (tl[i] == kOpen)
        {
            in_brace = true;
            operations_list.emplace_back();
            current_brace_pair++;
        }
        else if (tl[i] == kClose)
        {
            if (in_brace)
            {
                if (current_brace_pair - 1 == 0) in_brace = false;
                if (current_brace_pair > 0) current_brace_pair--;
            }
            else in_brace = true;
        }
        else
        {
            QVector<int> &ops = operations_list[current_brace_pair];

            if (tl[i] == kMinus && tl[i - 1] == kAllTagged)       
            {
                ops.push_back(i);
            }
            else if (tl[i] == kStar)                                 
            {
                int j = ops.length() - 1;
                while (j >= 0 &&
                       !(tl[ops[j]] == kPlus ||
                         (tl[ops[j]] == kMinus && tl[ops[j] - 1] != kAllTagged)))
                    j--;

                if (j < 0) ops.push_front(i);
                else       ops.insert(j + 1, i);
            }
            else if (tl[i] == kMinus && tl[i - 1] != kAllTagged)     
            {
                int j = ops.length() - 1;
                while (j >= 0 && tl[ops[j]] != kPlus) j--;

                if (j < 0) ops.push_front(i);
                else       ops.insert(j + 1, i);
            }
            else                                                      
            {
                ops.push_front(i);
            }
        }
    }
    qDebug() << "-> operations_list:" << operations_list;
    return operations_list;
}

QSet<QString> operandSet
(
    const QString &token,
    const QVector<QSet<QString>> &results,
    const TagLoader &load
)
{
    if (token == kAllTagged)
        return load(QString());
    if (token.contains(QLatin1Char('#')))
        return load(token.mid(1));

    bool ok = false;
    const int index = token.toInt(&ok);
    if (ok && index >= 0 && index < results.size())
        return results[index];
    return {};
}

// 6. Do operations
QVector<QSet<QString>> evaluate
(
    QStringList &tl,
    const QVector<QVector<int>> &operations_list,
    const TagLoader &load,
    bool &ok
)
{
    QVector<QSet<QString>> results;
    ok = true;

    for (int i = operations_list.length() - 1; i >= 0; i--)
    {
        for (int j = operations_list[i].length() - 1; j >= 0; j--)
        {
            const int pos = operations_list[i][j];
            const QString op = tl[pos];
            if (op != kPlus && op != kStar && op != kMinus)
                continue;

            int k = 1, t = 1;
            while (pos - k >= 0 && tl[pos - k].isEmpty()) k++;
            while (pos + t < tl.length() && tl[pos + t].isEmpty()) t++;
            if (pos - k < 0 || pos + t >= tl.length())
            {
                ok = false;
                return {};
            }
            qDebug() << "-> operation:" << op << "op1 =" << tl[pos - k] << "op2 =" << tl[pos + t];

            QSet<QString> result = operandSet(tl[pos - k], results, load);
            const QSet<QString> other = operandSet(tl[pos + t], results, load);

            if (op == kPlus)       result |= other;     
            else if (op == kStar)  result &= other;      
            else                   result -= other;     

            results.push_back(result);

            tl[pos] = QString::number(results.size() - 1);
            tl[pos - k] = QString();
            tl[pos + t] = QString();

            int left = pos - k - 1;
            int right = pos + t + 1;
            for (;;)
            {
                while (left >= 0 && tl[left].isEmpty()) left--;
                while (right < tl.length() && tl[right].isEmpty()) right++;

                if (left < 0 || right >= tl.length() || tl[left] != kOpen || tl[right] != kClose)
                    break;

                tl[left] = QString();
                tl[right] = QString();
                left--;
                right++;
            }
            qDebug() << "-> temp_list =" << tl;
        }
    }
    return results;
}
}

TagsProcessor::TagsProcessor(Storage *db, QObject *parent)
    : QObject(parent), m_db(db) {}

std::optional<QVector<FileEntry>> TagsProcessor::getFilesByExpression(const QString &expression)
{
    const QVector<FileEntry> empty;
    auto fail = [&](const QString &message, bool critical = false)
    {
        emit errorOccurred(message, critical);
        return empty;
    };

    QString temp = expression.simplified();
    if (!spaceOperators(temp))
        return fail(tr("Нарушен баланс скобок"));

    QStringList temp_list = temp.split(QLatin1Char(' '));

    if (hasMissingOperand(temp_list))
        return fail(tr("У оператора не хватает операнда"));

    stripRedundantBraces(temp_list);

    if (!insertImplicitOperators(temp_list))
        return fail(tr("В запросе с метками не должно быть путей"));

    const QVector<QVector<int>> operations_list = planOperations(temp_list);

    bool dbFailed = false;
    const TagLoader load = [this, &dbFailed](const QString &name)
    {
        const auto files = m_db->filesForTag(name);
        if (!files) dbFailed = true;
        return files.value_or(QSet<QString>{});
    };

    bool ok = true;
    const QVector<QSet<QString>> results = evaluate(temp_list, operations_list, load, ok);
    if (dbFailed)
    {
        emit errorOccurred("Ошибка чтения базы данных", true);
        return std::nullopt;
    }
    if (!ok)
        return fail(tr("Не удалось вычислить выражение"));

    QSet<QString> found;
    if (results.isEmpty())
    {
        for (const QString &tok : std::as_const(temp_list))
            if (isTag(tok))
            {
                const auto files = m_db->filesForTag(tok.mid(1));
                if (!files)
                {
                    emit errorOccurred("Ошибка чтения базы данных", true);
                    return std::nullopt;
                }
                found = *files;
                break;
            }
    }
    else
        found = results.last();

    QStringList paths = found.values();
    paths.sort();

    QVector<FileEntry> result;
    result.reserve(paths.size());
    for (const QString &path : std::as_const(paths))
        result.push_back(FileEntry::fromInfo(QFileInfo(path)));
    return result;
}

QSet<int> TagsProcessor::tagIdsForFile(const QList<QString> &paths) const
{
    return m_db->commonTagIds(paths);
}