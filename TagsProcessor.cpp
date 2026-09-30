#include "TagsProcessor.h"

TagsProcessor::TagsProcessor(QObject* parent)
    : QObject(parent) {}

QSet<QString> get_tag_set(QString tg)
{
    QSqlDatabase db = QSqlDatabase::database("app_connection");
    if (!db.isOpen()) 
    {
        qWarning() << "ERROR: database is not open";
        return QSet<QString>();
    }
    QSqlQuery query(db);

    if (tg == "##")
        query.prepare("SELECT file.path FROM tag_file JOIN file ON tag_file.file_id = file.id GROUP BY file.path");
    else 
    {
        query.prepare("SELECT file.path FROM tag_file JOIN file ON tag_file.file_id = file.id JOIN tag ON tag_file.tag_id = tag.id WHERE tag_name = :tag");
        query.bindValue(":tag", tg.remove(0, 1));
    }

    if (!query.exec()) 
    {
        qWarning() << "ERROR: select tag failed:" << query.lastError().text();
        return QSet<QString>();
    }

    QSet<QString> tag_set;
    while (query.next())
        tag_set.insert(query.value(0).toString());

    qDebug() << "-> tag_set:" << tag_set;
    return tag_set;
}

QSet<QString> subtract_op (QString tg1, QString tg2)
{       
    QSet<QString> set1 = get_tag_set(tg1);
    QSet<QString> set2 = get_tag_set(tg2);
    QSet<QString> result = set1.subtract(set2);
    return result;
}

QSet<QString> subtract_op (QSet<QString> set1, QString tg2)
{
    qDebug() << "-> tag_set:" << set1;
    QSet<QString> set2 = get_tag_set(tg2);
    QSet<QString> result = set1.subtract(set2);
    return result;
}

QSet<QString> subtract_op (QString tg1, QSet<QString> set2)
{
    QSet<QString> set1 = get_tag_set(tg1);
    qDebug() << "-> tag_set:" << set2;
    QSet<QString> result = set1.subtract(set2);
    return result;
}

QSet<QString> subtract_op (QSet<QString> set1, QSet<QString> set2)
{
    qDebug() << "-> tag_set:" << set1;
    qDebug() << "-> tag_set:" << set2;
    QSet<QString> result = set1.subtract(set2);
    return result;
}

QSet<QString> unite_op (QString tg1, QString tg2)
{
    QSet<QString> set1 = get_tag_set(tg1);
    QSet<QString> set2 = get_tag_set(tg2);
    QSet<QString> result = set1.unite(set2);
    return result;
}

QSet<QString> unite_op (QSet<QString> set1, QString tg2)
{
    qDebug() << "-> tag_set:" << set1;
    QSet<QString> set2 = get_tag_set(tg2);
    QSet<QString> result = set1.unite(set2);
    return result;
}

QSet<QString> unite_op (QString tg1, QSet<QString> set2)
{
    QSet<QString> set1 = get_tag_set(tg1);
    qDebug() << "-> tag_set:" << set2;
    QSet<QString> result = set1.unite(set2);
    return result;
}

QSet<QString> unite_op (QSet<QString> set1, QSet<QString> set2)
{
    qDebug() << "-> tag_set:" << set1;
    qDebug() << "-> tag_set:" << set2;
    QSet<QString> result = set1.unite(set2);
    return result;
}

QSet<QString> intersect_op (QString tg1, QString tg2)
{
    QSet<QString> set1 = get_tag_set(tg1);
    QSet<QString> set2 = get_tag_set(tg2);
    QSet<QString> result = set1.intersect(set2);  
    return result;
}

QSet<QString> intersect_op (QSet<QString> set1, QString tg2)
{
    qDebug() << "-> tag_set:" << set1;
    QSet<QString> set2 = get_tag_set(tg2);
    QSet<QString> result = set1.intersect(set2);
    return result;
}

QSet<QString> intersect_op (QString tg1, QSet<QString> set2)
{
    QSet<QString> set1 = get_tag_set(tg1);
    qDebug() << "-> tag_set:" << set2;
    QSet<QString> result = set1.intersect(set2);
    return result;
}

QSet<QString> intersect_op (QSet<QString> set1, QSet<QString> set2)
{
    qDebug() << "-> tag_set:" << set1;
    qDebug() << "-> tag_set:" << set2;
    QSet<QString> result = set1.intersect(set2);
    return result;
}

bool check_tag(QString tag)
{
    if (tag.length() <= 1) return false;
    if (!tag.startsWith('#') || tag.count('#') >= 2) return false;

    return true;
}

