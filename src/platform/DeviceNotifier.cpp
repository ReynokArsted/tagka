#include "DeviceNotifier.h"

#include <QDebug>

#include <dbt.h>

DeviceNotifier::DeviceNotifier(Listener *listener) : listener_(listener)
{
    static const wchar_t kClass[] = L"TaggerVolumeNotifyWindow";

    WNDCLASSW wc{};
    wc.lpfnWndProc = &DeviceNotifier::wndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = kClass;
    RegisterClassW(&wc);

    hwnd_ = CreateWindowExW(0, kClass, L"", WS_POPUP, 0, 0, 0, 0,
                            nullptr, nullptr, wc.hInstance, this);
    if (!hwnd_) qWarning() << "ERROR: cannot create device notification window" << GetLastError();
}

DeviceNotifier::~DeviceNotifier()
{
    if (hwnd_) DestroyWindow(hwnd_);
}

HDEVNOTIFY DeviceNotifier::subscribe(HANDLE volumeHandle)
{
    HDEVNOTIFY notify = nullptr;

    if (hwnd_)
    {
        DEV_BROADCAST_HANDLE filter{};
        filter.dbch_size = sizeof(filter);
        filter.dbch_devicetype = DBT_DEVTYP_HANDLE;
        filter.dbch_handle = volumeHandle;

        notify = RegisterDeviceNotificationW(hwnd_, &filter, DEVICE_NOTIFY_WINDOW_HANDLE);
        if (!notify)
            qWarning() << "WARN: cannot subscribe to removal notifications, error" << GetLastError();
    }
    return notify;
}

void DeviceNotifier::unsubscribe(HDEVNOTIFY notify)
{
    if (notify) UnregisterDeviceNotification(notify);
}

LRESULT CALLBACK DeviceNotifier::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE)
    {
        const auto *cs = reinterpret_cast<const CREATESTRUCTW *>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
    }
    else if (msg == WM_DEVICECHANGE)
    {
        auto *self = reinterpret_cast<DeviceNotifier *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        const bool release = wParam == DBT_DEVICEQUERYREMOVE
                          || wParam == DBT_DEVICEREMOVEPENDING
                          || wParam == DBT_DEVICEREMOVECOMPLETE;

        if (self && release && lParam)
        {
            const auto *hdr = reinterpret_cast<const DEV_BROADCAST_HDR *>(lParam);
            if (hdr->dbch_devicetype == DBT_DEVTYP_HANDLE)
                self->listener_->onReleaseRequested
                    (reinterpret_cast<const DEV_BROADCAST_HANDLE *>(lParam)->dbch_hdevnotify);
        }
        return TRUE;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
