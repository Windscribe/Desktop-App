#pragma once

class IKEv2IPSec
{
public:
    static bool setIKEv2IPSecParameters();

private:
    static bool setIKEv2IPSecParametersInPhoneBook();
    static bool setIKEv2IPSecParametersPowerShell();
    static void disableMODP2048Support();
};
