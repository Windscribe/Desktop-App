#include "split_tunneling.h"
#include "utils.h"
#include "logger.h"

SplitTunneling::SplitTunneling(): isExclude_(false),
    kextClient_(std::bind(&SplitTunneling::verifyApp, this, std::placeholders::_1,
    std::placeholders::_2, std::placeholders::_3))
{
    isSplitTunnelActive_ = false;
    isExclude_ = false;
    connectStatus_.isConnected = false;
    windscribeExecutables_.push_back("wsappcontrol");
    windscribeExecutables_.push_back("/WindscribeEngine.app");
}

SplitTunneling::~SplitTunneling()
{
}

void SplitTunneling::setKextPath(const std::string &kextPath)
{
    LOG("Set kext path: %s", kextPath.c_str());
    kextClient_.setKextPath(kextPath);
}

void SplitTunneling::setConnectParams(CMD_SEND_CONNECT_STATUS &connectStatus)
{
    std::lock_guard<std::mutex> guard(mutex_);
    LOG("isConnected: %d, protocol: %d", connectStatus.isConnected, (int)connectStatus_.protocol);
    connectStatus_ = connectStatus;
    routesManager_.updateState(connectStatus_, isSplitTunnelActive_, isExclude_);
    updateState();
}

void SplitTunneling::setSplitTunnelingParams(bool isActive, bool isExclude, const std::vector<std::string> &apps,
                             const std::vector<std::string> &ips, const std::vector<std::string> &hosts)
{
    std::lock_guard<std::mutex> guard(mutex_);
    
    LOG("isSplitTunnelingActive: %d, prisExcludeotocol: %d", isActive, isExclude);
        
    isSplitTunnelActive_ = isActive;
    isExclude_ = isExclude;
    apps_ = apps;
    
    ipHostnamesManager_.setSettings(isExclude, ips, hosts);
    
    applyExtraRules(apps_);
    routesManager_.updateState(connectStatus_, isSplitTunnelActive_, isExclude_);
    updateState();
}

void SplitTunneling::setLatestWireGuardAdapterSettings(const std::string &adapterName, const std::string &ipAddress, const std::string &dnsAddressList,
const std::vector<std::string> &allowedIps)
{
    std::lock_guard<std::mutex> guard(mutex_);
    
    wgIpAddress_ = ipAddress;
    wgDnsAddress_ = dnsAddressList;     //todo several DNS-IPs
    if (allowedIps.size() > 0)          // todo several allowed-IPs
    {
        wgAllowedIp_ = allowedIps[0];
    }
    else
    {
        wgAllowedIp_.clear();
    }
    
    routesManager_.setLatestWireGuardAdapterSettings(adapterName, ipAddress, dnsAddressList, allowedIps);
    
    std::string ips;
    for (auto it : allowedIps)
    {
        ips += it + ";";
    }
    LOG("ipAddress: %s, dnsAddressList: %s, allowedIps = %s", ipAddress.c_str(), dnsAddressList.c_str(), ips.c_str());
}

bool SplitTunneling::verifyApp(const std::string &appPath, std::string &outBindIp, bool &isExclude)
{
    std::lock_guard<std::mutex> guard(mutex_);
    
    isExclude = isExclude_;
    
    if (connectStatus_.interfaceIp.empty())
    {
        goto not_verify;
    }
    
    // check for Windscribe executables
    for(const auto &windscribePath: windscribeExecutables_)
    {
        if (Utils::findCaseInsensitive(appPath, windscribePath) != std::string::npos)
        {
            if (isExclude)
            {
                goto not_verify;
            }
            else
            {
                if (connectStatus_.protocol == CMD_PROTOCOL_OPENVPN || connectStatus_.protocol == CMD_PROTOCOL_STUNNEL_OR_WSTUNNEL)
                {
                    outBindIp = connectStatus_.ifconfigTunIp;
                }
                else if (connectStatus_.protocol == CMD_PROTOCOL_IKEV2)
                {
                    outBindIp = connectStatus_.vpnAdapterIp;
                }
                else if (connectStatus_.protocol == CMD_PROTOCOL_WIREGUARD)
                {
                    outBindIp = connectStatus_.vpnAdapterIp;
                }
                LOG("VerifyApp: %s, result: %d", appPath.c_str(), true);
                return true;
            }
        }
    }
    
    // check for app list
    for (std::vector<std::string>::iterator it = apps_.begin(); it != apps_.end(); ++it)
    {
        if (appPath.rfind(*it, 0) == 0)
        {
            if (isExclude)
            {
                outBindIp = connectStatus_.interfaceIp;
            }
            else
            {
                if (connectStatus_.protocol == CMD_PROTOCOL_OPENVPN || connectStatus_.protocol == CMD_PROTOCOL_STUNNEL_OR_WSTUNNEL)
                {
                    outBindIp = connectStatus_.ifconfigTunIp;
                }
                else if (connectStatus_.protocol == CMD_PROTOCOL_IKEV2)
                {
                    outBindIp = connectStatus_.vpnAdapterIp;
                }
                else if (connectStatus_.protocol == CMD_PROTOCOL_WIREGUARD)
                {
                    outBindIp = connectStatus_.vpnAdapterIp;
                }
            }
            LOG("VerifyApp: %s, result: %d", appPath.c_str(), true);
            return true;
        }
    }
    
not_verify:
    LOG("VerifyApp: %s, result: %d", appPath.c_str(), false);
    return false;
}

