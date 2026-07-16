#include "ws_branding.h"
#include <windows.h>
#include <Fwpmu.h>

#include <spdlog/spdlog.h>

#include "ipv6_firewall.h"
#include "utils.h"
#include "adapters_info.h"

#define FIREWALL_SUBLAYER_IPV6_NAMEW WS_APP_IDENTIFIER_W L"IPV6FirewallSublayer"
#define UUID_LAYER_IPV6 L"8b03e426-56a7-4669-a985-706406675e68"

Ipv6Firewall::Ipv6Firewall(FwpmWrapper &fwpmWrapper): fwpmWrapper_(fwpmWrapper)
{
    UuidFromString((RPC_WSTR)UUID_LAYER_IPV6, &subLayerGUID_);
}

void Ipv6Firewall::release()
{
    enableIPv6();
}

void Ipv6Firewall::enableIPv6()
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    if (!isDisabled_)
        return;

    HANDLE hEngine = fwpmWrapper_.getHandleAndLock();
    fwpmWrapper_.beginTransaction();

    if (Utils::deleteSublayerAndAllFilters(hEngine, &subLayerGUID_)) {
        spdlog::debug("Ipv6Firewall::enableIPv6(), all filters deleted");
    } else {
        spdlog::error("Ipv6Firewall::enableIPv6(), failed delete filters");
    }
    whitelistFilterIds_.clear();

    fwpmWrapper_.endTransaction();
    fwpmWrapper_.unlock();
    isDisabled_ = false;
}

void Ipv6Firewall::disableIPv6(bool allowLanTraffic, bool isCustomConfig)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    // Remember for rebuilds triggered by setSplitTunnelingInclusive().
    allowLanTraffic_ = allowLanTraffic;
    isCustomConfig_ = isCustomConfig;

    HANDLE hEngine = fwpmWrapper_.getHandleAndLock();
    fwpmWrapper_.beginTransaction();

    // Rebuild the filters from scratch on every call, not just on the disabled->enabled edge:
    // a reconnect may land on a different tunnel adapter, and the Allow LAN setting may have
    // changed while connected. Bailing out when already disabled would keep stale tunnel
    // permits / stale LAN rules.
    if (!Utils::deleteSublayerAndAllFilters(hEngine, &subLayerGUID_)) {
        spdlog::error("Ipv6Firewall::disableIPv6(), failed to delete existing filters");
    }
    whitelistFilterIds_.clear();

    FWPM_SUBLAYER0 subLayer = { 0 };

    subLayer.subLayerKey = subLayerGUID_;
    subLayer.displayData.name = (wchar_t *)FIREWALL_SUBLAYER_IPV6_NAMEW;
    subLayer.displayData.description = (wchar_t *)FIREWALL_SUBLAYER_IPV6_NAMEW;
    subLayer.flags = FWPM_SUBLAYER_FLAG_PERSISTENT;
    subLayer.weight = 0xFFFF;

    DWORD dwFwAPiRetCode = FwpmSubLayerAdd0(hEngine, &subLayer, NULL);
    if (dwFwAPiRetCode != ERROR_SUCCESS && dwFwAPiRetCode != FWP_E_ALREADY_EXISTS) {
        spdlog::error("Ipv6Firewall::disableIPv6(), FwpmSubLayerAdd0 failed");
        fwpmWrapper_.abortTransaction();
        fwpmWrapper_.unlock();
        return;
    }

    addFilters(hEngine, allowLanTraffic, isCustomConfig);

    fwpmWrapper_.endTransaction();
    fwpmWrapper_.unlock();
    isDisabled_ = true;
}

void Ipv6Firewall::setSplitTunnelingState(SplitTunnelMode mode, const AppsIds &apps)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    if (mode == splitTunnelMode_ && apps == splitTunnelAppsIds_) {
        return;
    }

    spdlog::debug("Ipv6Firewall::setSplitTunnelingState, mode={}, apps={}", (int)mode, apps.count());
    splitTunnelMode_ = mode;
    splitTunnelAppsIds_ = apps;

    // If the filters are currently installed, rebuild them with the new state.
    if (isDisabled_) {
        disableIPv6(allowLanTraffic_, isCustomConfig_);
    }
}

void Ipv6Firewall::setSplitTunnelingWhitelistIps(const std::vector<types::IpAddressRange> &ips)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    if (ips == whitelistIps_) {
        return;
    }

    // Not installed yet — just hold the list; disableIPv6()/addFilters() will pick it up.
    if (!isDisabled_) {
        whitelistIps_ = ips;
        return;
    }

    // Incremental update: replace only the destination filters, keeping the rest of the
    // sublayer intact (this is called on every hostname resolution).
    // Snapshot for rollback: committing with only the removals applied would drop the
    // inclusive off-tunnel BLOCK and leak forced destinations' v6.
    const std::vector<types::IpAddressRange> prevIps = std::move(whitelistIps_);
    const std::vector<UINT64> prevFilterIds = whitelistFilterIds_;
    whitelistIps_ = ips;

    HANDLE hEngine = fwpmWrapper_.getHandleAndLock();
    fwpmWrapper_.beginTransaction();

    removeWhitelistFiltersLocked(hEngine);
    if (applyWhitelistFiltersLocked(hEngine)) {
        fwpmWrapper_.endTransaction();
    } else {
        fwpmWrapper_.abortTransaction();
        whitelistIps_ = prevIps;
        whitelistFilterIds_ = prevFilterIds;
    }
    fwpmWrapper_.unlock();
}

