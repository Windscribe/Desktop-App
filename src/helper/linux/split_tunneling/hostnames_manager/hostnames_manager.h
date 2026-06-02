#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "dns_resolver.h"
#include "ip_routes.h"
#include "types/ipaddress.h"

class HostnamesManager
{
public:
    explicit HostnamesManager();
    ~HostnamesManager();

    // gatewayIp / gatewayIpV6 — gateways for the active "outside" adapter (either may be
    // invalid). For WireGuard inclusive mode the caller passes the WG adapter's own
    // address as the gateway, mirroring the existing v4 convention.
    //
    // adapterIp / adapterIpV6 — the same adapter's own addresses; used by IpRoutes
    // to detect the WireGuard point-to-point case (gateway == own address) where
    // the kernel refuses `via <addr>` and the route must be installed with `dev`
    // only. Either may be invalid.
    //
    // adapterName — interface used for `dev <iface>`; required so that v6 routes
    // with a link-local gateway (LAN exclude mode) and dev-only routes (WG
    // inclusive mode) can actually be added.
    //
    // adapterNameV6 — interface for v6 routes when the v6 default route lives on a
    // different NIC than the v4 default (multi-homed host). Empty means "same as
    // adapterName"; IpRoutes falls back accordingly so single-homed/VPN cases are
    // unchanged.
    void enable(const types::IpAddress &gatewayIp,
                const types::IpAddress &gatewayIpV6,
                const types::IpAddress &adapterIp,
                const types::IpAddress &adapterIpV6,
                const std::string &adapterName,
                const std::string &adapterNameV6);
    void disable();

    // Dual-stack: ips holds IPv4 and/or IPv6 ranges; family is dispatched by isV4()/isV6().
    void setSettings(const std::vector<types::IpAddressRange> &ips,
                     const std::vector<std::string> &hosts);

private:
    DnsResolver dnsResolver_;
    IpRoutes ipRoutes_;

    bool isEnabled_;
    std::recursive_mutex mutex_;

    // Latest IPs (dual-stack) and hosts list configured via setSettings().
    std::vector<types::IpAddressRange> ipsLatest_;
    std::vector<std::string> hostsLatest_;

    types::IpAddress gatewayIp_;
    types::IpAddress gatewayIpV6_;
    types::IpAddress adapterIp_;
    types::IpAddress adapterIpV6_;
    std::string adapterName_;
    std::string adapterNameV6_;

    void dnsResolverCallback(std::map<std::string, DnsResolver::HostInfo> hostInfos);
};
