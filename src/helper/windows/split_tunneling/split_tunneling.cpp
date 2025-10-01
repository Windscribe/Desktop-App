#include "../all_headers.h"
#include <spdlog/spdlog.h>

#include "split_tunneling.h"
#include "../close_tcp_connections.h"
#include "../utils.h"

SplitTunneling::SplitTunneling(FwpmWrapper& fwpmWrapper)
    : calloutFilter_(fwpmWrapper), hostnamesManager_()
{
    connectStatus_.isConnected = false;
    detectWindscribeExecutables();

    AppsIds appsIds;
    appsIds.addFrom(windscribeMainExecutableId_);
    appsIds.addFrom(windscribeOtherExecutablesId_);
    FirewallFilter::instance().setWindscribeAppsIds(appsIds);
}

void SplitTunneling::release()
{
    assert(isSplitTunnelEnabled_ == false);
}

void SplitTunneling::setSettings(bool isEnabled, bool isExclude, const std::vector<std::wstring>& apps,
                                 const std::vector<std::wstring>& ips, const std::vector<std::string>& hosts,
                                 bool isAllowLanTraffic)
{
    isSplitTunnelEnabled_ = isEnabled;
    isExclude_ = isExclude;
    isAllowLanTraffic_ = isAllowLanTraffic;

    apps_ = apps;

    std::vector<Ip4AddressAndMask> ipsList;
    for (auto it = ips.begin(); it != ips.end(); ++it) {
        ipsList.push_back(Ip4AddressAndMask(it->c_str()));
    }

    hostnamesManager_.setSettings(isExclude, ipsList, hosts);
    routesManager_.updateState(connectStatus_, isSplitTunnelEnabled_, isExclude_);

    updateState();
}

bool SplitTunneling::setConnectStatus(ConnectStatus &connectStatus)
{
    connectStatus_ = connectStatus;
    routesManager_.updateState(connectStatus_, isSplitTunnelEnabled_, isExclude_);
    return updateState();
}

void SplitTunneling::removeAllFilters(FwpmWrapper& fwpmWrapper)
{
    CalloutFilter::removeAllFilters(fwpmWrapper);
}

void SplitTunneling::detectWindscribeExecutables()
{
    std::wstring exePath = Utils::getExePath();
    std::wstring fileFilter = exePath + L"\\*.exe";

    WIN32_FIND_DATA ffd;
    HANDLE hFind = FindFirstFileEx(fileFilter.c_str(), FindExInfoBasic, &ffd, FindExSearchNameMatch, NULL, 0);

    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }

    std::vector<std::wstring> windscribeMainExecutable;
    std::vector<std::wstring> windscribeOtherExecutables;
    std::vector<std::wstring> ctrldExecutable;
    do {
        if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::wstring fileName = ffd.cFileName;
            std::wstring fullPath = exePath + L"\\" + ffd.cFileName;

            if (fileName == L"Windscribe.exe") {
                windscribeMainExecutable.push_back(fullPath);
            } else if (fileName == L"windscribectrld.exe") {
                ctrldExecutable.push_back(fullPath);
            } else if (fileName != L"WindscribeService.exe") {     // skip WindscribeService.exe
                windscribeOtherExecutables.push_back(fullPath);
            }
        }
    } while (FindNextFile(hFind, &ffd) != 0);

    FindClose(hFind);

    windscribeMainExecutableId_.setFromList(windscribeMainExecutable);
    windscribeOtherExecutablesId_.setFromList(windscribeOtherExecutables);
    ctrldExecutableId_.setFromList(ctrldExecutable);
}

bool SplitTunneling::updateState()
{
    spdlog::debug("SplitTunneling::updateState begin");
    AppsIds appsIds;
    appsIds.setFromList(apps_);
    Ip4AddressAndMask localAddr(connectStatus_.defaultAdapter.adapterIp.c_str());
    DWORD localIp = localAddr.ipNetworkOrder();
    bool isSplitTunnelActive = connectStatus_.isConnected && isSplitTunnelEnabled_;

    // Allow excluded traffic to bypass firewall even when not connected
    if (!connectStatus_.isConnected && isSplitTunnelEnabled_ && isExclude_
        && connectStatus_.defaultAdapter.ifIndex != 0 && FirewallFilter::instance().currentStatus())
    {
        hostnamesManager_.enable(connectStatus_.defaultAdapter.gatewayIp, connectStatus_.defaultAdapter.ifIndex);
        FirewallFilter::instance().setSplitTunnelingAppsIds(appsIds);
        FirewallFilter::instance().setSplitTunnelingEnabled(isExclude_);

        calloutFilter_.disable();
        splitTunnelServiceManager_.stop();
    } else if (isSplitTunnelActive) {
        if (!splitTunnelServiceManager_.start()) {
            return false;
        }

        DWORD vpnIp;
        Ip4AddressAndMask vpnAddr(connectStatus_.vpnAdapter.adapterIp.c_str());
        vpnIp = vpnAddr.ipNetworkOrder();

        FirewallFilter::instance().setSplitTunnelingAppsIds(appsIds);
        FirewallFilter::instance().setSplitTunnelingEnabled(isExclude_);

        if (isExclude_) {
            hostnamesManager_.enable(connectStatus_.defaultAdapter.gatewayIp, connectStatus_.defaultAdapter.ifIndex);
        } else {
            appsIds.addFrom(windscribeOtherExecutablesId_);
            hostnamesManager_.enable(connectStatus_.vpnAdapter.gatewayIp, connectStatus_.vpnAdapter.ifIndex);
        }

        // For ctrld utility and Windscribe.exe we need special rules for inclusive mode
        AppsIds windscribeExecutablesForInclusive;
        if (!isExclude_) {
            windscribeExecutablesForInclusive.addFrom(windscribeMainExecutableId_);
            windscribeExecutablesForInclusive.addFrom(ctrldExecutableId_);
        }
        calloutFilter_.enable(localIp, vpnIp, appsIds, windscribeExecutablesForInclusive, isExclude_, isAllowLanTraffic_);

    } else {
        calloutFilter_.disable();
        hostnamesManager_.disable();
        FirewallFilter::instance().setSplitTunnelingDisabled();
        splitTunnelServiceManager_.stop();
    }

    // close TCP sockets if state changed
    if (isSplitTunnelActive != prevIsSplitTunnelActive_ && connectStatus_.isTerminateSocket) {
        spdlog::info("SplitTunneling::threadFunc() close TCP sockets, exclude non VPN apps");
        CloseTcpConnections::closeAllTcpConnections(connectStatus_.isKeepLocalSocket, isExclude_, apps_);
    } else if (isSplitTunnelActive && isExclude_ != prevIsExclude_ && connectStatus_.isTerminateSocket) {
        spdlog::info("SplitTunneling::threadFunc() close all TCP sockets");
        CloseTcpConnections::closeAllTcpConnections(connectStatus_.isKeepLocalSocket);
    }

    prevIsSplitTunnelActive_ = isSplitTunnelActive;
    prevIsExclude_ = isExclude_;

    spdlog::debug("SplitTunneling::updateState end");

    return true;
}
