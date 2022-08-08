#ifndef AUTODETECTPROXY_WIN_H
#define AUTODETECTPROXY_WIN_H

#include "types/proxysettings.h"

class AutoDetectProxy_win
{
public:
    static types::ProxySettings detect(bool &bSuccessfully);
};

#endif // AUTODETECTPROXY_WIN_H
