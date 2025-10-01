#pragma once

#include "engine/helper/helper.h"

// manage BFE (Base filtering engine) service
class BFE_Service_win
{
public:
    static BFE_Service_win &instance()
    {
        static BFE_Service_win s;
        return s;
    }

    bool checkAndEnableBFE(Helper *helper);
    bool isBFEEnabled();

private:
    BFE_Service_win();

    void enableBFE(Helper *helper);
};