bool Ipv6Firewall::applyWhitelistFiltersLocked(HANDLE engineHandle)
{
    if (splitTunnelMode_ == SplitTunnelMode::Disabled || whitelistIps_.empty()) {
        return true;
    }

    // addFilterV6 skips non-v6 entries and no-ops if none remain, so the dual-stack list
    // is passed unfiltered. All ranges are packed into a single filter (OR'd conditions).
    bool ok;
    if (splitTunnelMode_ == SplitTunnelMode::Inclusive) {
        // BLOCK off-tunnel v6 to forced destinations (weight 3): it must fail so apps fall
        // back to v4, which the routes carry into the tunnel. On-tunnel traffic is permitted
        // by the weight-4 tunnel permit.
        ok = Utils::addFilterV6(engineHandle, &whitelistFilterIds_, FWP_ACTION_BLOCK, 3, subLayerGUID_,
                                FIREWALL_SUBLAYER_IPV6_NAMEW, nullptr, &whitelistIps_);
    } else {
        // Exclusive: PERMIT native v6 to excluded destinations for all apps. The callout
        // sublayer's hard BLOCK on the VPN LUID keeps them off the tunnel.
        ok = Utils::addFilterV6(engineHandle, &whitelistFilterIds_, FWP_ACTION_PERMIT, 2, subLayerGUID_,
                                FIREWALL_SUBLAYER_IPV6_NAMEW, nullptr, &whitelistIps_);
    }
    if (!ok) {
        spdlog::error("Ipv6Firewall::applyWhitelistFiltersLocked(), addFilterV6 failed (mode={})", (int)splitTunnelMode_);
    }
    return ok;
}

void Ipv6Firewall::removeWhitelistFiltersLocked(HANDLE engineHandle)
{
    for (UINT64 id : whitelistFilterIds_) {
        DWORD ret = FwpmFilterDeleteById0(engineHandle, id);
        if (ret != ERROR_SUCCESS) {
            spdlog::error("Ipv6Firewall::removeWhitelistFiltersLocked(), FwpmFilterDeleteById0 failed: {}", ret);
        }
    }
    whitelistFilterIds_.clear();
}

