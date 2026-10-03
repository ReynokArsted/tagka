// #include "TagsProcessor.h"

// TagsProcessor::TagsProcessor(QObject* parent)
//     : QObject(parent) {}

// inline QSet<QString> get_tag_set(QString tg)
// {
//     QSqlDatabase db = QSqlDatabase::database("app_connection");
//     if (!db.isOpen()) 
//     {
//         qWarning() << "ERROR: database is not open";
//         return QSet<QString>();
//     }
//     QSqlQuery query(db);

//     if (tg == "##")
//         query.prepare("SELECT file.path FROM tag_file JOIN file ON tag_file.file_id = file.id GROUP BY file.path");
//     else 
//     {
//         query.prepare("SELECT file.path FROM tag_file JOIN file ON tag_file.file_id = file.id JOIN tag ON tag_file.tag_id = tag.id WHERE tag_name = :tag");
//         query.bindValue(":tag", tg.remove(0, 1));
//     }

//     if (!query.exec()) 
//     {
//         qWarning() << "ERROR: select tag failed:" << query.lastError().text();
//         return QSet<QString>();
//     }

//     QSet<QString> tag_set;
//     while (query.next())
//         tag_set.insert(query.value(0).toString());

//     qDebug() << "-> tag_set:" << tag_set;
//     return tag_set;
// }

// inline QSet<QString> subtract_op (QString tg1, QString tg2)
// {       
//     QSet<QString> set1 = get_tag_set(tg1);
//     QSet<QString> set2 = get_tag_set(tg2);
//     QSet<QString> result = set1.subtract(set2);
//     return result;
// }

// inline QSet<QString> subtract_op (QSet<QString> set1, QString tg2)
// {
//     qDebug() << "-> tag_set:" << set1;
//     QSet<QString> set2 = get_tag_set(tg2);
//     QSet<QString> result = set1.subtract(set2);
//     return result;
// }

// inline QSet<QString> subtract_op (QString tg1, QSet<QString> set2)
// {
//     QSet<QString> set1 = get_tag_set(tg1);
//     qDebug() << "-> tag_set:" << set2;
//     QSet<QString> result = set1.subtract(set2);
//     return result;
// }

// inline QSet<QString> subtract_op (QSet<QString> set1, QSet<QString> set2)
// {
//     qDebug() << "-> tag_set:" << set1;
//     qDebug() << "-> tag_set:" << set2;
//     QSet<QString> result = set1.subtract(set2);
//     return result;
// }

// inline QSet<QString> unite_op (QString tg1, QString tg2)
// {
//     QSet<QString> set1 = get_tag_set(tg1);
//     QSet<QString> set2 = get_tag_set(tg2);
//     QSet<QString> result = set1.unite(set2);
//     return result;
// }

// inline QSet<QString> unite_op (QSet<QString> set1, QString tg2)
// {
//     qDebug() << "-> tag_set:" << set1;
//     QSet<QString> set2 = get_tag_set(tg2);
//     QSet<QString> result = set1.unite(set2);
//     return result;
// }

// inline QSet<QString> unite_op (QString tg1, QSet<QString> set2)
// {
//     QSet<QString> set1 = get_tag_set(tg1);
//     qDebug() << "-> tag_set:" << set2;
//     QSet<QString> result = set1.unite(set2);
//     return result;
// }

// inline QSet<QString> unite_op (QSet<QString> set1, QSet<QString> set2)
// {
//     qDebug() << "-> tag_set:" << set1;
//     qDebug() << "-> tag_set:" << set2;
//     QSet<QString> result = set1.unite(set2);
//     return result;
// }

// inline QSet<QString> intersect_op (QString tg1, QString tg2)
// {
//     QSet<QString> set1 = get_tag_set(tg1);
//     QSet<QString> set2 = get_tag_set(tg2);
//     QSet<QString> result = set1.intersect(set2);  
//     return result;
// }

// inline QSet<QString> intersect_op (QSet<QString> set1, QString tg2)
// {
//     qDebug() << "-> tag_set:" << set1;
//     QSet<QString> set2 = get_tag_set(tg2);
//     QSet<QString> result = set1.intersect(set2);
//     return result;
// }

