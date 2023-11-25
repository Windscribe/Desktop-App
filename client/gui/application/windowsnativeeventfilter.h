#pragma once

#include <QAbstractNativeEventFilter>

class WindowsNativeEventFilter : public QAbstractNativeEventFilter
{
public:
    WindowsNativeEventFilter();
    virtual bool nativeEventFilter(const QByteArray &b, void *message, qintptr *l);

private:
    bool bShutdownAlreadyReceived_;

#ifdef Q_OS_WIN
    unsigned int dwActivateMessage_;
#endif
};
