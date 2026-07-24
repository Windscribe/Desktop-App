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
};

// Single definition point for the connector ctor and the test fake.
inline ConnectorCapabilities wireGuardConnectorCapabilities()
{
    ConnectorCapabilities caps;
    caps.connectTimeoutMs = 20 * 1000;
    caps.supportsCachedConfig = true;
    caps.needsSystemDnsRestore = true;
    return caps;
}
