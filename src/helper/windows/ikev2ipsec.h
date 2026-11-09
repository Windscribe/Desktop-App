#pragma once

#include <Windows.h>

class IKEv2IPSec
{
public:
    static bool setIKEv2IPSecParameters(HANDLE clientPipe);

private:
    static bool setIKEv2IPSecParametersInPhoneBook(HANDLE clientPipe);
    static bool setIKEv2IPSecParametersPowerShell(HANDLE clientPipe);
    static void disableMODP2048Support();
};
