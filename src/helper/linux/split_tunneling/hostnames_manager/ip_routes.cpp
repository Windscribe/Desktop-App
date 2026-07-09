#include "ip_routes.h"

#include <set>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>
#include "../../utils.h"

namespace {

// Run `ip <args>` capturing stdout+stderr (trimmed of trailing newlines). Returns the
// process exit code and fills `*output`; logging is left to the caller so each operation
// can classify failures (e.g. a delete of an already-absent route) on its own terms.
int runIpCmd(const std::vector<std::string> &args, std::string *output)
{
    int rc = Utils::executeCommand("ip", args, output, /*appendFromStdErr=*/true);
    while (!output->empty() && (output->back() == '\n' || output->back() == '\r')) {
        output->pop_back();
    }
    return rc;
}

// True when `ip route del` failed because the route (or its device) is already gone.
// In that case the desired end-state — no such route — is already reached, so callers
// treat it as a successful delete rather than retrying a del that can never succeed.
bool isRouteAlreadyAbsent(const std::string &output)
{
    // v4/v6 missing route: "RTNETLINK answers: No such process".
    // Missing device:       "Cannot find device ...".
    return output.find("No such process") != std::string::npos
        || output.find("Cannot find") != std::string::npos;
}

// Build the argv + human-readable description for `ip route {add,del} …`.
//
// The kernel rejects v6 routes whose nexthop has different scope (link-local v6
// gateway from RA) without an explicit `dev`; it also rejects any `via <addr>`
// when `<addr>` is one of our own interface addresses (the WireGuard point-to-
// point hack puts the WG client IP into both adapterIp and gatewayIp). To handle
// both, callers tell us whether the gateway is "local" — when it is, we drop
// `via` and route by interface only.
std::pair<std::vector<std::string>, std::string>
buildRouteArgs(const std::string &op,
               const std::string &dst,
               const std::string &via,
               const std::string &iface,
               bool gatewayIsLocal)
{
    std::vector<std::string> args = {"route", op, dst};
    std::string desc = "ip route " + op + " " + dst;

    if (!gatewayIsLocal) {
        args.push_back("via");
        args.push_back(via);
        desc += " via " + via;
    }
    if (!iface.empty()) {
        args.push_back("dev");
        args.push_back(iface);
        desc += " dev " + iface;
    }
    return {std::move(args), std::move(desc)};
}

// True when the route must be installed `dev`-only (no `via`): either the gateway is one of
// our own interface addresses (the WireGuard point-to-point hack, which the kernel rejects as
// `via <local-addr>`), or there is no gateway at all but we have a real interface and local
// address (an on-link / point-to-point v6 default, e.g. PPP/cellular, which has no nexthop).
// The gateway-less case is v6-only: it exists for the "non-VPN IPv6" path, and getDefaultRoute
// (v4) reports an on-link default's gateway as 0.0.0.0 (a valid address), so a v4 destination
// with a genuinely absent gateway keeps its old skip-with-warning behavior.
bool useDevOnlyRoute(const types::IpAddress &gw, const types::IpAddress &adapterIp, const std::string &iface)
{
    if (!adapterIp.isValid()) {
        return false;
    }
    return gw == adapterIp || (!gw.isValid() && !iface.empty() && adapterIp.isV6());
}

} // namespace

