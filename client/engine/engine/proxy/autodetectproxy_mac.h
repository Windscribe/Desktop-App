#ifndef AUTODETECTPROXY_MAC_H
#define AUTODETECTPROXY_MAC_H

#include "proxysettings.h"

class AutoDetectProxy_mac
{
public:
    static ProxySettings detect(bool &bSuccessfully);
};

#endif // AUTODETECTPROXY_MAC_H
