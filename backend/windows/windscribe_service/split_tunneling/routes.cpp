#include "../all_headers.h"
#include "ip_forward_table.h"
#include "routes.h"
#include "../ip_address/ip4_address_and_mask.h"
#include "../logger.h"

Routes::Routes()
{
}

void Routes::deleteRoute(const IpForwardTable &curRouteTable, const std::string &destIp, const std::string &maskIp, const std::string &gatewayIp, unsigned long ifIndex)
{
    std::string log = "Routes::deleteRoute(), destIp=" + destIp + ", maskIp=" + maskIp + ", gatewayIp=" + gatewayIp;
    Logger::instance().out("%s", log.c_str());
    Ip4AddressAndMask dest(destIp.c_str());
    Ip4AddressAndMask mask(maskIp.c_str());
    Ip4AddressAndMask gateway(gatewayIp.c_str());
    for (DWORD i = 0; i < curRouteTable.count(); ++i)
    {
        const MIB_IPFORWARDROW *row = curRouteTable.getByIndex(i);
        if (row)
        {
            if (row->dwForwardDest == dest.ipNetworkOrder() && row->dwForwardMask == mask.ipNetworkOrder() && row->dwForwardNextHop == gateway.ipNetworkOrder()
                && row->dwForwardIfIndex == ifIndex)
            {
                MIB_IPFORWARDROW rowCopy = *row;
                DWORD dwErr = DeleteIpForwardEntry(&rowCopy);
                if (dwErr == NO_ERROR)
                {
                    deletedRoutes_.push_back(*row);
                }
                else
                {
                    Logger::instance().out("Routes::deleteRoute(), DeleteIpForwardEntry failed with error: %x", dwErr);
                }
            }
        }
    }

}
void Routes::addRoute(const IpForwardTable &curRouteTable, const std::string &destIp, const std::string &maskIp, const std::string &gatewayIp, unsigned long ifIndex, bool useMaxMetric)
{
    std::string log = "Routes::addRoute(), destIp=" + destIp + ", maskIp=" + maskIp + ", gatewayIp=" + gatewayIp;
    Logger::instance().out("%s", log.c_str());
    Ip4AddressAndMask dest(destIp.c_str());
    Ip4AddressAndMask mask(maskIp.c_str());
    Ip4AddressAndMask gateway(gatewayIp.c_str());

    ULONG metric;
    if (useMaxMetric)
    {
        metric = curRouteTable.getMaxMetric() + 1;
    }
    else
    {
        // get metric for interface with ifIndex
        MIB_IPINTERFACE_ROW interfaceRow;
        memset(&interfaceRow, 0, sizeof(interfaceRow));
        interfaceRow.Family = AF_INET;
        interfaceRow.InterfaceIndex = ifIndex;
        DWORD dwErr = GetIpInterfaceEntry(&interfaceRow);
        if (dwErr != NO_ERROR)
        {
            Logger::instance().out("Routes::addRoute(), GetIpInterfaceEntry failed with error: %x", dwErr);
            return;
        }
        metric = interfaceRow.Metric;
    }

    MIB_IPFORWARDROW row;
    memset(&row, 0, sizeof(row));
    row.dwForwardProto = MIB_IPPROTO_NETMGMT;
    row.dwForwardDest = dest.ipNetworkOrder();
    row.dwForwardMask = mask.ipNetworkOrder();
    row.dwForwardNextHop = gateway.ipNetworkOrder();
    row.dwForwardMetric1 = metric;

    row.dwForwardIfIndex = ifIndex;

    DWORD dwErr = CreateIpForwardEntry(&row);
    if (dwErr == NO_ERROR)
    {
        addedRoutes_.push_back(row);
    }
    else
    {
        Logger::instance().out("Routes::addRoute(), CreateIpForwardEntry failed with error: %x", dwErr);
    }
}

void Routes::revertRoutes()
{
    for (MIB_IPFORWARDROW &row : deletedRoutes_)
    {
        DWORD dwErr = CreateIpForwardEntry(&row);
        if (dwErr != NO_ERROR)
        {
            Logger::instance().out("Routes::revertRoutes(), CreateIpForwardEntry failed with error: %x", dwErr);
        }
    }
    deletedRoutes_.clear();

    for (MIB_IPFORWARDROW &row : addedRoutes_)
    {
        MIB_IPFORWARDROW delRow;
        memset(&delRow, 0, sizeof(delRow));
        delRow.dwForwardDest = row.dwForwardDest;
        delRow.dwForwardIfIndex = row.dwForwardIfIndex;
        delRow.dwForwardMask = row.dwForwardMask;
        delRow.dwForwardNextHop = row.dwForwardNextHop;
        delRow.dwForwardProto = row.dwForwardProto;

        DWORD dwErr = DeleteIpForwardEntry(&delRow);
        if (dwErr != NO_ERROR)
        {
            Logger::instance().out("Routes::revertRoutes(), DeleteIpForwardEntry failed with error: %x", dwErr);
        }
    }
    addedRoutes_.clear();
}