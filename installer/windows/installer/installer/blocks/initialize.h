#ifndef INITIALIZE_H
#define INITIALIZE_H
#include <string>

#include "../iinstall_block.h"
#include "../../Utils/services.h"


//Setup initialization

//1% Check run Windscribe.exe
//2% Check exist WindscribeService.exe
//3% Stop WindscribeService.exe
class Initialize : public IInstallBlock
{
 private:
    Services services;
    bool appRunning;
    bool service_exists;
 public:
    int executeStep();
    bool initializeSetup();
    Initialize();
    std::wstring getLastError(){return L"";}

};

#endif // INITIALIZE_H
