#pragma once

#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QVector>

struct FileEntry
{
    QString path;
    QString name;
    bool isDir = false;

    static FileEntry fromInfo(const QFileInfo &fi)
    {
        return { fi.absoluteFilePath(), fi.fileName(), fi.isDir() };
    }

    static QVector<FileEntry> listDir(const QString &dirPath, QDir::SortFlags sort,
                                      const QString &prefix = QString())
    {
        QVector<FileEntry> out;
        const QDir dir(dirPath);
        if (!dir.exists())
            return out;

        const QFileInfoList infos = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, sort);
        out.reserve(infos.size());
        for (const QFileInfo &fi : infos)
            if (prefix.isEmpty() || fi.fileName().startsWith(prefix, Qt::CaseSensitive))
                out.push_back(fromInfo(fi));
        return out;
    }
};