#pragma once
#include "engine/helper/helper.h"

// manage SstpSvc and RasMan services (required for IKEv2)
class RAS_Service_win
{
public:
    static RAS_Service_win &instance()
    {
        static RAS_Service_win s;
        return s;
    }

    bool restartRASServices(Helper *helper);
    bool isRASRunning();

private:
    RAS_Service_win();
};
