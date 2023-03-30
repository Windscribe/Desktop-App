#ifndef SERVICES_H
#define SERVICES_H

#include <string>
#include <Windows.h>

class Services
{
 
 public:
    bool serviceExists(std::wstring AService);
    void simpleStopService(std::wstring AService,bool Wait, bool IgnoreStopped);
    void simpleDeleteService(std::wstring AService);
    bool waitForService(SC_HANDLE ServiceHandle, unsigned int AStatus);
    Services();
};

#endif // SERVICES_H
