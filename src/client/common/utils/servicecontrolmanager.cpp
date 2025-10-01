#include "servicecontrolmanager.h"

#include <NTSecAPI.h>
#include <sddl.h>
#include <shellapi.h>

#include <codecvt>
#include <sstream>

namespace wsl
{

static std::string wstring_to_string(const std::wstring &wideStr)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(wideStr);
}

//---------------------------------------------------------------------------
// ServiceControlManager class implementation

/******************************************************************************
* METHOD:  ServiceControlManager constructor
*
* PURPOSE: Constructs an instance of the ServiceControlManager class and initializes the
*          member data.  The user must then call OpenSCM() to open a connection
*          to a specific Service Control Manager before calling any other
*          methods.
*******************************************************************************/
ServiceControlManager::ServiceControlManager()
{
}

/******************************************************************************
* METHOD:  ServiceControlManager destructor
*
* PURPOSE: Destructs an instance of the ServiceControlManager class.
*
* INPUT:   None.
*******************************************************************************/
ServiceControlManager::~ServiceControlManager()
{
    closeSCM();
}

/******************************************************************************
* METHOD:  openSCM
*
* PURPOSE: Opens a connection to the specified SCM.
*
* INPUT:   desiredAccess: desired access rights, as defined by the OpenSCManager
*             Win32 API.
*
*          serverName: Points to a null-terminated string that names the
*             target computer. If the pointer is NULL or points to an empty
*             string, the method connects to the SCM on the local computer.
*
* THROWS:  A system_error object.
*******************************************************************************/
void ServiceControlManager::openSCM(DWORD desiredAccess, LPCTSTR serverName)
{
    closeSCM();

    scm_ = ::OpenSCManager(serverName, NULL, desiredAccess);

    if (scm_ != NULL) {
        if (serverName != NULL) {
            serverName_ = serverName;
        }
        return;
    }

    DWORD lastError = ::GetLastError();
    std::wostringstream errorMsg;

    switch (lastError) {
    case ERROR_ACCESS_DENIED:
        errorMsg << "openSCM: insufficient user rights to open SCM on server " << serverNameForDebug();
        break;

    case ERROR_INVALID_PARAMETER:
        errorMsg << "openSCM: one of the parameters is incorrect";
        break;

    default:
        errorMsg << "openSCM failed to open SCM on server " << serverNameForDebug() << " - " << lastError;
        break;
    }

    throw std::system_error(lastError, std::system_category(), wstring_to_string(errorMsg.str()));
}

/******************************************************************************
* METHOD:  closeSCM()
*
* PURPOSE: Closes the open SCM and service handles.
*
* THROWS:  This method does not throw an exception.  The method may be called
*          for 'clean-up' after another ServiceControlManager method raises an exception.
*          Thus we don't want this method raising an exception thereby 'cancelling'
*          the first exception.
*******************************************************************************/
void ServiceControlManager::closeSCM() noexcept
{
    closeService();

    if (scm_ != NULL) {
        ::CloseServiceHandle(scm_);
        scm_ = NULL;
    }

    serverName_.clear();
}

/******************************************************************************
* METHOD:  openService
*
* PURPOSE: Opens a handle to an existing service.
*          OpenSCM must be called before calling this method.
*
* INPUT:   serviceName: Points to a null-terminated string that names the
*             service to open. The maximum string length is 256 characters. The
*             SCM database preserves the case of the characters, but service
*             name comparisons are always case insensitive. A slash (/),
*             backslash (\), comma, and space are invalid service name
*             characters.
*
* THROWS:  A system_error object.
*******************************************************************************/
void ServiceControlManager::openService(LPCTSTR serviceName, DWORD desiredAccess)
{
    std::error_code ec;
    auto result = openService(serviceName, desiredAccess, ec);

    if (result) {
        return;
    }

    std::wostringstream errorMsg;

    switch (ec.value()) {
    case ERROR_ACCESS_DENIED:
        errorMsg << "openService: insufficient user rights to open service - " << serviceName;
        break;

    case ERROR_INVALID_HANDLE:
        errorMsg << "openService: the SCM is not open to access service - " << serviceName;
        break;

    case ERROR_INVALID_NAME:
        errorMsg << "openService: service name parameter has an invalid format - " << serviceName;
        break;

    case ERROR_SERVICE_DOES_NOT_EXIST:
        errorMsg << "openService: no service with the specified name exists on this server - " << serviceName;
        break;

    default:
        errorMsg << "openService failed to open service " << serviceName << " - " << ec.value();
        break;
    }

    throw std::system_error(ec, wstring_to_string(errorMsg.str()));
}

