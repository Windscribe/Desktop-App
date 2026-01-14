#pragma once

#include <windows.h>
#include <string>

namespace wsl
{

class Win32Service
{
public:
    bool serviceExiting(void) const;
    DWORD serviceExitCode(void) const;

    // Starts the service and does not return until the service, or console app, has exited.
    void startService();

    const std::wstring& serviceName(void) const;

    // TECH NOTE: the 20 second shutdown delay can be changed by modifying the WaitToKillServiceTimeout
    // value in the registry key HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control.

protected:
    explicit Win32Service(const std::wstring &serviceName);
    virtual ~Win32Service(void) {};

    void setServiceExitCode(DWORD code);
    void setStatus(DWORD state, DWORD waitHint = 0);

    // Called when the service is initializing.  If the initialization time is
    // expected to be longer than one second, onInit should call the setStatus
    // method specifying SERVICE_START_PENDING for the state.  As initialization
    // continues, onInit should make additional calls to SetStatus to report its
    // progress.  Sending multiple setStatus calls is also useful for debugging
    // the service.
    virtual bool onInit(void) = 0;

    // This method must be provided by the inheriting class.  It implements
    // the actual processing performed by the service.  The service exits
    // when this method returns.
    virtual void runService(void) = 0;

    // The following virtual methods are called by the SCM control handler.
    // The control handler must return within 30 seconds, or the SCM will return
    // an error.  If a service needs to do lengthy processing within one of these
    // methods, it should create a secondary thread to perform the lengthy
    // processing, then return.  This prevents the service from tying up the
    // control dispatcher.  For example, when handling the stop request for a
    // service that will take a long time; create or signal another thread to
    // handle the stop process, call setStatus passing SERVICE_STOP_PENDING,
    // and return.

    // Called to handle a user request sent through the SCM.
    virtual void onUserControl(DWORD controlCode) = 0;

    // Called when the SCM is instructing the service to stop.  Do NOT perform any
    // lengthy processing in this method or you will block the SCM's control dispatcher
    // thread.
    virtual void onStopRequest(void) = 0;

    // Called when the SCM notifies the service that Windows is shutting down.
    // Due to extremely limited time available for shutdown, this method
    // should only be used by services that absolutely need to shut down
    // (eg. the service needs to shut down so that network connections
    // aren't made when the system is in the shutdown state).  If the service
    // takes time to shut down, and sends out STOP_PENDING status messages,
    // it is highly recommended that these messages include a waithint so
    // that the SCM will know how long to wait before indicating to the system
    // that service shutdown is complete.  The system gives the SCM a limited
    // amount of time (about 20 seconds) to complete service shutdown, after
    // which time system shutdown proceeds regardless of whether service
    // shutdown is complete.
    virtual void onWindowsShutdown(void) = 0;

private:
    const std::wstring serviceName_;

    SERVICE_STATUS_HANDLE serviceStatusHandle_ = NULL;
    SERVICE_STATUS serviceStatus;

    // WARNING: this limits the client app to only one Win32Service object.
    static Win32Service* pThis_;

private:
    // Static methods used as callbacks for the SCM.
    static void WINAPI serviceMain(DWORD dwArgc, wchar_t *lpszArgv[]);
    static DWORD WINAPI SCMHandler(DWORD controlCode, DWORD eventType, LPVOID eventData, LPVOID context);

#if defined(RUN_AS_CONSOLE_APP)
    static BOOL WINAPI consoleHandlerRoutine(DWORD ctrlType);
#endif
};

} // end namespace wsl
