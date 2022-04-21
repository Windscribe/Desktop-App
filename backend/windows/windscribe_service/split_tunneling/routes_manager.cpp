#include "../all_headers.h"
#include "routes_manager.h"
#include "../ip_address/ip4_address_and_mask.h"

RoutesManager::RoutesManager()
{
    isSplitTunnelActive_ = false;
    isExcludeMode_ = false;
    connectStatus_.isConnected = false;
}

void RoutesManager::updateState(const CMD_CONNECT_STATUS &connectStatus, bool isSplitTunnelActive, bool isExcludeMode)
{
    bool prevIsConnected = connectStatus_.isConnected;
    
    if  (prevIsConnected == false && connectStatus.isConnected == true)
    {
        if (isSplitTunnelActive)
        {
            if (!isExcludeMode)
            {
                if (connectStatus.protocol == CMD_PROTOCOL_OPENVPN || connectStatus.protocol == CMD_PROTOCOL_STUNNEL_OR_WSTUNNEL)
                {
					doActionsForInclusiveModeOpenVpn(connectStatus);
                }
                else if (connectStatus.protocol == CMD_PROTOCOL_IKEV2)
                {
					doActionsForInclusiveModeIkev2(connectStatus);
                }
                else if (connectStatus.protocol == CMD_PROTOCOL_WIREGUARD)
                {
					doActionsForInclusiveModeWireGuard(connectStatus);
                }
            }
        }
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
				if (connectStatus.protocol == CMD_PROTOCOL_OPENVPN || connectStatus.protocol == CMD_PROTOCOL_STUNNEL_OR_WSTUNNEL)
				{
					doActionsForInclusiveModeOpenVpn(connectStatus);
				}
				else if (connectStatus.protocol == CMD_PROTOCOL_IKEV2)
				{
					doActionsForInclusiveModeIkev2(connectStatus);
				}
				else if (connectStatus.protocol == CMD_PROTOCOL_WIREGUARD)
				{
					doActionsForInclusiveModeWireGuard(connectStatus);
				}
			}
		}
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

void RoutesManager::doActionsForInclusiveModeOpenVpn(const CMD_CONNECT_STATUS &connectStatus)
{
	// delete openvpn default routes
	IpForwardTable table;
	//openVpnRoutes_.deleteRoute(table, connectStatus.remoteIp, "255.255.255.255", connectStatus.defaultAdapter.gatewayIp, connectStatus.defaultAdapter.ifIndex);
	openVpnRoutes_.deleteRoute(table, "0.0.0.0", "128.0.0.0", connectStatus.vpnAdapter.gatewayIp, connectStatus.vpnAdapter.ifIndex);
	openVpnRoutes_.deleteRoute(table, "128.0.0.0", "128.0.0.0", connectStatus.vpnAdapter.gatewayIp, connectStatus.vpnAdapter.ifIndex);

	// add routes for DNS servers
	for (auto it = connectStatus.vpnAdapter.dnsServers.begin(); it != connectStatus.vpnAdapter.dnsServers.end(); ++it)
	{
		dnsServersRoutes_.addRoute(table, *it, "255.255.255.255", connectStatus.vpnAdapter.gatewayIp, connectStatus.vpnAdapter.ifIndex, false);
	}
	// add bound route
	boundRoute_.addRoute(table, "0.0.0.0", "0.0.0.0", connectStatus.vpnAdapter.gatewayIp, connectStatus.vpnAdapter.ifIndex, true);
}
void RoutesManager::doActionsForInclusiveModeWireGuard(const CMD_CONNECT_STATUS &connectStatus)
{
	// delete wireguard default route
	IpForwardTable table;
	wgRoutes_.deleteRoute(table, "0.0.0.0", "128.0.0.0", connectStatus.vpnAdapter.adapterIp, connectStatus.vpnAdapter.ifIndex);
	wgRoutes_.deleteRoute(table, "128.0.0.0", "128.0.0.0", connectStatus.vpnAdapter.adapterIp, connectStatus.vpnAdapter.ifIndex);
	// add routes for DNS servers
	for (auto it = connectStatus.vpnAdapter.dnsServers.begin(); it != connectStatus.vpnAdapter.dnsServers.end(); ++it)
	{
		dnsServersRoutes_.addRoute(table, *it, "255.255.255.255", connectStatus.vpnAdapter.gatewayIp, connectStatus.vpnAdapter.ifIndex, false);
	}
	// add bound route
	boundRoute_.addRoute(table, "0.0.0.0", "0.0.0.0", connectStatus.vpnAdapter.adapterIp, connectStatus.vpnAdapter.ifIndex, true);
}
void RoutesManager::doActionsForInclusiveModeIkev2(const CMD_CONNECT_STATUS &connectStatus)
{
	// delete ikev2 default route
	IpForwardTable table;
	ikev2Routes_.deleteRoute(table, "0.0.0.0", "0.0.0.0", connectStatus.vpnAdapter.adapterIp, connectStatus.vpnAdapter.ifIndex);
	// add bound route
	boundRoute_.addRoute(table, "0.0.0.0", "0.0.0.0", connectStatus.vpnAdapter.adapterIp, connectStatus.vpnAdapter.ifIndex, true);
}