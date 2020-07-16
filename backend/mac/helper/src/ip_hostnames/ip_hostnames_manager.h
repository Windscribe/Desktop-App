#ifndef IpHostnamesManager_h
#define IpHostnamesManager_h

#include <string>
#include <mutex>
#include <vector>
#include "ip_routes.h"

class IpHostnamesManager
{
public:
    IpHostnamesManager();
    ~IpHostnamesManager();
    
    void enable(const std::string &ipAddress);
    void disable();
    void setSettings(bool isExclude, const std::vector<std::string> &ips, const std::vector<std::string> &hosts);
    
private:
    IpRoutes ipRoutes_;
    bool isEnabled_;
    std::recursive_mutex mutex_;
    bool isExcludeMode_;
    
    std::string defaultRouteIp_;
    
    //latest ips and hosts list
    std::vector<std::string> ipsLatest_;
};

#endif /* IpHostnamesManager_h */
