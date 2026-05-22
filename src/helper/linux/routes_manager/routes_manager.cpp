#include "routes_manager.h"

#include "types/ipaddress.h"

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

    if (prevIsConnected == false && connectStatus.isConnected == true) {
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
                    // add openvpn default routes (v4 — Windscribe OpenVPN does not push v6)
                    vpnRoutes_.add(connectStatus.remoteIp, connectStatus.defaultAdapter.gatewayIp, 32);
                    vpnRoutes_.add(types::IpAddress("0.0.0.0"), connectStatus.vpnAdapter.gatewayIp, 1);
                    vpnRoutes_.add(types::IpAddress("128.0.0.0"), connectStatus.vpnAdapter.gatewayIp, 1);
                    // TODO if/when OVPN pushes v6: gated on connectStatus.vpnAdapter.adapterIpV6.isValid(),
                    //   vpnRoutes_.add(types::IpAddress("::"), connectStatus.vpnAdapter.gatewayIpV6, 1);
                    //   vpnRoutes_.add(types::IpAddress("8000::"), connectStatus.vpnAdapter.gatewayIpV6, 1);
                    boundRoute_.create(connectStatus.defaultAdapter.gatewayIp, connectStatus.vpnAdapter.adapterName);
                } else if (connectStatus.protocol == kCmdProtocolWireGuard) {
                    // add wireguard default routes
                    // (WG v6 default-route bypass is handled via fwmark tables in WireGuardAdapter::enableRouting,
                    //  not via routes_manager.)
                    vpnRoutes_.addWithInterface(types::IpAddress("0.0.0.0"), connectStatus.vpnAdapter.adapterName, 1);
                    vpnRoutes_.addWithInterface(types::IpAddress("128.0.0.0"), connectStatus.vpnAdapter.adapterName, 1);
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
    // and always goes through the tunnel. v4-only by design — Windscribe's on-node services
    // are reachable only over v4 (10.255.255.0/24); no v6 sibling needed.
    dnsServersRoutes_.addWithInterface(types::IpAddress("10.255.255.0"), connectStatus.vpnAdapter.adapterName, 24);
}

void RoutesManager::clearAllRoutes()
{
    dnsServersRoutes_.clear();
    vpnRoutes_.clear();
    boundRoute_.remove();
}
