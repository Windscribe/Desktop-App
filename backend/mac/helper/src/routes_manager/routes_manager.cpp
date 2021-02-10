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
                boundRoute_.create(connectStatus.defaultAdapter.gatewayIp, connectStatus.defaultAdapter.adapterName);
            }
            else
            {
                if (connectStatus.protocol == CMD_PROTOCOL_OPENVPN || connectStatus.protocol == CMD_PROTOCOL_STUNNEL_OR_WSTUNNEL)
                {
                    // delete openvpn default routes
                    deleteOpenVpnDefaultRoutes(connectStatus);
                    // add routes for DNS servers
                    for (auto it = connectStatus.vpnAdapter.dnsServers.begin(); it != connectStatus.vpnAdapter.dnsServers.end(); ++it)
                    {
                        dnsServersRoutes_.add(*it, connectStatus.vpnAdapter.gatewayIp, "255.255.255.255");
                    }
                    
                    // add bound route
                    boundRoute_.create(connectStatus.vpnAdapter.gatewayIp, connectStatus.vpnAdapter.adapterName);
                }
                else if (connectStatus.protocol == CMD_PROTOCOL_IKEV2)
                {
                    addIkev2RoutesForInclusiveMode(connectStatus);
                }
                else if (connectStatus.protocol == CMD_PROTOCOL_WIREGUARD)
                {
                    deleteWireGuardDefaultRoutes(connectStatus);
                    if (!connectStatus.vpnAdapter.dnsServers.empty())
                    {
                        dnsServersRoutes_.add(connectStatus.vpnAdapter.dnsServers[0], connectStatus.vpnAdapter.adapterIp, "255.255.255.255");
                    }
                    boundRoute_.create(connectStatus.vpnAdapter.adapterIp, connectStatus.vpnAdapter.adapterName);
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
                        vpnRoutes_.add(connectStatus.remoteIp, connectStatus.defaultAdapter.gatewayIp, "255.255.255.255");
                        vpnRoutes_.add("0.0.0.0", connectStatus.vpnAdapter.gatewayIp, "128.0.0.0");
                        vpnRoutes_.add("128.0.0.0", connectStatus.vpnAdapter.gatewayIp, "128.0.0.0");
                        boundRoute_.create(connectStatus.defaultAdapter.gatewayIp, connectStatus.vpnAdapter.adapterName);
                    }
                    else if (connectStatus.protocol == CMD_PROTOCOL_WIREGUARD)
                    {
                        // add wireguard default routes
                        vpnRoutes_.addWithInterface("0.0.0.0/1", connectStatus.vpnAdapter.adapterName);
                        vpnRoutes_.addWithInterface("128.0.0.0/1", connectStatus.vpnAdapter.adapterName);
                    }

                    boundRoute_.create(connectStatus.defaultAdapter.gatewayIp, connectStatus.defaultAdapter.adapterName);
                }
                else
                {
                    if (connectStatus.protocol == CMD_PROTOCOL_OPENVPN || connectStatus.protocol == CMD_PROTOCOL_STUNNEL_OR_WSTUNNEL)
                    {
                        deleteOpenVpnDefaultRoutes(connectStatus);
                        // add routes for DNS servers
                        for (auto it = connectStatus.vpnAdapter.dnsServers.begin(); it != connectStatus.vpnAdapter.dnsServers.end(); ++it)
                        {
                            dnsServersRoutes_.add(*it, connectStatus.vpnAdapter.gatewayIp, "255.255.255.255");
                        }
                        // add bound route
                        boundRoute_.create(connectStatus.vpnAdapter.gatewayIp, connectStatus.vpnAdapter.adapterName);
                    }
                    else if (connectStatus.protocol == CMD_PROTOCOL_IKEV2)
                    {
                        addIkev2RoutesForInclusiveMode(connectStatus);
                    }
                    else if (connectStatus.protocol == CMD_PROTOCOL_WIREGUARD)
                    {
                        deleteWireGuardDefaultRoutes(connectStatus);
                        // todo several DNS-servers
                        if (!connectStatus.vpnAdapter.dnsServers.empty())
                        {
                            dnsServersRoutes_.add(connectStatus.vpnAdapter.dnsServers[0], connectStatus.vpnAdapter.adapterIp, "255.255.255.255");
                        }
                        boundRoute_.create(connectStatus.vpnAdapter.adapterIp, connectStatus.vpnAdapter.adapterName);
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
    std::string cmd = "route delete -net " + connectStatus.remoteIp + " " + connectStatus.defaultAdapter.gatewayIp + " 255.255.255.255";
    LOG("execute: %s", cmd.c_str());
    Utils::executeCommand(cmd);
    
    cmd = "route delete -net 0.0.0.0 " + connectStatus.vpnAdapter.gatewayIp + " 128.0.0.0";
    LOG("execute: %s", cmd.c_str());
    Utils::executeCommand(cmd);
    
    cmd = "route delete -net 128.0.0.0 " + connectStatus.vpnAdapter.gatewayIp + " 128.0.0.0";
    LOG("execute: %s", cmd.c_str());
    Utils::executeCommand(cmd);
}

void RoutesManager::deleteWireGuardDefaultRoutes(const CMD_SEND_CONNECT_STATUS &connectStatus)
{
    std::string cmd = "route delete -inet 0.0.0.0/1 -interface " + connectStatus.vpnAdapter.adapterName;
    LOG("execute: %s", cmd.c_str());
    Utils::executeCommand(cmd);
    
    cmd = "route delete -inet 128.0.0.0/1 -interface " + connectStatus.vpnAdapter.adapterName;
    LOG("execute: %s", cmd.c_str());
    Utils::executeCommand(cmd);
}

void RoutesManager::addIkev2RoutesForInclusiveMode(const CMD_SEND_CONNECT_STATUS &connectStatus)
{
    // add routes for override ikev2 default routes
    ikev2OverrideRoutes_.add("0.0.0.0", connectStatus.defaultAdapter.gatewayIp, "128.0.0.0");
    ikev2OverrideRoutes_.add("128.0.0.0", connectStatus.defaultAdapter.gatewayIp, "128.0.0.0");
    
    // add DNS-servers
    for (auto it = connectStatus.vpnAdapter.dnsServers.begin(); it != connectStatus.vpnAdapter.dnsServers.end(); ++it)
    {
        dnsServersRoutes_.add(*it, connectStatus.vpnAdapter.adapterIp, "255.255.255.255");
    }
    
    // add bound route
    boundRoute_.create(connectStatus.vpnAdapter.adapterIp, connectStatus.vpnAdapter.adapterName);
}
void RoutesManager::clearAllRoutes()
{
    dnsServersRoutes_.clear();
    ikev2OverrideRoutes_.clear();
    boundRoute_.remove();
}
