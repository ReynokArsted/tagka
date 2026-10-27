#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVector>
#include <optional>
#include <QtQml/qqmlregistration.h>

#include "FileEntry.h"

class PathsProcessor : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(bool hasFolder READ hasFolder NOTIFY folderChanged)
    Q_PROPERTY(QVariantList candidates READ candidates NOTIFY candidatesChanged)
    Q_PROPERTY(bool showHidden READ showHidden WRITE setShowHidden NOTIFY showHiddenChanged)

public:
    explicit PathsProcessor(QObject *parent = nullptr);

    std::optional<QVector<FileEntry>> getFilesByPath(const QString &path);

    bool showHidden() const { return m_showHidden; }
    void setShowHidden(bool show);

    bool hasFolder() const { return !m_folder.isEmpty(); }
    Q_INVOKABLE QString parent_folder() const;
    Q_INVOKABLE QString parentOf(const QString &path) const;

    QVariantList candidates() const;                                 
    const QVector<FileEntry> &candidateEntries() const { return m_candidates; }

    Q_INVOKABLE void updateCandidates(const QString &input);
    Q_INVOKABLE void clearCandidates() { setCandidates({}); }
    Q_INVOKABLE QString completePrefix(const QString &input) const;
    Q_INVOKABLE QString acceptFirstCandidate(const QString &input) const;

signals:
    void folderChanged();
    void candidatesChanged();
    void showHiddenChanged();

private:
    void setCandidates(QVector<FileEntry> c);

    bool m_showHidden = false;
    QString m_folder;
    QVector<FileEntry> m_candidates;
};