#pragma once

#include "../../common/helper_commands.h"
#include "bound_route.h"
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

    BoundRoute boundRoute_;
    Routes dnsServersRoutes_;
    Routes vpnRoutes_;

    void addDnsRoutes(const ConnectStatus &connectStatus);
    void clearAllRoutes();
};
