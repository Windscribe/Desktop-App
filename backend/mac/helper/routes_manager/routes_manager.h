#pragma once

#include "../../../common/helper_commands.h"
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

    BoundRoute boundRoute_;
    Routes dnsServersRoutes_;
    Routes vpnRoutes_;

    RoutesManager();
    void addDnsRoutes(const ConnectStatus &connectStatus);
    void clearAllRoutes();
};