// add extra paths for Mac standard applications
void SplitTunneling::applyExtraRules(std::vector<std::string> &apps)
{
    std::vector<std::string> addedPaths;
    
    // If the system WebKit framework is excluded/vpnOnly, add this staged framework
    // path too. Newer versions of Safari use this.
    for (const auto &path: apps)
    {
        if (Utils::findCaseInsensitive(path, "/App Store.app") != std::string::npos)
        {
            addedPaths.push_back(WEBKIT_FRAMEWORK_PATH);
            addedPaths.push_back(STAGED_WEBKIT_FRAMEWORK_PATH);
            addedPaths.push_back("/System/Library/PrivateFrameworks/AppStoreDaemon.framework/Support/appstoreagent");
        }
        else if (Utils::findCaseInsensitive(path, "/Calendar.app") != std::string::npos)
        {
            addedPaths.push_back(WEBKIT_FRAMEWORK_PATH);
            addedPaths.push_back(STAGED_WEBKIT_FRAMEWORK_PATH);
            addedPaths.push_back("/System/Library/PrivateFrameworks/CalendarAgent.framework/Executables/CalendarAgent");
        }
        else if (Utils::findCaseInsensitive(path, "/Safari.app") != std::string::npos)
        {
            addedPaths.push_back(WEBKIT_FRAMEWORK_PATH);
            addedPaths.push_back(STAGED_WEBKIT_FRAMEWORK_PATH);
            
            addedPaths.push_back("/System/Library/CoreServices/SafariSupport.bundle/Contents/MacOS/SafariBookmarksSyncAgent");
            addedPaths.push_back("/System/Library/StagedFrameworks/Safari/WebKit.framework/Versions/A/XPCServices/com.apple.WebKit.Networking.xpc");
            addedPaths.push_back("/System/Library/PrivateFrameworks/SafariSafeBrowsing.framework/Versions/A/com.apple.Safari.SafeBrowsing.Service");
            addedPaths.push_back("/System/Library/StagedFrameworks/Safari/SafariShared.framework/Versions/A/XPCServices/com.apple.Safari.SearchHelper.xpc");
        }
    }
    
    for (const auto &path: addedPaths)
    {
        apps.push_back(path);
    }
}

void SplitTunneling::updateState()
{
    if (connectStatus_.isConnected && isSplitTunnelActive_)
    {
        if (isExclude_)
        {
            ipHostnamesManager_.enable(connectStatus_.gatewayIp);
        }
        else
        {
            if (connectStatus_.protocol == CMD_PROTOCOL_OPENVPN || connectStatus_.protocol == CMD_PROTOCOL_STUNNEL_OR_WSTUNNEL)
            {
                ipHostnamesManager_.enable(connectStatus_.routeVpnGateway);
            }
            else if (connectStatus_.protocol == CMD_PROTOCOL_IKEV2)
            {
                ipHostnamesManager_.enable(connectStatus_.vpnAdapterIp);
            }
            else if (connectStatus_.protocol == CMD_PROTOCOL_WIREGUARD)
            {
                ipHostnamesManager_.enable(connectStatus_.vpnAdapterIp);
            }
        }
        kextClient_.connect();
    }
    else
    {
        kextClient_.disconnect();
        ipHostnamesManager_.disable();
    }
}
