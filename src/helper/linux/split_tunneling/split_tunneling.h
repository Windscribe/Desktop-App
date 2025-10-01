#pragma once

#include <string>
#include <mutex>
#include <vector>
#include "cgroups.h"
#include "hostnames_manager/hostnames_manager.h"
#include "process_monitor.h"
#include "../routes_manager/routes.h"
#include "../routes_manager/routes_manager.h"
#include "../../common/helper_commands.h"

class SplitTunneling
{
public:
    static SplitTunneling& instance()
    {
        static SplitTunneling st;
        return st;
    }

    bool setConnectParams(ConnectStatus &connectStatus);
    void setSplitTunnelingParams(bool isActive, bool isExclude, const std::vector<std::string> &apps,
                                 const std::vector<std::string> &ips, const std::vector<std::string> &hosts, bool isAllowLanTraffic);

private:
    std::mutex mutex_;

    ConnectStatus connectStatus_;

    bool isSplitTunnelActive_;
    bool isExclude_;
    bool isAllowLanTraffic_;

    HostnamesManager hostnamesManager_;
    RoutesManager routesManager_;

    std::vector<std::string> apps_;

    SplitTunneling();
    ~SplitTunneling();
    bool updateState();
};
