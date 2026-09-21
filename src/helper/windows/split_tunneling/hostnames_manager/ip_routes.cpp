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

// True when the row is directly attached (unspecified next hop).
bool isOnLink(const MIB_IPFORWARD_ROW2 &row)
{
    static const IN6_ADDR kZeroV6 = {};
    if (row.NextHop.si_family == AF_INET)
        return row.NextHop.Ipv4.sin_addr.S_un.S_addr == 0;
    if (row.NextHop.si_family == AF_INET6)
        return std::memcmp(&row.NextHop.Ipv6.sin6_addr, &kZeroV6, sizeof(kZeroV6)) == 0;
    return false;
}

bool isInterfaceConnected(unsigned long ifIndex, ADDRESS_FAMILY family)
{
    MIB_IPINTERFACE_ROW interfaceRow;
    std::memset(&interfaceRow, 0, sizeof(interfaceRow));
    interfaceRow.Family = family;
    interfaceRow.InterfaceIndex = ifIndex;
    return GetIpInterfaceEntry(&interfaceRow) == NO_ERROR && interfaceRow.Connected;
}

types::IpAddressRange toRange(const IP_ADDRESS_PREFIX &prefix)
{
    if (prefix.Prefix.si_family == AF_INET) {
        return types::IpAddressRange(
            types::IpAddress(types::IpAddress::IPv4,
                             reinterpret_cast<const uint8_t *>(&prefix.Prefix.Ipv4.sin_addr), 4),
            static_cast<uint8_t>(prefix.PrefixLength));
    }
    if (prefix.Prefix.si_family == AF_INET6) {
        return types::IpAddressRange(
            types::IpAddress(types::IpAddress::IPv6,
                             reinterpret_cast<const uint8_t *>(&prefix.Prefix.Ipv6.sin6_addr), 16),
            static_cast<uint8_t>(prefix.PrefixLength));
    }
    return types::IpAddressRange();
}

// Destinations the OS can already reach without a gateway over a live interface.
// Windows resolves equal-prefix routes by effective metric (RouteMetric + InterfaceMetric), and the
// exclusion rows below carry the physical interface metric in both terms, so they can outrank a
// directly attached subnet's on-link row (metric 256 + interface metric) and hijack its traffic.
std::set<types::IpAddressRange> collectOnLinkPrefixes()
{
    std::set<types::IpAddressRange> result;
    static const ADDRESS_FAMILY kFamilies[] = { AF_INET, AF_INET6 };
    for (const ADDRESS_FAMILY family : kFamilies) {
        PMIB_IPFORWARD_TABLE2 table = nullptr;
        if (GetIpForwardTable2(family, &table) != NO_ERROR || table == nullptr) {
            spdlog::warn("IpRoutes: GetIpForwardTable2(family={}) failed", static_cast<int>(family));
            continue;
        }
        for (ULONG i = 0; i < table->NumEntries; ++i) {
            const MIB_IPFORWARD_ROW2 &row = table->Table[i];
            if (!isOnLink(row) || !isInterfaceConnected(row.InterfaceIndex, family))
                continue;
            const auto dest = toRange(row.DestinationPrefix);
            if (dest.isValid())
                result.insert(dest);
        }
        FreeMibTable(table);
    }
    return result;
}

} // namespace

IpRoutes::IpRoutes()
{
}

void IpRoutes::setIps(const types::IpAddress &gatewayIp,
                      const types::IpAddress &gatewayIpV6,
                      unsigned long ifIndex,
                      const std::vector<types::IpAddressRange> &ips,
                      bool isExclude)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    // Fetched lazily: only exclude mode can collide with an existing on-link route, and only
    // when there is at least one new destination to install.
    bool haveOnLinkPrefixes = false;
    std::set<types::IpAddressRange> onLinkPrefixes;

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
                if (status == ERROR_NOT_FOUND) {
                    spdlog::info("IpRoutes::setIps(), DeleteIpForwardEntry2 did not find {}", ip.toString());
                } else {
                    spdlog::error("IpRoutes::setIps(), DeleteIpForwardEntry2 failed: {}", status);
                }
            }
            activeRoutes_.erase(fr);
        }
    }

    // Add routes for any new entries.
    for (const auto &ip : ipsSet) {
        if (activeRoutes_.find(ip) != activeRoutes_.end())
            continue;

        if (isExclude) {
            if (!haveOnLinkPrefixes) {
                onLinkPrefixes = collectOnLinkPrefixes();
                haveOnLinkPrefixes = true;
            }
            if (onLinkPrefixes.count(ip) != 0) {
                spdlog::info("IpRoutes::setIps(), {} is directly attached, keeping the existing on-link route", ip.toString());
                continue;
            }
        }

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
            if (status == ERROR_NOT_FOUND) {
                spdlog::info("IpRoutes::clear(), DeleteIpForwardEntry2 did not find {}", kv.first.toString());
            } else {
                spdlog::error("IpRoutes::clear(), DeleteIpForwardEntry2 failed: {}", status);
            }
        }
    }
    activeRoutes_.clear();
}
