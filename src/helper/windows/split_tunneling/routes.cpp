#include <WinSock2.h>
#include <ws2ipdef.h>

// Must be included after above includes.
#include "routes.h"

#include <climits>
#include <cstring>
#include <map>
#include <spdlog/spdlog.h>

#include "ip_forward_table.h"

namespace {

// Fill a SOCKADDR_INET from types::IpAddress. Family is taken from ip.
void fillSockaddrInet(SOCKADDR_INET *out, const types::IpAddress &ip)
{
    std::memset(out, 0, sizeof(SOCKADDR_INET));
    if (ip.isV4()) {
        out->Ipv4.sin_family = AF_INET;
        std::memcpy(&out->Ipv4.sin_addr, ip.bytes(), 4);
    } else if (ip.isV6()) {
        out->Ipv6.sin6_family = AF_INET6;
        std::memcpy(&out->Ipv6.sin6_addr, ip.bytes(), 16);
    }
}

// Byte-compare the address portion of two SOCKADDR_INET values assumed to have the same family.
bool sameAddrBytes(const SOCKADDR_INET &a, const SOCKADDR_INET &b)
{
    if (a.si_family != b.si_family)
        return false;
    if (a.si_family == AF_INET)
        return std::memcmp(&a.Ipv4.sin_addr, &b.Ipv4.sin_addr, sizeof(a.Ipv4.sin_addr)) == 0;
    if (a.si_family == AF_INET6)
        return std::memcmp(&a.Ipv6.sin6_addr, &b.Ipv6.sin6_addr, sizeof(a.Ipv6.sin6_addr)) == 0;
    return false;
}

// Per-family interface metric with a small cache: the route table typically references
// only a handful of interfaces, and GetIpInterfaceEntry is a syscall per lookup.
// Returns 0 on failure — that only makes the caller's max-effective-metric estimate
// smaller for that row, and the row's own route metric is still counted.
ULONG getInterfaceMetric(ADDRESS_FAMILY family, NET_IFINDEX ifIndex,
                         std::map<NET_IFINDEX, ULONG> &cache)
{
    auto it = cache.find(ifIndex);
    if (it != cache.end())
        return it->second;

    MIB_IPINTERFACE_ROW interfaceRow;
    std::memset(&interfaceRow, 0, sizeof(interfaceRow));
    interfaceRow.Family = family;
    interfaceRow.InterfaceIndex = ifIndex;
    ULONG metric = 0;
    DWORD dwErr = GetIpInterfaceEntry(&interfaceRow);
    if (dwErr == NO_ERROR) {
        metric = interfaceRow.Metric;
    } else {
        spdlog::warn("Routes: GetIpInterfaceEntry failed for ifIndex={} (family={}): {}", ifIndex, family, dwErr);
    }
    cache[ifIndex] = metric;
    return metric;
}

// Route metric for a "last resort" route that must lose to every existing route of the
// same family. Windows selects among equal-prefix routes by the sum of the route metric
// and the owning interface's per-family metric, so taking max over bare route metrics
// (the old getMaxMetric()+1 approach) is not enough: wintun gets automatic interface
// metric 5, and 257 (= max route metric 256 from an RA default + 1) + 5 = 262 still
// undercuts a native RA default of 256 on a physical adapter with metric 20 (= 276),
// silently pulling all off-tunnel IPv6 into the tunnel in inclusive split tunneling.
// Compute the max *effective* metric across same-family routes instead, and derive a
// route metric that keeps our effective metric strictly above it.
ULONG calcLastResortMetric(const IpForwardTable &curRouteTable, ADDRESS_FAMILY family,
                           unsigned long ifIndex)
{
    std::map<NET_IFINDEX, ULONG> metricCache;
    ULONGLONG maxEffectiveMetric = 0;

    for (ULONG i = 0; i < curRouteTable.count(); ++i) {
        const MIB_IPFORWARD_ROW2 *row = curRouteTable.getByIndex(i);
        if (!row || row->DestinationPrefix.Prefix.si_family != family)
            continue;
        const ULONGLONG effective = static_cast<ULONGLONG>(row->Metric) +
                                    getInterfaceMetric(family, row->InterfaceIndex, metricCache);
        if (effective > maxEffectiveMetric)
            maxEffectiveMetric = effective;
    }

    const ULONG ownInterfaceMetric = getInterfaceMetric(family, ifIndex, metricCache);

    // If the lookup for our own interface failed (returned 0), the subtraction is skipped,
    // which only pushes the effective metric higher — still a losing route, as intended.
    ULONGLONG metric = maxEffectiveMetric + 1;
    metric = (metric > ownInterfaceMetric) ? (metric - ownInterfaceMetric) : 1;
    if (metric > ULONG_MAX)
        metric = ULONG_MAX;
    return static_cast<ULONG>(metric);
}

} // namespace


