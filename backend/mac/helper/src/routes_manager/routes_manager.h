#ifndef RoutesManager_h
#define RoutesManager_h

#include "../IPC/HelperCommands.h"
#include "BoundRoute.h"
#include "Routes.h"

// state-based route management, depends on parameters isConnected, isSplitTunnelActive,
// exclusive/inclusive split tunneling mode
class RoutesManager
{
public:
    RoutesManager();
    void updateState(const CMD_SEND_CONNECT_STATUS &connectStatus, bool isSplitTunnelActive, bool isExcludeMode);
    
private:
    CMD_SEND_CONNECT_STATUS connectStatus_;
    bool isSplitTunnelActive_;
    bool isExcludeMode_;
    
    BoundRoute boundRoute_;
    Routes dnsServersRoutes_;
    Routes vpnRoutes_;
    Routes ikev2OverrideRoutes_;

    void deleteOpenVpnDefaultRoutes(const CMD_SEND_CONNECT_STATUS &connectStatus);
    
    void addIkev2RoutesForInclusiveMode(const CMD_SEND_CONNECT_STATUS &connectStatus);
    void clearAllRoutes();
};

#endif /* RoutesManager_h */
