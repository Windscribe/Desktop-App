#include "routes_manager.h"
#include "../utils.h"
#include "../logger.h"

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

    if  (prevIsConnected == false && connectStatus.isConnected == true) {
        if (isSplitTunnelActive) {
            if (isExcludeMode) {
                // add bound route
                boundRoute_.create(connectStatus.defaultAdapter.gatewayIp, connectStatus.defaultAdapter.adapterName);
            } else {
                if (connectStatus.protocol == kCmdProtocolOpenvpn || connectStatus.protocol == kCmdProtocolStunnelOrWstunnel) {
                    // delete openvpn default routes
                    deleteOpenVpnDefaultRoutes(connectStatus);
                    // add bound route
                    boundRoute_.create(connectStatus.vpnAdapter.gatewayIp, connectStatus.vpnAdapter.adapterName);
                } else if (connectStatus.protocol == kCmdProtocolIkev2) {
                    addIkev2RoutesForInclusiveMode(connectStatus);
                } else if (connectStatus.protocol == kCmdProtocolWireGuard) {
                    deleteWireGuardDefaultRoutes(connectStatus);
                    boundRoute_.create(connectStatus.vpnAdapter.adapterIp, connectStatus.vpnAdapter.adapterName);
                }
            }
        }
        addDnsRoutes(connectStatus);
    } else if (prevIsConnected == true && connectStatus.isConnected == false) {
        clearAllRoutes();
    } else if (prevIsConnected == true && connectStatus.isConnected == true) {
        if (isSplitTunnelActive == false) {
            clearAllRoutes();
        } else { // if (isSplitTunnelActive == true)
            if (prevIsExcludeMode != isExcludeMode) {
                clearAllRoutes();

                if (isExcludeMode) {
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
                } else {
                    if (connectStatus.protocol == kCmdProtocolOpenvpn || connectStatus.protocol == kCmdProtocolStunnelOrWstunnel) {
                        deleteOpenVpnDefaultRoutes(connectStatus);
                        boundRoute_.create(connectStatus.vpnAdapter.gatewayIp, connectStatus.vpnAdapter.adapterName);
                    } else if (connectStatus.protocol == kCmdProtocolIkev2) {
                        addIkev2RoutesForInclusiveMode(connectStatus);
                    } else if (connectStatus.protocol == kCmdProtocolWireGuard) {
                        deleteWireGuardDefaultRoutes(connectStatus);
                        boundRoute_.create(connectStatus.vpnAdapter.adapterIp, connectStatus.vpnAdapter.adapterName);
                    }
                }
            }
        }
        addDnsRoutes(connectStatus);
    }

    connectStatus_ = connectStatus;
    isSplitTunnelActive_ = isSplitTunnelActive;
    isExcludeMode_ = isExcludeMode;
}

void RoutesManager::addDnsRoutes(const CMD_SEND_CONNECT_STATUS &connectStatus)
{
    // 10.255.255.0/24 contains the default DNS servers, on-node APIs, decoy traffic servers, etc
    // and always goes through the tunnel.
    dnsServersRoutes_.addWithInterface("10.255.255.0", connectStatus.vpnAdapter.adapterName, "24"); 

    // However the DNS server may fall outside the above range if configured with custom DNS after connecting.
    // Set a /32 for each DNS server to make sure they are routed correctly.
    for (auto it = connectStatus.vpnAdapter.dnsServers.begin(); it != connectStatus.vpnAdapter.dnsServers.end(); ++it) {
        dnsServersRoutes_.addWithInterface(*it, connectStatus.vpnAdapter.adapterName, "32");
    }
}

void RoutesManager::deleteOpenVpnDefaultRoutes(const CMD_SEND_CONNECT_STATUS &connectStatus)
{
    std::string cmd = "ip route del " + connectStatus.remoteIp + "/32";
    Logger::instance().out("execute: %s", cmd.c_str());
    Utils::executeCommand(cmd);

    cmd = "ip route del 0.0.0.0/1";
    Logger::instance().out("execute: %s", cmd.c_str());
    Utils::executeCommand(cmd);

    cmd = "route delete -net 128.0.0.0/1";
    Logger::instance().out("execute: %s", cmd.c_str());
    Utils::executeCommand(cmd);
}

void RoutesManager::deleteWireGuardDefaultRoutes(const CMD_SEND_CONNECT_STATUS &connectStatus)
{
    std::string cmd = "ip route del default dev " + connectStatus.vpnAdapter.adapterName + " table 51820";
    Logger::instance().out("execute: %s", cmd.c_str());
    Utils::executeCommand(cmd);
}

void RoutesManager::addIkev2RoutesForInclusiveMode(const CMD_SEND_CONNECT_STATUS &connectStatus)
{
    // add routes for override ikev2 default routes
    ikev2OverrideRoutes_.add("0.0.0.0", connectStatus.defaultAdapter.gatewayIp, "1");
    ikev2OverrideRoutes_.add("128.0.0.0", connectStatus.defaultAdapter.gatewayIp, "1");

    // add bound route
    boundRoute_.create(connectStatus.vpnAdapter.adapterIp, connectStatus.vpnAdapter.adapterName);
}

void RoutesManager::clearAllRoutes()
{
    dnsServersRoutes_.clear();
    ikev2OverrideRoutes_.clear();
    boundRoute_.remove();
}
