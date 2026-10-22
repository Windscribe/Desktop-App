#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "types/ipaddress.h"

// Snapshot of the `main` routing table (both families), as reported by `ip -j route show table main`.
//
// Exclude mode uses it to decide whether an excluded destination needs a host pin at all. A pin is
// itself a `main` route, and it beats the VPN's catch-all only through mechanisms that already let
// ANY sufficiently specific `main` route win:
//   - WireGuard: `from all lookup main suppress_prefixlength 0` sits in front of the `not fwmark`
//     rule that sends everything else to the WG table;
//   - OpenVPN: its 0.0.0.0/1 + 128.0.0.0/1 live in `main`, so plain longest-prefix match applies;
//   - app split tunneling (cgroups-up): `lookup main suppress_prefixlength 1` at priority 16383.
// So a destination already covered by such a route (a secondary LAN, a static route, the primary's
// own subnet) leaves the way it would without the VPN when it is left alone, and pinning it to the
// default gateway (the old behaviour) is exactly what misroutes it.
class MainRouteTable
{
public:
    struct Route
    {
        types::IpAddressRange dst;
        // A `throw` route (see hasSpecificRoute()).
        bool isThrow = false;
        // Outgoing interface; for a multipath route, the first nexthop's. Empty when there is none
        // (blackhole, unreachable, …).
        std::string dev;
        // As printed by `ip -N`: the bare number ("2" for kernel, "16" for dhcp, …). `ip` omits it for
        // proto boot, which is then stored as "3".
        std::string protocol;
        // Kernel metric (rtm_priority). Absent in the dump means 0, which is also the metric of a
        // route installed without one. Lower wins.
        uint32_t metric = 0;
        // `flags` on the route, or on its first nexthop, contain linkdown or dead.
        bool isDown = false;
    };

    // An exact-prefix route we installed: proto ownProtocol, or a legacy pin (proto boot) on pinDevs.
    struct ExactPin
    {
        std::string dev;
        std::string protocol;
    };

    // Replaces the snapshot with the current `main` table of both families. Returns false if either
    // dump could not be obtained or parsed; the snapshot then holds whatever was read successfully,
    // and isLoaded() tells which family that was (e.g. only IPv4 on a host booted without IPv6).
    bool load();

    // True when the last load() read this family's table.
    bool isLoaded(types::IpAddress::Family family) const;

    // Appends the routes of one family's `ip -j route show` dump. Returns false on malformed input.
    bool parse(const std::string &json, types::IpAddress::Family family);

    // True when the route that wins for `dst` already sends it around the tunnel. The winner is chosen
    // the way the kernel chooses: longest prefix, then lowest metric; an exact tie goes to the tunnel.
    // Only /2 and longer compete — /0 and /1 are what the VPN catch-alls and the suppress_prefixlength
    // rules compete with. Our own pins (`ownProtocol`), a legacy pin (proto boot, same prefix, on
    // `pinDevs`) and a linkdown/dead route are not candidates: a pin is exactly as specific as the
    // destination it was installed for, so it would always win and hide the system route underneath.
    // A winner that is a `throw`, or that sits on one of `ignoredDevs` (the tunnel), still delivers
    // the lookup to the VPN, so the destination is not covered. Empty `pinDevs` skips the legacy-pin check.
    bool hasSpecificRoute(const types::IpAddressRange &dst,
                          const std::vector<std::string> &ignoredDevs,
                          const std::string &ownProtocol,
                          const std::vector<std::string> &pinDevs = {}) const;

    // True when the snapshot holds one of our own pins (`ownProtocol`) for exactly `dst`.
    bool hasOwnRoute(const types::IpAddressRange &dst, const std::string &ownProtocol) const;

    // Exact pins for `dst` (see ExactPin). Empty when the snapshot has none. A kernel connected
    // route is never one of these.
    std::vector<ExactPin> exactPins(const types::IpAddressRange &dst,
                                    const std::vector<std::string> &pinDevs,
                                    const std::string &ownProtocol) const;

private:
    std::vector<Route> routes_;
    bool loadedV4_ = false;
    bool loadedV6_ = false;
};