bool ServiceControlManager::openService(LPCTSTR serviceName, DWORD desiredAccess, std::error_code& ec) noexcept
{
    ec.clear();

    if (serviceName == NULL) {
        ec.assign(ERROR_INVALID_NAME, std::system_category());
        return false;
    }

    closeService();

    service_ = ::OpenService(scm_, serviceName, desiredAccess);

    if (service_ == NULL) {
        ec.assign(::GetLastError(), std::system_category());
        return false;
    }

    serviceName_ = serviceName;
    return true;
}

/******************************************************************************
* METHOD:  closeService
*
* PURPOSE: Closes an open service handle.
*
* INPUT:   None.
*
* THROWS:  This method does not throw an exception.  The method may be called
*          for 'clean-up' after another ServiceControlManager method raises an exception.
*          Thus we don't want this method raising an exception thereby 'cancelling'
*          the first exception.
*******************************************************************************/
void ServiceControlManager::closeService() noexcept
{
    if (service_ != NULL) {
        ::CloseServiceHandle(service_);
        service_ = NULL;
        serviceName_.clear();
    }
}

/******************************************************************************
* METHOD:  queryServiceStatus
*
* PURPOSE: Determines the status (started, stopped, pending, etc.) of the
*          service.  OpenService must be called before calling this method.
*
* OUTPUT:  dwStatus: receives one of the following status codes:
*
*          SERVICE_STOPPED
*          SERVICE_START_PENDING
*          SERVICE_STOP_PENDING
*          SERVICE_RUNNING
*          SERVICE_CONTINUE_PENDING
*          SERVICE_PAUSE_PENDING
*          SERVICE_PAUSED
*
* THROWS:  A system_error object.
*******************************************************************************/
DWORD ServiceControlManager::queryServiceStatus() const
{
    std::error_code ec;
    const auto status = queryServiceStatus(ec);

    if (!ec) {
        return status;
    }

    std::wostringstream errorMsg;

    switch (ec.value()) {
    case ERROR_ACCESS_DENIED:
        errorMsg << "queryServiceStatus: insufficient user rights to query the service - " << serviceName_;
        break;

    case ERROR_INVALID_HANDLE:
        errorMsg << "queryServiceStatus: the service has not been opened - " << serviceName_;
        break;

    default:
        errorMsg << "queryServiceStatus failed to query the service " << serviceName_ << " - " << ec.value();
        break;
    }

    throw std::system_error(ec, wstring_to_string(errorMsg.str()));
}

DWORD ServiceControlManager::queryServiceStatus(std::error_code& ec) const noexcept
{
    ec.clear();

    SERVICE_STATUS status;
    if (::QueryServiceStatus(service_, &status)) {
        return status.dwCurrentState;
    }

    ec.assign(::GetLastError(), std::system_category());
    return 0;
}

