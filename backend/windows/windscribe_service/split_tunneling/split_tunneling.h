#pragma once

#include "callout_filter.h"
#include "routes_manager.h"
#include "split_tunnel_service_manager.h"
#include "hostnames_manager/hostnames_manager.h"
#include "../apps_ids.h"
#include "../firewallfilter.h"
#include "../ipc/servicecommunication.h"

class SplitTunneling
{
public:
    static SplitTunneling &instance(FwpmWrapper *wrapper = nullptr)
    {
        static SplitTunneling st(*wrapper);
        return st;
    }

    void release();

    void setSettings(bool isEnabled, bool isExclude, const std::vector<std::wstring>& apps, const std::vector<std::wstring>& ips,
                     const std::vector<std::string>& hosts, bool isAllowLanTraffic);
    bool setConnectStatus(CMD_CONNECT_STATUS& connectStatus);
    bool updateState();

    static void removeAllFilters(FwpmWrapper& fwmpWrapper);

private:
    CalloutFilter calloutFilter_;
    RoutesManager routesManager_;
    HostnamesManager hostnamesManager_;
    SplitTunnelServiceManager splitTunnelServiceManager_;

    AppsIds windscribeMainExecutableId_;
    AppsIds windscribeOtherExecutablesId_;
    AppsIds ctrldExecutableId_;

    CMD_CONNECT_STATUS connectStatus_;
    bool isSplitTunnelEnabled_ = false;
    bool isExclude_ = false;
    bool isAllowLanTraffic_ = false;
    std::vector<std::wstring> apps_;
    bool prevIsSplitTunnelActive_ = false;
    bool prevIsExclude_ = false;

    SplitTunneling(FwpmWrapper& fwpmWrapper);
    void detectWindscribeExecutables();
    void setEnableIPv6Dns(bool enable);
};

