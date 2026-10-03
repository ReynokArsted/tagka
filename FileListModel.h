#pragma once
#include <QAbstractListModel>
#include <QVector>
#include <QString>
#include <QFileInfo>
#include <QDir>
#include <QQuickWindow>
#include <QObject>
#include <QStringList>

#include "FileEntry.h"

class FileListModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasFolder READ hasFolder NOTIFY folderChanged)
    Q_PROPERTY(QVariantList get_files READ get_files NOTIFY itemsChanged)
    Q_PROPERTY(QVariantList candidates READ candidates NOTIFY candidatesChanged)
public:
    explicit FileListModel(QObject* parent = nullptr);

/// Files
    QString folder() const { return m_folder; }
    Q_INVOKABLE QString parent_folder() const;
    Q_INVOKABLE void setFolder(const QString &folderPath);
    QVariantList get_files() const { return m_items; }
    bool hasFolder() const { return !m_folder.isEmpty(); }
///

    Q_INVOKABLE QSet<int> tagIdsForFile(const QList<QString> &paths) const;

/// Autocomplete 
    struct Candidate 
    {
        QString name; 
        bool isDir;
    };

    QVariantList candidates() const { return m_candidates; }
    Q_INVOKABLE void showCandidates(const QVariantList &candidates);
    Q_INVOKABLE QVariantList getCandidates(QString dirPath, QString prefix);
    Q_INVOKABLE void updateCandidates(const QString &input);
    Q_INVOKABLE void clearCandidates();
    Q_INVOKABLE QString completePrefix(const QString &input) const;
    Q_INVOKABLE QString acceptFirstCandidate(const QString &input) const;
///

/// Files (new)
    void setEntries(QVector<FileEntry> entries);
///

private:
/// Files
    QVariantList m_items;
    QString m_folder;
///

///
    QVector<FileEntry> m_entries;
//

/// Autocomplete
    struct InputParts { QString dir; QString prefix; };
    QVariantList m_candidates;

    static QString normalizeDirPath(const QString& s);
    static InputParts splitInput(const QString &input);
    static QChar detectSep(const QString &input);
    static QString commonPrefixOf(const QStringList &names, const QString &prefix);
    QVariantList getCandidates(const QString &dirPath, const QString &prefix) const;
    void setCandidates(const QVariantList &c);
///

signals:
/// Files
    void folderChanged();
    void itemsChanged();
///

/// Autocomplete
    void candidatesChanged();
///

///
    void countChanged();
///
};