// inline QSet<QString> intersect_op (QString tg1, QSet<QString> set2)
// {
//     QSet<QString> set1 = get_tag_set(tg1);
//     qDebug() << "-> tag_set:" << set2;
//     QSet<QString> result = set1.intersect(set2);
//     return result;
// }

// inline QSet<QString> intersect_op (QSet<QString> set1, QSet<QString> set2)
// {
//     qDebug() << "-> tag_set:" << set1;
//     qDebug() << "-> tag_set:" << set2;
//     QSet<QString> result = set1.intersect(set2);
//     return result;
// }

// inline bool check_tag(QString tag)
// {
//     if (tag.length() <= 1) return false;
//     if (!tag.startsWith('#') || tag.count('#') >= 2) return false;

//     return true;
// }

// QVector<FileEntry> TagsProcessor::setFolder(const QString &folderPath) 
// {
//     QString temp = folderPath;
//     temp = temp.simplified();

//     QString ref = "()*+\\";
//     int operation_count = 0;

//     // check brace closing
//     int brace_pair_count = 0;
//     QVector<int> brace_balances;
//     int current_brace_index;
//     for (int i = 0; i < temp.length(); i++)
//     {
//         if (ref.contains(temp[i])) 
//         {
//             if (temp[i] != '(' && temp[i] != ')') operation_count++;
//             if (temp[i] == '(') 
//             {
//                 brace_balances.push_back(1);
//                 current_brace_index = brace_balances.length() - 1;
//             }
            
//             if (temp[i] == ')') 
//             {
//                 brace_balances[current_brace_index]--;
//                 brace_pair_count++;

//                 if (current_brace_index > 0) current_brace_index--;

//                 while (brace_balances[current_brace_index] == 0 && current_brace_index > 0)
//                     current_brace_index--;
//             }

//             if (i > 0 && temp[i - 1] != ' ') 
//             {
//                 temp.insert(i, ' ');
//                 i++;
//             }
//             else if (i + 1 < temp.length() && temp[i + 1] != ' ') 
//                 temp.insert(i + 1, ' ');
//         }
//     }

//     bool brace_balance_was_lost = false;
//     for (int i = 0; i < brace_balances.length(); i++)
//         if (brace_balances[i] != 0) 
//         {
//             brace_balance_was_lost = true;
//             current_brace_index = i;
//         }

//     if (brace_balance_was_lost) 
//     {
//         qDebug() << "ERROR: the " + QString::number(current_brace_index + 1) + "th brace wasn't closed";
//         m_items = QVariantList();
//         emit itemsChanged();
//         return  QVector<FileEntry>();
//     }

//     QStringList temp_list = temp.split(' ');
//     qDebug() << "-> temp_list =" << temp_list;

//     // check operands count 
//     bool other_operand_wasnt_found = false;
//     for (int i = 0; i < temp_list.length(); i++) {
//         if (ref.contains(temp_list[i]) && temp_list[i] != "(" && temp_list[i] != ")") {
//             if (temp_list[i] == "\\" && ref.contains(temp_list[i+1]))
//                 other_operand_wasnt_found = true;
//             else if (temp_list[i] != "\\" && 
//                 (i == temp_list.length() - 1 || i == 0 || 
//                 (ref.contains(temp_list[i-1]) && !(QString("()").contains(temp_list[i-1]))) || 
//                 (ref.contains(temp_list[i+1]) && !(QString("()").contains(temp_list[i+1]))))
//             )
//                 other_operand_wasnt_found = true;
//         }
//     }
//     if (other_operand_wasnt_found)
//     {
//         qDebug() << "ERROR: one operand wasn't found";
//         m_items = QVariantList();
//         emit itemsChanged();
//         return  QVector<FileEntry>();
//     }

//     qDebug() << "-> res:" << temp_list;
//     qDebug() << "-> operation_count:" << operation_count;
//     qDebug() << "-> brace_pair_count:" << brace_pair_count;

