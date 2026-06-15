#include "routes_manager.h"
#include "ip_forward_table.h"

RoutesManager::RoutesManager()
{
    isSplitTunnelActive_ = false;
    isExcludeMode_ = false;
    connectStatus_.isConnected = false;
}

void RoutesManager::updateState(const ConnectStatus &connectStatus, bool isSplitTunnelActive, bool isExcludeMode)
{
    bool prevIsConnected = connectStatus_.isConnected;

    if  (prevIsConnected == false && connectStatus.isConnected == true)
    {
        if (isSplitTunnelActive)
        {
            if (!isExcludeMode)
            {
                if (connectStatus.protocol == kCmdProtocolOpenvpn || connectStatus.protocol == kCmdProtocolStunnelOrWstunnel)
                {
                    doActionsForInclusiveModeOpenVpn(connectStatus);
                }
                else if (connectStatus.protocol == kCmdProtocolIkev2)
                {
                    doActionsForInclusiveModeIkev2(connectStatus);
                }
                else if (connectStatus.protocol == kCmdProtocolWireGuard)
                {
                    doActionsForInclusiveModeWireGuard(connectStatus);
                }
            }
        }
        addDnsRoutes(connectStatus);
    }
    else if (prevIsConnected == true && connectStatus.isConnected == false)
    {
        clearAllRoutes();
    }
    else if (prevIsConnected == true && connectStatus.isConnected == true)
    {
        if (isSplitTunnelActive == false)
        {
            clearAllRoutes();
        }
        else  // if (isSplitTunnelActive == true)
        {
            clearAllRoutes();

            if (!isExcludeMode)
            {
                if (connectStatus.protocol == kCmdProtocolOpenvpn || connectStatus.protocol == kCmdProtocolStunnelOrWstunnel)
                {
                    doActionsForInclusiveModeOpenVpn(connectStatus);
                }
                else if (connectStatus.protocol == kCmdProtocolIkev2)
                {
                    doActionsForInclusiveModeIkev2(connectStatus);
                }
                else if (connectStatus.protocol == kCmdProtocolWireGuard)
                {
                    doActionsForInclusiveModeWireGuard(connectStatus);
                }
            }
        }
        addDnsRoutes(connectStatus);
    }

    connectStatus_ = connectStatus;
    isSplitTunnelActive_ = isSplitTunnelActive;
    isExcludeMode_ = isExcludeMode;
}

void RoutesManager::clearAllRoutes()
{
    dnsServersRoutes_.revertRoutes();
    boundRoute_.revertRoutes();
    openVpnRoutes_.revertRoutes();
    ikev2Routes_.revertRoutes();
    wgRoutes_.revertRoutes();
}

void RoutesManager::doActionsForInclusiveModeOpenVpn(const ConnectStatus &connectStatus)
{
    IpForwardTable table;
    const auto &vpn = connectStatus.vpnAdapter;

    // IPv4: remove openvpn --redirect-gateway def1 halves, add bound default at max metric.
    openVpnRoutes_.deleteRoute(table, types::IpAddress("0.0.0.0"),   1, vpn.gatewayIp, vpn.ifIndex);
    openVpnRoutes_.deleteRoute(table, types::IpAddress("128.0.0.0"), 1, vpn.gatewayIp, vpn.ifIndex);
    boundRoute_.addRoute(table, types::IpAddress("0.0.0.0"), 0, vpn.gatewayIp, vpn.ifIndex, true);

    // IPv6 mirror — only if the server handed us a v6 gateway on the VPN adapter.
    // OpenVPN with `--redirect-gateway ipv6 def1` installs ::/1 + 8000::/1.
    if (vpn.gatewayIpV6.isValid()) {
        openVpnRoutes_.deleteRoute(table, types::IpAddress("::"),     1, vpn.gatewayIpV6, vpn.ifIndex);
        openVpnRoutes_.deleteRoute(table, types::IpAddress("8000::"), 1, vpn.gatewayIpV6, vpn.ifIndex);
        boundRoute_.addRoute(table, types::IpAddress("::"), 0, vpn.gatewayIpV6, vpn.ifIndex, true);
    }
}

