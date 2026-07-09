#include <Windows.h>
#include <Fwpmu.h>

#include <spdlog/spdlog.h>

#include "firewallfilter.h"
#include "utils.h"
#include "adapters_info.h"


FirewallFilter::FirewallFilter(FwpmWrapper &fwpmWrapper) :
    fwpmWrapper_(fwpmWrapper), lastAllowLocalTraffic_(false), isSplitTunnelingEnabled_(false),
    isSplitTunnelingExclusiveMode_(false)
{
    UuidFromString((RPC_WSTR)UUID_LAYER, &subLayerGUID_);
}

void FirewallFilter::release()
{
}

void FirewallFilter::on(const wchar_t *connectingIp, const std::vector<std::wstring> &ips, bool bAllowLocalTraffic, bool bIsCustomConfig)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    HANDLE hEngine = fwpmWrapper_.getHandleAndLock();

    fwpmWrapper_.beginTransaction();

    if (currentStatusImpl(hEngine) == true) {
        offImpl(hEngine);
    }

    FWPM_SUBLAYER0 subLayer = {0};
    subLayer.subLayerKey = subLayerGUID_;
    subLayer.displayData.name = (wchar_t *)FIREWALL_SUBLAYER_NAMEW;
    subLayer.displayData.description = (wchar_t *)FIREWALL_SUBLAYER_NAMEW;
    subLayer.flags = FWPM_SUBLAYER_FLAG_PERSISTENT;
    subLayer.weight = 0xFFFF;

    DWORD dwRet = FwpmSubLayerAdd0(hEngine, &subLayer, NULL );
    if (dwRet == ERROR_SUCCESS) {
        addFilters(hEngine, connectingIp, ips, bAllowLocalTraffic, bIsCustomConfig);
    } else {
        spdlog::error("FirewallFilter::on(), FwpmSubLayerAdd0 failed");
    }

    fwpmWrapper_.endTransaction();
    fwpmWrapper_.unlock();
}

void FirewallFilter::off()
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    HANDLE hEngine = fwpmWrapper_.getHandleAndLock();
    fwpmWrapper_.beginTransaction();
    offImpl(hEngine);
    fwpmWrapper_.endTransaction();
    fwpmWrapper_.unlock();
}

bool FirewallFilter::currentStatus()
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    HANDLE hEngine = fwpmWrapper_.getHandleAndLock();
    bool bRet = currentStatusImpl(hEngine);
    fwpmWrapper_.unlock();
    return bRet;
}

bool FirewallFilter::currentStatusImpl(HANDLE engineHandle)
{
    FWPM_SUBLAYER0 *subLayer;
    DWORD dwRet = FwpmSubLayerGetByKey0(engineHandle, &subLayerGUID_, &subLayer);
    return dwRet == ERROR_SUCCESS;
}

void FirewallFilter::offImpl(HANDLE engineHandle)
{
    if (Utils::deleteSublayerAndAllFilters(engineHandle, &subLayerGUID_)) {
        spdlog::info("FirewallFilter::offImpl(), all filters deleted");
    } else {
        spdlog::info("FirewallFilter::offImpl(), failed delete filters");
    }
    filterIdsApps_.clear();
    filterIdsSplitRoutingIps_.clear();
}

void FirewallFilter::setSplitTunnelingEnabled(bool isExclusiveMode)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    HANDLE hEngine = fwpmWrapper_.getHandleAndLock();
    fwpmWrapper_.beginTransaction();

    removeAllSplitTunnelingFilters(hEngine);
    isSplitTunnelingEnabled_ = true;
    isSplitTunnelingExclusiveMode_ = isExclusiveMode;

    if (currentStatusImpl(hEngine) == true) {
        addPermitFilterForAppsIds(hEngine);
        addPermitFilterForSplitRoutingWhitelistIps(hEngine);
    }

    fwpmWrapper_.endTransaction();
    fwpmWrapper_.unlock();
}

void FirewallFilter::setSplitTunnelingDisabled()
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    HANDLE hEngine = fwpmWrapper_.getHandleAndLock();
    fwpmWrapper_.beginTransaction();
    removeAllSplitTunnelingFilters(hEngine);
    fwpmWrapper_.endTransaction();
    fwpmWrapper_.unlock();

    isSplitTunnelingEnabled_ = false;
}

