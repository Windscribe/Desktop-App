#pragma once

#include <QSharedPointer>
#include "api_responses/portmap.h"
#include "engine/connectionmanager/connectors/ikev2/ikev2sessionparams.h"
#include "engine/connectionmanager/connectors/openvpn/openvpnsessionparams.h"
#include "engine/connectionmanager/connectors/wireguard/wireguardsessionparams.h"
#include "engine/locationsmodel/baselocationinfo.h"
#include "types/connectionsettings.h"
#include "types/proxysettings.h"

struct ConnectRequest
{
    // Per-protocol session data; the factory hands each connector its own struct at construction.
    // amneziawgPreset appears in both openVpn and wireGuard deliberately (both protocols consume it);
    // proxySettings also stays top-level because ConnectionManager reads it for the LAN-proxy warning.
    OpenVpnSessionParams openVpn;
    WireGuardSessionParams wireGuard;
    Ikev2SessionParams ikev2;

    QSharedPointer<locationsmodel::BaseLocationInfo> bli;
    types::ConnectionSettings connectionSettings;
    api_responses::PortMap portMap;
    types::ProxySettings proxySettings;
    bool bEmitAuthError = false;
    QString preferredNodeHostname;
};
