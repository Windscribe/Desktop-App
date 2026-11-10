#include "ws_branding.h"
#include "split_tunneling.h"

#include <algorithm>

#include <iphlpapi.h>
#include <spdlog/spdlog.h>

#include "../close_tcp_connections.h"
#include "../firewallfilter.h"
#include "../ipv6_firewall.h"
#include "../utils.h"

namespace {

// 0 when there is no interface with this index (any more).
UINT64 luidForIfIndex(unsigned long ifIndex)
{
    NET_LUID luid = { 0 };
    if (ifIndex == 0 || ConvertInterfaceIndexToLuid(static_cast<NET_IFINDEX>(ifIndex), &luid) != NO_ERROR) {
        return 0;
    }
    return luid.Value;
}

bool isTunnelAdapter(const ADAPTER_GATEWAY_INFO &adapter, UINT64 vpnLuid)
{
    return vpnLuid != 0 && luidForIfIndex(adapter.ifIndex) == vpnLuid;
}

// An IPv6-only adapter (NAT64/464XLAT) has no IPv4 but is still a redirect target: the callout
// rewrites excluded apps to that address. ::, ::1 and link-local are not a target — link-local
// cannot carry the traffic the redirect is there to steer.
bool hasRedirectAddress(const ADAPTER_GATEWAY_INFO &adapter)
{
    if (adapter.adapterIp.ipv4NetworkOrder() != 0) {
        return true;
    }
    const types::IpAddress &v6 = adapter.adapterIpV6;
    if (!v6.isValid() || !v6.isV6() || v6.isLoopbackOrUnspecified()) {
        return false;
    }
    const uint8_t *b = v6.bytes();
    const bool linkLocal = b[0] == 0xfe && (b[1] & 0xc0) == 0x80;
    return !linkLocal;
}

// Exclusive split tunneling redirects excluded apps at this adapter. Empty, gone, or the tunnel
// itself is never a safe target: a socket bound to the tunnel address stays there for its lifetime.
bool isUsableRedirectTarget(const ADAPTER_GATEWAY_INFO &adapter, UINT64 vpnLuid)
{
    if (adapter.ifIndex == 0 || !hasRedirectAddress(adapter)) {
        return false;
    }
    const UINT64 luid = luidForIfIndex(adapter.ifIndex);
    return luid != 0 && luid != vpnLuid;
}

// 0 when this LUID is no longer an interface. The index is not stored: Windows reuses it.
unsigned long ifIndexForLuid(UINT64 luid)
{
    if (luid == 0) {
        return 0;
    }
    NET_LUID netLuid = { 0 };
    netLuid.Value = luid;
    NET_IFINDEX index = 0;
    if (ConvertInterfaceLuidToIndex(&netLuid, &index) != NO_ERROR || index == 0) {
        return 0;
    }
    return index;
}

}  // namespace

SplitTunneling::SplitTunneling(FwpmWrapper& fwpmWrapper)
    : calloutFilter_(fwpmWrapper), hostnamesManager_(&calloutFilter_)
{
    connectStatus_.isConnected = false;
    detectVpnExecutables();

    AppsIds appsIds;
    appsIds.addFrom(vpnMainExecutableId_);
    appsIds.addFrom(vpnOtherExecutablesId_);
    FirewallFilter::instance().setVpnAppsIds(appsIds);
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

    auto ranges = types::IpAddressRange::fromStrings(ips);
    ranges.erase(std::remove_if(ranges.begin(), ranges.end(),
                                [](const types::IpAddressRange &r) { return !r.isValid(); }),
                 ranges.end());

    hostnamesManager_.setSettings(isExclude, ranges, hosts);
    routesManager_.updateState(connectStatus_, isSplitTunnelEnabled_, isExclude_);

    updateState();
}