/******************************************************************************
* METHOD:  queryServiceConfig
*
* PURPOSE: Retrieves the configuration parameters of the service.
*          OpenService must be called before calling this method.
*
* OUTPUT:  exePath: receives the full path to the service's executable.
*
*          accountName: receives the account name in the form of
*             DomainName\Username, which the service process will be logged
*             on as when it runs.
*
*          startType: receives one of the following status codes:
*
*             SERVICE_AUTO_START    - NT starts service automatically
*             SERVICE_DEMAND_START  - user must start the service
*             SERVICE_DISABLED      - service cannot be started
*
*          serviceShareProcess: true if the service shares a process with
*             other services.
*
* THROWS:  A system_error object.
*******************************************************************************/
void ServiceControlManager::queryServiceConfig(std::wstring &exePath, std::wstring &accountName,
                                               DWORD& startType, bool& serviceShareProcess) const
{
    DWORD numBytesNeeded = 0;
    DWORD bufferSize = 1024;
    std::unique_ptr< unsigned char[] > buffer(new unsigned char[bufferSize]);

    bool secondTry = false;

    for (int i = 0; i < 2; i++) {
        if (::QueryServiceConfig(service_, (LPQUERY_SERVICE_CONFIG)buffer.get(),
                                 bufferSize, &numBytesNeeded)) {
            break;
        }

        DWORD lastError = ::GetLastError();
        std::wostringstream errorMsg;

        switch (lastError) {
        case ERROR_INSUFFICIENT_BUFFER:
            if (secondTry) {
                errorMsg << "queryServiceConfig: insufficient buffer space to query the service's configuration - " << serviceName_;
                break;
            }

            bufferSize = numBytesNeeded;
            buffer.reset(new unsigned char[bufferSize]);
            numBytesNeeded = 0;
            secondTry = true;
            continue;

        case ERROR_ACCESS_DENIED:
            errorMsg << "queryServiceConfig: insufficient user rights to query the service's configuration - " << serviceName_;
            break;

        case ERROR_INVALID_HANDLE:
            errorMsg << "queryServiceConfig: the service has not been opened - " << serviceName_;
            break;

        default:
            errorMsg << "queryServiceConfig failed to query the service " << serviceName_ << " - " << lastError;
            break;
        }

        throw std::system_error(lastError, std::system_category(), wstring_to_string(errorMsg.str()));
    }

    LPQUERY_SERVICE_CONFIG lpConfig = (LPQUERY_SERVICE_CONFIG)buffer.get();
    startType           = lpConfig->dwStartType;
    exePath             = lpConfig->lpBinaryPathName;
    serviceShareProcess = (lpConfig->dwServiceType == SERVICE_WIN32_SHARE_PROCESS);
    accountName         = lpConfig->lpServiceStartName;
}

/******************************************************************************
* METHOD:  startService
*
* PURPOSE: Instructs the SCM to start the service.  OpenService must be called
*          before calling this method.
*
* INPUT:   None.
*
* THROWS:  A system_error object.
*******************************************************************************/
void ServiceControlManager::startService()
{
    std::error_code ec;
    if (startService(ec)) {
        return;
    }

    std::wostringstream errorMsg;

    switch (ec.value()) {
    case ERROR_ACCESS_DENIED:
        errorMsg << "startService: insufficient user rights to request the service to start - " << serviceName_;
        break;

    case ERROR_INVALID_HANDLE:
        errorMsg << "startService: the service has not been opened - " << serviceName_;
        break;

    case ERROR_PATH_NOT_FOUND:
        errorMsg << "startService: the executable for the service could not be found - " << serviceName_;
        break;

    case ERROR_SERVICE_DATABASE_LOCKED:
        errorMsg << "startService: the SCM database is locked by another process - " << serviceName_;
        break;

    case ERROR_SERVICE_DEPENDENCY_DELETED:
        errorMsg << "startService: the service depends on a service that does not exist or has been marked for deletion - " << serviceName_;
        break;

    case ERROR_SERVICE_DEPENDENCY_FAIL:
        errorMsg << "startService: the service depends on another service that has failed to start - " << serviceName_;
        break;

    case ERROR_SERVICE_DISABLED:
        errorMsg << "startService: the service has been disabled - " << serviceName_;
        break;

    case ERROR_SERVICE_LOGON_FAILED:
        errorMsg << "startService: the service failed to logon.  Check its startup logon parameters in the control panel - " << serviceName_;
        break;

    case ERROR_SERVICE_MARKED_FOR_DELETE:
        errorMsg << "startService: the service has been marked for deletion - " << serviceName_;
        break;

    case ERROR_SERVICE_NO_THREAD:
        errorMsg << "startService: a thread could not be created for the service - " << serviceName_;
        break;

    case ERROR_SERVICE_REQUEST_TIMEOUT:
        errorMsg << "startService: request timed out - " << serviceName_;
        break;

    case ERROR_SERVICE_NOT_ACTIVE:
        errorMsg << "startService: request succeeded, but the service aborted its startup - " << serviceName_;
        break;

    case ERROR_SERVICE_START_HANG:
        errorMsg << "startService: request succeeded, but the service did not report as running within the timeout period - " << serviceName_;
        break;

    default:
        errorMsg << "startService failed to start the service " << serviceName_ << " - " << ec.value();
        break;
    }

    throw std::system_error(ec, wstring_to_string(errorMsg.str()));
}

