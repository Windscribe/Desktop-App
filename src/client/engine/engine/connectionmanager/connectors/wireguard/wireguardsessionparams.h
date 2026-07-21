#pragma once

#include <QString>
#include "api_responses/amneziawgunblockparams.h"
#include "engine/connectionmanager/connectors/iconnection.h"
#include "types/enums.h"

// Session-scoped WireGuard data, fixed at clickConnect and handed to the connector by the factory.
struct WireGuardSessionParams
{
    QString amneziawgPreset;
    api_responses::AmneziawgUnblockParams amneziawgParams;

    // Controls whether IPv6 is enabled for VPN tunnels that can carry dual-stack traffic
    // (currently: WireGuard from API + WireGuard custom configs). kIPv4Only forces v6 to
    // be stripped from the tunnel config; kAuto keeps v6 if the node/config supports it.
    IpStack ipStackEgress = IpStack::kAuto;
};

// Single definition point for the connector ctor and the test fake.
inline ConnectorCapabilities wireGuardConnectorCapabilities()
{
    ConnectorCapabilities caps;
    caps.connectTimeoutMs = 20 * 1000;
    caps.dualStackEgress = true;
    caps.supportsCachedConfig = true;
    caps.needsSystemDnsRestore = true;
    return caps;
}
