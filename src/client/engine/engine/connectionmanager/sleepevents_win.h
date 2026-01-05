#pragma once

#include <QObject>
#include <qt_windows.h>

#include "isleepevents.h"
#include "utils/win32handle.h"

class SleepEvents_win : public ISleepEvents
{
    Q_OBJECT
public:
    explicit SleepEvents_win(QObject *parent);
    virtual ~SleepEvents_win();

private:
    wsl::Win32Handle hThread_;
    HWND hwnd_ = NULL;
    static SleepEvents_win *this_;

    static DWORD WINAPI hiddenWindowThread(void* param);
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
