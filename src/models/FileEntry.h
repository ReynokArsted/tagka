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

    static QVector<FileEntry> listDir
    (
        const QString &dirPath,
        QDir::SortFlags sort,
        const QString &prefix = QString(),
        bool showHidden = false
    )
    {
        QVector<FileEntry> out;
        const QDir dir(dirPath);
        if (!dir.exists())
            return out;

        QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot;
        if (showHidden)
            filters |= QDir::Hidden | QDir::System;

        const QFileInfoList infos = dir.entryInfoList(filters, sort);
        out.reserve(infos.size());
        for (const QFileInfo &fi : infos)
            if (prefix.isEmpty() || fi.fileName().startsWith(prefix, Qt::CaseSensitive))
                out.push_back(fromInfo(fi));
        return out;
    }

    bool operator==(const FileEntry &) const = default;
};