//     // delete superfluous braces
//     for (int i = 1; i < temp_list.length() - 1; i++)
//     {
//         if (temp_list[i].contains("#")) // #cat + (#iron + (#cat) ()) \ #chest ()
//         {
//             int k = 1;
//             bool braces_is_here = true;
//             while (braces_is_here && (i - k) >= 0 && (i + k) < temp_list.length())
//             {
//                 if (temp_list[i - k] == "(" && temp_list[i + k] == ")")
//                 {
//                     temp_list[i + k] = " ";
//                     temp_list[i - k] = " ";
//                 }
//                 else braces_is_here = false;
//                 k++;
//             }
//         }
//         else if (i < (temp_list.length() - 1) && temp_list[i] == "(" && temp_list[i + 1] == ")")
//         {
//             temp_list[i] = " ";
//             temp_list[i + 1] = " ";
//         }
//     }
//     temp_list.removeAll(" ");
//     qDebug() << "-> brace removing result:" << temp_list;

//     int last_tag_index = 0;
//     bool path_indicator = false;
//     bool error_was_found = false;
//     for (int i = 0; i < temp_list.length(); i++)
//     {
//         if (check_tag(temp_list[i]) || ref.contains(temp_list[i])) 
//         {
//             //qDebug() << temp_list[i] << "is a tag or an operation";
//             if (path_indicator) error_was_found = true;
//             if (temp_list[i] == "\\" && (i == 0 || temp_list[i - 1] == "(")) 
//             {
//                 temp_list.insert(i == 0? 0 : i - 1, "##");
//                 i++;
//             }
//             if (i + 1 != temp_list.length()  && temp_list[i].contains("#") && temp_list[i + 1].contains("#"))
//             {
//                 temp_list.insert(i + 1, "*");
//                 i++;
//             }
//             last_tag_index = i;
//         }
//         else path_indicator = true;
//     }
//     if (error_was_found)
//     {
//         qDebug() << "ERROR: seq has paths between tags";
//         m_items = QVariantList();
//         emit itemsChanged();
//         return QVector<FileEntry>();
//     }
//     qDebug() << "-> result temp_list:" << temp_list;
//     qDebug() << "-> last_tag_index =" << last_tag_index;

