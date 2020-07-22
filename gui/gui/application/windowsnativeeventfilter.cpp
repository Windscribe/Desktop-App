#include "windowsnativeeventfilter.h"
#include "windscribeapplication.h"
#include <QWidget>
#include "Utils/logger.h"

#ifdef Q_OS_WIN
    #include <Windows.h>
    #include "Utils/winutils.h"
#endif

WindowsNativeEventFilter::WindowsNativeEventFilter() : QAbstractNativeEventFilter()
{
    bShutdownAlreadyReceived_ = false;
#ifdef Q_OS_WIN
    dwActivateMessage_ = RegisterWindowMessage(WinUtils::wmActivateGui.c_str());
    dwOpenLocations_ = RegisterWindowMessage(WinUtils::wmOpenGuiLocations.c_str());
#endif
}

bool WindowsNativeEventFilter::nativeEventFilter(const QByteArray &b, void *message, long *l)
{
    Q_UNUSED(b);
    Q_UNUSED(l);
#ifdef Q_OS_WIN

    MSG* msg = reinterpret_cast<MSG*>(message);

    if ( msg->message == WM_QUERYENDSESSION || msg->message == WM_ENDSESSION )
    {

        if (msg->message == WM_ENDSESSION && msg->wParam == FALSE)
        {
            qCDebug(LOG_BASIC) << "Windows shutdown interrupted by user";
            WindscribeApplication::instance()->clearWasRestartOSFlag();
            bShutdownAlreadyReceived_ = false;
        }
        else
        {
            if (!bShutdownAlreadyReceived_)
            {
                qCDebug(LOG_BASIC) << "Windows shutdown received";
                WindscribeApplication::instance()->setWasRestartOSFlag();
                bShutdownAlreadyReceived_ = true;
            }
        }
    }
    else if ( msg->message == dwActivateMessage_)
    {
        qCDebug(LOG_BASIC) << "Windows activate app message received";
        WindscribeApplication::instance()->onActivateFromAnotherInstance();
        return true;
    }
    else if (msg->message == dwOpenLocations_)
    {
        qCDebug(LOG_BASIC) << "Windows open locations message received";
        WindscribeApplication::instance()->onOpenLocationsFromAnotherInstance();
        return true;
    }
#else
    Q_UNUSED(b);
    Q_UNUSED(message);
    Q_UNUSED(l);
#endif
    return false;
}

