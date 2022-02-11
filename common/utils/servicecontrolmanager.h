#ifndef SERVICECONTROLMANAGER_H
#define SERVICECONTROLMANAGER_H

#include <Windows.h>
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

   void deleteService(LPCSTR pszServiceName, bool bStopRunningService = true);

   void installService(LPCSTR pszServiceName, LPCSTR pszBinaryPathName,
                       LPCSTR pszDisplayName, LPCSTR pszDescription,
                       DWORD dwServiceType = SERVICE_WIN32_OWN_PROCESS,
                       DWORD dwStartType = SERVICE_AUTO_START,
                       LPCSTR pszDependencies = NULL,
                       bool bAllowInteractiveUserStart = false);

   bool isSCMOpen() const;
   bool isServiceInstalled(LPCSTR pszServiceName) const;
   bool isServiceOpen() const;

   void closeSCM() noexcept;
   void closeService() noexcept;
   void openSCM(DWORD dwDesiredAccess, LPCSTR pszServerName = NULL);
   void openService(LPCSTR pszServiceName, DWORD dwDesiredAccess = SERVICE_ALL_ACCESS);
   void queryServiceConfig(std::string& sExePath, std::string& sAccountName,
                           DWORD& dwStartType, bool& bServiceShareProcess) const;
   DWORD queryServiceStatus() const;
   void sendControlCode(DWORD dwCode) const;
   void setServiceDescription(LPCSTR pszDescription) const;
   void setServiceSIDType(DWORD dwServiceSidType) const;
   void startService() const;
   void stopService() const;

   LPCSTR getServerName() const;

private:
   std::string m_sServerName;
   std::string m_sServiceName;
   SC_HANDLE m_hSCM;
   SC_HANDLE m_hService;

private:
   void grantUserStartRight() const;
   std::string serverNameForDebug() const;
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

//---------------------------------------------------------------------------

} // end namespace wsl

#endif // SERVICECONTROLMANAGER_H