void FirewallFilter::setVpnAppsIds(const AppsIds &appsIds)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    if (vpnAppsIds_ == appsIds) {
        return;
    }
    vpnAppsIds_ = appsIds;

    HANDLE hEngine = fwpmWrapper_.getHandleAndLock();

    // if split tunneling is ON and firewall is ON then add filter for apps immediately
    if (isSplitTunnelingEnabled_ && currentStatusImpl(hEngine) == true) {
        fwpmWrapper_.beginTransaction();
        removeAppsSplitTunnelingFilter(hEngine);
        addPermitFilterForAppsIds(hEngine);
        fwpmWrapper_.endTransaction();
    }

    fwpmWrapper_.unlock();
}

void FirewallFilter::setSplitTunnelingAppsIds(const AppsIds &appsIds)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    if (appsIds_ == appsIds) {
        return;
    }
    appsIds_ = appsIds;

    HANDLE hEngine = fwpmWrapper_.getHandleAndLock();

    // if split tunneling is ON and firewall is ON then add filter for apps immediately
    if (isSplitTunnelingEnabled_ && currentStatusImpl(hEngine) == true) {
        fwpmWrapper_.beginTransaction();
        removeAppsSplitTunnelingFilter(hEngine);
        addPermitFilterForAppsIds(hEngine);
        fwpmWrapper_.endTransaction();
    }

    fwpmWrapper_.unlock();
}

void FirewallFilter::setSplitTunnelingWhitelistIps(const std::vector<types::IpAddressRange> &ips)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    if (splitRoutingIps_ == ips) {
        return;
    }
    splitRoutingIps_ = ips;

    HANDLE hEngine = fwpmWrapper_.getHandleAndLock();

    // if split tunneling is ON and firewall is ON then add filter for ips immediately
    if (isSplitTunnelingEnabled_ && currentStatusImpl(hEngine) == true) {
        fwpmWrapper_.beginTransaction();
        removeIpsSplitTunnelingFilter(hEngine);
        addPermitFilterForSplitRoutingWhitelistIps(hEngine);
        fwpmWrapper_.endTransaction();
    }
    fwpmWrapper_.unlock();
}