void Ipv6Firewall::addFilters(HANDLE engineHandle, bool allowLanTraffic, bool isCustomConfig)
{
    AdaptersInfo ai;
    std::vector<NET_IFINDEX> taps = ai.getTAPAdapters();

    // add block filter for all IPv6
    bool ret = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_BLOCK, 0, subLayerGUID_, FIREWALL_SUBLAYER_IPV6_NAMEW);
    if (!ret) {
        spdlog::error("Ipv6Firewall::addFilters(), FwpmFilterAdd0 failed");
    }

    // add permit filter IPv6 for TAP-adapters, but only when the tunnel actually carries a routable
    // (non-link-local) v6 address — mirrors the macOS/Linux hasV6Addr gate and the main FirewallFilter
    // sublayer, so v6 isn't permitted on a v4-only tunnel.
    for (std::vector<NET_IFINDEX>::iterator it = taps.begin(); it != taps.end(); ++it) {
        if (!ai.hasNonLinkLocalV6(*it)) {
            continue;
        }
        NET_LUID luid;
        ConvertInterfaceIndexToLuid(*it, &luid);

        ret = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_PERMIT, 0, subLayerGUID_, FIREWALL_SUBLAYER_IPV6_NAMEW, &luid);
        if (!ret) {
            spdlog::error("Ipv6Firewall::addFilters(), add permit filter failed");
        }
    }

    // add permit filter for ipv6 localhost address
    const std::vector<types::IpAddressRange> localhost = { types::IpAddressRange("::1/128") };
    ret = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_PERMIT, 4, subLayerGUID_, FIREWALL_SUBLAYER_IPV6_NAMEW, nullptr, &localhost);
    if (!ret) {
        spdlog::error("Could not add IPv6 localhost allow filter");
    }

    // Always permit link-local: mDNS/discovery responses arrive from fe80:: sources. FirewallFilter
    // already permits fe80::/10 in its sublayer, but the block-all above is a hard block
    // (FWPM_FILTER_FLAG_CLEAR_ACTION_RIGHT) and a hard block in any sublayer wins across sublayers
    // in WFP, so this sublayer must permit it too.
    const std::vector<types::IpAddressRange> linkLocal = { types::IpAddressRange("fe80::/10") };
    ret = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_PERMIT, 4, subLayerGUID_, FIREWALL_SUBLAYER_IPV6_NAMEW, nullptr, &linkLocal);
    if (!ret) {
        spdlog::error("Could not add IPv6 link-local allow filter");
    }

    // ICMPv6 NDP/MLD (types 130-137, 143): permit by type so neighbor/router discovery and
    // multicast-listener messages keep working even while IPv6 is otherwise blocked here. The
    // block-all above is a hard block (CLEAR_ACTION_RIGHT) that wins across sublayers, so these must
    // be permitted in this sublayer too. Weight 4, matching the other permits here, and mirroring
    // the main FirewallFilter sublayer.
    ret = Utils::addPermitFilterV6IcmpTypeRange(engineHandle, 130, 137, 4, subLayerGUID_, FIREWALL_SUBLAYER_IPV6_NAMEW);
    if (!ret) {
        spdlog::error("Could not add ICMPv6 NDP/MLD (130-137) allow filter (ipv6 sublayer)");
    }
    ret = Utils::addPermitFilterV6IcmpTypeRange(engineHandle, 143, 143, 4, subLayerGUID_, FIREWALL_SUBLAYER_IPV6_NAMEW);
    if (!ret) {
        spdlog::error("Could not add ICMPv6 MLDv2 (143) allow filter (ipv6 sublayer)");
    }

    // Permit LAN v6 (ULA + multicast) per the Allow LAN setting; custom configs need private
    // ranges allowed so third-party gateways/DNS work. No tunnel carve-out is needed here:
    // FirewallFilter's tunnel-scoped hard block for these ranges wins across sublayers.
    if (allowLanTraffic || isCustomConfig) {
        const std::vector<types::IpAddressRange> lanV6 = { types::IpAddressRange("fc00::/7"), types::IpAddressRange("ff00::/8") };
        ret = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_PERMIT, 4, subLayerGUID_, FIREWALL_SUBLAYER_IPV6_NAMEW, nullptr, &lanV6);
        if (!ret) {
            spdlog::error("Could not add IPv6 LAN traffic allow filter");
        }
    }

    // Inclusive split tunneling: non-included apps must keep native ISP IPv6 (v4 parity),
    // while tunnel-bound apps stay restricted to the tunnel.
    if (splitTunnelMode_ == SplitTunnelMode::Inclusive) {
        // Permit all v6 (weight 2, above the block-all) — restores native v6 for non-included apps.
        ret = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_PERMIT, 2, subLayerGUID_, FIREWALL_SUBLAYER_IPV6_NAMEW);
        if (!ret) {
            spdlog::error("Could not add IPv6 inclusive ST permit filter");
        }

        if (splitTunnelAppsIds_.count() > 0) {
            // Block tunnel-bound apps' v6 (weight 3): if the tunnel has no v6, their v6 must fail
            // so they fall back to v4, which the callout redirects into the tunnel. Loses to the
            // weight-4 permits above (localhost, link-local, NDP, LAN) and the tunnel permit below.
            ret = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_BLOCK, 3, subLayerGUID_, FIREWALL_SUBLAYER_IPV6_NAMEW,
                                     nullptr, nullptr, 0, 0, &splitTunnelAppsIds_);
            if (!ret) {
                spdlog::error("Could not add IPv6 inclusive ST apps block filter");
            }

            // Permit tunnel LUID at weight 4 so redirected apps' tunnel v6 beats the block above
            // (the base tunnel permit is at weight 0).
            for (std::vector<NET_IFINDEX>::iterator it = taps.begin(); it != taps.end(); ++it) {
                if (!ai.hasNonLinkLocalV6(*it)) {
                    continue;
                }
                NET_LUID luid;
                ConvertInterfaceIndexToLuid(*it, &luid);

                ret = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_PERMIT, 4, subLayerGUID_, FIREWALL_SUBLAYER_IPV6_NAMEW, &luid);
                if (!ret) {
                    spdlog::error("Could not add IPv6 inclusive ST tunnel permit filter");
                }
            }
        }
    }

    // Exclusive split tunneling: excluded apps use native ISP IPv6 (the callout redirects
    // them off-tunnel; the callout sublayer's hard blocks on the VPN LUID still prevent
    // their v6 from entering the tunnel when no redirect is possible).
    if (splitTunnelMode_ == SplitTunnelMode::Exclusive && splitTunnelAppsIds_.count() > 0) {
        ret = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_PERMIT, 2, subLayerGUID_, FIREWALL_SUBLAYER_IPV6_NAMEW,
                                 nullptr, nullptr, 0, 0, &splitTunnelAppsIds_);
        if (!ret) {
            spdlog::error("Could not add IPv6 exclusive ST apps permit filter");
        }
    }

    // Per-destination filters for the split tunneling IP/hostname list.
    applyWhitelistFiltersLocked(engineHandle);
}
