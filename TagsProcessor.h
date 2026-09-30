#pragma once
#include <QObject>

class TagsProcessor: public QObject
{
    Q_OBJECT
public:
    explicit TagsProcessor(QObject *parent = nullptr);

    QString folder() const { return m_folder; }
    Q_INVOKABLE QString parent_folder() const;
    Q_INVOKABLE void setFolder(const QString &folderPath);
    //Q_INVOKABLE void openWith(const QString &filePath, QQuickWindow *window = nullptr);
    //Q_INVOKABLE void openFile(const QString &filePath, QQuickWindow *window = nullptr);
    QVariantList get_files() const { return m_items; }
    bool hasFolder() const { return !m_folder.isEmpty(); }

    //Q_INVOKABLE QSet<int> tagIdsForFile(const QList<QString> &paths) const;

private:
    QVariantList m_items;
    QString m_folder;

signals:
    void folderChanged();
    void itemsChanged();
};