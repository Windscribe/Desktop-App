#pragma once

#include <Windows.h>
#include <atomic>
#include <string>
#include <system_error>

namespace wsl
{

//---------------------------------------------------------------------------
// ServiceControlManager class definition

class ServiceControlManager
{
public:
    explicit ServiceControlManager();
    ~ServiceControlManager();

    void deleteService(LPCTSTR serviceName, bool stopRunningService = true);
    bool deleteService(LPCTSTR serviceName, std::error_code& ec) noexcept;

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
    DWORD queryServiceStatus() const;
    DWORD queryServiceStatus(std::error_code& ec) const noexcept;
    void sendControlCode(DWORD code) const;
    void setServiceDescription(LPCTSTR description) const;
    void setServiceSIDType(DWORD serviceSidType) const;
    void startService();
    bool startService(std::error_code& ec) noexcept;
    void stopService();
    void stopService(LPCTSTR serviceName);
    bool stopService(std::error_code& ec) noexcept;

    // Prevents the initiation of, and aborts any currently running, start/stop requests.
    void blockStartStopRequests();
    void unblockStartStopRequests();

    LPCTSTR getServerName() const;

    std::wstring exePath() const;

private:
    std::wstring serverName_;
    std::wstring serviceName_;
    SC_HANDLE scm_ = NULL;
    SC_HANDLE service_ = NULL;
    std::atomic<bool> blockStartStopRequests_ = false;

private:
    void grantUserStartStopPermission() const;
    std::wstring serverNameForDebug() const;
};

//---------------------------------------------------------------------------
// ServiceControlManager inline methods

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

//---------------------------------------------------------------------------

} // end namespace wsl