void RoutesManager::doActionsForInclusiveModeWireGuard(const ConnectStatus &connectStatus)
{
    IpForwardTable table;
    const auto &vpn = connectStatus.vpnAdapter;

    // wireguard-windows installs AllowedIPs routes with NextHop = 0.0.0.0 / :: (on-link), see
    // tools/deps/custom_amneziawg-windows/addressconfig.go (`route.NextHop = net.IPv4zero`).
    // Routes::deleteRoute matches byte-exact on NextHop, so we must pass the same on-link
    // sentinel here — using vpn.adapterIp would silently fail to delete the /1 routes,
    // leaving them in place and forcing all v4 traffic through the tunnel even in inclusive
    // mode. The bound /0 we add is a regular helper-managed route and uses vpn.adapterIp as
    // its NextHop (this is what shows up in `route print` per test plan 7.2).
    wgRoutes_.deleteRoute(table, types::IpAddress("0.0.0.0"),   1, types::IpAddress("0.0.0.0"), vpn.ifIndex);
    wgRoutes_.deleteRoute(table, types::IpAddress("128.0.0.0"), 1, types::IpAddress("0.0.0.0"), vpn.ifIndex);
    boundRoute_.addRoute(table, types::IpAddress("0.0.0.0"), 0, vpn.adapterIp, vpn.ifIndex, true);

    if (vpn.adapterIpV6.isValid()) {
        wgRoutes_.deleteRoute(table, types::IpAddress("::"),     1, types::IpAddress("::"), vpn.ifIndex);
        wgRoutes_.deleteRoute(table, types::IpAddress("8000::"), 1, types::IpAddress("::"), vpn.ifIndex);
        boundRoute_.addRoute(table, types::IpAddress("::"), 0, vpn.adapterIpV6, vpn.ifIndex, true);
    }
}

void RoutesManager::doActionsForInclusiveModeIkev2(const ConnectStatus &connectStatus)
{
    IpForwardTable table;
    const auto &vpn = connectStatus.vpnAdapter;

    // Windows/RAS can install the IKEv2 default route as on-link (NextHop 0.0.0.0)
    // instead of vpn.adapterIp. Delete the adapter default by destination/interface so
    // the low-metric /0 cannot keep tunneling everything in inclusive split-tunnel mode.
    ikev2Routes_.deleteRouteByInterface(table, types::IpAddress("0.0.0.0"), 0, vpn.ifIndex);
    boundRoute_.addRoute(table, types::IpAddress("0.0.0.0"), 0, vpn.adapterIp, vpn.ifIndex, true);

    // IKEv2 is typically v4-only, but handle a v6 default route on the adapter if present.
    if (vpn.adapterIpV6.isValid()) {
        ikev2Routes_.deleteRouteByInterface(table, types::IpAddress("::"), 0, vpn.ifIndex);
        boundRoute_.addRoute(table, types::IpAddress("::"), 0, vpn.adapterIpV6, vpn.ifIndex, true);
    }
}

void RoutesManager::addDnsRoutes(const ConnectStatus &connectStatus)
{
    IpForwardTable table;
    const auto &vpn = connectStatus.vpnAdapter;

    // 10.255.255.0/24 contains the default DNS servers, on-node APIs, decoy traffic servers, etc
    // and always goes through the tunnel. No IPv6 analog in Windscribe DNS infrastructure.
    //
    // OpenVPN (TAP/Wintun) gets a real DHCP gateway in vpn.gatewayIp; WireGuard and IKEv2 are
    // point-to-point and FirstGatewayAddress is empty, so vpn.gatewayIp is invalid for them.
    // Fall back to the adapter address — for a P2P interface that is the only meaningful
    // next-hop and Windows resolves it on-link via the same interface. If both are invalid,
    // Routes::addRoute will reject the call and log the error itself.
    const types::IpAddress &nextHop = vpn.gatewayIp.isValid() ? vpn.gatewayIp : vpn.adapterIp;
    dnsServersRoutes_.addRoute(table, types::IpAddress("10.255.255.0"), 24, nextHop, vpn.ifIndex, false);
}
