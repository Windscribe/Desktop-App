#pragma once

#include <Windows.h>
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

namespace wsl
{

class ServiceControlManager
{
public:
    // Emits one diagnostic line.  Supplied by the caller of logServiceStatusAndConfig so it can
    // route the output to whichever logger it uses.
    using LogFunction = std::function<void(const std::string &message)>;

    explicit ServiceControlManager();
    ~ServiceControlManager();

    void deleteService(LPCTSTR serviceName, bool stopRunningService = true);
    bool deleteService(LPCTSTR serviceName, std::error_code& ec, int timeoutMs = 20000) noexcept;

    void installService(LPCTSTR serviceName, LPCTSTR binaryPathName,
                        LPCTSTR displayName, LPCTSTR description,
                        DWORD serviceType = SERVICE_WIN32_OWN_PROCESS,
                        DWORD startType = SERVICE_AUTO_START,
                        LPCTSTR dependencies = NULL,
                        bool allowInteractiveUserStartStop = false);

    bool isSCMOpen() const;
    bool isServiceInstalled(LPCTSTR serviceName) const;
    bool isServiceOpen() const;

    void closeSCM() noexcept;
    void closeService() noexcept;
    void openSCM(DWORD desiredAccess, LPCTSTR serverName = NULL);
    void openService(LPCTSTR serviceName, DWORD desiredAccess = SERVICE_ALL_ACCESS);
    bool openService(LPCTSTR serviceName, DWORD desiredAccess, std::error_code& ec) noexcept;
    void queryServiceConfig(std::wstring& exePath, std::wstring& accountName,
                            DWORD& startType, bool& serviceShareProcess) const;
    // Returns the names of the services and load-ordering groups the system must start before this
    // service. A name prefixed with SC_GROUP_IDENTIFIER ('+') identifies a load-ordering group
    // rather than an individual service.
    std::vector<std::wstring> queryServiceDependencies() const;
    DWORD queryServiceStartType() const;
    DWORD queryServiceStatus() const;
    DWORD queryServiceStatus(std::error_code& ec) const noexcept;
    void sendControlCode(DWORD code) const;
    void setServiceDescription(LPCTSTR description) const;
    void setServiceSIDType(DWORD serviceSidType) const;
    void startService(int timeoutMs = 20000);
    bool startService(std::error_code& ec, int timeoutMs = 20000) noexcept;
    void stopService(int timeoutMs = 20000);
    void stopService(LPCTSTR serviceName, int timeoutMs = 20000);
    bool stopService(std::error_code& ec, int timeoutMs = 20000) noexcept;

    // Prevents the initiation of, and aborts any currently running, start/stop requests.
    void blockStartStopRequests();
    void unblockStartStopRequests();

    LPCTSTR getServerName() const;

    std::wstring exePath() const;

    static std::string serviceStartTypeToString(DWORD startType);
    static std::string serviceStatusToString(DWORD status);

    // Logs the state and configuration of the named service, and of each of its dependencies,
    // through the caller-supplied callback.
    static void logServiceStatusAndConfig(LPCTSTR serviceName, bool logDependencies, const LogFunction &log);

private:
    std::wstring serverName_;
    std::wstring serviceName_;
    SC_HANDLE scm_ = NULL;
    SC_HANDLE service_ = NULL;
    std::atomic<bool> blockStartStopRequests_ = false;

private:
    void grantUserStartStopPermission() const;
    std::unique_ptr<unsigned char[]> queryServiceConfig() const;
    std::wstring serverNameForDebug() const;
};

inline bool
ServiceControlManager::isSCMOpen() const
{
    return (scm_ != NULL);
}

inline bool
ServiceControlManager::isServiceOpen() const
{
    return (service_ != NULL);
}

inline void
ServiceControlManager::blockStartStopRequests()
{
    blockStartStopRequests_ = true;
}

inline void
ServiceControlManager::unblockStartStopRequests()
{
    blockStartStopRequests_ = false;
}

} // end namespace wsl
