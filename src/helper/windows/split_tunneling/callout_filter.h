#pragma once

#include "../apps_ids.h"
#include "../fwpm_wrapper.h"
#include "types/ipaddress.h"

class CalloutFilter
{
public:
    explicit CalloutFilter(FwpmWrapper &fwmpWrapper);

    bool enable(UINT32 localIp, UINT32 vpnIp,
                const types::IpAddress &localIpV6, const types::IpAddress &vpnIpV6,
                const AppsIds &appsIds, const AppsIds &ctrldAppId,
                bool isExclude, bool allowLanTraffic,
                unsigned long vpnIfIndex);
    void disable();

    // Remote IPs from the split tunneling hostname/CIDR list (resolved hostnames + manual
    // entries). Installs per-destination v6 filters on the VPN LUID, mode-dependent:
    //  - Inclusive: PERMITs bypassing the v6 anti-leak BLOCK, so any app can reach
    //    hostname-routed destinations through the tunnel.
    //  - Exclusive: BLOCKs, unconditionally — enforces that excluded destinations' v6
    //    never egresses the tunnel. When IpRoutes steers them off-tunnel the BLOCK is
    //    idle (traffic uses the physical LUID); when it can't (no v6 address or gateway
    //    on the physical adapter, route races), blocked v6 falls back to v4, which the
    //    host routes carry outside.
    // v4 entries are ignored here. Stored across enable/disable cycles; the next enable()
    // reapplies the list.
    void setV6WhitelistIps(const std::vector<types::IpAddressRange> &ips);

    static bool removeAllFilters(FwpmWrapper &fwmpWrapper);

private:
    bool addProviderContext(HANDLE engineHandle, const GUID &guid, UINT32 localIp, UINT32 vpnIp, bool isExclude);
    bool addProviderContextV6(HANDLE engineHandle, const GUID &guid, const types::IpAddress &localIp, const types::IpAddress &vpnIp, bool isExclude);
    bool addCallouts(HANDLE engineHandle, bool withV6);
    bool addSubLayer(HANDLE engineHandle);
    bool addFilters(HANDLE engineHandle, bool withTcpFilters, bool withV6,
                    const AppsIds &appsIds, const AppsIds &ctrldAppId,
                    bool isExclude, unsigned long vpnIfIndex);

    static bool deleteSublayer(HANDLE engineHandle);

    // Apply / remove the per-destination v6 filters on the VPN LUID (PERMIT in inclusive
    // mode, BLOCK in exclusive mode — see setV6WhitelistIps). Caller must hold mutex_,
    // an open engine handle, and an active FwpmTransaction. apply returns false when a
    // filter failed to install; the caller must abort the transaction and roll back the
    // v6WhitelistIps_/v6WhitelistFilterIds_ bookkeeping to match the aborted WFP state.
    bool applyV6WhitelistFiltersLocked(HANDLE engineHandle);
    void removeV6WhitelistFiltersLocked(HANDLE engineHandle);

    FwpmWrapper &fwmpWrapper_;

    bool isEnabled_;
    std::recursive_mutex mutex_;

    AppsIds appsIds_;
    DWORD prevLocalIp_;
    DWORD prevVpnIp_;
    types::IpAddress prevLocalIpV6_;
    types::IpAddress prevVpnIpV6_;
    bool prevIsExclude_;
    bool prevIsAllowLanTraffic_;
    unsigned long prevVpnIfIndex_ = 0;

    // v6 anti-leak state. v6AntiLeakActive_ is true while per-destination v6 filters may
    // be installed on the VPN LUID: in inclusive mode a per-IP PERMIT alongside the
    // app-wide BLOCK, in exclusive mode (v6AntiLeakIsExclude_) a per-IP BLOCK for excluded
    // destinations. v6AntiLeakVpnLuid_ holds the LUID value (NET_LUID is a union with
    // ULONG64 Value; storing as UINT64 keeps Windows-specific includes out of the header).
    bool v6AntiLeakActive_ = false;
    bool v6AntiLeakIsExclude_ = false;
    UINT64 v6AntiLeakVpnLuid_ = 0;
    std::vector<types::IpAddressRange> v6WhitelistIps_;
    std::vector<UINT64> v6WhitelistFilterIds_;
};
