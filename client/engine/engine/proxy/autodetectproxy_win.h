#ifndef AUTODETECTPROXY_WIN_H
#define AUTODETECTPROXY_WIN_H

#include "proxysettings.h"

class AutoDetectProxy_win
{
public:
    static ProxySettings detect(bool &bSuccessfully);
};

#endif // AUTODETECTPROXY_WIN_H