bool ServiceControlManager::startService(std::error_code& ec) noexcept
{
    ec.clear();

    if (blockStartStopRequests_) {
        ec.assign(ERROR_CANCELLED, std::system_category());
        return false;
    }

    DWORD lastError;
    if (::StartService(service_, 0, NULL)) {
        // Wait for start service command to complete.
        for (int i = 0; !blockStartStopRequests_ && i < 200; i++) {
            DWORD status = queryServiceStatus(ec);
            if (ec) {
                return false;
            }

            if (status == SERVICE_RUNNING) {
                return true;
            }

            if (status != SERVICE_START_PENDING) {
                ec.assign(ERROR_SERVICE_NOT_ACTIVE, std::system_category());
                return false;
            }

            ::Sleep(100);
        }

        lastError = (blockStartStopRequests_ ? ERROR_CANCELLED : ERROR_SERVICE_START_HANG);
        ec.assign(lastError, std::system_category());
        return false;
    }

    lastError = ::GetLastError();

    if (lastError == ERROR_SERVICE_ALREADY_RUNNING) {
        return true;
    }

    ec.assign(lastError, std::system_category());
    return false;
}

/******************************************************************************
* METHOD:  stopService
*
* PURPOSE: Instructs the SCM to stop the service.  OpenService must be called
*          before calling this method.
*
* INPUT:   None.
*
* THROWS:  A system_error object.
*******************************************************************************/
void ServiceControlManager::stopService()
{
    std::error_code ec;
    if (stopService(ec)) {
        return;
    }

    std::wostringstream errorMsg;

    switch (ec.value()) {
    case ERROR_ACCESS_DENIED:
        errorMsg << "stopService: insufficient user rights request the service to stop - " << serviceName_;
        break;

    case ERROR_INVALID_HANDLE:
        errorMsg << "stopService: the service has not been opened - " << serviceName_;
        break;

    case ERROR_DEPENDENT_SERVICES_RUNNING:
        errorMsg << "stopService: the service cannot be stopped because other running services are dependent on it - " << serviceName_;
        break;

    case ERROR_INVALID_SERVICE_CONTROL:
        errorMsg << "stopService: the service does not accept stop requests - " << serviceName_;
        break;

    case ERROR_SERVICE_CANNOT_ACCEPT_CTRL:
        errorMsg << "stopService: the service is already stopped or is in a pending state - " << serviceName_;
        break;

    case ERROR_SERVICE_REQUEST_TIMEOUT:
        errorMsg << "stopService: request timed out - " << serviceName_;
        break;

    default:
        errorMsg << "stopService failed to stop the service " << serviceName_ << " - " << ec.value();
        break;
    }

    throw std::system_error(ec, wstring_to_string(errorMsg.str()));
}

bool ServiceControlManager::stopService(std::error_code& ec) noexcept
{
    ec.clear();

    if (blockStartStopRequests_) {
        ec.assign(ERROR_CANCELLED, std::system_category());
        return false;
    }

    SERVICE_STATUS status;
    auto result = ::ControlService(service_, SERVICE_CONTROL_STOP, &status);
    if (result) {
        // Wait for stop service command to complete.
        DWORD currentState = status.dwCurrentState;

        // Wait at most 20 seconds for the service to stop.
        for (int i = 0; (!blockStartStopRequests_ && (currentState != SERVICE_STOPPED) && (i < 200)); i++) {
            ::Sleep(100);
            currentState = queryServiceStatus(ec);
            if (ec) {
                return false;
            }
        }

        if (currentState == SERVICE_STOPPED) {
            return true;
        }

        DWORD error = (blockStartStopRequests_ ? ERROR_CANCELLED : ERROR_SERVICE_REQUEST_TIMEOUT);
        ec.assign(error, std::system_category());
        return false;
    }

    DWORD lastError = ::GetLastError();
    if (lastError == ERROR_SERVICE_NOT_ACTIVE) {
        return true;
    }

    ec.assign(lastError, std::system_category());
    return false;
}

/******************************************************************************
* METHOD:  stopService
*
* PURPOSE: Opens the SCM and instructs it to stop the service if it is installed.
*
* INPUT:   serviceName: Points to a null-terminated string that names the
*             service to open. The maximum string length is 256 characters. The
*             SCM database preserves the case of the characters, but service
*             name comparisons are always case insensitive. A slash (/),
*             backslash (\), comma, and space are invalid service name
*             characters.
*
* THROWS:  A system_error object.
*******************************************************************************/
void ServiceControlManager::stopService(LPCTSTR serviceName)
{
    openSCM(SC_MANAGER_CONNECT);
    if (isServiceInstalled(serviceName)) {
        openService(serviceName, SERVICE_QUERY_STATUS | SERVICE_STOP);
        stopService();
    }
}

