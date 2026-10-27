#pragma once

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QSet>
#include <QStringList>
#include <QObject>
#include <QQuickWindow>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <Windows.h>
#include <winioctl.h>
#include <atomic>
#include <functional>

class System: public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit System(QObject *parent = nullptr);

    Q_INVOKABLE void openWith(const QString &filePath, QQuickWindow *window);
    Q_INVOKABLE void openFile(const QString &filePath, QQuickWindow *window);

    Q_INVOKABLE QString renameFile(const QString &filePath, const QString &newName);
    Q_INVOKABLE QStringList moveToTrash(const QStringList &paths);

    struct UsnEvent
    {
        QByteArray fileId;
        QString    name;
        qint64     timestamp = 0;
        DWORD      reason = 0;
    };

    static constexpr USN InvalidUsn = -1;
    static bool isValidUsn(USN usn) { return usn >= 0; }

    static bool isValidHandle(HANDLE handle);
    static void closeHandle(HANDLE handle);

    static HANDLE openVolumeHandle(const QString &guidPath);

    static QStringList mountedNtfsVolumes();
    static bool ensureUsnJournal(HANDLE volumeHandle);
    static HANDLE openFileForWatch(const QString &path);
    static HANDLE openFileById(HANDLE volumeHandle, const QByteArray &fileId128, DWORD access);

    static QByteArray fileId128ForPath(const QString &path);
    static QString pathByHandle(HANDLE handle);
    static QString pathByFileId(HANDLE volumeHandle, const QByteArray &fileId128);
    static QString volumeGuidForPath(const QString &path);

    static qint64 fileSize(HANDLE handle);
    static bool isDeletePending(HANDLE handle);

    static QByteArray sha256OfHandle(HANDLE handle, qint64 &size,
                                     const std::atomic<bool> *cancel = nullptr);

    static QHash<QString, QList<QByteArray>> enumByNames(HANDLE volumeHandle, const QSet<QString> &namesLower,
                                                         const std::atomic<bool> *cancel = nullptr);

    static USN queryNextUsn(HANDLE volumeHandle);

    class UsnReader
    {
    public:
        UsnReader(HANDLE volumeHandle, USN startUsn, DWORD reasonMask);

        bool next(UsnEvent &event);
        USN position() const;

    private:
        bool readBlock();

        HANDLE volume_;
        USN_JOURNAL_DATA journal_{};
        READ_USN_JOURNAL_DATA request_{};
        QByteArray buffer_;
        DWORD bytesRead_ = 0;
        DWORD offset_ = 0;
        bool available_ = false;
        bool finished_ = false;
    };
};
