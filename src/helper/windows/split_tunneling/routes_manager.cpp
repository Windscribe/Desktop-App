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
    // delete openvpn default routes
    IpForwardTable table;
    //openVpnRoutes_.deleteRoute(table, connectStatus.remoteIp, "255.255.255.255", connectStatus.defaultAdapter.gatewayIp, connectStatus.defaultAdapter.ifIndex);
    openVpnRoutes_.deleteRoute(table, "0.0.0.0", "128.0.0.0", connectStatus.vpnAdapter.gatewayIp, connectStatus.vpnAdapter.ifIndex);
    openVpnRoutes_.deleteRoute(table, "128.0.0.0", "128.0.0.0", connectStatus.vpnAdapter.gatewayIp, connectStatus.vpnAdapter.ifIndex);
    // add bound route
    boundRoute_.addRoute(table, "0.0.0.0", "0.0.0.0", connectStatus.vpnAdapter.gatewayIp, connectStatus.vpnAdapter.ifIndex, true);
}

void RoutesManager::doActionsForInclusiveModeWireGuard(const ConnectStatus &connectStatus)
{
    // delete wireguard default route
    IpForwardTable table;
    wgRoutes_.deleteRoute(table, "0.0.0.0", "128.0.0.0", connectStatus.vpnAdapter.adapterIp, connectStatus.vpnAdapter.ifIndex);
    wgRoutes_.deleteRoute(table, "128.0.0.0", "128.0.0.0", connectStatus.vpnAdapter.adapterIp, connectStatus.vpnAdapter.ifIndex);
    // add bound route
    boundRoute_.addRoute(table, "0.0.0.0", "0.0.0.0", connectStatus.vpnAdapter.adapterIp, connectStatus.vpnAdapter.ifIndex, true);
}

void RoutesManager::doActionsForInclusiveModeIkev2(const ConnectStatus &connectStatus)
{
    // delete ikev2 default route
    IpForwardTable table;
    ikev2Routes_.deleteRoute(table, "0.0.0.0", "0.0.0.0", connectStatus.vpnAdapter.adapterIp, connectStatus.vpnAdapter.ifIndex);
    // add bound route
    boundRoute_.addRoute(table, "0.0.0.0", "0.0.0.0", connectStatus.vpnAdapter.adapterIp, connectStatus.vpnAdapter.ifIndex, true);
}

void RoutesManager::addDnsRoutes(const ConnectStatus &connectStatus)
{
    IpForwardTable table;

    // 10.255.255.0/24 contains the default DNS servers, on-node APIs, decoy traffic servers, etc
    // and always goes through the tunnel.
    dnsServersRoutes_.addRoute(table, "10.255.255.0", "255.255.255.0", connectStatus.vpnAdapter.gatewayIp, connectStatus.vpnAdapter.ifIndex, false);
}