/******************************************************************************
* METHOD:  deleteService
*
* PURPOSE: Initiates deletion of the service from the SCM database.
*
* NOTES:   - OpenSCM must be called before calling this method.
*          - The service must exist.
*          - This method does not wait for the service to be deleted.  If the
*            system/SCM are busy, it may take some time for the service to be
*            actually deleted from the SCM database.
*
* INPUT:   serviceName: Points to a null-terminated string that names the
*             service to delete. The maximum string length is 256 characters.
*             The SCM database preserves the character's case, but service
*             name comparisons are always case insensitive. A slash (/),
*             backslash (\), comma, and space are invalid service name
*             characters.
*
*          stopRunningService: attempt to stop the service, if it's running,
*             before asking the SCM to delete it.
*
* THROWS:  A system_error object.
*******************************************************************************/
void ServiceControlManager::deleteService(LPCTSTR serviceName, bool stopRunningService)
{
    openService(serviceName);

    if (stopRunningService) {
        if (queryServiceStatus() != SERVICE_STOPPED) {
            stopService();
        }
    }

    BOOL result = ::DeleteService(service_);

    // Get error code before CloseService possibly changes it.
    DWORD lastError = ::GetLastError();

    // Close our handle to the service so the SCM can delete it.
    closeService();

    if (result) {
        return;
    }

    std::wostringstream errorMsg;

    switch (lastError) {
    case ERROR_ACCESS_DENIED:
        errorMsg << "deleteService: insufficient user rights to delete service - " << serviceName;
        break;

    case ERROR_INVALID_HANDLE:
        errorMsg << "deleteService: the SCM is not open - " << serviceName;
        break;

    case ERROR_SERVICE_MARKED_FOR_DELETE:
        errorMsg << "deleteService: service has already been marked for deletion - " << serviceName;
        break;

    default:
        errorMsg << "deleteService failed to delete the service " << serviceName << " - " << lastError;
        break;
    }

    throw std::system_error(lastError, std::system_category(), wstring_to_string(errorMsg.str()));
}

/******************************************************************************
* METHOD:  deleteService
*
* PURPOSE: Stops the service if it is running and initiates deletion of the
*          service from the SCM database.  Waits at most 20s for the SCM to
*          report the service as deleted.
*
* NOTES:   OpenSCM must be called before calling this method.
*
* INPUT:   serviceName: Points to a null-terminated string that names the
*             service to delete. The maximum string length is 256 characters.
*             The SCM database preserves the character's case, but service
*             name comparisons are always case insensitive. A slash (/),
*             backslash (\), comma, and space are invalid service name
*             characters.
*
*          ec: contains the WIN32 error code if a failure occurs.
*******************************************************************************/
bool ServiceControlManager::deleteService(LPCTSTR serviceName, std::error_code& ec) noexcept
{
    ec.clear();

    auto result = openService(serviceName, SERVICE_ALL_ACCESS, ec);
    if (!result) {
        if (ec.value() == ERROR_SERVICE_DOES_NOT_EXIST) {
            ec.clear();
            return true;
        }

        return false;
    }

    // We'll assume the service is not running and attempt to delete it if we cannot
    // query its status.  It should be exceedingly rare for OpenService to succeed and
    // QueryServiceStatus to fail.
    SERVICE_STATUS status;
    if (::QueryServiceStatus(service_, &status)) {
        switch (status.dwCurrentState) {
        case SERVICE_STOPPED:
        case SERVICE_START_PENDING:
        case SERVICE_STOP_PENDING:
            // A service in one of these states will not accept stop commands.
            break;
        default:
            // Ignoring stopService failure, as we still need to mark the service for deletion even if we couldn't stop it.
            stopService(ec);
            ec.clear();
            break;
        }
    }

    result = ::DeleteService(service_);

    // Get error code before CloseService possibly changes it.
    DWORD lastError = (result ? NO_ERROR : ::GetLastError());

    // Close our handle to the service so the SCM can delete it.
    closeService();

    if (!result && lastError != ERROR_SERVICE_MARKED_FOR_DELETE) {
        ec.assign(lastError, std::system_category());
        return false;
    }

    // Wait at most 20 seconds for the service to be deleted.
    for (int i = 0; i < 200; i++) {
        result = openService(serviceName, SERVICE_QUERY_STATUS, ec);
        if (!result) {
            if (ec.value() == ERROR_SERVICE_DOES_NOT_EXIST) {
                ec.clear();
                return true;
            }

            return false;
        }

        closeService();
        ::Sleep(100);
    }

    return false;
}


