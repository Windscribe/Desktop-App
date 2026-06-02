#pragma once

#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "types/ipaddress.h"

// Manages a set of `ip route add` / `ip route del` entries used for split-tunnel
// host pinning. Dual-stack: each route's family is dispatched from the destination's
// family — v4 destinations are routed via gatewayV4, v6 destinations via gatewayV6.
// Either gateway may be invalid; destinations of an unavailable family are skipped
// with a warning.
//
// The active adapter's own IP per family and its interface name are also passed
// in so the kernel actually accepts the route in the two pathological cases:
//   1. v6 LAN exclude: gateway is link-local (fe80::/10 from RA) and `via <fe80…>`
//      without `dev <iface>` is rejected ("Gateway has different scope.").
//   2. WireGuard inclusive: the engine layer feeds the WG client address as both
//      adapterIp and gatewayIp (point-to-point hack — see
//      wireguardconnection_posix.cpp). `via <our-own-addr>` is rejected by the
//      kernel ("Gateway can not be a local address."), even with `dev`. We detect
//      this via `gateway == adapterIp` and emit `dev <iface>` only — WG owns the
//      relevant AllowedIPs and routes the destination through the tunnel.
class IpRoutes
{
public:
    // adapterName / adapterNameV6 — interface used for `dev <iface>`, chosen per
    // destination family. On a multi-homed host the v6 default route may live on a
    // different NIC than the v4 default, so a v6 route built with the v4 `dev` (and a
    // link-local nexthop learned on the other NIC) is rejected by the kernel. Pass an
    // empty adapterNameV6 to reuse adapterName (single-homed and VPN-adapter cases).
    void setIps(const types::IpAddress &gatewayV4,
                const types::IpAddress &gatewayV6,
                const types::IpAddress &adapterIpV4,
                const types::IpAddress &adapterIpV6,
                const std::string &adapterName,
                const std::string &adapterNameV6,
                const std::vector<types::IpAddressRange> &ips);
    void clear();

private:
    std::recursive_mutex mutex_;

    struct RouteDescr
    {
        // Gateway the route was actually installed with, kept so we can detect a
        // gateway change between updates and reinstall affected routes.
        types::IpAddress defaultRouteIp;
        types::IpAddressRange ip;

        // Interface emitted as `dev <interfaceName>`. May be empty if the helper
        // was driven without adapter context (defensive — current call sites
        // always supply it via SplitTunneling::updateState()).
        std::string interfaceName;

        // True when the gateway equals one of our own interface addresses (the
        // WireGuard point-to-point case). In that mode we drop `via` and emit
        // `dev <iface>` only; otherwise we emit `via <gw> dev <iface>`.
        bool gatewayIsLocal = false;
    };

    // Keyed by destination range so v4/v6 entries don't collide on string equality
    // (e.g. `fd00::1` vs `fd00:0:0:0:0:0:0:1` would otherwise be tracked twice).
    std::map<types::IpAddressRange, RouteDescr> activeRoutes_;

    // Negative cache of destinations whose `add` the kernel rejected, with the exact
    // parameters that were tried. setIps() runs on every DNS-callback TTL refresh; a
    // persistently-rejected route would otherwise look "new" each pass and be re-spawned
    // (and re-logged) indefinitely. An entry suppresses re-attempts only while the route
    // parameters are unchanged — it is dropped when the parameters change (so the retry
    // happens) or when the destination is no longer requested.
    std::map<types::IpAddressRange, RouteDescr> failedRoutes_;

    // True when two route descriptors install the same kernel route (same gateway,
    // interface and local-gateway form). The destination is the map key, so it is not
    // re-compared here.
    static bool sameRouteParams(const RouteDescr &a, const RouteDescr &b);

    // Both log `cmd: ip route …` before invocation and an `error: ip route … failed
    // (rc=…): <stderr>` on non-zero exit, returning true only on success. Callers use
    // this to keep `activeRoutes_` in sync with the kernel: record a destination after
    // a successful add, and retain a tracked entry whose delete failed so a later cycle
    // retries it instead of orphaning a live route.
    bool addRoute(const RouteDescr &rd);
    bool deleteRoute(const RouteDescr &rd);
};
