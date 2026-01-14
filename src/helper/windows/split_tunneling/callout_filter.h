#pragma once

#include "../apps_ids.h"
#include "../fwpm_wrapper.h"

class CalloutFilter
{
public:
    explicit CalloutFilter(FwpmWrapper &fwmpWrapper);

    void enable(UINT32 localIp, UINT32 vpnIp, const AppsIds &appsIds, const AppsIds &ctrldAppId, bool isExclude, bool allowLanTraffic);
    void disable();

    static bool removeAllFilters(FwpmWrapper &fwmpWrapper);

private:
    bool addProviderContext(HANDLE engineHandle, const GUID &guid, UINT32 localIp, UINT32 vpnIp, bool isExclude);
    bool addCallouts(HANDLE engineHandle);
    bool addSubLayer(HANDLE engineHandle);
    bool addFilters(HANDLE engineHandle, bool withTcpFilters, const AppsIds &appsIds, const AppsIds &ctrldAppId);

    static bool deleteSublayer(HANDLE engineHandle);

    FwpmWrapper &fwmpWrapper_;

    bool isEnabled_;
    std::recursive_mutex mutex_;

    AppsIds appsIds_;
    DWORD prevLocalIp_;
    DWORD prevVpnIp_;
    bool prevIsExclude_;
    bool prevIsAllowLanTraffic_;
};
