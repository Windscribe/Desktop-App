#pragma once

#include <QObject>
#include "isleepevents.h"
#include <windows.h>

class SleepEvents_win : public ISleepEvents
{
    Q_OBJECT
public:
    explicit SleepEvents_win(QObject *parent);
    virtual ~SleepEvents_win();

private:
    static DWORD WINAPI hiddenWindowThread(void* param);
    static LRESULT CALLBACK wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    HANDLE hThread_;
    HWND hwnd_;
    static SleepEvents_win *this_;
};
