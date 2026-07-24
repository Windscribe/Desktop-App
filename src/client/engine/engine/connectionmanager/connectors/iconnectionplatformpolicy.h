#pragma once

#include <QtGlobal>

#include "engine/adaptergatewayinfo.h"
#include "types/protocol.h"

// Platform truth tables shared by the production policy and the test fake's defaults, so the fake
// can't silently drift from the shipped behavior.
inline constexpr bool platformNeedsSleepEventAwareDisconnect()
{
#ifdef Q_OS_WIN
    return true;
#else
    return false;
#endif
}

inline constexpr bool platformReconnectsOnOnlineStateChange()
{
#ifdef Q_OS_MACOS
    return true;
#else
    return false;
#endif
}

// Single definition of the Lockdown Mode rule, shared by ConnectionManager's per-attempt gate and
// the auto strategy's protocol pre-filter; the OS fact itself comes from the policy seam.
inline bool lockdownBlocksProtocol(bool isLockdownMode, types::Protocol protocol)
{
    return isLockdownMode && protocol.isIkev2Protocol();
}

// The platform tweaks and OS reads ConnectionManager must perform around a connection attempt.
// Seam so CM stays free of platform #ifdefs and so tests can supply canned values instead of
// touching the OS. The platform-tweak methods are no-ops on platforms they do not apply to;
// detectDefaultAdapter() runs on every platform.
class IConnectionPlatformPolicy
{
public:
    virtual ~IConnectionPlatformPolicy() {}

    // macOS: whether Lockdown Mode is on (it blocks IKEv2's NEVPNManager path); false elsewhere.
    // Consumers combine it with a protocol via lockdownBlocksProtocol().
    virtual bool isLockdownMode() const = 0;
    // Windows: a sleep-event disconnect must not pump the event loop (the OS can suspend the process
    // mid-loop, and a processed wake event would start reconnecting while still disconnecting).
    virtual bool needsSleepEventAwareDisconnect() const = 0;
    // macOS: network online-state changes drive connection state transitions (disconnect/reconnect);
    // on Windows/Linux the connectors and OS handle network changes themselves.
    virtual bool shouldReconnectOnOnlineStateChange() const = 0;
    // Windows: disable the OS DoH settings so they can't bypass the tunnel DNS.
    virtual void disableDohIfNeeded() = 0;
    // Linux: toggle IPv4 priority in gai.conf.
    virtual void setGaiIpv4PriorityEnabled(bool isEnabled) = 0;
    // Detect the current default adapter/gateway. Behind the seam because it shells out to the OS;
    // tests supply a canned value instead.
    virtual AdapterGatewayInfo detectDefaultAdapter() = 0;
};
