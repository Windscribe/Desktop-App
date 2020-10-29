#ifndef RoutesManager_h
#define RoutesManager_h

#include "../ipc/helper_commands.h"
#include "bound_route.h"
#include "routes.h"

// state-based route management, depends on parameters isConnected, isSplitTunnelActive,
// exclusive/inclusive split tunneling mode
class RoutesManager
{
public:
    RoutesManager();
    void updateState(const CMD_SEND_CONNECT_STATUS &connectStatus, bool isSplitTunnelActive, bool isExcludeMode);
    
    void setLatestWireGuardAdapterSettings(const std::string &ipAddress, const std::string &dnsAddressList,
     const std::vector<std::string> &allowedIps);
    
private:
    CMD_SEND_CONNECT_STATUS connectStatus_;
    bool isSplitTunnelActive_;
    bool isExcludeMode_;
    
    BoundRoute boundRoute_;
    Routes dnsServersRoutes_;
    Routes vpnRoutes_;
    Routes ikev2OverrideRoutes_;
    
    std::string wgIpAddress_;
    std::string wgDnsAddress_;
    std::string wgAllowedIp_;

    void deleteOpenVpnDefaultRoutes(const CMD_SEND_CONNECT_STATUS &connectStatus);
    void deleteWireGuardDefaultRoutes(const CMD_SEND_CONNECT_STATUS &connectStatus);
    
    void addIkev2RoutesForInclusiveMode(const CMD_SEND_CONNECT_STATUS &connectStatus);
    void clearAllRoutes();
};

#endif /* RoutesManager_h */
