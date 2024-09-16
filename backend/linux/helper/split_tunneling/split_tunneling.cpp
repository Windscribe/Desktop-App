#include "split_tunneling.h"
#include "../firewallcontroller.h"
#include "../logger.h"
#include "../utils.h"

SplitTunneling::SplitTunneling(): isSplitTunnelActive_(false), isExclude_(false), isAllowLanTraffic_(false)
{
    connectStatus_.isConnected = false;
}

SplitTunneling::~SplitTunneling()
{
}

bool SplitTunneling::setConnectParams(CMD_SEND_CONNECT_STATUS &connectStatus)
{
    std::lock_guard<std::mutex> guard(mutex_);
    Logger::instance().out("isConnected: %d, protocol: %d", connectStatus.isConnected, (int)connectStatus_.protocol);
    connectStatus_ = connectStatus;
    routesManager_.updateState(connectStatus_, isSplitTunnelActive_, isExclude_);
    return updateState();
}

void SplitTunneling::setSplitTunnelingParams(bool isActive, bool isExclude, const std::vector<std::string> &apps,
    const std::vector<std::string> &ips, const std::vector<std::string> &hosts, bool isAllowLanTraffic)
{
    std::lock_guard<std::mutex> guard(mutex_);
    apps_ = apps;

    Logger::instance().out("isSplitTunnelingActive: %d, isExclude: %d, isAllowLanTraffic %d", isActive, isExclude, isAllowLanTraffic);

    isSplitTunnelActive_ = isActive;
    isExclude_ = isExclude;
    isAllowLanTraffic_ = isAllowLanTraffic;

    if (isActive && !isExclude) {
        apps_.push_back( "/opt/windscribe/Windscribe");
    }

    hostnamesManager_.setSettings(ips, hosts);
    routesManager_.updateState(connectStatus_, isSplitTunnelActive_, isExclude_);
    ProcessMonitor::instance().setApps(apps_);
    updateState();
}


bool SplitTunneling::updateState()
{
    if (connectStatus_.isConnected && isSplitTunnelActive_) {
        if (!apps_.empty()) {
            std::string gw = connectStatus_.vpnAdapter.adapterIp;
            if (connectStatus_.protocol == kCmdProtocolOpenvpn || connectStatus_.protocol == kCmdProtocolStunnelOrWstunnel) {
                gw = connectStatus_.vpnAdapter.gatewayIp;
            }

            bool ret = CGroups::instance().enable(connectStatus_, isAllowLanTraffic_, isExclude_);
            if (!ret) {
                return ret;
            }
            ret = ProcessMonitor::instance().enable();
            if (!ret) {
                return ret;
            }
        }

        if (isExclude_) {
            hostnamesManager_.enable(connectStatus_.defaultAdapter.gatewayIp);
        } else {
            if (connectStatus_.protocol == kCmdProtocolOpenvpn || connectStatus_.protocol == kCmdProtocolStunnelOrWstunnel) {
                hostnamesManager_.enable(connectStatus_.vpnAdapter.gatewayIp);
            } else {
                hostnamesManager_.enable(connectStatus_.vpnAdapter.adapterIp);
            }
        }
    } else {
        CGroups::instance().disable();
        ProcessMonitor::instance().disable();
        hostnamesManager_.disable();
    }

    FirewallController::instance().setSplitTunnelingEnabled(
        connectStatus_.isConnected,
        isSplitTunnelActive_,
        isExclude_,
        connectStatus_.defaultAdapter.adapterName,
        connectStatus_.defaultAdapter.adapterIp);
    return false;
}