//     QVector<QVector<int>> operations_list;
//     bool in_brace = false; 
//     operations_list.emplace_back();
//     int current_brace_pair = 0;
//     //for (int i = 0; i < last_tag_index; i++)
//     for (int i = 0; i < temp_list.length(); i++)
//     {
//         if (ref.contains(temp_list[i]))
//         {
//             if (temp_list[i] == "(") 
//             {
//                 in_brace = true;
//                 operations_list.emplace_back();
//                 current_brace_pair++;
//             }
//             else if (temp_list[i] == ")") 
//             {
//                 if (in_brace) 
//                 {
//                     if (current_brace_pair - 1 == 0) in_brace = false;
//                     current_brace_pair--;
//                 }
//                 else in_brace = true;
//             }
//             else 
//             {
//                 // take operation order
//                 if (temp_list[i] == "\\" && temp_list[i - 1] == "##")
//                 {
//                     qDebug() << "##\\";
//                     operations_list[current_brace_pair].push_back(i);
//                     qDebug() << "-> res:" << operations_list[current_brace_pair];
//                 }
//                 else if (temp_list[i] == "\\" && temp_list[i - 1] != "##")
//                 {
//                     qDebug() << "\\";
//                     operations_list[current_brace_pair].push_front(i);
//                     qDebug() << "-> res:" << operations_list[current_brace_pair];
//                 }
//                 else if (temp_list[i] == "*")
//                 {
//                     qDebug() << "*";
//                     int j = operations_list[current_brace_pair].length() - 1;
//                     bool jump_flag = false;
//                     while (j >= 0 && !jump_flag)
//                     { 
//                         qDebug() << "-> in * while";
//                         if (temp_list[operations_list[current_brace_pair][j]] == "+" || 
//                             (temp_list[operations_list[current_brace_pair][j]] == "\\" && 
//                                 temp_list[operations_list[current_brace_pair][j] - 1] != "##"))
//                         {
//                             jump_flag = true;
//                             if (j != (operations_list[current_brace_pair].length() - 1))
//                             {
//                                 operations_list[current_brace_pair].insert(j + 1, i);
//                                 qDebug() << "-> res:" << operations_list[current_brace_pair];
//                             }
//                             else
//                             { 
//                                 operations_list[current_brace_pair].push_back(i);
//                                 qDebug() << "-> res:" << operations_list[current_brace_pair];
//                             }
//                         }
//                         j--;
//                     }
//                     if (j < 0 && !jump_flag) 
//                     {
//                         operations_list[current_brace_pair].push_front(i);
//                         qDebug() << "-> res:" << operations_list[current_brace_pair];
//                     }
//                 }
//                 else if (temp_list[i] == "+")
//                 {
//                     qDebug() << "+";
//                     int j = operations_list[current_brace_pair].length() - 1;
//                     qDebug() << "-> start j =" << j;
//                     qDebug() << "-> start current_brace_pair =" << current_brace_pair;
//                     bool jump_flag = false;
//                     while (j >= 0 && !jump_flag)
//                     {
//                         qDebug() << "-> in + while";
//                         qDebug() << "->" << temp_list[operations_list[current_brace_pair][j]];
//                         if (temp_list[operations_list[current_brace_pair][j]] == "\\" && 
//                                 temp_list[operations_list[current_brace_pair][j] - 1] != "##")
//                         {
//                             jump_flag = true;
//                             if (j != (operations_list[current_brace_pair].length() - 1))
//                             {
//                                 operations_list[current_brace_pair].insert(j + 1, i);
//                                 qDebug() << "-> res:" << operations_list[current_brace_pair];
//                             }
//                             else 
//                             {
//                                 operations_list[current_brace_pair].push_back(i);
//                                 qDebug() << "-> res:" << operations_list[current_brace_pair];
//                             }
//                         }
//                         j--;
//                     }
//                     if (j < 0 && !jump_flag) 
//                     {
//                         operations_list[current_brace_pair].push_front(i);
//                         qDebug() << "-> res:" << operations_list[current_brace_pair];
//                     }
//                 }
//                 else 
//                 {
//                     qDebug() << "-> else op";
//                     operations_list[current_brace_pair].push_front(i);
//                     qDebug() << "-> res:" << operations_list[current_brace_pair];
//                 }
//             }
//         }
//     }
//     qDebug() << "-> operations_list:" << operations_list;

//     QVector<QSet<QString>> results;
//     QSet<QString> result;
//     int i = operations_list.length() - 1, j = -1;
//     while (i >= 0)
//     {
//         j = operations_list[i].length() - 1;
//         while (j >= 0)
//         {
//             qDebug() << "-> operation:" << temp_list[operations_list[i][j]];
//             if (temp_list[operations_list[i][j]] == "+")
//             {   
//                 int k = 1, t = 1; 
//                 while (temp_list[operations_list[i][j] - k] == "") k++;
//                 while (temp_list[operations_list[i][j] + t] == "") t++;
//                 qDebug() << "-> op1 =" << temp_list[operations_list[i][j] - k];
//                 qDebug() << "-> op2 =" << temp_list[operations_list[i][j] + t];

//                 //QSet<QString> result;
//                 if (!temp_list[operations_list[i][j] - k].contains("#") && 
//                     !temp_list[operations_list[i][j] + t].contains("#"))
//                 {
//                     int index1 = temp_list[operations_list[i][j] - k].toInt();
//                     int index2 = temp_list[operations_list[i][j] + t].toInt();
//                     result = unite_op(results[index1], results[index2]);
//                 }    
//                 else if (!temp_list[operations_list[i][j] - k].contains("#"))
//                 {
//                     int index = temp_list[operations_list[i][j] - k].toInt();
//                     result = unite_op(results[index], temp_list[operations_list[i][j] + t]);
//                 }
//                 else if (!temp_list[operations_list[i][j] + t].contains("#"))
//                 {
//                     int index = temp_list[operations_list[i][j] + t].toInt();
//                     result = unite_op(temp_list[operations_list[i][j] - k], results[index]);
//                 }    
//                 else
//                 {
//                     result = unite_op(
//                         temp_list[operations_list[i][j] - k], 
//                         temp_list[operations_list[i][j] + t]);
//                 }
//                 qDebug() << "-> result:" << result;

