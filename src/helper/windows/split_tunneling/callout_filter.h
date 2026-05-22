#pragma once

#include "../apps_ids.h"
#include "../fwpm_wrapper.h"
#include "types/ipaddress.h"

class CalloutFilter
{
public:
    explicit CalloutFilter(FwpmWrapper &fwmpWrapper);

    void enable(UINT32 localIp, UINT32 vpnIp,
                const types::IpAddress &localIpV6, const types::IpAddress &vpnIpV6,
                const AppsIds &appsIds, const AppsIds &ctrldAppId,
                bool isExclude, bool allowLanTraffic,
                unsigned long vpnIfIndex);
    void disable();

    // Whitelist of remote IPs that bypass the v6 inclusive-mode anti-leak block on the
    // VPN interface, regardless of which app issues the connection. Source: HostnamesManager
    // (resolved hostnames + manual IP/range entries from the inclusive list). Without this
    // every app that isn't in the inclusive apps list would be blocked from reaching
    // hostname-routed v6 destinations through the tunnel, undermining the host-based
    // inclusive policy. The list is dual-stack; v4 entries are stored but ignored here
    // (IPv4 inclusive mode has no anti-leak block to compensate for). Stored across
    // enable/disable cycles so the next enable() reapplies it.
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

    // Apply / remove the v6 whitelist permit on the VPN LUID. Caller must hold mutex_,
    // an open engine handle, and an active FwpmTransaction.
    void applyV6WhitelistPermitFiltersLocked(HANDLE engineHandle);
    void removeV6WhitelistPermitFiltersLocked(HANDLE engineHandle);

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

    // v6 inclusive-mode anti-leak state. v6AntiLeakActive_ is true while the BLOCK on
    // VPN LUID is in WFP and a per-IP PERMIT may be installed alongside it.
    // v6AntiLeakVpnLuid_ holds the LUID value (NET_LUID is a union with ULONG64 Value;
    // storing as UINT64 keeps Windows-specific includes out of the header).
    bool v6AntiLeakActive_ = false;
    UINT64 v6AntiLeakVpnLuid_ = 0;
    std::vector<types::IpAddressRange> v6WhitelistIps_;
    std::vector<UINT64> v6WhitelistFilterIds_;
};
