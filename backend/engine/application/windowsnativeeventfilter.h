#ifndef WINDOWSNATIVEEVENTFILTER_H
#define WINDOWSNATIVEEVENTFILTER_H

#include <QAbstractNativeEventFilter>

class WindowsNativeEventFilter : public QAbstractNativeEventFilter
{
public:
    WindowsNativeEventFilter();
    virtual bool nativeEventFilter(const QByteArray &b, void *message, long *l);

private:
    bool bShutdownAlreadyReceived_;

#ifdef Q_OS_WIN
    unsigned int dwActivateMessage_;
#endif
};

#endif // WINDOWSNATIVEEVENTFILTER_H