//                 QString index = QString::number(results.size());
//                 results.push_back(result);

//                 temp_list.replace(operations_list[i][j], index);
//                 temp_list.replace(operations_list[i][j] - k, "");
//                 temp_list.replace(operations_list[i][j] + t, "");
//                 int m = 1;
//                 bool braces_is_here = true;
//                 while(braces_is_here && operations_list[i][j] - (k + m) >= 0 && 
//                     operations_list[i][j] + (t + m) <= (temp_list.length() - 1))
//                 {
//                     if (temp_list[operations_list[i][j] - (k + m)] == "(" && 
//                         temp_list[operations_list[i][j] + (t + m)] == ")")
//                     {
//                         temp_list.replace(operations_list[i][j] - (k + m), "");
//                         temp_list.replace(operations_list[i][j] + (t + m), "");
//                     }
//                     else braces_is_here = false;
//                     m++;
//                 }
//             }
//             else if (temp_list[operations_list[i][j]] == "*")
//             {
//                 int k = 1, t = 1; 
//                 while (temp_list[operations_list[i][j] - k] == "") k++;
//                 while (temp_list[operations_list[i][j] + t] == "") t++;
//                 qDebug() << "-> op1 =" << temp_list[operations_list[i][j] - k];
//                 qDebug() << "-> op2 =" << temp_list[operations_list[i][j] + t];

//                 //QSet<QString> result;
//                 if (!temp_list[operations_list[i][j] - k].contains("#") && 
//                     !temp_list[operations_list[i][j] + t].contains("#"))
//                 {
//                     int index1 = temp_list[operations_list[i][j] - k].toInt();
//                     int index2 = temp_list[operations_list[i][j] + t].toInt();
//                     result = intersect_op(results[index1], results[index2]);
//                 }    
//                 else if (!temp_list[operations_list[i][j] - k].contains("#"))
//                 {
//                     int index = temp_list[operations_list[i][j] - k].toInt();
//                     result = intersect_op(results[index], temp_list[operations_list[i][j] + t]);
//                 }
//                 else if (!temp_list[operations_list[i][j] + t].contains("#"))
//                 {
//                     int index = temp_list[operations_list[i][j] + t].toInt();
//                     result = intersect_op(temp_list[operations_list[i][j] - k], results[index]);
//                 }    
//                 else
//                 {
//                     result = intersect_op(
//                         temp_list[operations_list[i][j] - k], 
//                         temp_list[operations_list[i][j] + t]);
//                 }
//                 qDebug() << "-> result:" << result;

//                 QString index = QString::number(results.size());
//                 results.push_back(result);

//                 temp_list.replace(operations_list[i][j], index);
//                 temp_list.replace(operations_list[i][j] - k, "");
//                 temp_list.replace(operations_list[i][j] + t, "");
//                 int m = 1;
//                 bool braces_is_here = true;
//                 while(braces_is_here && operations_list[i][j] - (k + m) >= 0 && 
//                     operations_list[i][j] + (t + m) <= (temp_list.length() - 1))
//                 {
//                     if (temp_list[operations_list[i][j] - (k + m)] == "(" && 
//                         temp_list[operations_list[i][j] + (t + m)] == ")")
//                     {
//                         temp_list.replace(operations_list[i][j] - (k + m), "");
//                         temp_list.replace(operations_list[i][j] + (t + m), "");
//                     }
//                     else braces_is_here = false;
//                     m++;
//                 }
//             }
//             else if (temp_list[operations_list[i][j]] == "\\")
//             {
//                 int k = 1, t = 1; 
//                 while (temp_list[operations_list[i][j] - k] == "") k++;
//                 while (temp_list[operations_list[i][j] + t] == "") t++;
//                 qDebug() << "-> op1 =" << temp_list[operations_list[i][j] - k];
//                 qDebug() << "-> op2 =" << temp_list[operations_list[i][j] + t];