Routes::Routes()
{
}

void Routes::deleteRoute(const IpForwardTable &curRouteTable,
                         const types::IpAddress &destIp, UINT8 prefixLength,
                         const types::IpAddress &gatewayIp, unsigned long ifIndex)
{
    if (!destIp.isValid() || !gatewayIp.isValid() || destIp.family() != gatewayIp.family()) {
        spdlog::error("Routes::deleteRoute(): destIp/gatewayIp invalid or family mismatch");
        return;
    }

    SOCKADDR_INET destSockaddr;
    fillSockaddrInet(&destSockaddr, destIp);
    SOCKADDR_INET gatewaySockaddr;
    fillSockaddrInet(&gatewaySockaddr, gatewayIp);

    int deletedCount = 0;
    for (ULONG i = 0; i < curRouteTable.count(); ++i) {
        const MIB_IPFORWARD_ROW2 *row = curRouteTable.getByIndex(i);
        if (!row)
            continue;
        if (row->DestinationPrefix.Prefix.si_family != destSockaddr.si_family)
            continue;
        if (row->NextHop.si_family != gatewaySockaddr.si_family)
            continue;
        if (row->DestinationPrefix.PrefixLength != prefixLength)
            continue;
        if (row->InterfaceIndex != ifIndex)
            continue;
        if (!sameAddrBytes(row->DestinationPrefix.Prefix, destSockaddr))
            continue;
        if (!sameAddrBytes(row->NextHop, gatewaySockaddr))
            continue;

        MIB_IPFORWARD_ROW2 rowCopy = *row;
        DWORD dwErr = DeleteIpForwardEntry2(&rowCopy);
        if (dwErr == NO_ERROR) {
            deletedRoutes_.push_back(*row);
            deletedCount++;
        } else {
            spdlog::error("Routes::deleteRoute(), DeleteIpForwardEntry2 failed with error: {}", dwErr);
        }
    }

    // 0 matches means the expected tunnel route was not in the table (wrong NextHop
    // assumption, adapter info raced ahead of route configuration, third-party
    // interference) — route surgery silently did nothing, so make it loud.
    if (deletedCount == 0) {
        spdlog::warn("Routes::deleteRoute(): deleted 0 routes matching dest={}/{}, nextHop={}, ifIndex={}",
                     destIp.toString(), prefixLength, gatewayIp.toString(), ifIndex);
    } else {
        spdlog::info("Routes::deleteRoute(): deleted {} route(s) matching dest={}/{}, nextHop={}, ifIndex={}",
                     deletedCount, destIp.toString(), prefixLength, gatewayIp.toString(), ifIndex);
    }
}

void Routes::deleteRouteByInterface(const IpForwardTable &curRouteTable,
                                    const types::IpAddress &destIp, UINT8 prefixLength,
                                    unsigned long ifIndex)
{
    if (!destIp.isValid()) {
        spdlog::error("Routes::deleteRouteByInterface(): destIp invalid");
        return;
    }

    SOCKADDR_INET destSockaddr;
    fillSockaddrInet(&destSockaddr, destIp);

    int deletedCount = 0;
    for (ULONG i = 0; i < curRouteTable.count(); ++i) {
        const MIB_IPFORWARD_ROW2 *row = curRouteTable.getByIndex(i);
        if (!row)
            continue;
        if (row->DestinationPrefix.Prefix.si_family != destSockaddr.si_family)
            continue;
        if (row->DestinationPrefix.PrefixLength != prefixLength)
            continue;
        if (row->InterfaceIndex != ifIndex)
            continue;
        if (!sameAddrBytes(row->DestinationPrefix.Prefix, destSockaddr))
            continue;

        MIB_IPFORWARD_ROW2 rowCopy = *row;
        DWORD dwErr = DeleteIpForwardEntry2(&rowCopy);
        if (dwErr == NO_ERROR) {
            deletedRoutes_.push_back(*row);
            deletedCount++;
        } else {
            spdlog::error("Routes::deleteRouteByInterface(), DeleteIpForwardEntry2 failed with error: {}", dwErr);
        }
    }

    // Same rationale as in deleteRoute(): a silent no-op here is the main blind spot when
    // diagnosing "inclusive mode tunnels everything" reports.
    if (deletedCount == 0) {
        spdlog::warn("Routes::deleteRouteByInterface(): deleted 0 routes matching dest={}/{}, ifIndex={}",
                     destIp.toString(), prefixLength, ifIndex);
    } else {
        spdlog::info("Routes::deleteRouteByInterface(): deleted {} route(s) matching dest={}/{}, ifIndex={}",
                     deletedCount, destIp.toString(), prefixLength, ifIndex);
    }
}

