#pragma once

#include "types/proxysettings.h"

class AutoDetectProxy_mac
{
public:
    static types::ProxySettings detect(bool &bSuccessfully);
};
