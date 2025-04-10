#include "routes_manager.h"
#include <spdlog/spdlog.h>
#include "../utils.h"

RoutesManager::RoutesManager()
{
    isSplitTunnelActive_ = false;
    isExcludeMode_ = false;
    connectStatus_.isConnected = false;
}

void RoutesManager::setSplitTunnelSettings(bool isSplitTunnelActive, bool isExcludeMode)
{
    updateState(connectStatus_, isSplitTunnelActive, isExcludeMode);
}

void RoutesManager::setConnectStatus(const ConnectStatus &connectStatus)
{
    updateState(connectStatus, isSplitTunnelActive_, isExcludeMode_);
}


void RoutesManager::updateState(const ConnectStatus &connectStatus, bool isSplitTunnelActive, bool isExcludeMode)
{
    bool prevIsConnected = connectStatus_.isConnected;
    bool prevIsActive = isSplitTunnelActive_;
    bool prevIsExcludeMode = isExcludeMode_;

    if  (prevIsConnected == false && connectStatus.isConnected == true) {
        if (isSplitTunnelActive) {
            boundRoute_.create(connectStatus.defaultAdapter.gatewayIp, connectStatus.defaultAdapter.adapterName);
        }
        addDnsRoutes(connectStatus);
    } else if (prevIsConnected == true && connectStatus.isConnected == false) {
        clearAllRoutes();
    } else if (prevIsConnected == true && connectStatus.isConnected == true) {
        if (prevIsActive != isSplitTunnelActive || prevIsExcludeMode != isExcludeMode) {
            clearAllRoutes();

            if (connectStatus.protocol == kCmdProtocolOpenvpn || connectStatus.protocol == kCmdProtocolStunnelOrWstunnel) {
                // add openvpn default routes
                vpnRoutes_.add(connectStatus.remoteIp, connectStatus.defaultAdapter.gatewayIp, "255.255.255.255");
                vpnRoutes_.add("0.0.0.0", connectStatus.vpnAdapter.gatewayIp, "128.0.0.0");
                vpnRoutes_.add("128.0.0.0", connectStatus.vpnAdapter.gatewayIp, "128.0.0.0");
                boundRoute_.create(connectStatus.defaultAdapter.gatewayIp, connectStatus.vpnAdapter.adapterName);
            } else if (connectStatus.protocol == kCmdProtocolWireGuard) {
                // add wireguard default routes
                vpnRoutes_.addWithInterface("0.0.0.0/1", connectStatus.vpnAdapter.adapterName);
                vpnRoutes_.addWithInterface("128.0.0.0/1", connectStatus.vpnAdapter.adapterName);
            }
            boundRoute_.create(connectStatus.defaultAdapter.gatewayIp, connectStatus.defaultAdapter.adapterName);
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
    dnsServersRoutes_.addWithInterface("10.255.255.0/24", connectStatus.vpnAdapter.adapterName);
}

void RoutesManager::clearAllRoutes()
{
    dnsServersRoutes_.clear();
    vpnRoutes_.clear();
    boundRoute_.remove();
}
