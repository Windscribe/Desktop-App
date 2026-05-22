#include "routes_manager.h"

#include <spdlog/spdlog.h>

#include "../utils.h"
#include "types/ipaddress.h"

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
            addLanMulticastRoutes(connectStatus);
        }
        addDnsRoutes(connectStatus);
    } else if (prevIsConnected == true && connectStatus.isConnected == false) {
        clearAllRoutes();
    } else if (prevIsConnected == true && connectStatus.isConnected == true) {
        if (prevIsActive != isSplitTunnelActive || prevIsExcludeMode != isExcludeMode) {
            clearAllRoutes();

            if (connectStatus.protocol == kCmdProtocolOpenvpn || connectStatus.protocol == kCmdProtocolStunnelOrWstunnel) {
                // OpenVPN/Stunnel/Wstunnel default routes. Windscribe-issued OVPN configs
                // are v4-only today; the v6 default-route sibling below is defence-in-depth
                // for custom configs that push ifconfig-ipv6. The remote-endpoint pin picks
                // its gateway / prefix off remoteIp's family so a custom OVPN with a v6
                // `remote` directive doesn't silently no-op through Routes::add's
                // same-family check.
                if (connectStatus.remoteIp.isV6()) {
                    if (connectStatus.defaultAdapter.gatewayIpV6.isValid()) {
                        vpnRoutes_.add(connectStatus.remoteIp, connectStatus.defaultAdapter.gatewayIpV6, 128);
                    } else {
                        // v6 endpoint over v4-only transit: no v6 default gateway to pin against.
                        // Skip the explicit pin — the system's default v6 route still carries the
                        // packets, just without re-pinning to the physical adapter.
                        spdlog::warn("RoutesManager: no IPv6 default gateway to pin v6 OpenVPN remote {}; "
                                     "skipping bound-route (tunnel may rely on system v6 default route)",
                                     connectStatus.remoteIp.toString());
                    }
                } else {
                    vpnRoutes_.add(connectStatus.remoteIp, connectStatus.defaultAdapter.gatewayIp, 32);
                }
                vpnRoutes_.add(types::IpAddress("0.0.0.0"), connectStatus.vpnAdapter.gatewayIp, 1);
                vpnRoutes_.add(types::IpAddress("128.0.0.0"), connectStatus.vpnAdapter.gatewayIp, 1);
                if (connectStatus.vpnAdapter.gatewayIpV6.isValid()) {
                    vpnRoutes_.add(types::IpAddress("::"),     connectStatus.vpnAdapter.gatewayIpV6, 1);
                    vpnRoutes_.add(types::IpAddress("8000::"), connectStatus.vpnAdapter.gatewayIpV6, 1);
                }
                boundRoute_.create(connectStatus.defaultAdapter.gatewayIp, connectStatus.vpnAdapter.adapterName);
            } else if (connectStatus.protocol == kCmdProtocolWireGuard) {
                // WireGuard default routes (v4). The v6 default-route bypass is added by
                // WireGuardAdapter::enableRouting on the v6 segment of the tunnel address,
                // not here, so no v6 sibling.
                vpnRoutes_.addWithInterface(types::IpAddressRange("0.0.0.0/1"), connectStatus.vpnAdapter.adapterName);
                vpnRoutes_.addWithInterface(types::IpAddressRange("128.0.0.0/1"), connectStatus.vpnAdapter.adapterName);
            }
            boundRoute_.create(connectStatus.defaultAdapter.gatewayIp, connectStatus.defaultAdapter.adapterName);
            if (connectStatus.defaultAdapter.gatewayIpV6.isValid()) {
                boundRouteV6_.create(connectStatus.defaultAdapter.gatewayIpV6, connectStatus.defaultAdapter.adapterName);
            }
            if (isSplitTunnelActive) {
                addLanMulticastRoutes(connectStatus);
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
    // and always goes through the tunnel. v4-only by design — Windscribe's on-node services are
    // reachable only over v4 (10.255.255.0/24); no v6 sibling needed.
    dnsServersRoutes_.addWithInterface(types::IpAddressRange("10.255.255.0/24"), connectStatus.vpnAdapter.adapterName);
}

void RoutesManager::addLanMulticastRoutes(const ConnectStatus &connectStatus)
{
    // v4-only: pf `pass quick on <iface> inet6 from any to ff00::/8` in firewallcontroller_mac.cpp /
    // firewallonboot.cpp already covers v6 multicast; no explicit route needed.
    lanMulticastRoutes_.addWithInterface(types::IpAddressRange("224.0.0.0/4"), connectStatus.defaultAdapter.adapterName);
}

void RoutesManager::clearAllRoutes()
{
    dnsServersRoutes_.clear();
    vpnRoutes_.clear();
    lanMulticastRoutes_.clear();
    boundRoute_.remove();
    boundRouteV6_.remove();
}
