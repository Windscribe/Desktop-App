#pragma once

#include <QAbstractNativeEventFilter>
#include <qt_windows.h>

class WindowsNativeEventFilter : public QAbstractNativeEventFilter
{
public:
    WindowsNativeEventFilter();
    virtual bool nativeEventFilter(const QByteArray &b, void *message, qintptr *result);

private:
    bool bShutdownAlreadyReceived_;
    unsigned int dwActivateMessage_;

    void initiateAppShutdown(HWND hwnd);
};