//                 //QSet<QString> result;
//                 if (!temp_list[operations_list[i][j] - k].contains("#") && 
//                     !temp_list[operations_list[i][j] + t].contains("#"))
//                 {
//                     int index1 = temp_list[operations_list[i][j] - k].toInt();
//                     int index2 = temp_list[operations_list[i][j] + t].toInt();
//                     result = subtract_op(results[index1], results[index2]);
//                 }    
//                 else if (!temp_list[operations_list[i][j] - k].contains("#"))
//                 {                  
//                     int index = temp_list[operations_list[i][j] - k].toInt();
//                     result = subtract_op(results[index], temp_list[operations_list[i][j] + t]);
//                 }
//                 else if (!temp_list[operations_list[i][j] + t].contains("#"))
//                 {
//                     int index = temp_list[operations_list[i][j] + t].toInt();
//                     result = subtract_op(temp_list[operations_list[i][j] - k], results[index]);
//                 }    
//                 else
//                 {
//                     result = subtract_op
//                     (
//                         temp_list[operations_list[i][j] - k], 
//                         temp_list[operations_list[i][j] + t]
//                     );
//                 }
//                 qDebug() << "-> result:" << result;
//                 results.push_back(result);

//                 QString index = QString::number(results.size() - 1);
//                 temp_list.replace(operations_list[i][j], index);
//                 temp_list.replace(operations_list[i][j] - k, "");
//                 temp_list.replace(operations_list[i][j] + t, "");

//                 int m = 1;
//                 bool braces_is_here = true;
//                 while(braces_is_here && operations_list[i][j] - (k + m) >= 0 && 
//                     operations_list[i][j] + (t + m) <= (temp_list.length() - 1))
//                 {
//                     if (temp_list[operations_list[i][j] - (k + m)] == "(" && 
//                         temp_list[operations_list[i][j] + (t + m)] == ")")
//                     {
//                         temp_list.replace(operations_list[i][j] - (k + m), "");
//                         temp_list.replace(operations_list[i][j] + (t + m), "");
//                     }
//                     else braces_is_here = false;
//                     m++;
//                 }
//             }
//             qDebug() << "-> temp_list =" << temp_list;
//             j--;
//         }
//         i--;
//     }

//     if (folderPath.contains("#"))
//     {
//         QString temp = folderPath;
//         temp.replace("#", "");

//         QVariantList newItems;

//         if (results.isEmpty())
//         {
//             QSqlDatabase db = QSqlDatabase::database("app_connection");
//             if (!db.isOpen()) 
//             {
//                 qWarning() << "ERROR: database is not open";
//                 return QVector<FileEntry>();
//             }
//             QSqlQuery query(db);

//             query.prepare("SELECT file.path FROM tag JOIN tag_file ON tag_file.tag_id = tag.id JOIN file ON file.id = tag_file.file_id WHERE tag.tag_name = :tag");
//             query.bindValue(":tag", temp);
//             if (!query.exec()) 
//             {
//                 qWarning() << "ERROR: select tag failed:" << query.lastError().text();
//                 return QVector<FileEntry>();
//             }

//             while (query.next()) 
//             {
//                 QFileInfo info(query.value(0).toString());
//                 QVariantMap m;
//                 m["name"]  = info.fileName();
//                 m["path"]  = info.absoluteFilePath();
//                 m["isDir"] = info.isDir();
//                 newItems.append(m);
//             }
//         }
//         else
//         {
//             const auto paths = results.last().values();
//             for (const QString &path : paths)
//             {
//                 QFileInfo info(path);
//                 QVariantMap m;
//                 m["name"]  = info.fileName();
//                 m["path"]  = info.absoluteFilePath();
//                 m["isDir"] = info.isDir();
//                 newItems.append(m);
//             }
//         }
//         m_items = newItems;
//         emit itemsChanged();
//     }
//     else 
//     {
//         QDir dir(folderPath);
//         if (!dir.exists()) return QVector<FileEntry>();

