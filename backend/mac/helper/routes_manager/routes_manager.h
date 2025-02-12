#pragma once

#include "../../../posix_common/helper_commands.h"
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
    void setConnectStatus(const CMD_SEND_CONNECT_STATUS &connectStatus);
    void updateState(const CMD_SEND_CONNECT_STATUS &connectStatus, bool isSplitTunnelActive, bool isExcludeMode);

private:
    CMD_SEND_CONNECT_STATUS connectStatus_;
    bool isSplitTunnelActive_;
    bool isExcludeMode_;

    BoundRoute boundRoute_;
    Routes dnsServersRoutes_;
    Routes vpnRoutes_;

    RoutesManager();
    void addDnsRoutes(const CMD_SEND_CONNECT_STATUS &connectStatus);
    void addIkev2RoutesForInclusiveMode(const CMD_SEND_CONNECT_STATUS &connectStatus);
    void clearAllRoutes();
};
