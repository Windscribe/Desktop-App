#include "routes_manager.h"

RoutesManager::RoutesManager()
{
    isSplitTunnelActive_ = false;
    isExcludeMode_ = false;
    connectStatus_.isConnected = false;
}

void RoutesManager::updateState(const ConnectStatus &connectStatus, bool isSplitTunnelActive, bool isExcludeMode)
{
    bool prevIsConnected = connectStatus_.isConnected;
    bool prevIsActive = isSplitTunnelActive_;

    if  (prevIsConnected == false && connectStatus.isConnected == true) {
        if (isSplitTunnelActive) {
            // add bound route
            boundRoute_.create(connectStatus.defaultAdapter.gatewayIp, connectStatus.defaultAdapter.adapterName);
        }
        addDnsRoutes(connectStatus);
    } else if (prevIsConnected == true && connectStatus.isConnected == false) {
        clearAllRoutes();
    } else if (prevIsConnected == true && connectStatus.isConnected == true) {
        clearAllRoutes();
        if (prevIsActive != isSplitTunnelActive) {
            if (!isSplitTunnelActive) {
                if (connectStatus.protocol == kCmdProtocolOpenvpn || connectStatus.protocol == kCmdProtocolStunnelOrWstunnel) {
                    // add openvpn default routes
                    vpnRoutes_.add(connectStatus.remoteIp, connectStatus.defaultAdapter.gatewayIp, "32");
                    vpnRoutes_.add("0.0.0.0", connectStatus.vpnAdapter.gatewayIp, "1");
                    vpnRoutes_.add("128.0.0.0", connectStatus.vpnAdapter.gatewayIp, "1");
                    boundRoute_.create(connectStatus.defaultAdapter.gatewayIp, connectStatus.vpnAdapter.adapterName);
                } else if (connectStatus.protocol == kCmdProtocolWireGuard) {
                    // add wireguard default routes
                    vpnRoutes_.addWithInterface("0.0.0.0", connectStatus.vpnAdapter.adapterName, "1");
                    vpnRoutes_.addWithInterface("128.0.0.0", connectStatus.vpnAdapter.adapterName, "1");
                }
                boundRoute_.create(connectStatus.defaultAdapter.gatewayIp, connectStatus.defaultAdapter.adapterName);
            }
        }
        addDnsRoutes(connectStatus);
    }

    connectStatus_ = connectStatus;
    isSplitTunnelActive_ = isSplitTunnelActive;
    isExcludeMode_ = isExcludeMode;
}

void RoutesManager::addDnsRoutes(const ConnectStatus &connectStatus)
{
    // 10.255.255.0/24 contains the default DNS servers, on-node APIs, decoy traffic servers, etc
    // and always goes through the tunnel.
    dnsServersRoutes_.addWithInterface("10.255.255.0", connectStatus.vpnAdapter.adapterName, "24");
}

void RoutesManager::clearAllRoutes()
{
    dnsServersRoutes_.clear();
    vpnRoutes_.clear();
    boundRoute_.remove();
}
