#pragma once

#include <string>
#include <mutex>
#include <vector>
#include "hostnames_manager/hostnames_manager.h"
#include "../routes_manager/routes.h"
#include "../routes_manager/routes_manager.h"
#include "../../../posix_common/helper_commands.h"

class SplitTunneling
{
public:
    static SplitTunneling& instance()
    {
        static SplitTunneling st;
        return st;
    }

    void setConnectParams(CMD_SEND_CONNECT_STATUS &connectStatus);
    void setSplitTunnelingParams(bool isActive, bool isExclude, const std::vector<std::string> &apps,
                                 const std::vector<std::string> &ips, const std::vector<std::string> &hosts);

private:
    std::mutex mutex_;

    CMD_SEND_CONNECT_STATUS connectStatus_;

    bool isSplitTunnelActive_;
    bool isExclude_;

    HostnamesManager hostnamesManager_;
    RoutesManager routesManager_;

    SplitTunneling();
    ~SplitTunneling();
    void updateState();
};
