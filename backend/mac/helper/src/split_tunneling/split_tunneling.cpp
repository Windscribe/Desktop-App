#include "split_tunneling.h"
#include "utils.h"
#include "logger.h"

SplitTunneling::SplitTunneling(): isExclude_(false)
{
    isSplitTunnelActive_ = false;
    isExclude_ = false;
    connectStatus_.isConnected = false;
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
    kextClient_.setConnectParams(connectStatus);
    routesManager_.updateState(connectStatus_, isSplitTunnelActive_, isExclude_);
    updateState();
}

void SplitTunneling::setSplitTunnelingParams(bool isActive, bool isExclude, const std::vector<std::string> &apps,
                             const std::vector<std::string> &ips, const std::vector<std::string> &hosts)
{
    std::lock_guard<std::mutex> guard(mutex_);
    
    LOG("isSplitTunnelingActive: %d, isExclude: %d", isActive, isExclude);
        
    isSplitTunnelActive_ = isActive;
    isExclude_ = isExclude;
    
    kextClient_.setSplitTunnelingParams(isExclude, apps);
    
    ipHostnamesManager_.setSettings(isExclude, ips, hosts);
    
    routesManager_.updateState(connectStatus_, isSplitTunnelActive_, isExclude_);
    updateState();
}


void SplitTunneling::updateState()
{
    if (connectStatus_.isConnected && isSplitTunnelActive_)
    {
        if (isExclude_)
        {
            ipHostnamesManager_.enable(connectStatus_.defaultAdapter.gatewayIp);
        }
        else
        {
            if (connectStatus_.protocol == kCmdProtocolOpenvpn || connectStatus_.protocol == kCmdProtocolStunnelOrWstunnel)
            {
                ipHostnamesManager_.enable(connectStatus_.vpnAdapter.gatewayIp);
            }
            else if (connectStatus_.protocol == kCmdProtocolIkev2)
            {
                ipHostnamesManager_.enable(connectStatus_.vpnAdapter.adapterIp);
            }
            else if (connectStatus_.protocol == kCmdProtocolWireGuard)
            {
                ipHostnamesManager_.enable(connectStatus_.vpnAdapter.adapterIp);
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
