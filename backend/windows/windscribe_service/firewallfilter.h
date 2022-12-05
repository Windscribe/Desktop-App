#ifndef FIREWALLFILTER_H
#define FIREWALLFILTER_H

#include "apps_ids.h"
#include "ip_address/ip4_address_and_mask.h"
#include "ip_address/ip6_address_and_prefix.h"
#include "fwpm_wrapper.h"

#define UUID_LAYER L"367016b3-3af8-4966-8442-d8bb6435f4a0"
#define FIREWALL_SUBLAYER_NAMEW          L"WindscribeFirewall"

class FirewallFilter
{
public:
    explicit FirewallFilter(FwpmWrapper &fwmpWrapper);
    ~FirewallFilter();

    void on(const wchar_t *ip, bool bAllowLocalTraffic, bool bIsCustomConfig);
    void off();
    bool currentStatus();

    // split tunnelling parameters
    void setSplitTunnelingEnabled(bool isExclusiveMode);
    void setSplitTunnelingDisabled();

    void setSplitTunnelingAppsIds(const AppsIds &appsIds);
    void setSplitTunnelingWhitelistIps(const std::vector<Ip4AddressAndMask> &ips);

private:
    FwpmWrapper &fwpmWrapper_;
    GUID   subLayerGUID_;

    std::recursive_mutex mutex_;
    bool lastAllowLocalTraffic_;

    bool currentStatusImpl(HANDLE engineHandle);
    void offImpl(HANDLE engineHandle);

    void addFilters(HANDLE engineHandle, const wchar_t *ip, bool bAllowLocalTraffic, bool bIsCustomConfig);
    void addPermitFilterForAppsIds(HANDLE engineHandle);
    void addPermitFilterForAppsIdsExclusiveMode(HANDLE engineHandle);
    void addPermitFilterForAppsIdsInclusiveMode(HANDLE engineHandle);
    void addPermitFilterForSplitRoutingWhitelistIps(HANDLE engineHandle);

    void removeAppsSplitTunnelingFilter(HANDLE engineHandle);
    void removeIpsSplitTunnelingFilter(HANDLE engineHandle);
    void removeAllSplitTunnelingFilters(HANDLE engineHandle);

    std::vector<std::wstring> &split(const std::wstring &s, wchar_t delim, std::vector<std::wstring> &elems);
    std::vector<std::wstring> split(const std::wstring &s, wchar_t delim);


    std::vector<std::wstring> tapAdapters_;

    bool isSplitTunnelingEnabled_;
    bool isSplitTunnelingExclusiveMode_;
    AppsIds appsIds_;
    std::vector<Ip4AddressAndMask> splitRoutingIps_;

    std::vector<UINT64> filterIdsApps_;
    std::vector<UINT64> filterIdsSplitRoutingIps_;
};

#endif // FIREWALLFILTER_H
