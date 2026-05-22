#include <WinSock2.h>
#include <ws2ipdef.h>

#include "ip_routes.h"

#include <cstring>
#include <set>
#include <spdlog/spdlog.h>

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

// Returns interface metric for the given family/ifIndex; 0 on error.
ULONG getInterfaceMetric(unsigned long ifIndex, ADDRESS_FAMILY family)
{
    MIB_IPINTERFACE_ROW interfaceRow;
    std::memset(&interfaceRow, 0, sizeof(interfaceRow));
    interfaceRow.Family = family;
    interfaceRow.InterfaceIndex = ifIndex;
    DWORD dwErr = GetIpInterfaceEntry(&interfaceRow);
    if (dwErr != NO_ERROR) {
        spdlog::error("IpRoutes: GetIpInterfaceEntry(family={}) failed with error: {}",
                      static_cast<int>(family), dwErr);
        return 0;
    }
    return interfaceRow.Metric;
}

} // namespace

IpRoutes::IpRoutes()
{
}

void IpRoutes::setIps(const types::IpAddress &gatewayIp,
                      const types::IpAddress &gatewayIpV6,
                      unsigned long ifIndex,
                      const std::vector<types::IpAddressRange> &ips)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    // Cache per-family metric — fetched lazily on first need.
    bool haveMetricV4 = false, haveMetricV6 = false;
    ULONG metricV4 = 0, metricV6 = 0;

    // Deduplicate input.
    std::set<types::IpAddressRange> ipsSet;
    for (const auto &ip : ips) {
        if (ip.isValid())
            ipsSet.insert(ip);
    }

    // Find active routes that are no longer requested — delete them.
    std::vector<types::IpAddressRange> ipsDelete;
    for (const auto &kv : activeRoutes_) {
        if (ipsSet.find(kv.first) == ipsSet.end())
            ipsDelete.push_back(kv.first);
    }
    for (const auto &ip : ipsDelete) {
        auto fr = activeRoutes_.find(ip);
        if (fr != activeRoutes_.end()) {
            DWORD status = DeleteIpForwardEntry2(&fr->second);
            if (status != NO_ERROR) {
                spdlog::error("IpRoutes::setIps(), DeleteIpForwardEntry2 failed: {}", status);
            }
            activeRoutes_.erase(fr);
        }
    }

    // Add routes for any new entries.
    for (const auto &ip : ipsSet) {
        if (activeRoutes_.find(ip) != activeRoutes_.end())
            continue;

        // Pick the right gateway for this destination's family.
        const types::IpAddress *gw = nullptr;
        ULONG metric = 0;
        if (ip.isV4()) {
            if (!gatewayIp.isValid() || !gatewayIp.isV4()) {
                spdlog::warn("IpRoutes::setIps(), no IPv4 gateway for v4 destination {}", ip.toString());
                continue;
            }
            gw = &gatewayIp;
            if (!haveMetricV4) {
                metricV4 = getInterfaceMetric(ifIndex, AF_INET);
                haveMetricV4 = true;
            }
            metric = metricV4;
        } else if (ip.isV6()) {
            if (!gatewayIpV6.isValid() || !gatewayIpV6.isV6()) {
                spdlog::warn("IpRoutes::setIps(), no IPv6 gateway for v6 destination {}", ip.toString());
                continue;
            }
            gw = &gatewayIpV6;
            if (!haveMetricV6) {
                metricV6 = getInterfaceMetric(ifIndex, AF_INET6);
                haveMetricV6 = true;
            }
            metric = metricV6;
        } else {
            continue;
        }

        MIB_IPFORWARD_ROW2 row;
        InitializeIpForwardEntry(&row);
        row.InterfaceIndex = ifIndex;
        row.DestinationPrefix.PrefixLength = ip.prefixLength();
        fillSockaddrInet(&row.DestinationPrefix.Prefix, ip.address());
        fillSockaddrInet(&row.NextHop, *gw);
        row.Protocol = MIB_IPPROTO_NETMGMT;
        row.Metric = metric;

        DWORD status = CreateIpForwardEntry2(&row);
        if (status != NO_ERROR) {
            spdlog::error("IpRoutes::setIps(), CreateIpForwardEntry2 failed: {}", status);
            continue;
        }
        activeRoutes_[ip] = row;
    }
}

void IpRoutes::clear()
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    for (auto &kv : activeRoutes_) {
        DWORD status = DeleteIpForwardEntry2(&kv.second);
        if (status != NO_ERROR) {
            spdlog::error("IpRoutes::clear(), DeleteIpForwardEntry2 failed: {}", status);
        }
    }
    activeRoutes_.clear();
}