void FirewallFilter::addFilters(HANDLE engineHandle, const wchar_t *connectingIp, const std::vector<std::wstring> &ips, bool bAllowLocalTraffic, bool bIsCustomConfig)
{
    AdaptersInfo ai;
    std::vector<NET_IFINDEX> taps = ai.getTAPAdapters();

    // add block filter for all IPs, for all adapters, IPv4.
    bool ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_BLOCK, 0, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW);
    if (!ret) {
        spdlog::error("Could not add IPv4 block filter");
    }
    // add block filter for all IPs, for all adapters, IPv6.
    ret = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_BLOCK, 0, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW);
    if (!ret) {
        spdlog::error("Could not add IPv6 block filter");
    }

    // The per-interface (tunnel-scoped) filters in this loop are added with persistent=false: they
    // are runtime-only and must NOT survive into the firewall-on-boot persisted ruleset, which should
    // be the curated minimal set (default block + server IPs + DHCP + loopback + link-local + NDP/MLD
    // + LAN carve-outs + the reserved-DNS block, all left persistent). The connecting-IP and
    // split-tunnel permits below are non-persistent for the same reason.
    // add permit filter for TAP-adapters
    for (std::vector<NET_IFINDEX>::iterator it = taps.begin(); it != taps.end(); ++it) {
        NET_LUID luid;
        ConvertInterfaceIndexToLuid(*it, &luid);

        // Always allow 10.255.255.0/24 (Windscribe reserved) regardless of custom config,
        // because the Allow LAN flag later blocks the entire 10/8 range
        const std::vector<types::IpAddressRange> reserved = { types::IpAddressRange("10.255.255.0/24") };
        ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_PERMIT, 8, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, &luid, &reserved, 0, 0, nullptr, false);
        if (!ret) {
            spdlog::error(L"Could not add reserved allow filter on VPN interface");
        }

        if (!bIsCustomConfig) {
            // We want to allow access to local VPN interface addresses regardless of which interface the packet will go through, not just a VPN interface
            const std::vector<types::IpAddressRange> localAddrs = types::IpAddressRange::fromStrings(ai.getAdapterAddresses(*it));
            if (!localAddrs.empty()) {
                ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_PERMIT, 8, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &localAddrs, 0, 0, nullptr, false);
                if (!ret) {
                    spdlog::error("Could not add reserved allow filter on private address for VPN interface");
                }
            }
            // Carve-out (LAN-into-tunnel blocks): only needed when "Allow LAN traffic" is on. With
            // LAN off, the weight-4 global LAN block already outranks the weight-1 tunnel permit, so
            // these weight-6 blocks would be redundant. Gated on bAllowLocalTraffic to match the
            // macOS/Linux runtime rulesets and avoid emitting dead rules.
            if (bAllowLocalTraffic) {
                // Disallow all other private, link-local, loopback networks from going over tunnel
                const std::vector<types::IpAddressRange> priv = types::IpAddressRange::fromStrings(
                    {"10.0.0.0/8", "172.16.0.0/12", "192.168.0.0/16", "169.254.0.0/16", "224.0.0.0/4"});
                ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_BLOCK, 6, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, &luid, &priv, 0, 0, nullptr, false);
                if (!ret) {
                    spdlog::error("Could not add private network block filter on VPN interface");
                }

                // Same carve-out for IPv6: LAN-destined ULA/multicast must not enter the (dual-stack)
                // tunnel. Weight 6 outweighs the unscoped weight-4 LAN permits and the weight-1 tunnel
                // permit, so these ranges are dropped on the tunnel but still pass on the physical NIC.
                const std::vector<types::IpAddressRange> privV6 = { types::IpAddressRange("fc00::/7"), types::IpAddressRange("ff00::/8") };
                ret = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_BLOCK, 6, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, &luid, &privV6, 0, 0, nullptr, false);
                if (!ret) {
                    spdlog::error("Could not add private network IPv6 block filter on VPN interface");
                }
            }
        }

        // Allow other IPv4 traffic on this interface
        ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_PERMIT, 1, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, &luid, nullptr, 0, 0, nullptr, false);
        if (!ret) {
            spdlog::error("Could not add IPv4 allow filter on VPN interface");
        }
        // Allow IPv6 traffic on this interface, but only when the tunnel actually carries a routable
        // (non-link-local) v6 address — mirrors the macOS/Linux hasV6Addr gate (probeInterfaceAddresses)
        // so v6 isn't blanket-permitted on a v4-only tunnel.
        if (ai.hasNonLinkLocalV6(*it)) {
            ret = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_PERMIT, 1, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, &luid, nullptr, 0, 0, nullptr, false);
            if (!ret) {
                spdlog::error("Could not add IPv6 allow filter on VPN interface");
            }
        }
    }

    // add permit filter for split tunneling app ids and ips
    if (isSplitTunnelingEnabled_) {
        addPermitFilterForAppsIds(engineHandle);
        addPermitFilterForSplitRoutingWhitelistIps(engineHandle);
    }

    addPermitFilterForVpnAndSystemServices(engineHandle, connectingIp);

    // add permit filter for specific IPs
    for (size_t i = 0; i < ips.size(); ++i) {
        const std::vector<types::IpAddressRange> ipAddr = { types::IpAddressRange(ips[i]) };
        ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_PERMIT, 2, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &ipAddr);
        if (!ret) {
            spdlog::error("Could not add " WS_PRODUCT_NAME " IPs allow filter");
        }
    }

    // add permit filter for DHCP.
    // Weight must be higher than the LAN block below (weight 4): DHCP lease renewal is unicast to
    // the DHCP server, which usually lives in a private range. With a lower weight the veto LAN
    // block would override this permit when "Allow LAN" is off, breaking renewal and dropping Wi-Fi.
    ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_PERMIT, 5, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, nullptr, 68);
    if (!ret) {
        spdlog::error("Could not add DHCP allow filter (68)");
    }
    // add permit filter for DHCP
    ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_PERMIT, 5, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, nullptr, 67);
    if (!ret) {
        spdlog::error("Could not add DHCP allow filter (67)");
    }

    // DHCPv6 (client port 546, server port 547)
    ret = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_PERMIT, 5, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, nullptr, 546);
    if (!ret) {
        spdlog::error("Could not add DHCPv6 allow filter (546)");
    }
    ret = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_PERMIT, 5, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, nullptr, 547);
    if (!ret) {
        spdlog::error("Could not add DHCPv6 allow filter (547)");
    }

    // Always allow localhost
    const std::vector<types::IpAddressRange> localhostV4 = { types::IpAddressRange("127.0.0.0/8") };
    ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_PERMIT, 8, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &localhostV4);
    if (!ret) {
        spdlog::error("Could not add localhost v4 allow filter");
    }
    const std::vector<types::IpAddressRange> localhostV6 = { types::IpAddressRange("::1/128") };
    ret = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_PERMIT, 8, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &localhostV6);
    if (!ret) {
        spdlog::error("Could not add localhost v6 allow filter.");
    }

    // Always allow IPv6 link-local (required for neighbor discovery, Apple Continuity, etc.)
    const std::vector<types::IpAddressRange> linkLocalV6 = { types::IpAddressRange("fe80::/10") };
    ret = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_PERMIT, 8, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &linkLocalV6);
    if (!ret) {
        spdlog::error("Could not add IPv6 link-local allow filter.");
    }

    // ICMPv6 NDP/MLD (types 130-137, 143): always permit, independent of the Allow LAN setting, so
    // neighbor/router discovery and multicast-listener messages keep working even when LAN traffic
    // is blocked. NS/RS and MLD reports use ff02:: multicast destinations that would otherwise be
    // caught by the gated ff00::/8 rule (block when LAN off) or the weight-0 block-all. Weight 8,
    // like the other always-on essentials. Mirrors the Linux/macOS rulesets.
    ret = Utils::addPermitFilterV6IcmpTypeRange(engineHandle, 130, 137, 8, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW);
    if (!ret) {
        spdlog::error("Could not add ICMPv6 NDP/MLD (130-137) allow filter.");
    }
    ret = Utils::addPermitFilterV6IcmpTypeRange(engineHandle, 143, 143, 8, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW);
    if (!ret) {
        spdlog::error("Could not add ICMPv6 MLDv2 (143) allow filter.");
    }

    // add permit/block filters for Local Network.
    // We explicitly block if not allowed, since this setting should take precedence over split tunneling filters.
    // Custom configs need private ranges allowed so third-party VPN DNS/gateway/routes work.
    bool bAllowLan = bAllowLocalTraffic || bIsCustomConfig;
    const std::vector<types::IpAddressRange> privV4 = types::IpAddressRange::fromStrings(
        {"10.0.0.0/8", "172.16.0.0/12", "192.168.0.0/16", "169.254.0.0/16"});
    ret = Utils::addFilterV4(engineHandle, nullptr, bAllowLan ? FWP_ACTION_PERMIT : FWP_ACTION_BLOCK, 4, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &privV4);
    if (!ret) {
        spdlog::error("Could not add IPv4 LAN traffic allow/block filter.");
    }

    // 10.255.255.0/24 (in-tunnel WG DNS): always blocked off the tunnel, regardless of the Allow LAN
    // setting. The weight-8 reserved permit on the VPN adapter (above) keeps it reachable in-tunnel;
    // this weight-5 block outranks the weight-4 LAN rule (so it drops on the physical NIC) but is
    // outranked by the weight-8 tunnel permit. Matches the Linux/macOS "tunnel pass, physical block".
    const std::vector<types::IpAddressRange> reservedDns = { types::IpAddressRange("10.255.255.0/24") };
    ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_BLOCK, 5, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &reservedDns);
    if (!ret) {
        spdlog::error("Could not add reserved in-tunnel DNS block filter.");
    }

    // add permit/block filters for multicast
    const std::vector<types::IpAddressRange> multicastV4 = { types::IpAddressRange("224.0.0.0/4") };
    ret = Utils::addFilterV4(engineHandle, nullptr, bAllowLan ? FWP_ACTION_PERMIT : FWP_ACTION_BLOCK, 4, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &multicastV4);
    if (!ret) {
        spdlog::error("Could not add IPv4 multicast allow/block filter.");
    }

    const std::vector<types::IpAddressRange> multicastV6 = { types::IpAddressRange("ff00::/8") };
    ret = Utils::addFilterV6(engineHandle, nullptr, bAllowLan ? FWP_ACTION_PERMIT : FWP_ACTION_BLOCK, 4, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &multicastV6);
    if (!ret) {
        spdlog::error("Could not add IPv6 multicast allow/block filter.");
    }

    // IPv6 ULA (the v6 analogue of the private v4 ranges above). NOTE (#1687): while connected the
    // Ipv6Firewall sublayer (ipv6_firewall.cpp) gates LAN v6 too, with the same allowLan condition,
    // so the two sublayers must be kept in sync.
    const std::vector<types::IpAddressRange> ulaV6 = { types::IpAddressRange("fc00::/7") };
    ret = Utils::addFilterV6(engineHandle, nullptr, bAllowLan ? FWP_ACTION_PERMIT : FWP_ACTION_BLOCK, 4, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &ulaV6);
    if (!ret) {
        spdlog::error("Could not add IPv6 ULA LAN traffic allow/block filter.");
    }
}