/******************************************************************************
* METHOD:  isServiceInstalled
*
* PURPOSE: Queries the SCM database to see if the service is installed.
*          OpenSCM must be called before calling this method.
*
* INPUT:   serviceName: Points to a null-terminated string that names the
*             service to query. The maximum string length is 256 characters.
*             The SCM database preserves the character's case, but service
*             name comparisons are always case insensitive. A slash (/),
*             backslash (\), comma, and space are invalid service name
*             characters.
*
* THROWS:  A system_error object.
*******************************************************************************/
bool ServiceControlManager::isServiceInstalled(LPCTSTR serviceName) const
{
    if (serviceName == NULL) {
        throw std::system_error(ERROR_INVALID_NAME, std::system_category(), "isServiceInstalled: the service name parameter cannot be null");
    }

    SC_HANDLE service = ::OpenService(scm_, serviceName, SERVICE_QUERY_STATUS);

    bool installed = (service != NULL);

    if (installed) {
        ::CloseServiceHandle(service);
    }

    return installed;
}

/******************************************************************************
* METHOD:  installService
*
* PURPOSE: Installs a service.
*          OpenSCM must be called before calling this method.
*
* INPUT:   serviceName: Points to a null-terminated string that names the
*             service to install. The maximum string length is 256 characters.
*             The SCM database preserves the character's case, but service
*             name comparisons are always case insensitive. A slash (/),
*             backslash (\), comma, and space are invalid service name
*             characters.
*
*          binaryPathName: Points to a null-terminated string that contains
*             the fully qualified path to the service binary file.
*
*          displayName: Points to a null-terminated string that is to be
*             used by user interface programs to identify the service. This
*             string has a maximum length of 256 characters. The name is
*             case-preserved in the SCM. display name comparisons are always
*             case-insensitive.
*
*          description: Points to a null-terminated string that specifies
*             the description of the service.
*
*          serviceType: specify one of the following service types:
*
*             SERVICE_WIN32_OWN_PROCESS
*             SERVICE_WIN32_SHARE_PROCESS
*
*          startType: specify one of the following start types:
*
*             SERVICE_AUTO_START   - started automatically by the SCM during
*                                    system startup.
*
*             SERVICE_DEMAND_START - started by the SCM when a process calls
*                                    the StartService function.
*
*          dependencies: Points to a double null-terminated array of
*             null-separated names of services that the system must start
*             before this service. Specify NULL or an empty string if the
*             service has no dependencies.
*
*          allowInteractiveUserStartStop: if true logged in non-admin users can
*             start and stop the service.
*
* THROWS:  A system_error object.
*******************************************************************************/
void ServiceControlManager::installService(LPCTSTR serviceName,  LPCTSTR binaryPathName,
                                           LPCTSTR displayName,  LPCTSTR description,
                                           DWORD   serviceType,  DWORD   startType,
                                           LPCTSTR dependencies, bool    allowInteractiveUserStartStop)
{
    // Can only call this method to install a service on a local machine.
    if (!serverName_.empty()) {
        throw std::system_error(ERROR_NOT_SUPPORTED, std::system_category(), "installService can only be run on the local machine");
    }

    closeService();

    service_ = ::CreateService(scm_, serviceName, displayName, SERVICE_ALL_ACCESS, serviceType, startType,
                               SERVICE_ERROR_NORMAL, binaryPathName, NULL, NULL, dependencies, NULL, NULL);
    if (service_ != NULL) {
        serviceName_ = serviceName;
        setServiceDescription(description);

        if (allowInteractiveUserStartStop) {
            grantUserStartStopPermission();
        }

        return;
    }

    DWORD lastError = ::GetLastError();
    std::wostringstream errorMsg;

    switch (lastError) {
    case ERROR_ACCESS_DENIED:
        errorMsg << "installService: insufficient user rights to install service - " << serviceName;
        break;

    case ERROR_INVALID_HANDLE:
        errorMsg << "installService: the SCM is not open - " << serviceName;
        break;

    case ERROR_INVALID_NAME:
        errorMsg << "installService: service name has an invalid format - " << serviceName;
        break;

    case ERROR_SERVICE_EXISTS:
        errorMsg << "installService: a service with this name already exists on this server - " << serviceName;
        break;

    case ERROR_INVALID_PARAMETER:
        errorMsg << "installService: one of the parameters is incorrect - " << serviceName;
        break;

    case ERROR_CIRCULAR_DEPENDENCY:
        errorMsg << "installService: a circular service dependency was specified - " << serviceName;
        break;

    case ERROR_DUP_NAME:
        errorMsg << "installService: the display name already exists in the SCM database either as a service name or as another display name - " << displayName;
        break;

    default:
        errorMsg << "installService failed to install the service " << serviceName << " - " << lastError;
        break;
    }

    throw std::system_error(lastError, std::system_category(), wstring_to_string(errorMsg.str()));
}