void IpRoutes::setIps(const types::IpAddress &gatewayV4,
                      const types::IpAddress &gatewayV6,
                      const types::IpAddress &adapterIpV4,
                      const types::IpAddress &adapterIpV6,
                      const std::string &adapterName,
                      const std::string &adapterNameV6,
                      const std::vector<types::IpAddressRange> &ips)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    // Per-family interface name. The v6 default route can live on a different NIC than
    // the v4 default on a multi-homed host; an empty adapterNameV6 means "same as v4".
    const std::string &ifaceV4 = adapterName;
    const std::string &ifaceV6 = adapterNameV6.empty() ? adapterName : adapterNameV6;

    // Deduplicate input and drop any invalid entries up front.
    std::set<types::IpAddressRange> ipsSet;
    for (const auto &ip : ips) {
        if (ip.isValid()) {
            ipsSet.insert(ip);
        }
    }

    // Drop negative-cache entries for destinations no longer requested so the cache
    // doesn't accumulate across DNS changes; a re-requested destination will be retried.
    for (auto it = failedRoutes_.begin(); it != failedRoutes_.end();) {
        if (ipsSet.find(it->first) == ipsSet.end()) {
            it = failedRoutes_.erase(it);
        } else {
            ++it;
        }
    }

    // Find routes that are no longer wanted, or whose installed parameters changed
    // (gateway swap on adapter switch, interface rename, exclude<->inclusive flip
    // toggling the local-gateway form). All three need a delete-then-add cycle.
    std::set<types::IpAddressRange> ipsDelete;
    for (auto it = activeRoutes_.begin(); it != activeRoutes_.end(); ++it) {
        const auto &rd = it->second;
        const auto &gwForFamily = rd.ip.isV6() ? gatewayV6 : gatewayV4;
        const auto &adapterIpForFamily = rd.ip.isV6() ? adapterIpV6 : adapterIpV4;
        const auto &ifaceForFamily = rd.ip.isV6() ? ifaceV6 : ifaceV4;
        const bool gwIsLocalNow = useDevOnlyRoute(gwForFamily, adapterIpForFamily, ifaceForFamily);
        if (rd.defaultRouteIp != gwForFamily
            || rd.interfaceName != ifaceForFamily
            || rd.gatewayIsLocal != gwIsLocalNow
            || ipsSet.find(it->first) == ipsSet.end()) {
            ipsDelete.insert(it->first);
        }
    }

    for (auto it = ipsDelete.begin(); it != ipsDelete.end(); ++it) {
        auto fr = activeRoutes_.find(*it);
        if (fr == activeRoutes_.end()) {
            continue;
        }
        // Drop tracking only when the kernel route is actually gone. If `del` failed
        // but the destination is still wanted, the add loop below reinstalls it with
        // the new parameters via idempotent `ip route replace`, so the stale entry is
        // safe to erase. If `del` failed for a no-longer-wanted destination, keep the
        // entry so a later cycle (or clear()) retries the delete instead of orphaning
        // a live kernel route.
        const bool stillWanted = ipsSet.find(*it) != ipsSet.end();
        if (deleteRoute(fr->second) || stillWanted) {
            activeRoutes_.erase(fr);
        }
    }

    for (auto it = ipsSet.begin(); it != ipsSet.end(); ++it) {
        if (activeRoutes_.find(*it) != activeRoutes_.end()) {
            continue;
        }

        // Pick the gateway and interface by destination family.
        const auto &gw = it->isV6() ? gatewayV6 : gatewayV4;
        const auto &adapterIpForFamily = it->isV6() ? adapterIpV6 : adapterIpV4;
        const std::string &ifaceForFamily = it->isV6() ? ifaceV6 : ifaceV4;

        // WireGuard point-to-point (gateway == our own address — see addGatewayIp(ip) paired
        // with addAdapterIp(ip) in wireguardconnection_posix.cpp) and gateway-less on-link /
        // point-to-point defaults are both installed `dev`-only: the kernel refuses `via <addr>`
        // for the former and there is no nexthop for the latter.
        const bool gwIsLocal = useDevOnlyRoute(gw, adapterIpForFamily, ifaceForFamily);

        // Skip only when there is neither a usable gateway nor a dev-only fallback: e.g. a v6
        // destination on a v4-only host (no v6 gateway, no v6 address), or inclusive-mode
        // OpenVPN-on-Linux which is v4-only.
        if (!gw.isValid() && !gwIsLocal) {
            spdlog::warn("IpRoutes::setIps(), no {} gateway or interface for destination {}",
                         it->isV6() ? "IPv6" : "IPv4", it->toString());
            continue;
        }

        RouteDescr rd;
        rd.ip = *it;
        rd.defaultRouteIp = gw;
        rd.interfaceName = ifaceForFamily;
        rd.gatewayIsLocal = gwIsLocal;

        // Negative cache: if this exact route was already rejected by the kernel, don't
        // re-spawn `ip` (or re-log the same error) on this refresh. The retry only fires
        // once the parameters change (gateway swap, iface rename, local-gateway flip),
        // which produces a different RouteDescr and so misses the cache.
        auto failedIt = failedRoutes_.find(*it);
        if (failedIt != failedRoutes_.end() && sameRouteParams(failedIt->second, rd)) {
            continue;
        }

        // `addRoute` uses idempotent `ip route replace`, so success means the route is
        // installed regardless of whether it already existed in the kernel (e.g. a
        // host-pin left over by a crashed/restarted/upgraded helper, which would have
        // made plain `ip route add` fail with "File exists"). Recording on success
        // therefore also adopts such stale routes into tracking so teardown removes them.
        if (addRoute(rd)) {
            activeRoutes_[*it] = rd;
            failedRoutes_.erase(*it);
        } else {
            // Remember the rejected attempt (with the parameters tried) so the next
            // refresh suppresses the duplicate spawn + error log until something changes.
            failedRoutes_[*it] = rd;
        }
    }
}

