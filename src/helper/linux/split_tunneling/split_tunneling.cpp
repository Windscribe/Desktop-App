#include "split_tunneling.h"

#include <algorithm>
#include <spdlog/spdlog.h>
#include "../firewallcontroller.h"
#include "../utils.h"
#include "types/ipaddress.h"

SplitTunneling::SplitTunneling(): isSplitTunnelActive_(false), isExclude_(false), isAllowLanTraffic_(false)
{
    connectStatus_.isConnected = false;

    // Cause the process monitor to start.  We want this because the process monitor needs to perform a self-test
    ProcessMonitor::instance();
}

SplitTunneling::~SplitTunneling()
{
}

bool SplitTunneling::setConnectParams(ConnectStatus &connectStatus)
{
    std::lock_guard<std::mutex> guard(mutex_);
    spdlog::debug("isConnected: {}, protocol: {}", connectStatus.isConnected, (int)connectStatus.protocol);
    connectStatus_ = connectStatus;
    routesManager_.updateState(connectStatus_, isSplitTunnelActive_, isExclude_);
    return updateState();
}

void SplitTunneling::setSplitTunnelingParams(bool isActive, bool isExclude, const std::vector<std::string> &apps,
    const std::vector<std::string> &ips, const std::vector<std::string> &hosts, bool isAllowLanTraffic)
{
    std::lock_guard<std::mutex> guard(mutex_);
    apps_ = apps;

    spdlog::debug("isSplitTunnelingActive: {}, isExclude: {}, isAllowLanTraffic {}", isActive, isExclude, isAllowLanTraffic);

    isSplitTunnelActive_ = isActive;
    isExclude_ = isExclude;
    isAllowLanTraffic_ = isAllowLanTraffic;

    if (isActive && !isExclude) {
        apps_.push_back(WS_LINUX_INSTALL_DIR "/" WS_APP_EXECUTABLE_NAME);
    }

    // Convert raw user/ROBERT-supplied IP strings into typed dual-stack ranges so the
    // HostnamesManager / IpRoutes / FirewallController layers can dispatch by family.
    // Invalid entries are dropped here so they never reach the shell-command boundary.
    auto ipRanges = types::IpAddressRange::fromStrings(ips);
    ipRanges.erase(
        std::remove_if(ipRanges.begin(), ipRanges.end(),
                       [](const types::IpAddressRange &r) { return !r.isValid(); }),
        ipRanges.end());

    hostnamesManager_.setSettings(ipRanges, hosts);
    routesManager_.updateState(connectStatus_, isSplitTunnelActive_, isExclude_);
    ProcessMonitor::instance().setApps(apps_);
    updateState();
}


bool SplitTunneling::updateState()
{
    if (connectStatus_.isConnected && isSplitTunnelActive_) {
        if (!apps_.empty()) {
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
            // Exclude mode: host pins for excluded apps route via the default adapter.
            hostnamesManager_.enable(connectStatus_.defaultAdapter.gatewayIp,
                                     connectStatus_.defaultAdapter.gatewayIpV6,
                                     connectStatus_.defaultAdapter.adapterIp,
                                     connectStatus_.defaultAdapter.adapterIpV6,
                                     connectStatus_.defaultAdapter.adapterName,
                                     connectStatus_.defaultAdapter.adapterNameV6);
        } else {
            // Inclusive mode: host pins for included apps route via the VPN adapter.
            // The adapter IPs are also forwarded so HostnamesManager/IpRoutes can
            // detect the WireGuard point-to-point case (gatewayIp == adapterIp,
            // see wireguardconnection_posix.cpp) and emit `dev <iface>` instead
            // of `via <our-own-addr>`, which the kernel rejects.
            hostnamesManager_.enable(connectStatus_.vpnAdapter.gatewayIp,
                                     connectStatus_.vpnAdapter.gatewayIpV6,
                                     connectStatus_.vpnAdapter.adapterIp,
                                     connectStatus_.vpnAdapter.adapterIpV6,
                                     connectStatus_.vpnAdapter.adapterName,
                                     connectStatus_.vpnAdapter.adapterNameV6);
        }
    } else {
        CGroups::instance().disable();
        ProcessMonitor::instance().disable();
        hostnamesManager_.disable();
    }

    // Pass both families' default-adapter addresses; the firewall layer dispatches the
    // ingress-connmark path per-family. adapterIpV6.toString() returns "" for an invalid
    // IpAddress (no v6 detected), which the firewall layer treats as "skip the v6 path".
    FirewallController::instance().setSplitTunnelingEnabled(
        connectStatus_.isConnected,
        isSplitTunnelActive_,
        isExclude_,
        connectStatus_.defaultAdapter.adapterName,
        connectStatus_.defaultAdapter.adapterIp.toString(),
        connectStatus_.defaultAdapter.adapterIpV6.toString());
    return true;
}
