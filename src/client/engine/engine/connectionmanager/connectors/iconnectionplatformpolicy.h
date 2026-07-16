#pragma once

#include "engine/connectionmanager/adaptergatewayinfo.h"

// The platform tweaks and OS reads ConnectionManager must perform around a connection attempt.
// Seam so CM stays free of platform #ifdefs and so tests can supply canned values instead of
// touching the OS. The platform-tweak methods are no-ops on platforms they do not apply to;
// detectDefaultAdapter() runs on every platform.
class IConnectionPlatformPolicy
{
public:
    virtual ~IConnectionPlatformPolicy() {}

    // macOS: Lockdown Mode blocks IKEv2; false elsewhere.
    virtual bool isLockdownBlockingIkev2() const = 0;
    // Windows: disable the OS DoH settings so they can't bypass the tunnel DNS.
    virtual void disableDohIfNeeded() = 0;
    // Linux: toggle IPv4 priority in gai.conf.
    virtual void setGaiIpv4PriorityEnabled(bool isEnabled) = 0;
    // Detect the current default adapter/gateway. Behind the seam because it shells out to the OS;
    // tests supply a canned value instead.
    virtual AdapterGatewayInfo detectDefaultAdapter() = 0;
};
