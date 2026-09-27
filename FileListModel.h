#pragma once
#include <QAbstractListModel>
#include <QVector>
#include <QString>
#include <QFileInfo>
#include <QDir>
#include <QQuickWindow>
#include <QObject>
#include <QStringList>

class FileListModel : public QObject
{
    Q_OBJECT
public:
    struct Candidate 
    {
        QString name; 
        bool isDir;
    };

    explicit FileListModel(QObject* parent = nullptr);

    QString folder() const { return m_folder; }

    Q_INVOKABLE QString parent_folder() const;
    Q_INVOKABLE QString current_folder() const;
    Q_INVOKABLE void setFolder(const QString &folderPath);
    Q_INVOKABLE void setHomeFolder();

    Q_INVOKABLE QSet<int> tagIdsForFile(const QList<QString> &paths) const;
  
    Q_INVOKABLE void openWith(const QString &filePath, QQuickWindow *window = nullptr);
    Q_INVOKABLE void openFile(const QString &filePath, QQuickWindow *window = nullptr);

    Q_INVOKABLE void showCandidates(const QVariantList &candidates);
    Q_INVOKABLE QVariantList getCandidates(QString dirPath, QString prefix);


    Q_PROPERTY(bool hasFolder READ hasFolder NOTIFY folderChanged)
    Q_PROPERTY(QVariantList get_files READ get_files NOTIFY itemsChanged)
    QVariantList get_files() const { return m_items; }

    bool hasFolder() const { return !m_folder.isEmpty(); }

private:
    QVariantList m_items;
    QString m_folder;

    static QString normalizeDirPath(const QString& s);

signals:
    void folderChanged();
    void itemsChanged();
};
