#pragma once

#include "ws_branding.h"
#include "apps_ids.h"
#include "ip_address/ip4_address_and_mask.h"
#include "fwpm_wrapper.h"

#define UUID_LAYER L"367016b3-3af8-4966-8442-d8bb6435f4a0"
#define FIREWALL_SUBLAYER_NAMEW WS_APP_IDENTIFIER_W L"Firewall"

class FirewallFilter
{
public:
    static FirewallFilter &instance(FwpmWrapper *wrapper = nullptr)
    {
        static FirewallFilter ff(*wrapper);
        return ff;
    }

    void release();

    void on(const wchar_t *connectingIp, const wchar_t *ip, bool bAllowLocalTraffic, bool bIsCustomConfig);
    void off();
    bool currentStatus();

    // split tunnelling parameters
    void setSplitTunnelingEnabled(bool isExclusiveMode);
    void setSplitTunnelingDisabled();

    void setSplitTunnelingAppsIds(const AppsIds &appsIds);
    void setSplitTunnelingWhitelistIps(const std::vector<Ip4AddressAndMask> &ips);
    void setVpnAppsIds(const AppsIds &appsIds);

private:
    FwpmWrapper &fwpmWrapper_;
    GUID   subLayerGUID_;

    std::recursive_mutex mutex_;
    bool lastAllowLocalTraffic_;

    explicit FirewallFilter(FwpmWrapper &fwmpWrapper);

    bool currentStatusImpl(HANDLE engineHandle);
    void offImpl(HANDLE engineHandle);

    void addFilters(HANDLE engineHandle, const wchar_t *connectingIp, const wchar_t *ip, bool bAllowLocalTraffic, bool bIsCustomConfig);
    void addPermitFilterForVpnAndSystemServices(HANDLE engineHandle, const wchar_t *ip);
    void addPermitFilterForAppsIds(HANDLE engineHandle);
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
    AppsIds vpnAppsIds_;
    std::vector<Ip4AddressAndMask> splitRoutingIps_;

    std::vector<UINT64> filterIdsApps_;
    std::vector<UINT64> filterIdsSplitRoutingIps_;
};
