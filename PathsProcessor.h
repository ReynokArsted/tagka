#pragma once
#include <QObject>
#include <QDir>

class PathsProcessor : public QObject
{
    Q_OBJECT
public:
    explicit PathsProcessor(QObject *parent = nullptr);

    //Q_INVOKABLE QSet<int> tagIdsForFile(const QList<QString> &paths) const;

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

private:
    struct InputParts { QString dir; QString prefix; };
    QVariantList m_candidates;

    static QString normalizeDirPath(const QString& s);
    static InputParts splitInput(const QString &input);
    static QChar detectSep(const QString &input);
    static QString commonPrefixOf(const QStringList &names, const QString &prefix);
    QVariantList getCandidates(const QString &dirPath, const QString &prefix) const;
    void setCandidates(const QVariantList &c);

signals:
    void candidatesChanged();
};