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
                    deleteWireGuardDefaultRoutes(connectStatus);
                    dnsServersRoutes_.add(wgDnsAddress_, connectStatus.vpnAdapterIp, "255.255.255.255");
                    boundRoute_.create(connectStatus.vpnAdapterIp, connectStatus.vpnAdapterName);
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
                        boundRoute_.create(connectStatus.gatewayIp, connectStatus.interfaceName);
                    }
                    else if (connectStatus.protocol == CMD_PROTOCOL_WIREGUARD)
                    {
                        // add wireguard default routes
                        vpnRoutes_.addWithInterface("0.0.0.0/1", wgAdapterName_);
                        vpnRoutes_.addWithInterface("128.0.0.0/1", wgAdapterName_);
                    }

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
                        deleteWireGuardDefaultRoutes(connectStatus);
                        dnsServersRoutes_.add(wgDnsAddress_, connectStatus.vpnAdapterIp, "255.255.255.255");
                        boundRoute_.create(connectStatus.vpnAdapterIp, connectStatus.vpnAdapterName);
                    }
                }
            }
        }
    }
    
    connectStatus_ = connectStatus;
    isSplitTunnelActive_ = isSplitTunnelActive;
    isExcludeMode_ = isExcludeMode;
}
void RoutesManager::setLatestWireGuardAdapterSettings(const std::string &adapterName, const std::string &ipAddress, const std::string &dnsAddressList,
const std::vector<std::string> &allowedIps)
{
    wgAdapterName_ = adapterName;
    wgIpAddress_ = ipAddress;
    wgDnsAddress_ = dnsAddressList;     //todo several DNS-IPs
    if (allowedIps.size() > 0)          // todo several allowed-IPs
    {
        wgAllowedIp_ = allowedIps[0];
    }
    else
    {
        wgAllowedIp_.clear();
    }
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

void RoutesManager::deleteWireGuardDefaultRoutes(const CMD_SEND_CONNECT_STATUS &connectStatus)
{
    std::string cmd = "route delete -inet 0.0.0.0/1 -interface " + wgAdapterName_;
    LOG("execute: %s", cmd.c_str());
    Utils::executeCommand(cmd);
    
    cmd = "route delete -inet 128.0.0.0/1 -interface " + wgAdapterName_;
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
        dnsServersRoutes_.add(*it, connectStatus.vpnAdapterIp, "255.255.255.255");
    }
    
    // add bound route
    boundRoute_.create(connectStatus.vpnAdapterIp, connectStatus.vpnAdapterName);
}
void RoutesManager::clearAllRoutes()
{
    dnsServersRoutes_.clear();
    ikev2OverrideRoutes_.clear();
    boundRoute_.remove();
}
