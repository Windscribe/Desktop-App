#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

// Decides whether FirewallFilter::on can update the installed filter set in place, or must delete
// and rebuild it. Pure (no WFP calls) so it can be unit-tested.
//
// Only two groups of filters are ever updated in place: the connecting-IP permit and the per-IP
// permits for the whitelist. Everything else (the per-VPN-adapter filters, the LAN and Allow LAN
// rules, the block-all filters) depends on the adapter snapshot and the two flags, so any change
// there forces a full rebuild. The split tunneling permits are kept in sync by their own setters
// and are not part of this decision.
namespace firewall_incremental
{

// What addFilters derives the filters for one VPN adapter from.
struct TapSnapshot
{
    unsigned long ifIndex = 0;
    uint64_t luid = 0;
    std::vector<std::string> localAddrs;
    bool hasNonLinkLocalV6 = false;

    bool operator==(const TapSnapshot &other) const
    {
        return ifIndex == other.ifIndex && luid == other.luid && localAddrs == other.localAddrs &&
               hasNonLinkLocalV6 == other.hasNonLinkLocalV6;
    }
    bool operator!=(const TapSnapshot &other) const { return !(*this == other); }
};

// The inputs of one FirewallFilter::on call.
struct Inputs
{
    std::wstring connectingIp;
    std::set<std::wstring> ips;   // whitelisted IPs/CIDRs, deduplicated
    bool allowLocalTraffic = false;
    bool isCustomConfig = false;
    std::vector<TapSnapshot> taps;
};

// What the installed filter set was built from, with the IDs of the filters that can be replaced.
struct BuiltState
{
    bool allowLocalTraffic = false;
    bool isCustomConfig = false;
    std::vector<TapSnapshot> taps;
    std::wstring connectingIp;
    std::vector<uint64_t> connectingIpFilterIds;
    // Set when an input of the connecting-IP filter other than the IP (the VPN app IDs) changed
    // after it was built, so the next update must replace it even if the IP is the same.
    bool connectingIpFilterStale = false;
    std::map<std::wstring, std::vector<uint64_t>> ipFilterIds;
};

struct Plan
{
    bool fullRebuild = true;
    std::string reason;   // why a full rebuild is needed; empty otherwise
    bool replaceConnectingIp = false;
    std::vector<std::wstring> ipsToAdd;
    std::vector<std::wstring> ipsToRemove;

    bool isNoOp() const { return !fullRebuild && !replaceConnectingIp && ipsToAdd.empty() && ipsToRemove.empty(); }
};

inline Plan fullRebuild(std::string reason)
{
    Plan plan;
    plan.fullRebuild = true;
    plan.reason = std::move(reason);
    return plan;
}

// `built` is empty when no build from this process is known to be installed (helper start, after
// the firewall was turned off, or after a failed update).
inline Plan planIncremental(const std::optional<BuiltState> &built, const Inputs &in)
{
    if (!built) {
        return fullRebuild("no cached build");
    }
    if (built->allowLocalTraffic != in.allowLocalTraffic) {
        return fullRebuild("Allow LAN traffic changed");
    }
    if (built->isCustomConfig != in.isCustomConfig) {
        return fullRebuild("custom config changed");
    }
    if (built->taps != in.taps) {
        return fullRebuild("VPN adapters changed");
    }

    Plan plan;
    plan.fullRebuild = false;
    plan.replaceConnectingIp = built->connectingIp != in.connectingIp || built->connectingIpFilterStale;
    for (const std::wstring &ip : in.ips) {
        if (built->ipFilterIds.find(ip) == built->ipFilterIds.end()) {
            plan.ipsToAdd.push_back(ip);
        }
    }
    for (const auto &entry : built->ipFilterIds) {
        if (in.ips.find(entry.first) == in.ips.end()) {
            plan.ipsToRemove.push_back(entry.first);
        }
    }
    return plan;
}

} // namespace firewall_incremental
