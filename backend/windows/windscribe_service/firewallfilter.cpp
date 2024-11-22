#include "all_headers.h"
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

void FirewallFilter::on(const wchar_t *connectingIp, const wchar_t *ip, bool bAllowLocalTraffic, bool bIsCustomConfig)
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
        addFilters(hEngine, connectingIp, ip, bAllowLocalTraffic, bIsCustomConfig);
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

void FirewallFilter::setWindscribeAppsIds(const AppsIds &appsIds)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    if (windscribeAppsIds_ == appsIds) {
        return;
    }
    windscribeAppsIds_ = appsIds;

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

void FirewallFilter::setSplitTunnelingWhitelistIpsV4(const std::vector<Ip4AddressAndMask> &ips)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    if (splitRoutingIpsV4_ == ips) {
        return;
    }
    splitRoutingIpsV4_ = ips;

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

void FirewallFilter::setSplitTunnelingWhitelistIpsV6(const std::vector<Ip6AddressAndPrefix> &ips)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    if (splitRoutingIpsV6_ == ips) {
        return;
    }
    splitRoutingIpsV6_ = ips;

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

void FirewallFilter::addFilters(HANDLE engineHandle, const wchar_t *connectingIp, const wchar_t *ip, bool bAllowLocalTraffic, bool bIsCustomConfig)
{
    std::vector<std::wstring> ipAddresses = split(ip, L';');

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

    // add permit filter for TAP-adapters
    for (std::vector<NET_IFINDEX>::iterator it = taps.begin(); it != taps.end(); ++it) {
        NET_LUID luid;
        ConvertInterfaceIndexToLuid(*it, &luid);

        if (!bIsCustomConfig) {
            // Explicitly allow 10.255.255.0/24
            const std::vector<Ip4AddressAndMask> reserved = Ip4AddressAndMask::fromVector({L"10.255.255.0/24"});
            ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_PERMIT, 8, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, &luid, &reserved);
            if (!ret) {
                spdlog::error(L"Could not add reserved allow filter on VPN interface");
            }

            // Explicitly allow local addresses on this interface
            const std::vector<Ip4AddressAndMask> localAddrs = Ip4AddressAndMask::fromVector(ai.getAdapterAddresses(*it));
            if (!localAddrs.empty()) {
                ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_PERMIT, 8, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &localAddrs);
                if (!ret) {
                    spdlog::error("Could not add reserved allow filter on private address for VPN interface");
                }
            }
            // Disallow all other private, link-local, loopback networks from going over tunnel
            const std::vector<Ip4AddressAndMask> priv = Ip4AddressAndMask::fromVector(
                {L"10.0.0.0/8", L"172.16.0.0/12", L"192.168.0.0/16", L"169.254.0.0/16", L"224.0.0.0/4"});
            ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_BLOCK, 6, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, &luid, &priv);
            if (!ret) {
                spdlog::error("Could not add private network block filter on VPN interface");
            }
        }
        // Allow other IPv4 traffic on this interface
        ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_PERMIT, 1, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, &luid);
        if (!ret) {
            spdlog::error("Could not add IPv4 allow filter on VPN interface");
        }
    }

    // add permit filter for split tunneling app ids and ips
    if (isSplitTunnelingEnabled_) {
        addPermitFilterForAppsIds(engineHandle);
        addPermitFilterForSplitRoutingWhitelistIps(engineHandle);
    }

    addPermitFilterForWindscribeAndSystemServices(engineHandle, connectingIp);

    // add permit filter for specific IPs
    for (size_t i = 0; i < ipAddresses.size(); ++i) {
        const std::vector<Ip4AddressAndMask> ipAddr = Ip4AddressAndMask::fromVector({ipAddresses[i].c_str()});
        ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_PERMIT, 2, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &ipAddr);
        if (!ret) {
            spdlog::error("Could not add Windscribe IPs allow filter");
        }
    }

    // add permit filter for DHCP
    ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_PERMIT, 3, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, nullptr, 68);
    if (!ret) {
        spdlog::error("Could not add DHCP allow filter (68)");
    }
    // add permit filter for DHCP
    ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_PERMIT, 3, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, nullptr, 67);
    if (!ret) {
        spdlog::error("Could not add DHCP allow filter (67)");
    }

    // Always allow localhost
    const std::vector<Ip4AddressAndMask> localhostV4 = Ip4AddressAndMask::fromVector({L"127.0.0.0/8"});
    ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_PERMIT, 8, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &localhostV4);
    if (!ret) {
        spdlog::error("Could not add localhost v4 allow filter");
    }
    const std::vector<Ip6AddressAndPrefix> localhostV6 = Ip6AddressAndPrefix::fromVector({L"::1/128"});
    ret = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_PERMIT, 8, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &localhostV6);
    if (!ret) {
        spdlog::error("Could not add localhost v6 allow filter.");
    }

    // add permit filters for unicast addresses
    const std::vector<Ip4AddressAndMask> unicastIps = Ip4AddressAndMask::fromVector({L"224.0.0.0/4"});
    ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_PERMIT, 4, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &unicastIps);
    if (!ret) {
        spdlog::error("Could not add IPv4 unicast allow filter.");
    }

    // add permit filters for Local Network
    if (bAllowLocalTraffic) {
        const std::vector<Ip4AddressAndMask> privV4 = Ip4AddressAndMask::fromVector(
            {L"10.0.0.0/8", L"172.16.0.0/12", L"192.168.0.0/16", L"169.254.0.0/16"});
        ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_PERMIT, 4, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &privV4);
        if (!ret) {
            spdlog::error("Could not add IPv4 LAN traffic allow filter.");
        }

        const std::vector<Ip6AddressAndPrefix> privV6 = Ip6AddressAndPrefix::fromVector(
            {L"fe80::/10", L"ff00::/64"});
        ret = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_PERMIT, 4, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &privV6);
        if (!ret) {
            spdlog::error("Could not add IPv6 LAN traffic allow filter.");
        }
    }
}

