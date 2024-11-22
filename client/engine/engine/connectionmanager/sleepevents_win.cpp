#include "sleepevents_win.h"
#include "utils/ws_assert.h"
#include "utils/log/categories.h"

SleepEvents_win *SleepEvents_win::this_ = NULL;

SleepEvents_win::SleepEvents_win(QObject *parent) : ISleepEvents(parent), hwnd_(0)
{
    WS_ASSERT(this_ == NULL);
    this_ = this;
    hThread_ = CreateThread(NULL, 0, hiddenWindowThread, NULL, 0, NULL);
    if (!hThread_)
    {
        qCDebug(LOG_BASIC) << "SleepEvents_win::SleepEvents_win(), can't create thread";
    }
}

SleepEvents_win::~SleepEvents_win()
{
    PostMessage(hwnd_, WM_CLOSE, 0, 0);
    CloseWindow(hwnd_);
    WaitForSingleObject(hThread_, INFINITE);
    CloseHandle(hThread_);
    this_ = NULL;
}

DWORD SleepEvents_win::hiddenWindowThread(void *param)
{
    Q_UNUSED(param);
    static const wchar_t* className = L"WINDSCRIBE_HIDDEN_WINDOW";
    this_->hwnd_ = NULL;
    WNDCLASSEX wx = {};
    wx.cbSize = sizeof(WNDCLASSEX);
    wx.lpfnWndProc = wndProc;        // function which will handle messages
    wx.hInstance = (HINSTANCE)::GetModuleHandle(NULL);
    wx.lpszClassName = className;
    if ( RegisterClassEx(&wx) )
    {
        this_->hwnd_ = CreateWindow( className, className, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
        if (!this_->hwnd_)
        {
            qCDebug(LOG_BASIC) << "SleepEvents_win::hiddenWindowThread(), can't create window";
            return 0;
        }
    }
    else
    {
        qCDebug(LOG_BASIC) << "SleepEvents_win::hiddenWindowThread(), can't register window class";
        return 0;
    }

    BOOL bRet;
    MSG msg;
    while( (bRet = GetMessage( &msg, this_->hwnd_, 0, 0 )) != 0)
    {
        if (bRet == -1)
        {
            return 0;
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return 0;
}

LRESULT SleepEvents_win::wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_POWERBROADCAST)
    {
        if (wParam == PBT_APMRESUMEAUTOMATIC)
        {
            emit this_->gotoWake();
        }
        else if (wParam == PBT_APMSUSPEND)
        {
            emit this_->gotoSleep();
        }
    }
    else if (uMsg == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