/******************************************************************************
* METHOD:  sendControlCode
*
* PURPOSE: Sends a control code to the service.  The service will ignore the
*          code if it does not recognize it.
*          OpenService must be called before calling this method.
*
* INPUT:   code: the control code to send.
*
* THROWS:  A system_error object.
*******************************************************************************/
void ServiceControlManager::sendControlCode(DWORD code) const
{
    DWORD lastError;

    if ((code >= 128) && (code <= 256)) {
        SERVICE_STATUS status;
        if (::ControlService(service_, code, &status)) {
            return;
        }

        lastError = ::GetLastError();
    }
    else {
        lastError = ERROR_INVALID_SERVICE_CONTROL;
    }

    std::wostringstream errorMsg;

    switch (lastError) {
    case ERROR_SERVICE_NOT_ACTIVE:
        errorMsg << "sendControlCode: the service is not running - " << serviceName_;
        break;

    case ERROR_ACCESS_DENIED:
        errorMsg << "sendControlCode: insufficient user rights to control the service - " << serviceName_;
        break;

    case ERROR_INVALID_HANDLE:
        errorMsg << "sendControlCode: the service has not been opened - " << serviceName_;
        break;

    case ERROR_DEPENDENT_SERVICES_RUNNING:
        errorMsg << "sendControlCode: the service cannot be controlled because other running services are dependent on it- " << serviceName_;
        break;

    case ERROR_INVALID_SERVICE_CONTROL:
        errorMsg << "sendControlCode: the service does not accept user control code " << code << " - " << serviceName_;
        break;

    case ERROR_SERVICE_CANNOT_ACCEPT_CTRL:
        errorMsg << "sendControlCode: the service is not in a state where it can accept user control codes - " << serviceName_;
        break;

    case ERROR_SERVICE_REQUEST_TIMEOUT:
        errorMsg << "sendControlCode: the service did not respond to the control request in a timely fashion - " << serviceName_;
        break;

    default:
        errorMsg << "sendControlCode failed to control the service " << serviceName_ << " - " << lastError;
        break;
    }

    throw std::system_error(lastError, std::system_category(), wstring_to_string(errorMsg.str()));
}

LPCTSTR ServiceControlManager::getServerName() const
{
    if (serverName_.empty()) {
        return NULL;
    }

    return serverName_.c_str();
}

void ServiceControlManager::setServiceDescription(LPCTSTR description) const
{
    if (description != NULL) {
        SERVICE_DESCRIPTION svcDesc = { (LPTSTR)description };
        BOOL result = ::ChangeServiceConfig2(service_, SERVICE_CONFIG_DESCRIPTION, &svcDesc);

        if (result == FALSE) {
            DWORD lastError = ::GetLastError();
            throw std::system_error(lastError, std::system_category(),
                std::string("setServiceDescription: ChangeServiceConfig2 failed - ") + std::to_string(lastError));
        }
    }
}

std::wstring ServiceControlManager::serverNameForDebug() const
{
    if (serverName_.empty()) {
        return std::wstring(L"local_pc");
    }

    return serverName_;
}