void FirewallFilter::addPermitFilterForWindscribeAndSystemServices(HANDLE engineHandle, const wchar_t *connectingIp)
{
    int ret = 0;

    AppsIds allowedIds;
    std::wstring svchost = Utils::getSystemDir() + L"\\svchost.exe";

    allowedIds.setFromList({svchost, L"System"});
    allowedIds.addFrom(windscribeAppsIds_);

    // add allow filter for connecting IP
    const std::vector<Ip4AddressAndMask> connectingIpAddr = Ip4AddressAndMask::fromVector({connectingIp});
    ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_PERMIT, 2, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &connectingIpAddr, 0, 0, &allowedIds);
    if (!ret) {
        spdlog::error("Could not add connecting IP allow filter");
    }
}

void FirewallFilter::addPermitFilterForAppsIds(HANDLE engineHandle)
{
    DWORD ret;

    if (isSplitTunnelingExclusiveMode_) {
        if (appsIds_.count() != 0) {
            ret = Utils::addFilterV6(engineHandle, &filterIdsApps_, FWP_ACTION_PERMIT, 2, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, nullptr, &appsIds_);
            if (!ret) {
                spdlog::error("Could not add split tunnel (v6) app filters");
            }

            ret = Utils::addFilterV4(engineHandle, &filterIdsApps_, FWP_ACTION_PERMIT, 2, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, nullptr, 0, 0, &appsIds_);
            if (!ret) {
                spdlog::error("Could not add split tunnel (v4) app filters");
            }
        }
    } else {
        // In inclusive mode, block IPv6 traffic for the included apps, since we can't pass IPv6 traffic over the IPv4 tunnel.
        ret = Utils::addFilterV6(engineHandle, &filterIdsApps_, FWP_ACTION_BLOCK, 4, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, nullptr, &appsIds_);
        if (!ret) {
            spdlog::error("Could not add split tunnel (v6) app block filters");
        }
        // Allow other IPv6 traffic.
        ret = Utils::addFilterV6(engineHandle, &filterIdsApps_, FWP_ACTION_PERMIT, 2, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW);
        if (!ret) {
            spdlog::error("Could not add split tunnel (v6) app permit filters");
        }
        // Allow IPv4 traffic.
        ret = Utils::addFilterV4(engineHandle, &filterIdsApps_, FWP_ACTION_PERMIT, 2, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW);
        if (!ret) {
            spdlog::error("Could not add split tunnel (v4) app filters");
        }
    }
}

void FirewallFilter::addPermitFilterForSplitRoutingWhitelistIps(HANDLE engineHandle)
{
    std::vector<UINT64> filterId;

    if (!isSplitTunnelingExclusiveMode_) {
        // In inclusive mode, we want to drop IPv6 traffic matching the IPs, because we can't pass IPv6 over an IPv4 tunnel.
        DWORD ret = Utils::addFilterV6(engineHandle, &filterIdsSplitRoutingIps_, FWP_ACTION_BLOCK, 4, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &splitRoutingIpsV6_);
        if (!ret) {
            spdlog::error("Could not add split tunnel IP (v6) filters");
        }
        return;
    }

    if (splitRoutingIpsV4_.size() > 0) {
        DWORD ret = Utils::addFilterV4(engineHandle, &filterIdsSplitRoutingIps_, FWP_ACTION_PERMIT, 2, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &splitRoutingIpsV4_);
        if (!ret) {
            spdlog::error("Could not add split tunnel IP (v4) filters");
        }
    }

    if (splitRoutingIpsV6_.size() > 0) {
        DWORD ret = Utils::addFilterV6(engineHandle, &filterIdsSplitRoutingIps_, FWP_ACTION_PERMIT, 2, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &splitRoutingIpsV6_);
        if (!ret) {
            spdlog::error("Could not add split tunnel IP (v6) filters");
        }
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

std::vector<std::wstring> &FirewallFilter::split(const std::wstring &s, wchar_t delim, std::vector<std::wstring> &elems)
{
    std::wstringstream ss(s);
    std::wstring item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::wstring> FirewallFilter::split(const std::wstring &s, wchar_t delim)
{
    std::vector<std::wstring> elems;
    split(s, delim, elems);
    return elems;
}
