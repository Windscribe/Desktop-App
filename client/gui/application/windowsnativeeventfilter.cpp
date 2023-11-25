#include "windowsnativeeventfilter.h"
#include "windscribeapplication.h"
#include <QWidget>
#include "utils/logger.h"

#ifdef Q_OS_WIN
    #include <windows.h>
    #include "utils/winutils.h"
    #include "dpiscaleawarewidget.h"
#endif

WindowsNativeEventFilter::WindowsNativeEventFilter() : QAbstractNativeEventFilter()
{
    bShutdownAlreadyReceived_ = false;
#ifdef Q_OS_WIN
    dwActivateMessage_ = RegisterWindowMessage(WinUtils::wmActivateGui.c_str());
#endif
}

bool WindowsNativeEventFilter::nativeEventFilter(const QByteArray &b, void *message, qintptr *l)
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
    else if (msg->message == WM_DPICHANGED)
    {
        auto *widget = QWidget::find(WId(msg->hwnd));
        if (widget) {
            auto *dpi_scale_aware_widget = dynamic_cast<DPIScaleAwareWidget*>(widget);
            if (dpi_scale_aware_widget)
                dpi_scale_aware_widget->checkForAutoResize_win();
        }
    }
    else if ( msg->message == dwActivateMessage_)
    {
        qCDebug(LOG_BASIC) << "Windows activate app message received";
        WindscribeApplication::instance()->onActivateFromAnotherInstance();
        return true;
    }
    else if (msg->message == WM_WININICHANGE)
    {
        // WM_WININICHANGE fires when OS light/dark mode is updated
        WindscribeApplication::instance()->onWinIniChanged();
        return true;
    }
#else
    Q_UNUSED(b);
    Q_UNUSED(message);
    Q_UNUSED(l);
#endif
    return false;
}

