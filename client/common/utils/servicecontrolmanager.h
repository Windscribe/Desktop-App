#pragma once

#include <Windows.h>
#include <atomic>
#include <string>

namespace wsl
{

//---------------------------------------------------------------------------
// ServiceControlManager class definition

class ServiceControlManager
{
public:
   explicit ServiceControlManager();
   ~ServiceControlManager();

   void deleteService(LPCTSTR pszServiceName, bool bStopRunningService = true);

   void installService(LPCTSTR pszServiceName, LPCTSTR pszBinaryPathName,
                       LPCTSTR pszDisplayName, LPCTSTR pszDescription,
                       DWORD dwServiceType = SERVICE_WIN32_OWN_PROCESS,
                       DWORD dwStartType = SERVICE_AUTO_START,
                       LPCTSTR pszDependencies = NULL,
                       bool bAllowInteractiveUserStart = false);

   bool isSCMOpen() const;
   bool isServiceInstalled(LPCTSTR pszServiceName) const;
   bool isServiceOpen() const;

   void closeSCM() noexcept;
   void closeService() noexcept;
   void openSCM(DWORD dwDesiredAccess, LPCTSTR pszServerName = NULL);
   void openService(LPCTSTR pszServiceName, DWORD dwDesiredAccess = SERVICE_ALL_ACCESS);
   void queryServiceConfig(std::wstring& sExePath, std::wstring& sAccountName,
                           DWORD& dwStartType, bool& bServiceShareProcess) const;
   DWORD queryServiceStatus() const;
   void sendControlCode(DWORD dwCode) const;
   void setServiceDescription(LPCTSTR pszDescription) const;
   void setServiceSIDType(DWORD dwServiceSidType) const;
   void startService();
   void stopService();
   void stopService(LPCTSTR pszServiceName);

   // Prevents the initiation of, and aborts any currently running, start/stop requests.
   void blockStartStopRequests();
   void unblockStartStopRequests();

   LPCTSTR getServerName() const;

private:
   std::wstring m_sServerName;
   std::wstring m_sServiceName;
   SC_HANDLE m_hSCM;
   SC_HANDLE m_hService;
   std::atomic<bool> m_bBlockStartStopRequests;

private:
   void grantUserStartPermission() const;
   std::wstring serverNameForDebug() const;
};

//---------------------------------------------------------------------------
// ServiceControlManager inline methods

inline bool
ServiceControlManager::isSCMOpen() const
{
   return (m_hSCM != NULL);
}

inline bool
ServiceControlManager::isServiceOpen() const
{
   return (m_hService != NULL);
}

inline void
ServiceControlManager::blockStartStopRequests()
{
    m_bBlockStartStopRequests = true;
}

inline void
ServiceControlManager::unblockStartStopRequests()
{
    m_bBlockStartStopRequests = false;
}

//---------------------------------------------------------------------------

} // end namespace wsl
