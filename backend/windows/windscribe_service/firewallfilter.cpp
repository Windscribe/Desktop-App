#include "all_headers.h"

#include "firewallfilter.h"
#include "logger.h"
#include "utils.h"
#include "adapters_info.h"


FirewallFilter::FirewallFilter(FwpmWrapper &fwpmWrapper) :
    fwpmWrapper_(fwpmWrapper), lastAllowLocalTraffic_(false), isSplitTunnelingEnabled_(false),
    isSplitTunnelingExclusiveMode_(false)
{
    UuidFromString((RPC_WSTR)UUID_LAYER, &subLayerGUID_);
}

FirewallFilter::~FirewallFilter()
{
}

void FirewallFilter::on(const wchar_t *ip, bool bAllowLocalTraffic, bool bIsCustomConfig)
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
        addFilters(hEngine, ip, bAllowLocalTraffic, bIsCustomConfig);
    } else {
        Logger::instance().out(L"FirewallFilter::on(), FwpmSubLayerAdd0 failed");
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
        Logger::instance().out(L"FirewallFilter::offImpl(), all filters deleted");
    } else {
        Logger::instance().out(L"FirewallFilter::offImpl(), failed delete filters");
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

void FirewallFilter::setSplitTunnelingWhitelistIps(const std::vector<Ip4AddressAndMask> &ips)
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

void FirewallFilter::addFilters(HANDLE engineHandle, const wchar_t *ip, bool bAllowLocalTraffic, bool bIsCustomConfig)
{
    std::vector<std::wstring> ipAddresses = split(ip, L';');

    AdaptersInfo ai;
    std::vector<NET_IFINDEX> taps = ai.getTAPAdapters();

    // add block filter for all IPs, for all adapters, IPv4.
    bool ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_BLOCK, 0, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW);
    if (!ret) {
        Logger::instance().out("Could not add IPv4 block filter");
    }
    // add block filter for all IPs, for all adapters, IPv6.
    ret = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_BLOCK, 0, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW);
    if (!ret) {
        Logger::instance().out("Could not add IPv6 block filter");
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
                Logger::instance().out(L"Could not add reserved allow filter on VPN interface");
            }

            // Explicitly allow local addresses on this interface
            const std::vector<Ip4AddressAndMask> localAddrs = Ip4AddressAndMask::fromVector(ai.getAdapterAddresses(*it));
            if (!localAddrs.empty()) {
                ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_PERMIT, 8, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &localAddrs);
                if (!ret) {
                    Logger::instance().out(L"Could not add reserved allow filter on private address for VPN interface");
                }
            }
            // Disallow all other private, link-local, loopback networks from going over tunnel
            const std::vector<Ip4AddressAndMask> priv = Ip4AddressAndMask::fromVector(
                {L"10.0.0.0/8", L"172.16.0.0/12", L"192.168.0.0/16", L"169.254.0.0/16", L"224.0.0.0/24"});
            ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_BLOCK, 6, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, &luid, &priv);
            if (!ret) {
                Logger::instance().out(L"Could not add private network block filter on VPN interface");
            }
        }
        // Allow other IPv4 traffic on this interface
        ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_PERMIT, 1, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, &luid);
        if (!ret) {
            Logger::instance().out(L"Could not add IPv4 allow filter on VPN interface");
        }
        // Allow IPv6 traffic on this interface
        Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_PERMIT, 1, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, &luid);
        if (!ret) {
            Logger::instance().out(L"Could not add IPv6 allow filter on VPN interface");
        }
    }

    // add permit filter for split tunneling app ids and ips
    if (isSplitTunnelingEnabled_) {
        addPermitFilterForAppsIds(engineHandle);
        addPermitFilterForSplitRoutingWhitelistIps(engineHandle);
    }

    // add permit filter for specific IPs
    for (size_t i = 0; i < ipAddresses.size(); ++i) {
        const std::vector<Ip4AddressAndMask> ipAddr = Ip4AddressAndMask::fromVector({ipAddresses[i].c_str()});
        ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_PERMIT, 2, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &ipAddr);
        if (!ret) {
            Logger::instance().out(L"Could not add Windscribe IPs allow filter");
        }
    }

    // add permit filter for DHCP
    // (Exec("netsh advfirewall firewall add rule name=\"VPN - Out - DHCP\" dir=out action=allow protocol=UDP
    // localport=68 remoteport=67 program=\"%SystemRoot%\\system32\\svchost.exe\" service=\"dhcp\"");
    ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_PERMIT, 3, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, nullptr, 68, 67);
    if (!ret) {
        Logger::instance().out(L"Could not add DHCP allow filter");
    }

    // Add permit filter for Windscribe reserved range (10.255.255.0 - 10.255.255.255)
    const std::vector<Ip4AddressAndMask> reserved = Ip4AddressAndMask::fromVector({L"10.255.255.0/24"});
    ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_PERMIT, 4, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &reserved);
    if (!ret) {
        Logger::instance().out(L"Could not add reserved allow filter");
    }

    // Always allow localhost
    const std::vector<Ip4AddressAndMask> localhostV4 = Ip4AddressAndMask::fromVector({L"127.0.0.0/8"});
    ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_PERMIT, 8, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &localhostV4);
    if (!ret) {
        Logger::instance().out(L"Could not add localhost v4 allow filter");
    }
    const std::vector<Ip6AddressAndPrefix> localhostV6 = Ip6AddressAndPrefix::fromVector({L"::1/128"});
    ret = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_PERMIT, 8, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &localhostV6);
    if (!ret) {
        Logger::instance().out(L"Could not add localhost v6 allow filter.");
    }

    // add permit filters for Local Network
    if (bAllowLocalTraffic) {
        const std::vector<Ip4AddressAndMask> privV4 = Ip4AddressAndMask::fromVector(
            {L"10.0.0.0/8", L"172.16.0.0/12", L"192.168.0.0/16", L"169.254.0.0/16", L"224.0.0.0/24"});
        ret = Utils::addFilterV4(engineHandle, nullptr, FWP_ACTION_PERMIT, 4, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &privV4);
        if (!ret) {
            Logger::instance().out(L"Could not add IPv4 LAN traffic allow filter.");
        }

        const std::vector<Ip6AddressAndPrefix> privV6 = Ip6AddressAndPrefix::fromVector(
            {L"fe80::/10", L"ff00::/64"});
        ret = Utils::addFilterV6(engineHandle, nullptr, FWP_ACTION_PERMIT, 4, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &privV6);
        if (!ret) {
            Logger::instance().out(L"Could not add IPv6 LAN traffic allow filter.");
        }
    }
}