void ServiceControlManager::setServiceSIDType(DWORD serviceSidType) const
{
    SERVICE_SID_INFO ssi;
    ssi.dwServiceSidType = serviceSidType;
    BOOL result = ::ChangeServiceConfig2(service_, SERVICE_CONFIG_SERVICE_SID_INFO, &ssi);

    if (result == FALSE) {
        DWORD lastError = ::GetLastError();
        throw std::system_error(lastError, std::system_category(),
            std::string("setServiceSIDType: ChangeServiceConfig2 failed - ") + std::to_string(lastError));
    }
}

void ServiceControlManager::grantUserStartStopPermission() const
{
    wchar_t sddl[] = L"D:"
      L"(A;;CCLCSWRPWPDTLOCRRC;;;SY)"           // default permissions for local system
      L"(A;;CCDCLCSWRPWPDTLOCRSDRCWDWO;;;BA)"   // default permissions for administrators
      L"(A;;CCLCSWLOCRRC;;;AU)"                 // default permissions for authenticated users
      L"(A;;CCLCSWRPWPDTLOCRRC;;;PU)"           // default permissions for power users
      L"(A;;RPWP;;;IU)"                         // added permission: start/stop service for interactive users
      ;

    PSECURITY_DESCRIPTOR sd;
    BOOL result = ::ConvertStringSecurityDescriptorToSecurityDescriptor(sddl, SDDL_REVISION_1, &sd, NULL);

    if (result == FALSE) {
        DWORD lastError = ::GetLastError();
        throw std::system_error(lastError, std::system_category(),
            std::string("grantUserStartStopPermission: ConvertStringSecurityDescriptorToSecurityDescriptorA failed - ") + std::to_string(lastError));
    }

    result = ::SetServiceObjectSecurity(service_, DACL_SECURITY_INFORMATION, sd);

    if (result == FALSE) {
        DWORD lastError = ::GetLastError();
        throw std::system_error(lastError, std::system_category(),
            std::string("grantUserStartStopPermission: SetServiceObjectSecurity failed - ") + std::to_string(lastError));
    }
}

/******************************************************************************
* METHOD:  exePath
*
* PURPOSE: Retrieves the full path to the service's executable.
*
* THROWS:  A system_error object.
*******************************************************************************/
std::wstring ServiceControlManager::exePath() const
{
    DWORD numBytesNeeded = 0;
    DWORD bufferSize = 1024;
    std::unique_ptr< unsigned char[] > buffer(new unsigned char[bufferSize]);

    bool secondTry = false;

    for (int i = 0; i < 2; i++) {
        if (::QueryServiceConfig(service_, (LPQUERY_SERVICE_CONFIG)buffer.get(),
                                 bufferSize, &numBytesNeeded)) {
            break;
        }

        DWORD lastError = ::GetLastError();
        std::wostringstream errorMsg;

        switch (lastError) {
        case ERROR_INSUFFICIENT_BUFFER:
            if (secondTry) {
                errorMsg << "queryServiceConfig: insufficient buffer space to query the service's configuration - " << serviceName_;
                break;
            }

            buffer.reset(new unsigned char[numBytesNeeded]);
            numBytesNeeded = 0;
            secondTry = true;
            continue;

        case ERROR_ACCESS_DENIED:
            errorMsg << "queryServiceConfig: insufficient user rights to query the service's configuration - " << serviceName_;
            break;

        case ERROR_INVALID_HANDLE:
            errorMsg << "queryServiceConfig: the service has not been opened - " << serviceName_;
            break;

        default:
            errorMsg << "queryServiceConfig failed to query the service " << serviceName_ << " - " << lastError;
            break;
        }

        throw std::system_error(lastError, std::system_category(), wstring_to_string(errorMsg.str()));
    }

    LPQUERY_SERVICE_CONFIG lpConfig = (LPQUERY_SERVICE_CONFIG)buffer.get();
    std::wstring cmdLine(lpConfig->lpBinaryPathName);
    if (cmdLine.empty()) {
        return std::wstring();
    }

    // Separate the exe path from its arguments, if any.
    int numArgs = 0;
    LPWSTR *argList = ::CommandLineToArgvW(cmdLine.c_str(), &numArgs);
    if (argList == NULL) {
        return std::wstring();
    }

    std::wstring exePath;
    if (numArgs > 0) {
        exePath = std::wstring(argList[0]);
    }

    ::LocalFree(argList);

    return exePath;
}

} // end namespace wsl