bool IpRoutes::sameRouteParams(const RouteDescr &a, const RouteDescr &b)
{
    return a.defaultRouteIp == b.defaultRouteIp
        && a.interfaceName == b.interfaceName
        && a.gatewayIsLocal == b.gatewayIsLocal;
}

void IpRoutes::clear()
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    // The negative cache only tracks add attempts; nothing to tear down for it.
    failedRoutes_.clear();
    // Keep entries whose `del` failed (transient: iface already torn down, route mutated
    // by NetworkManager, …) so the next clear()/setIps() retries them instead of leaving
    // a live but untracked kernel route behind.
    for (auto it = activeRoutes_.begin(); it != activeRoutes_.end();) {
        if (deleteRoute(it->second)) {
            it = activeRoutes_.erase(it);
        } else {
            ++it;
        }
    }
}

bool IpRoutes::addRoute(const RouteDescr &rd)
{
    // `ip route replace` autodetects family from the destination literal, so the same
    // command shape works for v4 and v6 — provided we paired the right gateway. We use
    // `replace` rather than `add` so the operation is idempotent: a host-pin route that
    // survived a helper crash/restart/upgrade is overwritten in place instead of failing
    // with "File exists" and being left untracked (and thus never cleaned up).
    const std::string dst = rd.ip.toString();
    const std::string via = rd.defaultRouteIp.toString();
    auto cmd = buildRouteArgs("replace", dst, via, rd.interfaceName, rd.gatewayIsLocal);
    spdlog::info("cmd: {}", cmd.second);
    std::string output;
    int rc = runIpCmd(cmd.first, &output);
    if (rc != 0) {
        spdlog::error("{} failed (rc={}): {}", cmd.second, rc, output);
        return false;
    }
    return true;
}

bool IpRoutes::deleteRoute(const RouteDescr &rd)
{
    const std::string dst = rd.ip.toString();
    const std::string via = rd.defaultRouteIp.toString();
    auto cmd = buildRouteArgs("del", dst, via, rd.interfaceName, rd.gatewayIsLocal);
    spdlog::info("cmd: {}", cmd.second);
    std::string output;
    int rc = runIpCmd(cmd.first, &output);
    if (rc == 0) {
        return true;
    }
    if (isRouteAlreadyAbsent(output)) {
        // Route is already gone — desired state reached. Report success so the caller
        // drops tracking instead of retaining a "ghost" entry that would block a future
        // reinstall of the same destination.
        spdlog::debug("{} skipped: route already absent ({})", cmd.second, output);
        return true;
    }
    spdlog::error("{} failed (rc={}): {}", cmd.second, rc, output);
    return false;
}
