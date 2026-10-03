#pragma once

#include <QObject>
#include <QString>
#include <QString>
#include <QFileInfo>
#include <QDir>
#include <QQuickWindow>
#include <Windows.h>
#include <optional>
#include <cstring>
#include <functional>

class System: public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit System(QObject *parent = nullptr);

    Q_INVOKABLE void openWith(const QString &filePath, QQuickWindow *window);
    Q_INVOKABLE void openFile(const QString &filePath, QQuickWindow *window);

    struct UsnEvent
{
    QByteArray fileId;    
    QString    name;
    qint64     timestamp = 0;
    DWORD      reason = 0;
};

    static QByteArray fileId128ForPath(const QString &path);
    static QString pathByHandle(HANDLE handle);
    static HANDLE openVolumeHandle(const QString &guidPath);

    static void closeHandle(HANDLE handle);
    static std::optional<USN> queryNextUsn(HANDLE volumeHandle);
    static DWORD lastError();
    static QString pathByFileId(HANDLE volumeHandle, const QByteArray &fileId128);
    //static std::optional<USN_JOURNAL_DATA> queryUsnJournal(HANDLE volumeHandle);
    // static std::optional<USN> scanUsnJournal
    // (
    //     HANDLE volumeHandle,
    //     USN startUsn,
    //     DWORD reasonMask,
    //     const std::function<bool(const UsnEvent &)> &callback
    // );
    static std::optional<USN> scanUsnJournal
    (
        HANDLE volumeHandle, USN startUsn, DWORD reasonMask,
        const std::function<bool(const UsnEvent &)> &callback
    );

    static bool isValidHandle(HANDLE handle);
    static HANDLE openFileForWatch(const QString &path);

    // static QByteArray usnRecordFileId(const USN_RECORD *rec);   
    // static QString    usnRecordFileName(const USN_RECORD *rec);
    // static qint64     usnRecordTimestamp(const USN_RECORD *rec);

    static std::optional<USN> readUsnJournalOnce
    (
        HANDLE volumeHandle,
        USN startUsn,
        DWORD reasonMask,
        const std::function<void(const USN_RECORD *)> &onRecord
    );
};