bool SplitTunneling::setConnectStatus(ConnectStatus &connectStatus)
{
    // A connected status with no VPN index must not wipe a known tunnel LUID: the guard below
    // keys off it. ifIndex 0 means the tunnel adapter was not captured.
    if (connectStatus.isConnected && connectStatus.vpnAdapter.ifIndex != 0) {
        const UINT64 vpnLuid = luidForIfIndex(connectStatus.vpnAdapter.ifIndex);
        if (vpnLuid != 0) {
            lastVpnLuid_ = vpnLuid;
        } else {
            spdlog::warn("SplitTunneling::setConnectStatus(), no LUID for VPN ifIndex={}, keeping VPN LUID={:#x}",
                         connectStatus.vpnAdapter.ifIndex, lastVpnLuid_);
        }
    }

    // In a VPN session (connecting, connected, reconnecting, disconnecting) the default adapter is
    // the exclusive redirect target: a sample that is empty, gone or the tunnel never replaces a
    // usable one, so the redirect is not armed at 0.0.0.0 or the tunnel address.
    // Outside a session (DISCONNECTED, network change) only the tunnel is rejected. GetBestInterface
    // reports it while the tunnel route is still the best, and if it stuck, the next session treated
    // it as the adapter to keep (#2048). Any other sample is taken as is: an empty one means the
    // network is gone, and the firewall exceptions and routes must not outlive it.
    const bool isInSession = connectStatus.isConnected || connectStatus.isVpnSessionActive;
    const bool isRejected = isInSession ? !isUsableRedirectTarget(connectStatus.defaultAdapter, lastVpnLuid_)
                                        : isTunnelAdapter(connectStatus.defaultAdapter, lastVpnLuid_);
    if (isRejected) {
        const ADAPTER_GATEWAY_INFO incoming = connectStatus.defaultAdapter;
        if (isUsableRedirectTarget(connectStatus_.defaultAdapter, lastVpnLuid_)) {
            spdlog::warn("SplitTunneling::setConnectStatus(), ignoring default adapter ifIndex={} (VPN LUID={:#x}), keeping ifIndex={}",
                         incoming.ifIndex, lastVpnLuid_, connectStatus_.defaultAdapter.ifIndex);
            connectStatus.defaultAdapter = connectStatus_.defaultAdapter;
        } else if (incoming.ifIndex != 0 || connectStatus_.defaultAdapter.ifIndex != 0) {
            // Neither sample is a physical adapter. Adopting the tunnel would aim the redirect
            // into the tunnel. Leave it unset until a physical adapter arrives.
            spdlog::warn("SplitTunneling::setConnectStatus(), dropping default adapter ifIndex={} (VPN LUID={:#x}); stored ifIndex={} is not a usable redirect target",
                         incoming.ifIndex, lastVpnLuid_, connectStatus_.defaultAdapter.ifIndex);
            connectStatus.defaultAdapter = ADAPTER_GATEWAY_INFO();
        }
    }

    connectStatus_ = connectStatus;
    routesManager_.updateState(connectStatus_, isSplitTunnelEnabled_, isExclude_);
    return updateState();
}

void SplitTunneling::removeAllFilters(FwpmWrapper& fwpmWrapper)
{
    CalloutFilter::removeAllFilters(fwpmWrapper);
}

void SplitTunneling::detectVpnExecutables()
{
    std::wstring exePath = Utils::getExePath();
    std::wstring fileFilter = exePath + L"\\*.exe";

    WIN32_FIND_DATA ffd;
    HANDLE hFind = FindFirstFileEx(fileFilter.c_str(), FindExInfoBasic, &ffd, FindExSearchNameMatch, NULL, 0);

    if (hFind == INVALID_HANDLE_VALUE) {
        DWORD lastError = ::GetLastError();
        spdlog::error(L"SplitTunneling::detectVpnExecutables(), FindFirstFileEx failed for {}: {}", fileFilter, lastError);
        return;
    }

    std::vector<std::wstring> vpnMainExecutable;
    std::vector<std::wstring> vpnOtherExecutables;
    std::vector<std::wstring> ctrldExecutable;
    do {
        if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::wstring fileName = ffd.cFileName;
            std::wstring fullPath = exePath + L"\\" + ffd.cFileName;

            if (fileName == WS_PRODUCT_NAME_W L".exe") {
                vpnMainExecutable.push_back(fullPath);
            } else if (fileName == WS_PRODUCT_NAME_LOWER_W L"ctrld.exe") {
                ctrldExecutable.push_back(fullPath);
            } else if (fileName != WS_APP_IDENTIFIER_W L"Service.exe") {     // skip the service executable
                vpnOtherExecutables.push_back(fullPath);
            }
        }
    } while (FindNextFile(hFind, &ffd) != 0);

    FindClose(hFind);

    vpnMainExecutableId_.setFromList(vpnMainExecutable);
    vpnOtherExecutablesId_.setFromList(vpnOtherExecutables);
    ctrldExecutableId_.setFromList(ctrldExecutable);
}

