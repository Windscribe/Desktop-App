#include "initialize.h"
#include "../../Utils/applicationInfo.h"


using namespace std;

Initialize::Initialize()
{
 this->interval = 3;
 count = 0;
 appRunning = false;
 service_exists = false;

 name = L"initialize";
}


int Initialize::executeStep()
{
 count++;

 switch(count)
 {
 case 1: appRunning = ApplicationInfo::instance().appIsRunning();
          break;
  case 2: if(appRunning==true)
          {
           service_exists = services.serviceExists(L"WindscribeService");
          }
          break;
  case 3: if(service_exists==true)
          {
           services.simpleStopService(L"WindscribeService", true, true);
          }
          break;

  default: break;
 }

 return count;
}







