#include "win32service.h"

#include <spdlog/spdlog.h>

namespace wsl
{

Win32Service* Win32Service::pThis_ = NULL;

Win32Service::Win32Service(const std::wstring &serviceName)
    : serviceName_(serviceName)
{
    pThis_ = this;

    ::ZeroMemory(&serviceStatus_, sizeof(serviceStatus_));
    serviceStatus_.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    serviceStatus_.dwCurrentState = SERVICE_STOPPED;
    serviceStatus_.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
}

const std::wstring &Win32Service::serviceName(void) const
{
    return serviceName_;
}

bool Win32Service::serviceExiting(void) const
{
    std::lock_guard<std::mutex> lock(statusMutex_);
    return ((serviceStatus_.dwCurrentState == SERVICE_STOP_PENDING) ||
            (serviceStatus_.dwCurrentState == SERVICE_STOPPED));
}

void Win32Service::setStatus(DWORD state, DWORD waitHint)
{
    std::lock_guard<std::mutex> lock(statusMutex_);

    serviceStatus_.dwCurrentState = state;
    serviceStatus_.dwWaitHint     = waitHint;

    switch (state) {
    case SERVICE_STOP_PENDING:
    case SERVICE_START_PENDING:
        serviceStatus_.dwCheckPoint += 1L;
        break;
    default:
        serviceStatus_.dwCheckPoint = 0L;
        break;
    }

    // Report to the SCM outside the lock, using a consistent snapshot taken under it.
    if (serviceStatusHandle_ != NULL) {
        ::SetServiceStatus(serviceStatusHandle_, &serviceStatus_);
    }
}

void Win32Service::startService()
{
#if defined(RUN_AS_CONSOLE_APP)
    if (::SetConsoleCtrlHandler(consoleHandlerRoutine, TRUE) == 0) {
        spdlog::error("SetConsoleCtrlHandler failed: {}", ::GetLastError());
        return;
    }

    serviceMain(0, NULL);
#else
    SERVICE_TABLE_ENTRY st[] = {
        { (wchar_t*)serviceName_.c_str(), serviceMain },
        { NULL, NULL }
    };

    if (!::StartServiceCtrlDispatcher(st)) {
        spdlog::error("The control dispatcher for the service could not be started.  The service startup was stopped with error {}", ::GetLastError());
    }
#endif
}

void WINAPI Win32Service::serviceMain(DWORD dwArgc, wchar_t* lpszArgv[])
{
    (void)dwArgc;
    (void)lpszArgv;

#if !defined(RUN_AS_CONSOLE_APP)
    pThis_->serviceStatusHandle_ = ::RegisterServiceCtrlHandlerEx(pThis_->serviceName_.c_str(), SCMHandler, NULL);
    if (pThis_->serviceStatusHandle_ == NULL) {
        spdlog::error("The control handler for the service could not be installed: {}", ::GetLastError());
        return;
    }
#endif

    pThis_->setStatus(SERVICE_START_PENDING, 1000);
    if (pThis_->onInit()) {
        pThis_->setStatus(SERVICE_RUNNING);
        pThis_->runService();
    }
    pThis_->setStatus(SERVICE_STOPPED);
}

DWORD WINAPI Win32Service::SCMHandler(DWORD controlCode, DWORD eventType, LPVOID eventData, LPVOID context)
{
    switch (controlCode) {
    case SERVICE_CONTROL_STOP:
        pThis_->onStopRequest();
        break;

    // Requests the service to immediately report its current status information to the SCM.
    case SERVICE_CONTROL_INTERROGATE: {
        std::lock_guard<std::mutex> lock(pThis_->statusMutex_);
        ::SetServiceStatus(pThis_->serviceStatusHandle_, &pThis_->serviceStatus_);
        break;
    }

    case SERVICE_CONTROL_SHUTDOWN:
        pThis_->onWindowsShutdown();
        break;

    default:
        // Messages sent by the Win32 ControlService() API call are handled by this method.  User-defined messages must be in the range 128-256.
        if ((controlCode >= 128) && (controlCode <= 256)) {
            pThis_->onUserControl(controlCode);
        }
        break;
    }

    return NO_ERROR;
}

#if defined(RUN_AS_CONSOLE_APP)
BOOL WINAPI Win32Service::consoleHandlerRoutine(DWORD ctrlType)
{
    BOOL bHandled;

    switch (ctrlType) {
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        bHandled = TRUE;
        pThis_->onWindowsShutdown();
        break;

    default:
        bHandled = FALSE;
        break;
    }

    return bHandled;
}
#endif

} // end namespace wsl
