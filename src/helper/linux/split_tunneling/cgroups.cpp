#include "cgroups.h"

#include <fstream>
#include <sstream>
#include <spdlog/spdlog.h>
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

    int ret = Utils::executeCommand("/opt/windscribe/scripts/cgroups-up",
                                    { mark_.c_str(),
                                      connectStatus.defaultAdapter.gatewayIp,
                                      connectStatus.defaultAdapter.adapterName,
                                      connectStatus.vpnAdapter.gatewayIp,
                                      connectStatus.vpnAdapter.adapterName,
                                      connectStatus.remoteIp,
                                      netClassId_.c_str(),
                                      isAllowLanTraffic ? "allow": "disallow",
                                      isExclude ? "exclusive": "inclusive"},
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

    Utils::executeCommand("/opt/windscribe/scripts/cgroups-down");
}

void CGroups::addApp(pid_t pid)
{
    std::ofstream out;
    out.open((findNetclsRoot() + "/windscribe/cgroup.procs").c_str());
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