//         m_folder = dir.absolutePath();

//         QFileInfoList infos = dir.entryInfoList
//         (
//             QDir::NoDotAndDotDot | QDir::AllEntries,
//             QDir::DirsFirst | QDir::Name
//         );

//         QVariantList newItems;
//         for (const QFileInfo &fi : infos) 
//         {
//             QVariantMap m;
//             m["name"]  = fi.fileName();
//             m["path"]  = fi.absoluteFilePath();
//             m["isDir"] = fi.isDir();
//             newItems.append(m);
//         }
        
//         m_items = newItems;
//         emit itemsChanged();
//         emit folderChanged();
//     }

//     QStringList paths = result.values();  
//     paths.sort();                      

//     QVector<FileEntry> result_paths;
//     result_paths.reserve(paths.size());
//     for (const QString &path : std::as_const(paths))
//         result_paths.push_back(FileEntry::fromInfo(QFileInfo(path)));
//     return result_paths;

//     // return result;
// }

#include "TagsProcessor.h"

#include <QDebug>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <functional>

// Разбор выражения из меток. Алгоритм прежний; код разложен по шагам.
//
// Приоритет операций, от высшего к низшему:
//   скобки, унарный \ (в начале или после "("), *, бинарный \, +
// Операции одного уровня выполняются слева направо.
//
// Шаги: spaceOperators -> hasMissingOperand -> stripRedundantBraces ->
//       insertImplicitOperators -> planOperations -> evaluate.

namespace {

const QString kOps       = QStringLiteral("()*\\+");
const QString kAllTagged = QStringLiteral("##");   // «все файлы с метками» (для унарного минуса)
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

// 1. Пробелы вокруг скобок и операций + проверка баланса скобок.
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
            if (brace_balances.isEmpty())                       // лишняя ')'
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

// 2. У каждой операции должны быть операнды.
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

// 3. Лишние скобки: (#a), () и т. п.
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

// 4. Унарный \ получает "##" слева, между соседними операндами появляется "*".
// false — между метками встретился посторонний текст (путь).
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

// 5. Порядок выполнения: для каждого уровня скобок список позиций операций.
// Список выполняется с конца.
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

            if (tl[i] == kMinus && tl[i - 1] == kAllTagged)          // унарный \: первым
            {
                ops.push_back(i);
            }
            else if (tl[i] == kStar)                                  // после последних «+» и бинарного минуса
            {
                int j = ops.length() - 1;
                while (j >= 0 &&
                       !(tl[ops[j]] == kPlus ||
                         (tl[ops[j]] == kMinus && tl[ops[j] - 1] != kAllTagged)))
                    j--;

                if (j < 0) ops.push_front(i);
                else       ops.insert(j + 1, i);
            }
            else if (tl[i] == kMinus && tl[i - 1] != kAllTagged)      // бинарный \: после последнего +
            {
                int j = ops.length() - 1;
                while (j >= 0 && tl[ops[j]] != kPlus) j--;

                if (j < 0) ops.push_front(i);
                else       ops.insert(j + 1, i);
            }
            else                                                       // + и всё прочее: самыми последними
            {
                ops.push_front(i);
            }
        }
    }
    qDebug() << "-> operations_list:" << operations_list;
    return operations_list;
}

// Операнд: метка, "##" или номер ранее вычисленного результата.
QSet<QString> operandSet(const QString &token,
                         const QVector<QSet<QString>> &results,
                         const TagLoader &load)
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

// 6. Вычисление. ok = false — не нашлись операнды.
QVector<QSet<QString>> evaluate(QStringList &tl,
                                const QVector<QVector<int>> &operations_list,
                                const TagLoader &load,
                                bool &ok)
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

            if (op == kPlus)       result |= other;      // объединение
            else if (op == kStar)  result &= other;      // пересечение
            else                   result -= other;      // разность

            results.push_back(result);

            tl[pos] = QString::number(results.size() - 1);
            tl[pos - k] = QString();
            tl[pos + t] = QString();

            // Скобки вокруг только что вычисленной части больше не нужны.
            // Между скобкой и операндом могут лежать пустые токены от прошлых шагов, их пропускаем.
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

