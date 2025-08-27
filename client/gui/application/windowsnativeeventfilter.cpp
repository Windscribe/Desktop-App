#include "windowsnativeeventfilter.h"

#include <QWidget>

#include "dpiscaleawarewidget.h"
#include "windscribeapplication.h"
#include "utils/log/categories.h"
#include "utils/winutils.h"

WindowsNativeEventFilter::WindowsNativeEventFilter() : QAbstractNativeEventFilter()
{
    bShutdownAlreadyReceived_ = false;
    dwActivateMessage_ = RegisterWindowMessage(WinUtils::wmActivateGui.c_str());
}

bool WindowsNativeEventFilter::nativeEventFilter(const QByteArray &b, void *message, qintptr *result)
{
    Q_UNUSED(b);
    MSG* msg = reinterpret_cast<MSG*>(message);

    if (msg->message == WM_QUERYENDSESSION || msg->message == WM_ENDSESSION) {
        if (msg->message == WM_ENDSESSION && msg->wParam == FALSE) {
            // We log the shutdown was cancelled, but continue shutting down the app since it will
            // have already started cleaning up the backend (engine) by this point.
            qCDebug(LOG_BASIC) << "Windows shutdown interrupted by user";
        }
        // if lParam is 0, the system is shutting down or restarting, otherwise it's a logout
        else if (msg->lParam == 0 && !bShutdownAlreadyReceived_) {
            initiateAppShutdown(msg->hwnd);
            // If we don't want a message to be processed by Qt, return true and set result to
            // the value that the window procedure should return. Otherwise return false.
            *result = FALSE;
            return true;
        }
    } else if (msg->message == WM_DPICHANGED) {
        auto *widget = QWidget::find(WId(msg->hwnd));
        if (widget) {
            auto *dpi_scale_aware_widget = dynamic_cast<DPIScaleAwareWidget*>(widget);
            if (dpi_scale_aware_widget) {
                dpi_scale_aware_widget->checkForAutoResize_win();
            }
        }
    } else if ( msg->message == dwActivateMessage_) {
        qCDebug(LOG_BASIC) << "Windows activate app message received";
        WindscribeApplication::instance()->onActivateFromAnotherInstance();
        return true;
    }

    return false;
}

void WindowsNativeEventFilter::initiateAppShutdown(HWND hwnd)
{
    qCInfo(LOG_BASIC) << "Windows shutdown notification received";
    bShutdownAlreadyReceived_ = true;

    // https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ms700677(v=vs.85)
    // Inform Windows to block the OS shutdown until we have cleanly shut down the VPN.  We must do
    // this before returning FALSE for the WM_QUERYENDSESSION message.
    bool result = ::ShutdownBlockReasonCreate(hwnd, L"Stopping Windscribe VPN");
    if (!result) {
        qCWarning(LOG_BASIC) << "WindowsNativeEventFilter ShutdownBlockReasonCreate failed:" << ::GetLastError();
    }

    // Invoke this method queued so we can return from the WM_QUERYENDSESSION immediately, allowing Windows to
    // display the block reason.
    result = QMetaObject::invokeMethod(WindscribeApplication::instance(), &WindscribeApplication::initiateAppShutdown, Qt::QueuedConnection);
    if (!result) {
        qCWarning(LOG_BASIC) << "WindowsNativeEventFilter invokeMethod failed";
    }
}
