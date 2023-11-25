#pragma once

#include "types/proxysettings.h"

class AutoDetectProxy_win
{
public:
    static types::ProxySettings detect(bool &bSuccessfully);
};
