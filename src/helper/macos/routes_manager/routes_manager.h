#pragma once

#include "../../common/helper_commands.h"
#include "bound_route.h"
#include "routes.h"

class RoutesManager
{
public:
    static RoutesManager& instance()
    {
        static RoutesManager instance;
        return instance;
    }
    RoutesManager(const RoutesManager&) = delete;
    RoutesManager& operator=(const RoutesManager&) = delete;

    void setSplitTunnelSettings(bool isSplitTunnelActive, bool isExcludeMode);
    void setConnectStatus(const ConnectStatus &connectStatus);
    void updateState(const ConnectStatus &connectStatus, bool isSplitTunnelActive, bool isExcludeMode);

private:
    ConnectStatus connectStatus_;
    bool isSplitTunnelActive_;
    bool isExcludeMode_;

    // Two BoundRoute instances — one per family — so dual-stack tunnels can pin v4 and v6
    // default-route overrides independently. The v6 one stays dormant on connections that
    // don't fill connectStatus.vpnAdapter.gatewayIpV6 (everything except a future v6-capable
    // OpenVPN config; WG handles v6 default-route bypass inside WireGuardAdapter).
    BoundRoute boundRoute_;
    BoundRoute boundRouteV6_;
    Routes dnsServersRoutes_;
    Routes vpnRoutes_;
    Routes lanMulticastRoutes_;

    RoutesManager();
    void addDnsRoutes(const ConnectStatus &connectStatus);
    void addLanMulticastRoutes(const ConnectStatus &connectStatus);
    void clearAllRoutes();
};
