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
    if (!isDisabled_)
        return;

    HANDLE hEngine = fwpmWrapper_.getHandleAndLock();
    fwpmWrapper_.beginTransaction();

    if (Utils::deleteSublayerAndAllFilters(hEngine, &subLayerGUID_)) {
        spdlog::debug("Ipv6Firewall::enableIPv6(), all filters deleted");
    } else {
        spdlog::error("Ipv6Firewall::enableIPv6(), failed delete filters");
    }

    fwpmWrapper_.endTransaction();
    fwpmWrapper_.unlock();
    isDisabled_ = false;
}

void Ipv6Firewall::disableIPv6(bool allowLanTraffic, bool isCustomConfig)
{
    HANDLE hEngine = fwpmWrapper_.getHandleAndLock();
    fwpmWrapper_.beginTransaction();

    // Rebuild the filters from scratch on every call, not just on the disabled->enabled edge:
    // a reconnect may land on a different tunnel adapter, and the Allow LAN setting may have
    // changed while connected. Bailing out when already disabled would keep stale tunnel
    // permits / stale LAN rules.
    if (!Utils::deleteSublayerAndAllFilters(hEngine, &subLayerGUID_)) {
        spdlog::error("Ipv6Firewall::disableIPv6(), failed to delete existing filters");
    }

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
}
