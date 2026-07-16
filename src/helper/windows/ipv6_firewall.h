#pragma once

#include <mutex>
#include <vector>

#include "apps_ids.h"
#include "fwpm_wrapper.h"
#include "types/ipaddress.h"

class Ipv6Firewall
{
public:
    enum class SplitTunnelMode { Disabled, Inclusive, Exclusive };

    static Ipv6Firewall &instance(FwpmWrapper *wrapper = nullptr)
    {
        static Ipv6Firewall ip6f(*wrapper);
        return ip6f;
    }

    void release();

    void enableIPv6();
    void disableIPv6(bool allowLanTraffic, bool isCustomConfig);

    // Split tunneling carve-outs in the global v6 block.
    // Inclusive: apps = tunnel-bound apps (included apps + VPN executables); everything else
    // gets native v6. Exclusive: apps = excluded apps; they get native v6.
    void setSplitTunnelingState(SplitTunnelMode mode, const AppsIds &apps);

    // Split tunneling IP/hostname list (manual entries + resolved hostnames, dual-stack;
    // v4 entries are ignored). Inclusive: off-tunnel v6 to these destinations is blocked so
    // it falls back to v4, which the routes carry into the tunnel. Exclusive: native v6 to
    // these destinations is permitted. Updates incrementally when the filters are installed.
    void setSplitTunnelingWhitelistIps(const std::vector<types::IpAddressRange> &ips);

private:
    explicit Ipv6Firewall(FwpmWrapper &fwpmWrapper);

    FwpmWrapper &fwpmWrapper_;
    GUID subLayerGUID_;
    std::recursive_mutex mutex_;
    bool isDisabled_ = false;
    bool allowLanTraffic_ = false;
    bool isCustomConfig_ = false;
    SplitTunnelMode splitTunnelMode_ = SplitTunnelMode::Disabled;
    AppsIds splitTunnelAppsIds_;
    std::vector<types::IpAddressRange> whitelistIps_;
    std::vector<UINT64> whitelistFilterIds_;
    void addFilters(HANDLE engineHandle, bool allowLanTraffic, bool isCustomConfig);
    bool applyWhitelistFiltersLocked(HANDLE engineHandle);
    void removeWhitelistFiltersLocked(HANDLE engineHandle);
};