void FirewallFilter::addPermitFilterForVpnAndSystemServices(HANDLE engineHandle, const wchar_t *connectingIp)
{
    int ret = 0;

    AppsIds allowedIds;
    std::wstring svchost = Utils::getSystemDir() + L"\\svchost.exe";

    allowedIds.setFromList({svchost, L"System"});
    allowedIds.addFrom(vpnAppsIds_);

    // add allow filter for connecting IP (IPv4-only — connectingIp is always IPv4)
    const std::vector<types::IpAddressRange> connectingIpAddr = { types::IpAddressRange(connectingIp) };
    ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_PERMIT, 2, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &connectingIpAddr, 0, 0, &allowedIds, false);
    if (!ret) {
        spdlog::error("Could not add connecting IP allow filter");
    }
}

void FirewallFilter::addPermitFilterForAppsIds(HANDLE engineHandle)
{
    bool ret = true;
    if (isSplitTunnelingExclusiveMode_) {
        if (appsIds_.count() > 0) {
            ret = Utils::addFilterV4(engineHandle, &filterIdsApps_, FWP_ACTION_PERMIT, 2, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, nullptr, 0, 0, &appsIds_, false);
            ret = Utils::addFilterV6(engineHandle, &filterIdsApps_, FWP_ACTION_PERMIT, 2, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, nullptr, 0, 0, &appsIds_, false) && ret;
        }
    } else {
        ret = Utils::addFilterV4(engineHandle, &filterIdsApps_, FWP_ACTION_PERMIT, 2, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, nullptr, 0, 0, nullptr, false);
        ret = Utils::addFilterV6(engineHandle, &filterIdsApps_, FWP_ACTION_PERMIT, 2, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, nullptr, 0, 0, nullptr, false) && ret;
    }

    if (!ret) {
        spdlog::error("Could not add split tunnel app filters");
    }
}

