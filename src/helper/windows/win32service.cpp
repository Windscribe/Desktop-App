#include "win32service.h"

#include <spdlog/spdlog.h>

namespace wsl
{

Win32Service* Win32Service::pThis_ = NULL;

Win32Service::Win32Service(const std::wstring &serviceName)
    : serviceName_(serviceName)
{
    pThis_ = this;

    ::ZeroMemory(&serviceStatus, sizeof(serviceStatus));
    serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    serviceStatus.dwCurrentState = SERVICE_STOPPED;
    serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
}

void Win32Service::setServiceExitCode(DWORD code)
{
    serviceStatus.dwWin32ExitCode = code;
}

DWORD Win32Service::serviceExitCode(void) const
{
    return serviceStatus.dwWin32ExitCode;
}

const std::wstring &Win32Service::serviceName(void) const
{
    return serviceName_;
}

bool Win32Service::serviceExiting(void) const
{
    return ((serviceStatus.dwCurrentState == SERVICE_STOP_PENDING) ||
            (serviceStatus.dwCurrentState == SERVICE_STOPPED));
}

void Win32Service::setStatus(DWORD state, DWORD waitHint)
{
    serviceStatus.dwCurrentState = state;
    serviceStatus.dwWaitHint     = waitHint;

    switch (state) {
    case SERVICE_STOP_PENDING:
    case SERVICE_START_PENDING:
        serviceStatus.dwCheckPoint += 1L;
        break;
    default:
        serviceStatus.dwCheckPoint = 0L;
        break;
    }

    if (serviceStatusHandle_ != NULL) {
        ::SetServiceStatus(serviceStatusHandle_, &serviceStatus);
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
        pThis_->serviceStatus.dwWin32ExitCode = NO_ERROR;
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
    case SERVICE_CONTROL_INTERROGATE:
        ::SetServiceStatus(pThis_->serviceStatusHandle_, &pThis_->serviceStatus);
        break;

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
