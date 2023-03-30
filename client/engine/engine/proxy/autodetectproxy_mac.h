#ifndef AUTODETECTPROXY_MAC_H
#define AUTODETECTPROXY_MAC_H

#include "types/proxysettings.h"

class AutoDetectProxy_mac
{
public:
    static types::ProxySettings detect(bool &bSuccessfully);
};

#endif // AUTODETECTPROXY_MAC_H
