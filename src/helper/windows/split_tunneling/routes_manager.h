#pragma once

#include "../../common/helper_commands.h"
#include "routes.h"

// state-based route management, depends on parameters isConnected, isSplitTunnelActive,
// exclusive/inclusive split tunneling mode
class RoutesManager
{
public:
    RoutesManager();
    void updateState(const ConnectStatus &connectStatus, bool isSplitTunnelActive, bool isExcludeMode);

private:
    ConnectStatus connectStatus_;
    bool isSplitTunnelActive_;
    bool isExcludeMode_;

    Routes openVpnRoutes_;
    Routes ikev2Routes_;
    Routes wgRoutes_;
    Routes dnsServersRoutes_;
    Routes boundRoute_;

    void clearAllRoutes();

    void doActionsForInclusiveModeOpenVpn(const ConnectStatus &connectStatus);
    void doActionsForInclusiveModeWireGuard(const ConnectStatus &connectStatus);
    void doActionsForInclusiveModeIkev2(const ConnectStatus &connectStatus);
    void addDnsRoutes(const ConnectStatus &connectStatus);
};
