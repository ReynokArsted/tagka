#pragma once

#include <Windows.h>

class DeviceNotifier
{
public:
    class Listener
    {
    public:
        virtual void onReleaseRequested(HDEVNOTIFY notify) = 0;

    protected:
        ~Listener() = default;
    };

    explicit DeviceNotifier(Listener *listener);
    ~DeviceNotifier();

    DeviceNotifier(const DeviceNotifier &) = delete;
    DeviceNotifier &operator=(const DeviceNotifier &) = delete;

    HDEVNOTIFY subscribe(HANDLE volumeHandle);
    void unsubscribe(HDEVNOTIFY notify);

private:
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND hwnd_ = nullptr;
    Listener *listener_;
};
