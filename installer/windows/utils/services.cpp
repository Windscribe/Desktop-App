#include "services.h"


using namespace std;

Services::Services()
{

}


void Services::simpleDeleteService(wstring AService)
{
 SC_HANDLE SCMHandle;
 SC_HANDLE ServiceHandle;

 wchar_t buffer[100];

 SCMHandle = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);

 if(SCMHandle == nullptr)
  {
   swprintf_s(buffer, L"%lu",GetLastError());
   Log::instance().out(L"(services) OpenSCManager() %s failed, error: %s", AService.c_str(), buffer);
  }


 ServiceHandle = OpenService(SCMHandle, AService.c_str(), SERVICE_ALL_ACCESS);

 if(ServiceHandle == nullptr)
  {
   swprintf_s(buffer, L"%lu",GetLastError());
   Log::instance().out(L"(services) OpenService() %s failed, error: %s", AService.c_str(), buffer);
  }

 if(DeleteService(ServiceHandle) == 0)
  {
   swprintf_s(buffer, L"%lu",GetLastError());
   Log::instance().out(L"(services) DeleteService() %s failed, error: %s", AService.c_str(), buffer);
  }

 if(ServiceHandle != nullptr)
  {
   CloseServiceHandle(ServiceHandle);
  }

 if(SCMHandle !=nullptr)
  {
   CloseServiceHandle(SCMHandle);
  }
}

bool Services::serviceExists(wstring AService)
{
 //https://docs.microsoft.com/en-us/windows/desktop/api/winsvc/nf-winsvc-openservicea
 //https://www.tenouk.com/ModuleEE2.html
 SC_HANDLE schSCManager;

//Open a handle to the SC Manager database...
 schSCManager = OpenSCManager(

     nullptr,                    // local machine

     nullptr,                    // SERVICES_ACTIVE_DATABASE database is opened by default

     SC_MANAGER_ALL_ACCESS);  // full access rights

 if (nullptr == schSCManager)
 {
  wchar_t buffer[100];
  swprintf_s(buffer, L"%lu",GetLastError());
  Log::instance().out(L"(services) OpenSCManager() failed, error: %s", buffer);
 }
 else
 {
  Log::instance().out(L"(services) OpenSCManager() looks OK.");


 SC_HANDLE schService;

 //SERVICE_STATUS ssStatus;

 //DWORD dwOldCheckPoint;
 //DWORD dwStartTickCount;
 //DWORD dwWaitTime;

 schService = OpenService(

     schSCManager,         // SCM database

     AService.c_str(),       // service name

     SERVICE_ALL_ACCESS);

 if (schService == nullptr)
 {
  DWORD errorMessageID = GetLastError();
  wchar_t buffer[100];
  swprintf_s(buffer, L"%lu",errorMessageID);
  Log::instance().out(L"(services) OpenService() failed, error: %s", buffer);


  if(errorMessageID == ERROR_SERVICE_DOES_NOT_EXIST)
  {

   if(schService != nullptr)
   {
    if (CloseServiceHandle(schService) == 0)
    {
     wchar_t buffer[100];
     swprintf_s(buffer, L"%lu",GetLastError());
     Log::instance().out(L"(services) CloseServiceHandle() failed, error: %s", buffer);
    }
    else
    {
     Log::instance().out(L"(services) CloseServiceHandle() looks OK.");
    }
   }



   CloseServiceHandle(schSCManager);
   return false;
  }

 }
 else
 {
     if (CloseServiceHandle(schService) == 0)
     {
      wchar_t buffer[100];
      swprintf_s(buffer, L"%lu",GetLastError());
      Log::instance().out(L"(services) CloseServiceHandle() failed, error: %s", buffer);
     }
     else
     {
      Log::instance().out(L"(services) CloseServiceHandle() looks OK.");
     }
 }

CloseServiceHandle(schSCManager);
 }

return true;
}


bool Services::waitForService(SC_HANDLE ServiceHandle, unsigned int AStatus)
{
  unsigned int PendingStatus=0;
  _SERVICE_STATUS ServiceStatus;
  //int Error;

  bool Result = false;

  switch(AStatus)
  {
   case SERVICE_RUNNING: PendingStatus = SERVICE_START_PENDING; break;
   case SERVICE_STOPPED: PendingStatus = SERVICE_STOP_PENDING; break;
  }


  while (Result==false)
  {

   if (ControlService(ServiceHandle, SERVICE_CONTROL_INTERROGATE, &ServiceStatus) == 0)
    {
     wchar_t buffer[100];
     swprintf_s(buffer, L"%lu",GetLastError());
     Log::instance().out(L"(services) ControlService failed, error: %s", buffer);
    }

    if (ServiceStatus.dwWin32ExitCode != 0)
    {
     break;
    }

    Result = ServiceStatus.dwCurrentState = AStatus;
    if ((Result==false) && (ServiceStatus.dwCurrentState == PendingStatus))
    {
      Sleep(ServiceStatus.dwWaitHint);
    }
    else
     {
      break;
     }
  }

  return Result;
}

void Services::simpleStopService(wstring AService,bool Wait, bool IgnoreStopped)
{
//https://docs.microsoft.com/en-us/windows/desktop/services/stopping-a-service

    SC_HANDLE schSCManager;
    SC_HANDLE schService;

    SERVICE_STATUS_PROCESS ssp;

 //Get a handle to the SCM database.

     schSCManager = OpenSCManager(
           nullptr,                    // local computer
           nullptr,                    // ServicesActive database
           SC_MANAGER_ALL_ACCESS);  // full access rights

       if (nullptr == schSCManager)
       {
        wchar_t buffer[100];
        swprintf_s(buffer, L"%lu",GetLastError());
        Log::instance().out(L"(services) OpenSCManager failed, error: %s", buffer);
        return;
       }

       // Get a handle to the service.

       schService = OpenService(
           schSCManager,         // SCM database
           AService.c_str(),            // name of service
           SERVICE_ALL_ACCESS);

       if (schService == nullptr)
       {
           wchar_t buffer[100];
           swprintf_s(buffer, L"%lu",GetLastError());
           Log::instance().out(L"(services) OpenService failed, error: %s", buffer);
           CloseServiceHandle(schSCManager);
           return;
       }



          if ( !ControlService(
                  schService,
                  SERVICE_CONTROL_STOP,
                   reinterpret_cast<LPSERVICE_STATUS>(&ssp) ) )
          {
              DWORD Error = GetLastError();




if ((IgnoreStopped==true)&&(Error == ERROR_SERVICE_NOT_ACTIVE))
{
 goto stop_cleanup;
}
else
{
  wchar_t buffer[100];
  swprintf_s(buffer, L"%lu",Error);
  Log::instance().out(L"(services) ControlService failed, error: %s", buffer);
}


 if(Wait==true)
 {
  waitForService(schService, SERVICE_STOPPED);
 }

          }


   stop_cleanup:
       CloseServiceHandle(schService);
       CloseServiceHandle(schSCManager);

 return;
}
