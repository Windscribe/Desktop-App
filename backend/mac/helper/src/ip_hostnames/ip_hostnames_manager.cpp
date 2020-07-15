#include "IpHostnamesManager.h"
#include "Logger.h"

IpHostnamesManager::IpHostnamesManager(): isEnabled_(false), isExcludeMode_(true)
{
}

IpHostnamesManager::~IpHostnamesManager()
{
    disable();
}

void IpHostnamesManager::enable(const std::string &ipAddress)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    
    defaultRouteIp_ = ipAddress;
    ipRoutes_.setIps(defaultRouteIp_, ipsLatest_);
    
    isEnabled_ = true;
}

void IpHostnamesManager::disable()
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    
    if (!isEnabled_)
    {
        return;
    }
    
    ipRoutes_.clear();
    isEnabled_ = false;
    
}

void IpHostnamesManager::setSettings(bool isExclude, const std::vector<std::string> &ips, const std::vector<std::string> &hosts)
{
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    
    ipsLatest_ = ips;
    ipsLatest_.insert( ipsLatest_.end(), hosts.begin(), hosts.end() );
    isExcludeMode_ = isExclude;
    
    if (isEnabled_ && isExclude)
    {
        ipRoutes_.setIps(defaultRouteIp_, ipsLatest_);
    }
}
