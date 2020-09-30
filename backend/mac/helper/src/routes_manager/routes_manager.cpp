#include "routes_manager.h"
#include "utils.h"
#include "logger.h"

RoutesManager::RoutesManager()
{
    isSplitTunnelActive_ = false;
    isExcludeMode_ = false;
    connectStatus_.isConnected = false;
}

void RoutesManager::updateState(const CMD_SEND_CONNECT_STATUS &connectStatus, bool isSplitTunnelActive, bool isExcludeMode)
{
    bool prevIsConnected = connectStatus_.isConnected;
    bool prevIsExcludeMode = isExcludeMode_;
    
    if  (prevIsConnected == false && connectStatus.isConnected == true)
    {
        if (isSplitTunnelActive)
        {
            if (isExcludeMode)
            {
                // add bound route
                boundRoute_.create(connectStatus.gatewayIp, connectStatus.interfaceName);
            }
            else
            {
                if (connectStatus.protocol == CMD_PROTOCOL_OPENVPN || connectStatus.protocol == CMD_PROTOCOL_STUNNEL_OR_WSTUNNEL)
                {
                    // delete openvpn default routes
                    deleteOpenVpnDefaultRoutes(connectStatus);
                    // add routes for DNS servers
                    for (auto it = connectStatus.dnsServers.begin(); it != connectStatus.dnsServers.end(); ++it)
                    {
                        dnsServersRoutes_.add(*it, connectStatus.routeVpnGateway, "255.255.255.255");
                    }
                    
                    // add bound route
                    boundRoute_.create(connectStatus.routeVpnGateway, connectStatus.vpnAdapterName);
                }
                else if (connectStatus.protocol == CMD_PROTOCOL_IKEV2)
                {
                    addIkev2RoutesForInclusiveMode(connectStatus);
                }
                else if (connectStatus.protocol == CMD_PROTOCOL_WIREGUARD)
                {
                    // TODO(wireguard)
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
            if (prevIsExcludeMode != isExcludeMode)
            {
                clearAllRoutes();
                
                if (isExcludeMode)
                {
                    if (connectStatus.protocol == CMD_PROTOCOL_OPENVPN || connectStatus.protocol == CMD_PROTOCOL_STUNNEL_OR_WSTUNNEL)
                    {
                        // add openvpn default routes
                        vpnRoutes_.add(connectStatus.remote_1, connectStatus.routeNetGateway, "255.255.255.255");
                        vpnRoutes_.add("0.0.0.0", connectStatus.routeVpnGateway, "128.0.0.0");
                        vpnRoutes_.add("128.0.0.0", connectStatus.routeVpnGateway, "128.0.0.0");
                        
                    }
                    // add bound route
                    boundRoute_.create(connectStatus.gatewayIp, connectStatus.interfaceName);
                }
                else
                {
                    if (connectStatus.protocol == CMD_PROTOCOL_OPENVPN || connectStatus.protocol == CMD_PROTOCOL_STUNNEL_OR_WSTUNNEL)
                    {
                        deleteOpenVpnDefaultRoutes(connectStatus);
                        // add routes for DNS servers
                        for (auto it = connectStatus.dnsServers.begin(); it != connectStatus.dnsServers.end(); ++it)
                        {
                            dnsServersRoutes_.add(*it, connectStatus.routeVpnGateway, "255.255.255.255");
                        }
                        // add bound route
                        boundRoute_.create(connectStatus.routeVpnGateway, connectStatus.vpnAdapterName);
                    }
                    else if (connectStatus.protocol == CMD_PROTOCOL_IKEV2)
                    {
                        addIkev2RoutesForInclusiveMode(connectStatus);
                    }
                    else if (connectStatus.protocol == CMD_PROTOCOL_WIREGUARD)
                    {
                        // TODO(wireguard)
                    }
                }
            }
        }
    }
    
    connectStatus_ = connectStatus;
    isSplitTunnelActive_ = isSplitTunnelActive;
    isExcludeMode_ = isExcludeMode;
}

void RoutesManager::deleteOpenVpnDefaultRoutes(const CMD_SEND_CONNECT_STATUS &connectStatus)
{
    std::string cmd = "route delete -net " + connectStatus.remote_1 + " " + connectStatus.routeNetGateway + " 255.255.255.255";
    LOG("execute: %s", cmd.c_str());
    Utils::executeCommand(cmd);
    
    cmd = "route delete -net 0.0.0.0 " + connectStatus.routeVpnGateway + " 128.0.0.0";
    LOG("execute: %s", cmd.c_str());
    Utils::executeCommand(cmd);
    
    cmd = "route delete -net 128.0.0.0 " + connectStatus.routeVpnGateway + " 128.0.0.0";
    LOG("execute: %s", cmd.c_str());
    Utils::executeCommand(cmd);
}

void RoutesManager::addIkev2RoutesForInclusiveMode(const CMD_SEND_CONNECT_STATUS &connectStatus)
{
    // add routes for override ikev2 default routes
    ikev2OverrideRoutes_.add("0.0.0.0", connectStatus.gatewayIp, "128.0.0.0");
    ikev2OverrideRoutes_.add("128.0.0.0", connectStatus.gatewayIp, "128.0.0.0");
    
    // add DNS-servers
    for (auto it = connectStatus.dnsServers.begin(); it != connectStatus.dnsServers.end(); ++it)
    {
        dnsServersRoutes_.add(*it, connectStatus.ikev2AdapterAddress, "255.255.255.255");
    }
    
    // add bound route
    boundRoute_.create(connectStatus.ikev2AdapterAddress, connectStatus.vpnAdapterName);
}
void RoutesManager::clearAllRoutes()
{
    dnsServersRoutes_.clear();
    ikev2OverrideRoutes_.clear();
    boundRoute_.remove();
}
