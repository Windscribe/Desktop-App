#include "cgroups.h"

#include <fstream>
#include <sstream>
#include <spdlog/spdlog.h>
#include "../network_marks.h"
#include "../utils.h"

CGroups::CGroups()
{
}

CGroups::~CGroups()
{
}

bool CGroups::enable(const ConnectStatus &connectStatus, bool isAllowLanTraffic, bool isExclude)
{
    spdlog::debug("cgroups enable");

    std::string out;

    // IpAddress fields are converted to strings at the script-argument boundary; mark_ /
    // netClassId_ / "allow"|"disallow" / "exclusive"|"inclusive" are family-agnostic.
    // The trailing args carry the v6 default-gateway, v6 VPN-gateway, the physical v6
    // default interface, and the physical v6 default address; all are empty strings when no
    // v6 is configured. The interface gives the v6 route the correct `dev` on a multi-homed
    // host; the address is the authoritative "host has physical v6" signal — the interface
    // name falls back to the v4 name (AdapterGatewayInfo::adapterNameV6) so it can't be used
    // to distinguish a real v6 default from a v4-only host.
    // wgFwmark is the WireGuard fwmark in hex (e.g. "0xca6c"), matching iproute2's
    // `ip rule show` output so the script can locate the WG `not fwmark` rule independently
    // of its (possibly varying) routing-table id. Single source of truth: marks::kWireGuardFwMark.
    std::ostringstream wgFwmarkStream;
    wgFwmarkStream << "0x" << std::hex << marks::kWireGuardFwMark;
    const std::string wgFwmark = wgFwmarkStream.str();

    int ret = Utils::executeCommand(WS_LINUX_INSTALL_DIR "/scripts/cgroups-up",
                                    { mark_,
                                      connectStatus.defaultAdapter.gatewayIp.toString(),
                                      connectStatus.defaultAdapter.adapterName,
                                      connectStatus.vpnAdapter.gatewayIp.toString(),
                                      connectStatus.vpnAdapter.adapterName,
                                      connectStatus.remoteIp.toString(),
                                      netClassId_,
                                      isAllowLanTraffic ? "allow" : "disallow",
                                      isExclude ? "exclusive" : "inclusive",
                                      // v6 args (empty when no v6 is configured).
                                      connectStatus.defaultAdapter.gatewayIpV6.toString(),
                                      connectStatus.vpnAdapter.gatewayIpV6.toString(),
                                      connectStatus.defaultAdapter.adapterNameV6,
                                      connectStatus.defaultAdapter.adapterIpV6.toString(),
                                      wgFwmark },
                                    &out);
    if (ret != 0) {
        spdlog::error("cgroups-up script failed: {}", out);
        return false;
    }

    return true;
}

void CGroups::disable()
{
    spdlog::debug("cgroups disable");

    Utils::executeCommand(WS_LINUX_INSTALL_DIR "/scripts/cgroups-down");
}

void CGroups::addApp(pid_t pid)
{
    std::ofstream out;
    out.open((findNetclsRoot() + "/" WS_PRODUCT_NAME_LOWER "/cgroup.procs").c_str());
    out << pid;
    out.close();
}

void CGroups::removeApp(pid_t pid)
{
    std::ofstream out;
    out.open((findNetclsRoot() + "/cgroup.procs").c_str());
    out << pid;
    out.close();
}

std::string CGroups::findNetclsRoot()
{
    if (!net_cls_root_.empty()) {
        return net_cls_root_;
    }

    std::string response;
    std::string line;

    Utils::executeCommand("mount", { "-l", "-t", "cgroup" }, &response);
    std::istringstream stream(response);
    while (std::getline(stream, line)) {
        int pos = line.find("net_cls on ");
        if (pos == std::string::npos) {
            continue;
        }
        // path is from after "net_cls on " until we see " type cgroup"
        net_cls_root_ = line.substr(pos + 11, line.find(" type cgroup") - 11);
        return net_cls_root_;
    }
    return std::string();
}

