#pragma once

#include <QString>
#include "api_responses/servercredentials.h"
#include "engine/connectionmanager/connectors/iconnection.h"
#include "types/proxysettings.h"

// Session-scoped OpenVPN data, fixed at clickConnect and handed to the connector by the factory.
struct OpenVpnSessionParams
{
    QString ovpnConfig;
    api_responses::ServerCredentials serverCredentials;
    types::ProxySettings proxySettings;
    QString customSni;
    QString amneziawgPreset;
    // The emergency-connect servers are old and do not support the DCO driver. isAntiCensorship
    // carries the user's API anti-censorship setting for the emergency session; regular sessions
    // express anti-censorship through amneziawgPreset instead.
    bool isEmergencyConnect = false;
    bool isAntiCensorship = false;
};

// Single definition point for the connector ctor and the test fake.
inline ConnectorCapabilities openVpnConnectorCapabilities()
{
    ConnectorCapabilities caps;
    caps.needsSystemDnsRestore = true;
    return caps;
}