bool SplitTunneling::updateState()
{
    spdlog::debug("SplitTunneling::updateState begin");
    AppsIds appsIds;
    appsIds.setFromList(apps_);
    // adapterIp is types::IpAddress; ipv4NetworkOrder() returns 0 for non-v4/invalid which
    // matches the previous Ip4AddressAndMask behaviour for empty/invalid input.
    DWORD localIp = connectStatus_.defaultAdapter.adapterIp.ipv4NetworkOrder();
    bool isSplitTunnelActive = connectStatus_.isConnected && isSplitTunnelEnabled_;

    // Exclusive mode, VPN session in progress but not connected (connecting, reconnecting,
    // disconnecting). Tunnel routes can appear before isConnected arrives or still be there after
    // it is gone; a socket an excluded app binds or connects meanwhile would get the tunnel source
    // and keep it for its lifetime, since the redirect never revisits existing sockets (#2048).
    // The exclusive redirect target is the physical adapter, which is known before connecting, so
    // keep the callout on for the whole session. An IPv6-only adapter has no localIp and is still
    // enough: the callout rewrites v6 to it. The per-app and per-destination blocks need the tunnel
    // interface. The connected status is what records its LUID; while this session is not connected
    // the blocks stay on that adapter, and drop only once it is gone.
    const bool isExclusiveSessionRedirect = !connectStatus_.isConnected && connectStatus_.isVpnSessionActive &&
                                            isSplitTunnelEnabled_ && isExclude_ &&
                                            hasRedirectAddress(connectStatus_.defaultAdapter);

    if (isExclusiveSessionRedirect) {
        if (!splitTunnelServiceManager_.start()) {
            Ipv6Firewall::instance().setSplitTunnelingState(Ipv6Firewall::SplitTunnelMode::Disabled, AppsIds());
            return false;
        }

        // Same firewall/hostname handling as the disconnected state below.
        if (connectStatus_.defaultAdapter.ifIndex != 0 && FirewallFilter::instance().currentStatus()) {
            hostnamesManager_.enable(connectStatus_.defaultAdapter.gatewayIp,
                                     connectStatus_.defaultAdapter.gatewayIpV6,
                                     connectStatus_.defaultAdapter.ifIndex);
            FirewallFilter::instance().setSplitTunnelingAppsIds(appsIds);
            FirewallFilter::instance().setSplitTunnelingEnabled(isExclude_);
        } else {
            hostnamesManager_.disable();
            FirewallFilter::instance().setSplitTunnelingDisabled();
        }
        Ipv6Firewall::instance().setSplitTunnelingState(Ipv6Firewall::SplitTunnelMode::Disabled, AppsIds());

        // 0 on the first connect, and again once the previous tunnel adapter has been removed.
        const unsigned long vpnIfIndex = ifIndexForLuid(lastVpnLuid_);
        if (!calloutFilter_.enable(localIp, 0,
                                   connectStatus_.defaultAdapter.adapterIpV6, types::IpAddress(),
                                   appsIds, AppsIds(), isExclude_, isAllowLanTraffic_, vpnIfIndex)) {
            spdlog::error("SplitTunneling::updateState(), CalloutFilter::enable() failed for the VPN session");
            return false;
        }
    } else if (// Allow excluded traffic to bypass firewall even when not connected
               !connectStatus_.isConnected && isSplitTunnelEnabled_ && isExclude_
        && connectStatus_.defaultAdapter.ifIndex != 0 && FirewallFilter::instance().currentStatus())
    {
        hostnamesManager_.enable(connectStatus_.defaultAdapter.gatewayIp,
                                 connectStatus_.defaultAdapter.gatewayIpV6,
                                 connectStatus_.defaultAdapter.ifIndex);
        FirewallFilter::instance().setSplitTunnelingAppsIds(appsIds);
        FirewallFilter::instance().setSplitTunnelingEnabled(isExclude_);
        Ipv6Firewall::instance().setSplitTunnelingState(Ipv6Firewall::SplitTunnelMode::Disabled, AppsIds());

        calloutFilter_.disable();
        splitTunnelServiceManager_.stop();
    } else if (isSplitTunnelActive) {
        if (!splitTunnelServiceManager_.start()) {
            Ipv6Firewall::instance().setSplitTunnelingState(Ipv6Firewall::SplitTunnelMode::Disabled, AppsIds());
            return false;
        }

        DWORD vpnIp = connectStatus_.vpnAdapter.adapterIp.ipv4NetworkOrder();

        FirewallFilter::instance().setSplitTunnelingAppsIds(appsIds);
        FirewallFilter::instance().setSplitTunnelingEnabled(isExclude_);

        if (isExclude_) {
            hostnamesManager_.enable(connectStatus_.defaultAdapter.gatewayIp,
                                     connectStatus_.defaultAdapter.gatewayIpV6,
                                     connectStatus_.defaultAdapter.ifIndex);
        } else {
            appsIds.addFrom(vpnOtherExecutablesId_);
            // WireGuard/IKEv2 are point-to-point: vpn.gatewayIp(V6) is invalid for them.
            // Fall back to the adapter address (matches RoutesManager::addDnsRoutes and
            // doActionsForInclusiveMode{WireGuard,Ikev2}), which Windows resolves on-link
            // via the tunnel interface.
            const auto &vpn = connectStatus_.vpnAdapter;
            const types::IpAddress &gw   = vpn.gatewayIp.isValid()   ? vpn.gatewayIp   : vpn.adapterIp;
            const types::IpAddress &gwV6 = vpn.gatewayIpV6.isValid() ? vpn.gatewayIpV6 : vpn.adapterIpV6;
            hostnamesManager_.enable(gw, gwV6, vpn.ifIndex);
        }

        // For ctrld utility and the main app executable we need special rules for inclusive mode
        AppsIds vpnExecutablesForInclusive;
        if (!isExclude_) {
            vpnExecutablesForInclusive.addFrom(vpnMainExecutableId_);
            vpnExecutablesForInclusive.addFrom(ctrldExecutableId_);
        }

        if (!calloutFilter_.enable(localIp, vpnIp,
                                   connectStatus_.defaultAdapter.adapterIpV6,
                                   connectStatus_.vpnAdapter.adapterIpV6,
                                   appsIds, vpnExecutablesForInclusive, isExclude_, isAllowLanTraffic_,
                                   connectStatus_.vpnAdapter.ifIndex)) {
            spdlog::error("SplitTunneling::updateState(), CalloutFilter::enable() failed");
            Ipv6Firewall::instance().setSplitTunnelingState(Ipv6Firewall::SplitTunnelMode::Disabled, AppsIds());
            return false;
        }

        // Only after the callout is up: fail-closed if it didn't start.
        if (isExclude_) {
            Ipv6Firewall::instance().setSplitTunnelingState(Ipv6Firewall::SplitTunnelMode::Exclusive, appsIds);
        } else {
            AppsIds tunnelBoundApps = appsIds;  // included apps + vpnOtherExecutablesId_
            tunnelBoundApps.addFrom(vpnMainExecutableId_);
            tunnelBoundApps.addFrom(ctrldExecutableId_);
            Ipv6Firewall::instance().setSplitTunnelingState(Ipv6Firewall::SplitTunnelMode::Inclusive, tunnelBoundApps);
        }
    } else {
        Ipv6Firewall::instance().setSplitTunnelingState(Ipv6Firewall::SplitTunnelMode::Disabled, AppsIds());
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