void FirewallFilter::addPermitFilterForSplitRoutingWhitelistIps(HANDLE engineHandle)
{
    if (!isSplitTunnelingExclusiveMode_ || splitRoutingIps_.empty()) {
        return;
    }

    // addFilterV4/addFilterV6 internally filter the dual-stack vector by family and become
    // no-ops if no entries of their family are present. Pass the same container to both.
    bool ret = Utils::addFilterV4(engineHandle, &filterIdsSplitRoutingIps_, FWP_ACTION_PERMIT, 2, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &splitRoutingIps_, 0, 0, nullptr, false);
    if (!ret) {
        spdlog::error("Could not add split tunnel IPv4 filters");
    }
    ret = Utils::addFilterV6(engineHandle, &filterIdsSplitRoutingIps_, FWP_ACTION_PERMIT, 2, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &splitRoutingIps_, 0, 0, nullptr, false);
    if (!ret) {
        spdlog::error("Could not add split tunnel IPv6 filters");
    }
}

void FirewallFilter::removeAllSplitTunnelingFilters(HANDLE engineHandle)
{
    removeAppsSplitTunnelingFilter(engineHandle);
    removeIpsSplitTunnelingFilter(engineHandle);
}

void FirewallFilter::removeAppsSplitTunnelingFilter(HANDLE engineHandle)
{
    for (size_t i = 0; i < filterIdsApps_.size(); ++i) {
        DWORD ret = FwpmFilterDeleteById0(engineHandle, filterIdsApps_[i]);
        if (ret != ERROR_SUCCESS) {
            spdlog::error("FirewallFilter::removeAllSplitTunnelingFilters(), FwpmFilterDeleteById0 failed");
        }
    }
    filterIdsApps_.clear();
}

void FirewallFilter::removeIpsSplitTunnelingFilter(HANDLE engineHandle)
{
    for (size_t i = 0; i < filterIdsSplitRoutingIps_.size(); ++i) {
        DWORD ret = FwpmFilterDeleteById0(engineHandle, filterIdsSplitRoutingIps_[i]);
        if (ret != ERROR_SUCCESS) {
            spdlog::error("FirewallFilter::removeIpsSplitTunnelingFilter(), FwpmFilterDeleteById0 failed");
        }
    }
    filterIdsSplitRoutingIps_.clear();
}
