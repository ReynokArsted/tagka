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

public:
    explicit PathsProcessor(QObject *parent = nullptr);

    std::optional<QVector<FileEntry>> entries(const QString &path);

    bool hasFolder() const { return !m_folder.isEmpty(); }
    Q_INVOKABLE QString parent_folder() const;

    QVariantList candidates() const;                                 
    const QVector<FileEntry> &candidateEntries() const { return m_candidates; }

    Q_INVOKABLE void updateCandidates(const QString &input);
    Q_INVOKABLE void clearCandidates() { setCandidates({}); }
    Q_INVOKABLE QString completePrefix(const QString &input) const;
    Q_INVOKABLE QString acceptFirstCandidate(const QString &input) const;

signals:
    void folderChanged();
    void candidatesChanged();

private:
    void setCandidates(QVector<FileEntry> c);

    QString m_folder;
    QVector<FileEntry> m_candidates;
};