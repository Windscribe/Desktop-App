#include "routes_manager.h"
#include "../logger.h"
#include "../utils.h"

RoutesManager::RoutesManager()
{
    connectStatus_.isConnected = false;
}

void RoutesManager::updateState(const CMD_SEND_CONNECT_STATUS &connectStatus)
{
    bool prevIsConnected = connectStatus_.isConnected;

    if (prevIsConnected)
        clearAllRoutes();
    if (connectStatus.isConnected)
        addDnsRoutes(connectStatus);

    connectStatus_ = connectStatus;
}

void RoutesManager::addDnsRoutes(const CMD_SEND_CONNECT_STATUS &connectStatus)
{
    // 10.255.255.0/24 contains the default DNS servers, on-node APIs, decoy traffic servers, etc
    // and always goes through the tunnel.
    dnsServersRoutes_.addWithInterface("10.255.255.0", connectStatus.vpnAdapter.adapterName, "24");

    // However the DNS server may fall outside the above range if configured with custom DNS after connecting.
    // Set a /32 for each DNS server to make sure they are routed correctly.
    for (auto it = connectStatus.vpnAdapter.dnsServers.begin(); it != connectStatus.vpnAdapter.dnsServers.end(); ++it)
        dnsServersRoutes_.addWithInterface(*it, connectStatus.vpnAdapter.adapterName, "32");
}

void RoutesManager::clearAllRoutes()
{
    dnsServersRoutes_.clear();
}