void FileListModel::setFolder(const QString &folderPath) 
{
    QString temp = folderPath;
    temp = temp.simplified();

    QString ref = "()*+\\";
    int operation_count = 0;

    // check brace closing
    int brace_pair_count = 0;
    QVector<int> brace_balances;
    int current_brace_index;
    for (int i = 0; i < temp.length(); i++)
    {
        if (ref.contains(temp[i])) 
        {
            if (temp[i] != '(' && temp[i] != ')') operation_count++;
            if (temp[i] == '(') 
            {
                brace_balances.push_back(1);
                current_brace_index = brace_balances.length() - 1;
            }
            
            if (temp[i] == ')') 
            {
                brace_balances[current_brace_index]--;
                brace_pair_count++;

                if (current_brace_index > 0) current_brace_index--;

                while (brace_balances[current_brace_index] == 0 && current_brace_index > 0)
                    current_brace_index--;
            }

            if (i > 0 && temp[i - 1] != ' ') 
            {
                temp.insert(i, ' ');
                i++;
            }
            else if (i + 1 < temp.length() && temp[i + 1] != ' ') 
                temp.insert(i + 1, ' ');
        }
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
        m_items = QVariantList();
        emit itemsChanged();
        return;
    }

    QStringList temp_list = temp.split(' ');
    qDebug() << "-> temp_list =" << temp_list;

    // check operands count 
    bool other_operand_wasnt_found = false;
    for (int i = 0; i < temp_list.length(); i++) {
        if (ref.contains(temp_list[i]) && temp_list[i] != "(" && temp_list[i] != ")") {
            if (temp_list[i] == "\\" && ref.contains(temp_list[i+1]))
                other_operand_wasnt_found = true;
            else if (temp_list[i] != "\\" && 
                (i == temp_list.length() - 1 || i == 0 || 
                (ref.contains(temp_list[i-1]) && !(QString("()").contains(temp_list[i-1]))) || 
                (ref.contains(temp_list[i+1]) && !(QString("()").contains(temp_list[i+1]))))
            )
                other_operand_wasnt_found = true;
        }
    }
    if (other_operand_wasnt_found)
    {
        qDebug() << "ERROR: one operand wasn't found";
        m_items = QVariantList();
        emit itemsChanged();
        return;
    }

    qDebug() << "-> res:" << temp_list;
    qDebug() << "-> operation_count:" << operation_count;
    qDebug() << "-> brace_pair_count:" << brace_pair_count;

    // delete superfluous braces
    for (int i = 1; i < temp_list.length() - 1; i++)
    {
        if (temp_list[i].contains("#")) // #cat + (#iron + (#cat) ()) \ #chest ()
        {
            int k = 1;
            bool braces_is_here = true;
            while (braces_is_here && (i - k) >= 0 && (i + k) < temp_list.length())
            {
                if (temp_list[i - k] == "(" && temp_list[i + k] == ")")
                {
                    temp_list[i + k] = " ";
                    temp_list[i - k] = " ";
                }
                else braces_is_here = false;
                k++;
            }
        }
        else if (i < (temp_list.length() - 1) && temp_list[i] == "(" && temp_list[i + 1] == ")")
        {
            temp_list[i] = " ";
            temp_list[i + 1] = " ";
        }
    }
    temp_list.removeAll(" ");
    qDebug() << "-> brace removing result:" << temp_list;

    int last_tag_index = 0;
    bool path_indicator = false;
    bool error_was_found = false;
    for (int i = 0; i < temp_list.length(); i++)
    {
        if (check_tag(temp_list[i]) || ref.contains(temp_list[i])) 
        {
            //qDebug() << temp_list[i] << "is a tag or an operation";
            if (path_indicator) error_was_found = true;
            if (temp_list[i] == "\\" && (i == 0 || temp_list[i - 1] == "(")) 
            {
                temp_list.insert(i == 0? 0 : i - 1, "##");
                i++;
            }
            if (i + 1 != temp_list.length()  && temp_list[i].contains("#") && temp_list[i + 1].contains("#"))
            {
                temp_list.insert(i + 1, "*");
                i++;
            }
            last_tag_index = i;
        }
        else path_indicator = true;
    }
    if (error_was_found)
    {
        qDebug() << "ERROR: seq has paths between tags";
        m_items = QVariantList();
        emit itemsChanged();
        return;
    }
    qDebug() << "-> result temp_list:" << temp_list;
    qDebug() << "-> last_tag_index =" << last_tag_index;

    QVector<QVector<int>> operations_list;
    bool in_brace = false; 
    operations_list.emplace_back();
    int current_brace_pair = 0;
    //for (int i = 0; i < last_tag_index; i++)
    for (int i = 0; i < temp_list.length(); i++)
    {
        if (ref.contains(temp_list[i]))
        {
            if (temp_list[i] == "(") 
            {
                in_brace = true;
                operations_list.emplace_back();
                current_brace_pair++;
            }
            else if (temp_list[i] == ")") 
            {
                if (in_brace) 
                {
                    if (current_brace_pair - 1 == 0) in_brace = false;
                    current_brace_pair--;
                }
                else in_brace = true;
            }
            else 
            {
                // take operation order
                if (temp_list[i] == "\\" && temp_list[i - 1] == "##")
                {
                    qDebug() << "##\\";
                    operations_list[current_brace_pair].push_back(i);
                    qDebug() << "-> res:" << operations_list[current_brace_pair];
                }
                else if (temp_list[i] == "\\" && temp_list[i - 1] != "##")
                {
                    qDebug() << "\\";
                    operations_list[current_brace_pair].push_front(i);
                    qDebug() << "-> res:" << operations_list[current_brace_pair];
                }
                else if (temp_list[i] == "*")
                {
                    qDebug() << "*";
                    int j = operations_list[current_brace_pair].length() - 1;
                    bool jump_flag = false;
                    while (j >= 0 && !jump_flag)
                    { 
                        qDebug() << "-> in * while";
                        if (temp_list[operations_list[current_brace_pair][j]] == "+" || 
                            (temp_list[operations_list[current_brace_pair][j]] == "\\" && 
                                temp_list[operations_list[current_brace_pair][j] - 1] != "##"))
                        {
                            jump_flag = true;
                            if (j != (operations_list[current_brace_pair].length() - 1))
                            {
                                operations_list[current_brace_pair].insert(j + 1, i);
                                qDebug() << "-> res:" << operations_list[current_brace_pair];
                            }
                            else
                            { 
                                operations_list[current_brace_pair].push_back(i);
                                qDebug() << "-> res:" << operations_list[current_brace_pair];
                            }
                        }
                        j--;
                    }
                    if (j < 0 && !jump_flag) 
                    {
                        operations_list[current_brace_pair].push_front(i);
                        qDebug() << "-> res:" << operations_list[current_brace_pair];
                    }
                }
                else if (temp_list[i] == "+")
                {
                    qDebug() << "+";
                    int j = operations_list[current_brace_pair].length() - 1;
                    qDebug() << "-> start j =" << j;
                    qDebug() << "-> start current_brace_pair =" << current_brace_pair;
                    bool jump_flag = false;
                    while (j >= 0 && !jump_flag)
                    {
                        qDebug() << "-> in + while";
                        qDebug() << "->" << temp_list[operations_list[current_brace_pair][j]];
                        if (temp_list[operations_list[current_brace_pair][j]] == "\\" && 
                                temp_list[operations_list[current_brace_pair][j] - 1] != "##")
                        {
                            jump_flag = true;
                            if (j != (operations_list[current_brace_pair].length() - 1))
                            {
                                operations_list[current_brace_pair].insert(j + 1, i);
                                qDebug() << "-> res:" << operations_list[current_brace_pair];
                            }
                            else 
                            {
                                operations_list[current_brace_pair].push_back(i);
                                qDebug() << "-> res:" << operations_list[current_brace_pair];
                            }
                        }
                        j--;
                    }
                    if (j < 0 && !jump_flag) 
                    {
                        operations_list[current_brace_pair].push_front(i);
                        qDebug() << "-> res:" << operations_list[current_brace_pair];
                    }
                }
                else 
                {
                    qDebug() << "-> else op";
                    operations_list[current_brace_pair].push_front(i);
                    qDebug() << "-> res:" << operations_list[current_brace_pair];
                }
            }
        }
    }
    qDebug() << "-> operations_list:" << operations_list;

    QVector<QSet<QString>> results;
    int i = operations_list.length() - 1, j = -1;
    while (i >= 0)
    {
        j = operations_list[i].length() - 1;
        while (j >= 0)
        {
            qDebug() << "-> operation:" << temp_list[operations_list[i][j]];
            if (temp_list[operations_list[i][j]] == "+")
            {   
                int k = 1, t = 1; 
                while (temp_list[operations_list[i][j] - k] == "") k++;
                while (temp_list[operations_list[i][j] + t] == "") t++;
                qDebug() << "-> op1 =" << temp_list[operations_list[i][j] - k];
                qDebug() << "-> op2 =" << temp_list[operations_list[i][j] + t];

                QSet<QString> result;
                if (!temp_list[operations_list[i][j] - k].contains("#") && 
                    !temp_list[operations_list[i][j] + t].contains("#"))
                {
                    int index1 = temp_list[operations_list[i][j] - k].toInt();
                    int index2 = temp_list[operations_list[i][j] + t].toInt();
                    result = unite_op(results[index1], results[index2]);
                }    
                else if (!temp_list[operations_list[i][j] - k].contains("#"))
                {
                    int index = temp_list[operations_list[i][j] - k].toInt();
                    result = unite_op(results[index], temp_list[operations_list[i][j] + t]);
                }
                else if (!temp_list[operations_list[i][j] + t].contains("#"))
                {
                    int index = temp_list[operations_list[i][j] + t].toInt();
                    result = unite_op(temp_list[operations_list[i][j] - k], results[index]);
                }    
                else
                {
                    result = unite_op(
                        temp_list[operations_list[i][j] - k], 
                        temp_list[operations_list[i][j] + t]);
                }
                qDebug() << "-> result:" << result;

                QString index = QString::number(results.size());
                results.push_back(result);

                temp_list.replace(operations_list[i][j], index);
                temp_list.replace(operations_list[i][j] - k, "");
                temp_list.replace(operations_list[i][j] + t, "");
                int m = 1;
                bool braces_is_here = true;
                while(braces_is_here && operations_list[i][j] - (k + m) >= 0 && 
                    operations_list[i][j] + (t + m) <= (temp_list.length() - 1))
                {
                    if (temp_list[operations_list[i][j] - (k + m)] == "(" && 
                        temp_list[operations_list[i][j] + (t + m)] == ")")
                    {
                        temp_list.replace(operations_list[i][j] - (k + m), "");
                        temp_list.replace(operations_list[i][j] + (t + m), "");
                    }
                    else braces_is_here = false;
                    m++;
                }
            }
            else if (temp_list[operations_list[i][j]] == "*")
            {
                int k = 1, t = 1; 
                while (temp_list[operations_list[i][j] - k] == "") k++;
                while (temp_list[operations_list[i][j] + t] == "") t++;
                qDebug() << "-> op1 =" << temp_list[operations_list[i][j] - k];
                qDebug() << "-> op2 =" << temp_list[operations_list[i][j] + t];

                QSet<QString> result;
                if (!temp_list[operations_list[i][j] - k].contains("#") && 
                    !temp_list[operations_list[i][j] + t].contains("#"))
                {
                    int index1 = temp_list[operations_list[i][j] - k].toInt();
                    int index2 = temp_list[operations_list[i][j] + t].toInt();
                    result = intersect_op(results[index1], results[index2]);
                }    
                else if (!temp_list[operations_list[i][j] - k].contains("#"))
                {
                    int index = temp_list[operations_list[i][j] - k].toInt();
                    result = intersect_op(results[index], temp_list[operations_list[i][j] + t]);
                }
                else if (!temp_list[operations_list[i][j] + t].contains("#"))
                {
                    int index = temp_list[operations_list[i][j] + t].toInt();
                    result = intersect_op(temp_list[operations_list[i][j] - k], results[index]);
                }    
                else
                {
                    result = intersect_op(
                        temp_list[operations_list[i][j] - k], 
                        temp_list[operations_list[i][j] + t]);
                }
                qDebug() << "-> result:" << result;

                QString index = QString::number(results.size());
                results.push_back(result);

                temp_list.replace(operations_list[i][j], index);
                temp_list.replace(operations_list[i][j] - k, "");
                temp_list.replace(operations_list[i][j] + t, "");
                int m = 1;
                bool braces_is_here = true;
                while(braces_is_here && operations_list[i][j] - (k + m) >= 0 && 
                    operations_list[i][j] + (t + m) <= (temp_list.length() - 1))
                {
                    if (temp_list[operations_list[i][j] - (k + m)] == "(" && 
                        temp_list[operations_list[i][j] + (t + m)] == ")")
                    {
                        temp_list.replace(operations_list[i][j] - (k + m), "");
                        temp_list.replace(operations_list[i][j] + (t + m), "");
                    }
                    else braces_is_here = false;
                    m++;
                }
            }
            else if (temp_list[operations_list[i][j]] == "\\")
            {
                int k = 1, t = 1; 
                while (temp_list[operations_list[i][j] - k] == "") k++;
                while (temp_list[operations_list[i][j] + t] == "") t++;
                qDebug() << "-> op1 =" << temp_list[operations_list[i][j] - k];
                qDebug() << "-> op2 =" << temp_list[operations_list[i][j] + t];

                QSet<QString> result;
                if (!temp_list[operations_list[i][j] - k].contains("#") && 
                    !temp_list[operations_list[i][j] + t].contains("#"))
                {
                    int index1 = temp_list[operations_list[i][j] - k].toInt();
                    int index2 = temp_list[operations_list[i][j] + t].toInt();
                    result = subtract_op(results[index1], results[index2]);
                }    
                else if (!temp_list[operations_list[i][j] - k].contains("#"))
                {                  
                    int index = temp_list[operations_list[i][j] - k].toInt();
                    result = subtract_op(results[index], temp_list[operations_list[i][j] + t]);
                }
                else if (!temp_list[operations_list[i][j] + t].contains("#"))
                {
                    int index = temp_list[operations_list[i][j] + t].toInt();
                    result = subtract_op(temp_list[operations_list[i][j] - k], results[index]);
                }    
                else
                {
                    result = subtract_op
                    (
                        temp_list[operations_list[i][j] - k], 
                        temp_list[operations_list[i][j] + t]
                    );
                }
                qDebug() << "-> result:" << result;
                results.push_back(result);

                QString index = QString::number(results.size() - 1);
                temp_list.replace(operations_list[i][j], index);
                temp_list.replace(operations_list[i][j] - k, "");
                temp_list.replace(operations_list[i][j] + t, "");

                int m = 1;
                bool braces_is_here = true;
                while(braces_is_here && operations_list[i][j] - (k + m) >= 0 && 
                    operations_list[i][j] + (t + m) <= (temp_list.length() - 1))
                {
                    if (temp_list[operations_list[i][j] - (k + m)] == "(" && 
                        temp_list[operations_list[i][j] + (t + m)] == ")")
                    {
                        temp_list.replace(operations_list[i][j] - (k + m), "");
                        temp_list.replace(operations_list[i][j] + (t + m), "");
                    }
                    else braces_is_here = false;
                    m++;
                }
            }
            qDebug() << "-> temp_list =" << temp_list;
            j--;
        }
        i--;
    }

    if (folderPath.contains("#"))
    {
        QString temp = folderPath;
        temp.replace("#", "");

        QVariantList newItems;

        if (results.isEmpty())
        {
            QSqlDatabase db = QSqlDatabase::database("app_connection");
            if (!db.isOpen()) 
            {
                qWarning() << "ERROR: database is not open";
                return;
            }
            QSqlQuery query(db);

            query.prepare("SELECT file.path FROM tag JOIN tag_file ON tag_file.tag_id = tag.id JOIN file ON file.id = tag_file.file_id WHERE tag.tag_name = :tag");
            query.bindValue(":tag", temp);
            if (!query.exec()) 
            {
                qWarning() << "ERROR: select tag failed:" << query.lastError().text();
                return;
            }

            while (query.next()) 
            {
                QFileInfo info(query.value(0).toString());
                QVariantMap m;
                m["name"]  = info.fileName();
                m["path"]  = info.absoluteFilePath();
                m["isDir"] = info.isDir();
                newItems.append(m);
            }
        }
        else
        {
            const auto paths = results.last().values();
            for (const QString &path : paths)
            {
                QFileInfo info(path);
                QVariantMap m;
                m["name"]  = info.fileName();
                m["path"]  = info.absoluteFilePath();
                m["isDir"] = info.isDir();
                newItems.append(m);
            }
        }
        m_items = newItems;
        emit itemsChanged();
    }
    else 
    {
        QDir dir(folderPath);
        if (!dir.exists()) return;

        m_folder = dir.absolutePath();

        QFileInfoList infos = dir.entryInfoList
        (
            QDir::NoDotAndDotDot | QDir::AllEntries,
            QDir::DirsFirst | QDir::Name
        );

        QVariantList newItems;
        for (const QFileInfo &fi : infos) 
        {
            QVariantMap m;
            m["name"]  = fi.fileName();
            m["path"]  = fi.absoluteFilePath();
            m["isDir"] = fi.isDir();
            newItems.append(m);
        }
        
        m_items = newItems;
        emit itemsChanged();
        emit folderChanged();
    }
}