TagsProcessor::TagsProcessor(QObject *parent): QObject(parent) {}

void TagsProcessor::setConnectionName(const QString &name)
{
    if (m_connectionName == name)
        return;
    m_connectionName = name;
    emit connectionNameChanged();
}

QSqlDatabase TagsProcessor::database() const
{
    return QSqlDatabase::database(m_connectionName);
}

QSet<QString> TagsProcessor::filesForTag(const QString &tagName) const
{
    const QSqlDatabase db = database();
    if (!db.isOpen())
    {
        qWarning() << "ERROR: database is not open";
        return {};
    }

    QSqlQuery query(db);

    if (tagName.isEmpty())
    {
        query.prepare(
            "SELECT file.path FROM tag_file "
            "JOIN file ON tag_file.file_id = file.id "
            "GROUP BY file.path");
    }
    else
    {
        query.prepare(
            "SELECT file.path FROM tag_file "
            "JOIN file ON tag_file.file_id = file.id "
            "JOIN tag ON tag_file.tag_id = tag.id "
            "WHERE tag_name = :tag");
        query.bindValue(QStringLiteral(":tag"), tagName);
    }

    if (!query.exec())
    {
        qWarning() << "ERROR: select tag failed:" << query.lastError().text();
        return {};
    }

    QSet<QString> files;
    while (query.next())
        files.insert(query.value(0).toString());

    qDebug() << "-> tag_set:" << files;
    return files;
}

std::optional<QVector<FileEntry>> TagsProcessor::entries(const QString &expression) const
{
    const QVector<FileEntry> empty;          // ошибка в выражении

    QString temp = expression.simplified();
    if (!spaceOperators(temp))
        return empty;

    QStringList temp_list = temp.split(QLatin1Char(' '));
    qDebug() << "-> temp_list =" << temp_list;

    if (hasMissingOperand(temp_list))
    {
        qDebug() << "ERROR: one operand wasn't found";
        return empty;
    }

    stripRedundantBraces(temp_list);
    qDebug() << "-> brace removing result:" << temp_list;

    if (!insertImplicitOperators(temp_list))
    {
        qDebug() << "ERROR: seq has paths between tags";
        return empty;
    }
    qDebug() << "-> result temp_list:" << temp_list;

    const QVector<QVector<int>> operations_list = planOperations(temp_list);

    const TagLoader load = [this](const QString &name) { return filesForTag(name); };

    bool ok = true;
    const QVector<QSet<QString>> results = evaluate(temp_list, operations_list, load, ok);
    if (!ok)
        return empty;

    QSet<QString> found;
    if (results.isEmpty())
    {
        // выражение из одной метки
        if (!database().isOpen())
            return std::nullopt;

        for (const QString &tok : std::as_const(temp_list))
            if (isTag(tok))
            {
                found = filesForTag(tok.mid(1));
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
    QSet<int> result;
    if (paths.isEmpty())
        return result;

    const QSqlDatabase db = database();
    if (!db.isOpen())
    {
        qWarning() << "ERROR: database is not open";
        return result;
    }

    QSqlQuery query(db);
    query.prepare(
        "SELECT tag_file.tag_id FROM tag_file "
        "JOIN file ON file.id = tag_file.file_id "
        "WHERE file.path = :path");

    bool first = true;
    for (const QString &path : paths)
    {
        query.bindValue(QStringLiteral(":path"), path);
        if (!query.exec())
        {
            qWarning() << "ERROR: select tagIdsForFile failed with error" << query.lastError().text();
            return {};
        }

        QSet<int> ids;
        while (query.next())
            ids.insert(query.value(0).toInt());

        if (first)
        {
            result = ids;
            first = false;
        }
        else
            result &= ids;

        if (result.isEmpty())
            break;
    }
    return result;
}