void FirewallFilter::addPermitFilterForAppsIds(HANDLE engineHandle)
{
    DWORD ret;

    if (isSplitTunnelingExclusiveMode_) {
        if (appsIds_.count() != 0) {
            ret = Utils::addFilterV4(engineHandle, &filterIdsApps_, FWP_ACTION_PERMIT, 2, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, nullptr, 0, 0, &appsIds_);
        }
    } else {
        ret = Utils::addFilterV4(engineHandle, &filterIdsApps_, FWP_ACTION_PERMIT, 2, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW);
    }

    if (!ret) {
        Logger::instance().out("Could not add split tunnel app filters");
    }
}

void FirewallFilter::addPermitFilterForSplitRoutingWhitelistIps(HANDLE engineHandle)
{
    std::vector<UINT64> filterId;

    if (!isSplitTunnelingExclusiveMode_ || splitRoutingIps_.size() == 0) {
        return;
    }

    DWORD ret = Utils::addFilterV4(engineHandle, &filterIdsSplitRoutingIps_, FWP_ACTION_PERMIT, 2, subLayerGUID_, FIREWALL_SUBLAYER_NAMEW, nullptr, &splitRoutingIps_);
    if (!ret) {
        Logger::instance().out("Could not add split tunnel IP filters");
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
            Logger::instance().out(L"FirewallFilter::removeAllSplitTunnelingFilters(), FwpmFilterDeleteById0 failed");
        }
    }
    filterIdsApps_.clear();
}

void FirewallFilter::removeIpsSplitTunnelingFilter(HANDLE engineHandle)
{
    for (size_t i = 0; i < filterIdsSplitRoutingIps_.size(); ++i) {
        DWORD ret = FwpmFilterDeleteById0(engineHandle, filterIdsSplitRoutingIps_[i]);
        if (ret != ERROR_SUCCESS) {
            Logger::instance().out(L"FirewallFilter::removeIpsSplitTunnelingFilter(), FwpmFilterDeleteById0 failed");
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