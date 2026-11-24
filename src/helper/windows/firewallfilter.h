#pragma once

#include "ws_branding.h"

#include <optional>

#include "apps_ids.h"
#include "firewall_incremental.h"
#include "fwpm_wrapper.h"
#include "types/ipaddress.h"

class AdaptersInfo;

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

    void on(const wchar_t *connectingIp, const std::vector<std::wstring> &ips, bool bAllowLocalTraffic, bool bIsCustomConfig);
    void off();
    bool currentStatus();

    // split tunnelling parameters
    void setSplitTunnelingEnabled(bool isExclusiveMode);
    void setSplitTunnelingDisabled();

    void setSplitTunnelingAppsIds(const AppsIds &appsIds);
    // Dual-stack: ips holds IPv4 and/or IPv6 ranges; family is dispatched in
    // addPermitFilterForSplitRoutingWhitelistIps via isV4()/isV6().
    void setSplitTunnelingWhitelistIps(const std::vector<types::IpAddressRange> &ips);
    void setVpnAppsIds(const AppsIds &appsIds);

private:
    FwpmWrapper &fwpmWrapper_;
    GUID   subLayerGUID_;

    std::recursive_mutex mutex_;
    bool lastAllowLocalTraffic_;

    explicit FirewallFilter(FwpmWrapper &fwmpWrapper);

    bool currentStatusImpl(HANDLE engineHandle);
    void offImpl(HANDLE engineHandle);

    static std::vector<firewall_incremental::TapSnapshot> captureTapSnapshot(AdaptersInfo &ai);

    // Deletes everything in the sublayer and builds it again from `in`, in one transaction.
    void rebuild(HANDLE engineHandle, const firewall_incremental::Inputs &in);
    // Replaces only the connecting-IP and per-IP permits named by `plan`, in one transaction.
    // Returns false (with nothing changed and the cache dropped) if any step fails.
    bool applyIncremental(HANDLE engineHandle, const firewall_incremental::Inputs &in, const firewall_incremental::Plan &plan);

    // Each returns false if any filter could not be added. The IDs of the filters added go to
    // `state` or `ids`, so they can be replaced later without a rebuild.
    bool addFilters(HANDLE engineHandle, const firewall_incremental::Inputs &in, firewall_incremental::BuiltState &state);
    // The full rebuild and the incremental update both add these two groups through these
    // functions, so the filters are identical either way.
    bool addConnectingIpFilter(HANDLE engineHandle, const std::wstring &connectingIp, std::vector<UINT64> &ids);
    bool addIpPermitFilter(HANDLE engineHandle, const std::wstring &ip, std::vector<UINT64> &ids);
    static bool deleteFilters(HANDLE engineHandle, const std::vector<UINT64> &ids);

    void addPermitFilterForAppsIds(HANDLE engineHandle);
    void addPermitFilterForSplitRoutingWhitelistIps(HANDLE engineHandle);

    void removeAppsSplitTunnelingFilter(HANDLE engineHandle);
    void removeIpsSplitTunnelingFilter(HANDLE engineHandle);
    void removeAllSplitTunnelingFilters(HANDLE engineHandle);

    // What the installed filter set was built from. Empty when unknown: at helper start (filters
    // persisted at boot), after off(), or after a failed build or update.
    std::optional<firewall_incremental::BuiltState> built_;

    bool isSplitTunnelingEnabled_;
    bool isSplitTunnelingExclusiveMode_;
    AppsIds appsIds_;
    AppsIds vpnAppsIds_;
    std::vector<types::IpAddressRange> splitRoutingIps_;

    std::vector<UINT64> filterIdsApps_;
    std::vector<UINT64> filterIdsSplitRoutingIps_;
};
