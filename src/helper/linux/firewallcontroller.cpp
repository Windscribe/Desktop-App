#include "firewallcontroller.h"

#include <algorithm>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <spdlog/spdlog.h>

#include "../common/helper_commands.h"
#include "../common/io_posix.h"
#include "split_tunneling/cgroups.h"
#include "utils.h"

FirewallController::FirewallController() : connected_(false), splitTunnelEnabled_(false), splitTunnelExclude_(true)
{
}

FirewallController::~FirewallController()
{
}

bool FirewallController::enable(bool ipv6, const std::string &rules)
{
    int fd;

    // open()'s mode is ignored on pre-existing files; unlink first to force the intended perms
    if (ipv6) {
        unlink(WS_LINUX_RUN_DIR "/rules.v6");
        fd = open(WS_LINUX_RUN_DIR "/rules.v6", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    } else {
        unlink(WS_LINUX_RUN_DIR "/rules.v4");
        fd = open(WS_LINUX_RUN_DIR "/rules.v4", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    }

    if (fd < 0) {
        spdlog::error("Could not open firewall rules for writing");
        return 1;
    }

    if (!IO::writeAll(fd, rules)) {
        spdlog::error("Could not write rules: {}", IO::strerror(errno));
        close(fd);
        return 1;
    }
    close(fd);

    if (ipv6) {
        Utils::executeCommand("ip6tables-restore", {"-n", WS_LINUX_RUN_DIR "/rules.v6"});
    } else {
        Utils::executeCommand("iptables-restore", {"-n", WS_LINUX_RUN_DIR "/rules.v4"});
    }

    // The freshly-restored rule blob wiped any per-chain rules the split-tunnel
    // pipeline previously appended (cgroup ACCEPTs, IP exceptions, ingress connmark).
    // Re-emit them now so split-tunnel state survives a kill-switch refresh on the
    // family that was just rewritten — the helpers no-op on the family that didn't
    // change.
    setSplitTunnelIpExceptions(splitTunnelIps_);
    setSplitTunnelAppExceptions();
    setSplitTunnelIngressRules(defaultAdapterIp_, defaultAdapterIpV6_);

    return 0;
}

void FirewallController::getRules(bool ipv6, std::string *outRules)
{
    std::string filename;

    if (ipv6) {
        filename = WS_LINUX_RUN_DIR "/rules.v6";
        Utils::executeCommand("ip6tables-save", {"-f", filename.c_str()});
    } else {
        filename = WS_LINUX_RUN_DIR "/rules.v4";
        Utils::executeCommand("iptables-save", {"-f", filename.c_str()});
    }

    std::ifstream ifs(filename.c_str());
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    *outRules = buffer.str();
}

bool FirewallController::enabled(const std::string &tag)
{
    // Kill-switch is considered enabled if the INPUT jump into windscribe_input
    // exists in *either* family. The engine always installs v4 and v6 together,
    // but enable(ipv6=…) is invoked as two separate helper commands — checking
    // only one family used to make the second call's post-restore
    // setSplitTunnelIpExceptions() take the tear-down branch if the per-family
    // order were ever swapped. Symmetric probe removes that footgun and matches
    // the engine-level "kill-switch is on" semantic for external callers
    // (checkFirewallState).
    if (Utils::executeCommand("iptables", {"--check", "INPUT", "-j", WS_PRODUCT_NAME_LOWER "_input", "-m", "comment", "--comment", tag.c_str()}) == 0) {
        return true;
    }
    return Utils::executeCommand("ip6tables", {"--check", "INPUT", "-j", WS_PRODUCT_NAME_LOWER "_input", "-m", "comment", "--comment", tag.c_str()}) == 0;
}

void FirewallController::disable()
{
    std::error_code ec;
    std::filesystem::remove(WS_LINUX_RUN_DIR "/rules.v4", ec);
    if (ec) {
        spdlog::warn("Failed to remove rules.v4: {}", ec.message());
    }
    std::filesystem::remove(WS_LINUX_RUN_DIR "/rules.v6", ec);
    if (ec) {
        spdlog::warn("Failed to remove rules.v6: {}", ec.message());
    }
}

void FirewallController::setSplitTunnelingEnabled(bool isConnected, bool isEnabled, bool isExclude,
                                                  const std::string &defaultAdapter,
                                                  const std::string &defaultAdapterIp,
                                                  const std::string &defaultAdapterIpV6)
{
    connected_ = isConnected;
    splitTunnelEnabled_ = isEnabled;
    splitTunnelExclude_ = isExclude;
    prevAdapter_ = defaultAdapter_;
    defaultAdapter_ = defaultAdapter;
    defaultAdapterIp_ = defaultAdapterIp;
    defaultAdapterIpV6_ = defaultAdapterIpV6;

    setSplitTunnelIpExceptions(splitTunnelIps_);
    setSplitTunnelAppExceptions();
    setSplitTunnelIngressRules(defaultAdapterIp_, defaultAdapterIpV6_);
}

void FirewallController::removeExclusiveIpRules()
{
    for (const auto &ip : splitTunnelIps_) {
        if (!ip.isValid()) {
            continue;
        }
        const char *tool = ip.isV6() ? "ip6tables" : "iptables";
        const std::string ipStr = ip.toString();
        Utils::executeCommand(tool, {"-D", WS_PRODUCT_NAME_LOWER "_input", "-s", ipStr.c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
        Utils::executeCommand(tool, {"-D", WS_PRODUCT_NAME_LOWER "_output", "-d", ipStr.c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
    }
}

void FirewallController::removeInclusiveIpRules()
{
    // The unconditional ACCEPT rule is added (appended) to both v4 and v6
    // windscribe_input/_output chains in setSplitTunnelAppExceptions /
    // setSplitTunnelIpExceptions, so the cleanup path has to walk both families.
    //
    // Delete by rule-spec rather than by chain position: it doesn't depend on
    // parsing `iptables -S` output and survives reordering inside the chain.
    // iptables -D is a no-op (non-zero exit) when the rule isn't present, which
    // matches the desired idempotent semantics here.
    for (const char *tool : {"iptables", "ip6tables"}) {
        Utils::executeCommand(tool, {"-D", WS_PRODUCT_NAME_LOWER "_input",  "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
        Utils::executeCommand(tool, {"-D", WS_PRODUCT_NAME_LOWER "_output", "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
    }
}

void FirewallController::removeExclusiveAppRules()
{
    Utils::executeCommand("iptables", {"-D", "OUTPUT", "-t", "mangle", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "MARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag});
    Utils::executeCommand("ip6tables", {"-D", "OUTPUT", "-t", "mangle", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "MARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag});
    if (!prevAdapter_.empty()) {
        Utils::executeCommand("iptables", {"-D", "POSTROUTING", "-t", "nat", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-o", prevAdapter_.c_str(), "-j", "MASQUERADE", "-m", "comment", "--comment", kTag});
        Utils::executeCommand("ip6tables", {"-D", "POSTROUTING", "-t", "nat", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-o", prevAdapter_.c_str(), "-j", "MASQUERADE", "-m", "comment", "--comment", kTag});
    }
}

void FirewallController::removeInclusiveAppRules()
{
    Utils::executeCommand("iptables", {"-D", "OUTPUT", "-t", "mangle", "-m", "cgroup", "!", "--cgroup", CGroups::instance().netClassId(), "-j", "MARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag});
    Utils::executeCommand("ip6tables", {"-D", "OUTPUT", "-t", "mangle", "-m", "cgroup", "!", "--cgroup", CGroups::instance().netClassId(), "-j", "MARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag});
    if (!prevAdapter_.empty()) {
        Utils::executeCommand("iptables", {"-D", "POSTROUTING", "-t", "nat", "-m", "cgroup", "!", "--cgroup", CGroups::instance().netClassId(), "-o", prevAdapter_.c_str(), "-j", "MASQUERADE", "-m", "comment", "--comment", kTag});
        Utils::executeCommand("ip6tables", {"-D", "POSTROUTING", "-t", "nat", "-m", "cgroup", "!", "--cgroup", CGroups::instance().netClassId(), "-o", prevAdapter_.c_str(), "-j", "MASQUERADE", "-m", "comment", "--comment", kTag});
    }
}

void FirewallController::setSplitTunnelIngressRules(const std::string &defaultAdapterIp, const std::string &defaultAdapterIpV6)
{
    if (defaultAdapterIp.empty() && defaultAdapterIpV6.empty()) {
        return;
    }

    if (!connected_ || !splitTunnelEnabled_ || splitTunnelExclude_) {
        if (!defaultAdapterIp.empty()) {
            Utils::executeCommand("iptables", {"-D", "PREROUTING", "-t", "mangle", "-d", defaultAdapterIp.c_str(), "-j", "CONNMARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag});
            Utils::executeCommand("iptables", {"-D", "OUTPUT", "-t", "mangle", "-j", "CONNMARK", "--restore-mark", "-m", "comment", "--comment", kTag});
        }
        if (!defaultAdapterIpV6.empty()) {
            Utils::executeCommand("ip6tables", {"-D", "PREROUTING", "-t", "mangle", "-d", defaultAdapterIpV6.c_str(), "-j", "CONNMARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag});
            Utils::executeCommand("ip6tables", {"-D", "OUTPUT", "-t", "mangle", "-j", "CONNMARK", "--restore-mark", "-m", "comment", "--comment", kTag});
        }
        return;
    }

    if (!defaultAdapterIp.empty()) {
        addRule({"PREROUTING", "-t", "mangle", "-d", defaultAdapterIp.c_str(), "-j", "CONNMARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag}, true);
        addRule({"OUTPUT", "-t", "mangle", "-j", "CONNMARK", "--restore-mark", "-m", "comment", "--comment", kTag}, true);
    }
    if (!defaultAdapterIpV6.empty()) {
        // connmark itself is family-agnostic at the kernel level, but the per-family
        // PREROUTING -d match has to live in the matching ip6tables table so v6
        // ingress on the physical adapter sets the mark.
        addRule({"PREROUTING", "-t", "mangle", "-d", defaultAdapterIpV6.c_str(), "-j", "CONNMARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag}, true, /*isV6=*/true);
        addRule({"OUTPUT", "-t", "mangle", "-j", "CONNMARK", "--restore-mark", "-m", "comment", "--comment", kTag}, true, /*isV6=*/true);
    }
}

void FirewallController::setSplitTunnelAppExceptions()
{
    if (!connected_ || !splitTunnelEnabled_) {
        removeExclusiveAppRules();
        removeInclusiveAppRules();
        return;
    }

    if (splitTunnelExclude_) {
        removeInclusiveAppRules();

        // The cgroup match works for both v4 and v6 packets (single net_cls cgroup),
        // so MARK/MASQUERADE/ACCEPT mirror unchanged into ip6tables. MASQUERADE
        // requires nf_nat_ipv6 (built into nf_nat on modern kernels); on hosts with
        // no v6 connectivity the v6 rule is a harmless no-op.
        addRule({"POSTROUTING",  "-t", "nat", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-o", defaultAdapter_.c_str(), "-j", "MASQUERADE", "-m", "comment", "--comment", kTag}, true);
        addRule({"POSTROUTING",  "-t", "nat", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-o", defaultAdapter_.c_str(), "-j", "MASQUERADE", "-m", "comment", "--comment", kTag}, true, /*isV6=*/true);
        addRule({"OUTPUT", "-t", "mangle", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "MARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag}, false);
        addRule({"OUTPUT", "-t", "mangle", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "MARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag}, false, /*isV6=*/true);

        // allow packets from excluded apps, if firewall is on
        if (enabled()) {
            addRule({WS_PRODUCT_NAME_LOWER "_input", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
            addRule({WS_PRODUCT_NAME_LOWER "_output", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
            addRule({WS_PRODUCT_NAME_LOWER "_input", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, false, /*isV6=*/true);
            addRule({WS_PRODUCT_NAME_LOWER "_output", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, false, /*isV6=*/true);
        }
    } else {
        removeExclusiveAppRules();

        addRule({"POSTROUTING", "-t", "nat", "-m", "cgroup", "!", "--cgroup", CGroups::instance().netClassId(), "-o", defaultAdapter_.c_str(), "-j", "MASQUERADE", "-m", "comment", "--comment", kTag}, true);
        addRule({"POSTROUTING", "-t", "nat", "-m", "cgroup", "!", "--cgroup", CGroups::instance().netClassId(), "-o", defaultAdapter_.c_str(), "-j", "MASQUERADE", "-m", "comment", "--comment", kTag}, true, /*isV6=*/true);
        addRule({"OUTPUT", "-t", "mangle", "-m", "cgroup", "!", "--cgroup", CGroups::instance().netClassId(), "-j", "MARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag}, false);
        addRule({"OUTPUT", "-t", "mangle", "-m", "cgroup", "!", "--cgroup", CGroups::instance().netClassId(), "-j", "MARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag}, false, /*isV6=*/true);

        // For inclusive, allow all packets
        if (enabled()) {
            addRule({WS_PRODUCT_NAME_LOWER "_input", "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
            addRule({WS_PRODUCT_NAME_LOWER "_output", "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
            addRule({WS_PRODUCT_NAME_LOWER "_input", "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, false, /*isV6=*/true);
            addRule({WS_PRODUCT_NAME_LOWER "_output", "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, false, /*isV6=*/true);
        }
    }
}

void FirewallController::setSplitTunnelIpExceptions(const std::vector<types::IpAddressRange> &ips)
{
    if (!connected_ || !splitTunnelEnabled_ || !enabled()) {
        removeInclusiveIpRules();
        removeExclusiveIpRules();
        splitTunnelIps_ = ips;
        return;
    }

    // Otherwise, split tunneling is still enabled.
    if (splitTunnelExclude_) {
        removeInclusiveIpRules();

        // For exclusive, remove rules for addresses no longer in "ips" (per-family).
        for (const auto &ip : splitTunnelIps_) {
            if (!ip.isValid()) {
                continue;
            }
            if (std::find(ips.begin(), ips.end(), ip) == ips.end()) {
                const char *tool = ip.isV6() ? "ip6tables" : "iptables";
                const std::string ipStr = ip.toString();
                Utils::executeCommand(tool, {"-D", WS_PRODUCT_NAME_LOWER "_input", "-s", ipStr.c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
                Utils::executeCommand(tool, {"-D", WS_PRODUCT_NAME_LOWER "_output", "-d", ipStr.c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
            }
        }

        // Add rules for IPs in the new list, dispatching to iptables / ip6tables
        // by family. Invalid entries (shouldn't happen — caller filters) are skipped.
        for (const auto &ip : ips) {
            if (!ip.isValid()) {
                continue;
            }
            const std::string ipStr = ip.toString();
            const bool isV6 = ip.isV6();
            addRule({WS_PRODUCT_NAME_LOWER "_input", "-s", ipStr.c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, false, isV6);
            addRule({WS_PRODUCT_NAME_LOWER "_output", "-d", ipStr.c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, false, isV6);
        }
    } else {
        removeExclusiveIpRules();

        // For inclusive, keep the "allow all" rules; these rules only apply to non-included apps.
        // Mirror to ip6tables so v6 traffic on the windscribe_input/_output chains is also let
        // through for non-included flows.
        addRule({WS_PRODUCT_NAME_LOWER "_input", "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
        addRule({WS_PRODUCT_NAME_LOWER "_output", "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
        addRule({WS_PRODUCT_NAME_LOWER "_input", "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, false, /*isV6=*/true);
        addRule({WS_PRODUCT_NAME_LOWER "_output", "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, false, /*isV6=*/true);
    }

    splitTunnelIps_ = ips;
}

void FirewallController::addRule(const std::vector<std::string> &args, bool prepend, bool isV6)
{
    const char *tool = isV6 ? "ip6tables" : "iptables";
    std::vector<std::string> checkArgs = args;
    checkArgs.insert(checkArgs.begin(), "-C");
    int ret = Utils::executeCommand(tool, checkArgs);
    if (ret) {
        std::vector<std::string> insertArgs = args;
        if (prepend) {
            insertArgs.insert(insertArgs.begin(), "-I");
        } else {
            insertArgs.insert(insertArgs.begin(), "-A");
        }
        Utils::executeCommand(tool, insertArgs);
    }
}
