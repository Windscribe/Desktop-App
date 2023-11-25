#pragma once

#include "../ipc/servicecommunication.h"
#include "routes.h"

// state-based route management, depends on parameters isConnected, isSplitTunnelActive,
// exclusive/inclusive split tunneling mode
class RoutesManager
{
public:
    RoutesManager();
    void updateState(const CMD_CONNECT_STATUS &connectStatus, bool isSplitTunnelActive, bool isExcludeMode);

private:
    CMD_CONNECT_STATUS connectStatus_;
    bool isSplitTunnelActive_;
    bool isExcludeMode_;

    Routes openVpnRoutes_;
    Routes ikev2Routes_;
    Routes wgRoutes_;
    Routes dnsServersRoutes_;
    Routes boundRoute_;

    void clearAllRoutes();

    void doActionsForInclusiveModeOpenVpn(const CMD_CONNECT_STATUS &connectStatus);
    void doActionsForInclusiveModeWireGuard(const CMD_CONNECT_STATUS &connectStatus);
    void doActionsForInclusiveModeIkev2(const CMD_CONNECT_STATUS &connectStatus);
    void addDnsRoutes(const CMD_CONNECT_STATUS &connectStatus);
};
