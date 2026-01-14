#include <WinSock2.h>
#include <ws2ipdef.h>

#include "ip_routes.h"

#include <set>
#include <spdlog/spdlog.h>

IpRoutes::IpRoutes()
{
}

void IpRoutes::setIps(const std::string gatewayIp, unsigned long ifIndex, const std::vector<Ip4AddressAndMask> &ips)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    // get metric for interface with ifIndex
    MIB_IPINTERFACE_ROW interfaceRow;
    memset(&interfaceRow, 0, sizeof(interfaceRow));
    interfaceRow.Family = AF_INET;
    interfaceRow.InterfaceIndex = ifIndex;
    DWORD dwErr = GetIpInterfaceEntry(&interfaceRow);
    if (dwErr != NO_ERROR)
    {
        spdlog::error("IpRoutes::setIps(), GetIpInterfaceEntry failed with error: {}", dwErr);
        return;
    }


    // exclude duplicates
    std::set<Ip4AddressAndMask> ipsSet;
    for (auto ip = ips.begin(); ip != ips.end(); ++ip)
    {
        ipsSet.insert(*ip);
    }

    // find route which need delete
    std::set<Ip4AddressAndMask> ipsDelete;
    for (auto it = activeRoutes_.begin(); it != activeRoutes_.end(); ++it)
    {
        if (ipsSet.find(it->first) == ipsSet.end())
        {
            ipsDelete.insert(it->first);
        }
    }

    // delete routes
    for (auto ip = ipsDelete.begin(); ip != ipsDelete.end(); ++ip)
    {
        auto fr = activeRoutes_.find(*ip);
        if (fr != activeRoutes_.end())
        {
            DWORD status = DeleteIpForwardEntry(&fr->second);
            if (status != NO_ERROR)
            {
                spdlog::error("IpRoutes::enable(), DeleteIpForwardEntry failed: {}", status);
            }
            activeRoutes_.erase(fr);
        }
    }

    for (auto ip = ipsSet.begin(); ip != ipsSet.end(); ++ip)
    {
        auto ar = activeRoutes_.find(*ip);
        if (ar != activeRoutes_.end())
        {
            continue;
        }
        else
        {
            Ip4AddressAndMask gateway(gatewayIp.c_str());
            // add route
            MIB_IPFORWARDROW row;
            memset(&row, 0, sizeof(row));
            row.dwForwardProto = MIB_IPPROTO_NETMGMT;
            row.dwForwardDest = ip->ipNetworkOrder();
            row.dwForwardMask = ip->maskNetworkOrder();
            row.dwForwardNextHop = gateway.ipNetworkOrder();
            row.dwForwardMetric1 = interfaceRow.Metric;
            row.dwForwardIfIndex = ifIndex;

            DWORD status = CreateIpForwardEntry(&row);
            if (status != NO_ERROR)
            {
                spdlog::error("IpRoutes::enable(), CreateIpForwardEntry failed: {}", status);
            }

            activeRoutes_[*ip] = row;
        }
    }
}

void IpRoutes::clear()
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    for (auto it = activeRoutes_.begin(); it != activeRoutes_.end(); ++it)
    {
        DWORD status = DeleteIpForwardEntry(&it->second);
        if (status != NO_ERROR)
        {
            spdlog::error("IpRoutes::enable(), DeleteIpForwardEntry failed: {}", status);
        }
    }
    activeRoutes_.clear();
}
