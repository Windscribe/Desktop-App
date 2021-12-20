#ifndef SplitTunneling_h
#define SplitTunneling_h

#include <string>
#include <mutex>
#include <vector>
#include "../kext_client/kext_client.h"
#include "../routes_manager/routes_manager.h"
#include "../ip_hostnames/ip_hostnames_manager.h"
#include "routes.h"
#include "../../../../posix_common/helper_commands.h"

class SplitTunneling
{
    
public:
    SplitTunneling();
    ~SplitTunneling();
    
    void setKextPath(const std::string &kextPath);
    void setConnectParams(CMD_SEND_CONNECT_STATUS &connectStatus);
    void setSplitTunnelingParams(bool isActive, bool isExclude, const std::vector<std::string> &apps,
                                 const std::vector<std::string> &ips, const std::vector<std::string> &hosts);
    
private:
    std::mutex mutex_;
    
    CMD_SEND_CONNECT_STATUS connectStatus_;
    
    bool isSplitTunnelActive_;
    bool isExclude_;
    
    KextClient kextClient_;
    RoutesManager routesManager_;
    IpHostnamesManager ipHostnamesManager_;
        
    void updateState();
    
};

#endif /* SplitTunneling_h */
