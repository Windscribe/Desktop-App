#include "split_tunneling.h"
#include "../firewallcontroller.h"
#include "../logger.h"
#include "../utils.h"

SplitTunneling::SplitTunneling(): isExclude_(false)
{
    isSplitTunnelActive_ = false;
    isExclude_ = false;
    connectStatus_.isConnected = false;
}

SplitTunneling::~SplitTunneling()
{
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
    std::vector<std::string> allIps = ips;

    LOG("isSplitTunnelingActive: %d, isExclude: %d", isActive, isExclude);

    isSplitTunnelActive_ = isActive;
    isExclude_ = isExclude;

    if (isActive && !isExclude) {
        // Add "checkip.windscribe.com" as an IP, not a hostname, so that it is passed directly to the 'route' command.
        // This will block until the name is resolved or it times out.  This is so we always have the correct address
        // for the tunnel test.
        allIps.push_back("checkip.windscribe.com");
    }

    ipHostnamesManager_.setSettings(allIps, hosts);
    routesManager_.updateState(connectStatus_, isSplitTunnelActive_, isExclude_);
    updateState();
}


void SplitTunneling::updateState()
{
    FirewallController::instance().setSplitTunnelingEnabled(connectStatus_.isConnected, isSplitTunnelActive_, isExclude_);

    if (connectStatus_.isConnected && isSplitTunnelActive_) {
        if (isExclude_) {
            ipHostnamesManager_.enable(connectStatus_.defaultAdapter.gatewayIp);
        } else {
            if (connectStatus_.protocol == kCmdProtocolOpenvpn || connectStatus_.protocol == kCmdProtocolStunnelOrWstunnel) {
                ipHostnamesManager_.enable(connectStatus_.vpnAdapter.gatewayIp);
            } else {
                ipHostnamesManager_.enable(connectStatus_.vpnAdapter.adapterIp);
            }
        }
    } else {
        ipHostnamesManager_.disable();
    }
}