void Routes::addRoute(const IpForwardTable &curRouteTable,
                      const types::IpAddress &destIp, UINT8 prefixLength,
                      const types::IpAddress &gatewayIp, unsigned long ifIndex,
                      bool isLastResort)
{
    if (!destIp.isValid() || !gatewayIp.isValid() || destIp.family() != gatewayIp.family()) {
        spdlog::error("Routes::addRoute(): destIp/gatewayIp invalid or family mismatch");
        return;
    }

    ULONG metric;
    if (isLastResort) {
        metric = calcLastResortMetric(curRouteTable, destIp.isV6() ? AF_INET6 : AF_INET, ifIndex);
    } else {
        MIB_IPINTERFACE_ROW interfaceRow;
        std::memset(&interfaceRow, 0, sizeof(interfaceRow));
        interfaceRow.Family = destIp.isV6() ? AF_INET6 : AF_INET;
        interfaceRow.InterfaceIndex = ifIndex;
        DWORD dwErr = GetIpInterfaceEntry(&interfaceRow);
        if (dwErr != NO_ERROR) {
            spdlog::error("Routes::addRoute(), GetIpInterfaceEntry failed with error: {}", dwErr);
            return;
        }
        metric = interfaceRow.Metric;
    }

    MIB_IPFORWARD_ROW2 row;
    InitializeIpForwardEntry(&row);
    row.InterfaceIndex = ifIndex;
    row.DestinationPrefix.PrefixLength = prefixLength;
    fillSockaddrInet(&row.DestinationPrefix.Prefix, destIp);
    fillSockaddrInet(&row.NextHop, gatewayIp);
    row.Protocol = MIB_IPPROTO_NETMGMT;
    row.Metric = metric;

    DWORD dwErr = CreateIpForwardEntry2(&row);
    if (dwErr == NO_ERROR) {
        addedRoutes_.push_back(row);
        spdlog::info("Routes::addRoute(): added dest={}/{}, nextHop={}, ifIndex={}, metric={}{}",
                     destIp.toString(), prefixLength, gatewayIp.toString(), ifIndex, metric,
                     isLastResort ? " (last resort)" : "");
    } else {
        spdlog::error("Routes::addRoute(), CreateIpForwardEntry2 failed with error: {}", dwErr);
    }
}

void Routes::revertRoutes()
{
    for (MIB_IPFORWARD_ROW2 &row : deletedRoutes_) {
        DWORD dwErr = CreateIpForwardEntry2(&row);
        if (dwErr != NO_ERROR)
            spdlog::error("Routes::revertRoutes(), CreateIpForwardEntry2 failed with error: {}", dwErr);
    }
    deletedRoutes_.clear();

    for (MIB_IPFORWARD_ROW2 &row : addedRoutes_) {
        DWORD dwErr = DeleteIpForwardEntry2(&row);
        if (dwErr != NO_ERROR) {
            if (dwErr == ERROR_NOT_FOUND) {
                spdlog::info("Routes::revertRoutes(), DeleteIpForwardEntry2 did not find interface index {}", row.InterfaceIndex);
            } else {
                spdlog::error("Routes::revertRoutes(), DeleteIpForwardEntry2 failed with error: {}", dwErr);
            }
        }
    }
    addedRoutes_.clear();
}
