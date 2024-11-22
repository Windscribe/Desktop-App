#include "firewallcontroller.h"

#include <algorithm>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <spdlog/spdlog.h>

#include "split_tunneling/cgroups.h"
#include "utils.h"

FirewallController::FirewallController() : connected_(false), splitTunnelEnabled_(false), splitTunnelExclude_(true)
{
    // If firewall on boot is enabled, restore boot rules
    if (Utils::isFileExists("/etc/windscribe/boot_rules.v4")) {
        Utils::executeCommand("iptables-restore", {"-n", "/etc/windscribe/boot_rules.v4"});
    }
    if (Utils::isFileExists("/etc/windscribe/boot_rules.v6")) {
        Utils::executeCommand("ip6tables-restore", {"-n", "/etc/windscribe/boot_rules.v6"});
    }
}

FirewallController::~FirewallController()
{
}

bool FirewallController::enable(bool ipv6, const std::string &rules)
{
    int fd;

    if (ipv6) {
        fd = open("/etc/windscribe/rules.v6", O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRGRP | S_IROTH);
    } else {
        fd = open("/etc/windscribe/rules.v4", O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRGRP | S_IROTH);
    }

    if (fd < 0) {
        spdlog::error("Could not open firewall rules for writing");
        return 1;
    }

    int bytes = write(fd, rules.c_str(), rules.length());
    close(fd);
    if (bytes <= 0) {
        spdlog::error("Could not write rules");
        return 1;
    }

    if (ipv6) {
        Utils::executeCommand("ip6tables-restore", {"-n", "/etc/windscribe/rules.v6"});
    } else {
        Utils::executeCommand("iptables-restore", {"-n", "/etc/windscribe/rules.v4"});
    }

    // reapply split tunneling rules if necessary
    setSplitTunnelIpExceptions(splitTunnelIps_);
    setSplitTunnelAppExceptions();
    setSplitTunnelIngressRules(defaultAdapterIp_);

    return 0;
}

void FirewallController::getRules(bool ipv6, std::string *outRules)
{
    std::string filename;

    if (ipv6) {
        filename = "/etc/windscribe/rules.v6";
        Utils::executeCommand("ip6tables-save", {"-f", filename.c_str()});
    } else {
        filename = "/etc/windscribe/rules.v4";
        Utils::executeCommand("iptables-save", {"-f", filename.c_str()});
    }

    std::ifstream ifs(filename.c_str());
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    *outRules = buffer.str();
}

bool FirewallController::enabled(const std::string &tag)
{
    return Utils::executeCommand("iptables", {"--check", "INPUT", "-j", "windscribe_input", "-m", "comment", "--comment", tag.c_str()}) == 0;
}

void FirewallController::disable()
{
    Utils::executeCommand("rm", {"-f", "/etc/windscribe/rules.v4"});
    Utils::executeCommand("rm", {"-f", "/etc/windscribe/rules.v6"});
}

void FirewallController::setSplitTunnelingEnabled(bool isConnected, bool isEnabled, bool isExclude, const std::string &defaultAdapter, const std::string &defaultAdapterIp)
{
    connected_ = isConnected;
    splitTunnelEnabled_ = isEnabled;
    splitTunnelExclude_ = isExclude;
    prevAdapter_ = defaultAdapter_;
    defaultAdapter_ = defaultAdapter;
    defaultAdapterIp_ = defaultAdapterIp;

    setSplitTunnelIpExceptions(splitTunnelIps_);
    setSplitTunnelAppExceptions();
    setSplitTunnelIngressRules(defaultAdapterIp_);
}

void FirewallController::removeExclusiveIpRules()
{
    for (auto ip : splitTunnelIps_) {
        removeRule({"windscribe_input", "-s", ip.c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, Utils::isValidIpv6Address(ip));
        removeRule({"windscribe_output", "-d", ip.c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, Utils::isValidIpv6Address(ip));
    }
}

void FirewallController::removeInclusiveIpRules()
{
    // v4
    removeRule({"windscribe_input", "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
    removeRule({"windscribe_output", "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
    // v6
    removeRule({"windscribe_input", "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, true);
    removeRule({"windscribe_output", "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, true);
}

void FirewallController::removeExclusiveAppRules()
{
    // v4
    removeRule({"OUTPUT", "-t", "mangle", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "MARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag});
    if (!prevAdapter_.empty()) {
        removeRule({"POSTROUTING", "-t", "nat", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-o", prevAdapter_.c_str(), "-j", "MASQUERADE", "-m", "comment", "--comment", kTag});
    }
    removeRule({"windscribe_input", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
    removeRule({"windscribe_output", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag});

    // v6
    removeRule({"OUTPUT", "-t", "mangle", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "MARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag}, true);
    if (!prevAdapter_.empty()) {
        removeRule({"POSTROUTING", "-t", "nat", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-o", prevAdapter_.c_str(), "-j", "MASQUERADE", "-m", "comment", "--comment", kTag}, true);
    }
    removeRule({"windscribe_input", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, true);
    removeRule({"windscribe_output", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, true);
}

void FirewallController::removeInclusiveAppRules()
{
    // v4
    removeRule({"OUTPUT", "-t", "mangle", "-m", "cgroup", "!", "--cgroup", CGroups::instance().netClassId(), "-j", "MARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag});
    if (!prevAdapter_.empty()) {
        removeRule({"POSTROUTING", "-t", "nat", "-m", "cgroup", "!", "--cgroup", CGroups::instance().netClassId(), "-o", prevAdapter_.c_str(), "-j", "MASQUERADE", "-m", "comment", "--comment", kTag});
    }

    // v6
    removeRule({"OUTPUT", "-t", "mangle", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "DROP", "-m", "comment", "--comment", kTag}, true);
}

void FirewallController::setSplitTunnelIngressRules(const std::string &defaultAdapterIp)
{
    if (defaultAdapterIp.empty()) {
        return;
    }

    if (!connected_ || !splitTunnelEnabled_ || splitTunnelExclude_) {
        // v4
        removeRule({"PREROUTING", "-t", "mangle", "-d", defaultAdapterIp.c_str(), "-j", "CONNMARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag});
        removeRule({"OUTPUT", "-t", "mangle", "-j", "CONNMARK", "--restore-mark", "-m", "comment", "--comment", kTag});
        // v6
        removeRule({"PREROUTING", "-t", "mangle", "-d", defaultAdapterIp.c_str(), "-j", "CONNMARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag}, true);
        removeRule({"OUTPUT", "-t", "mangle", "-j", "CONNMARK", "--restore-mark", "-m", "comment", "--comment", kTag}, true);
        return;
    }

    // v4
    addRule({"PREROUTING", "-t", "mangle", "-d", defaultAdapterIp.c_str(), "-j", "CONNMARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag});
    addRule({"OUTPUT", "-t", "mangle", "-j", "CONNMARK", "--restore-mark", "-m", "comment", "--comment", kTag});
    // v6
    addRule({"PREROUTING", "-t", "mangle", "-d", defaultAdapterIp.c_str(), "-j", "CONNMARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag}, true);
    addRule({"OUTPUT", "-t", "mangle", "-j", "CONNMARK", "--restore-mark", "-m", "comment", "--comment", kTag}, true);
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

        // v4
        addRule({"POSTROUTING",  "-t", "nat", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-o", defaultAdapter_.c_str(), "-j", "MASQUERADE", "-m", "comment", "--comment", kTag});
        addRule({"OUTPUT", "-t", "mangle", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "MARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag}, false, true);

        // v6
        addRule({"POSTROUTING",  "-t", "nat", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-o", defaultAdapter_.c_str(), "-j", "MASQUERADE", "-m", "comment", "--comment", kTag}, true);
        addRule({"OUTPUT", "-t", "mangle", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "MARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag}, true, true);

        // allow packets from excluded apps, if firewall is on
        if (enabled()) {
            // v4
            addRule({"windscribe_input", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
            addRule({"windscribe_output", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag});

            // v6
            addRule({"windscribe_input", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, true);
            addRule({"windscribe_output", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, true);
        }
    } else {
        removeExclusiveAppRules();

        // v4
        addRule({"POSTROUTING", "-t", "nat", "-m", "cgroup", "!", "--cgroup", CGroups::instance().netClassId(), "-o", defaultAdapter_.c_str(), "-j", "MASQUERADE", "-m", "comment", "--comment", kTag});
        addRule({"OUTPUT", "-t", "mangle", "-m", "cgroup", "!", "--cgroup", CGroups::instance().netClassId(), "-j", "MARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag}, false, true);

        // v6 -- We can't route IPv6 traffic into the v4 tunnel, so we drop IPv6 traffic for included apps
        addRule({"OUTPUT", "-t", "mangle", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "DROP", "-m", "comment", "--comment", kTag}, true, true);

        // For inclusive, allow all packets
        if (enabled()) {
            // v4
            addRule({"windscribe_input", "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
            addRule({"windscribe_output", "-j", "ACCEPT", "-m", "comment", "--comment", kTag});

            // v6
            addRule({"windscribe_input", "-j", "DROP", "-m", "comment", "--comment", kTag}, true);
            addRule({"windscribe_output", "-j", "DROP", "-m", "comment", "--comment", kTag}, true);
        }
    }
}

void FirewallController::setSplitTunnelIpExceptions(const std::vector<std::string> &ips)
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

        // Remove rules for addresses no longer in "ips"
        for (auto ip : splitTunnelIps_) {
            if (std::find(ips.begin(), ips.end(), ip) == ips.end()) {
                removeRule({"windscribe_input", "-s", ip.c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, Utils::isValidIpv6Address(ip));
                removeRule({"windscribe_output", "-d", ip.c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, Utils::isValidIpv6Address(ip));
            }
        }

        // Add rules for new IPs
        for (auto ip : ips) {
            addRule({"windscribe_input", "-s", ip.c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, Utils::isValidIpv6Address(ip));
            addRule({"windscribe_output", "-d", ip.c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, Utils::isValidIpv6Address(ip));
        }
    } else {
        removeExclusiveIpRules();

        // For inclusive, keep the "allow all" rules; these rules only apply to non-included apps
        // v4
        addRule({"windscribe_input", "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
        addRule({"windscribe_output", "-j", "ACCEPT", "-m", "comment", "--comment", kTag});

        // v6
        addRule({"windscribe_input", "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, true);
        addRule({"windscribe_output", "-j", "ACCEPT", "-m", "comment", "--comment", kTag}, true);

        // Remove rules for addresses no longer in "ips"
        for (auto ip : splitTunnelIps_) {
            if (Utils::isValidIpv6Address(ip)) {
                if (std::find(ips.begin(), ips.end(), ip) == ips.end()) {
                    removeRule({"windscribe_input", "-s", ip.c_str(), "-j", "DROP", "-m", "comment", "--comment", kTag}, true);
                    removeRule({"windscribe_output", "-d", ip.c_str(), "-j", "DROP", "-m", "comment", "--comment", kTag}, true);
                }
            }
        }

        for (auto ip : ips) {
            if (Utils::isValidIpv6Address(ip)) {
                addRule({"windscribe_input", "-s", ip.c_str(), "-j", "DROP", "-m", "comment", "--comment", kTag}, true);
                addRule({"windscribe_output", "-d", ip.c_str(), "-j", "DROP", "-m", "comment", "--comment", kTag}, true);
            }
        }
    }

    splitTunnelIps_ = ips;
}

void FirewallController::addRule(const std::vector<std::string> &args, bool ipv6, bool append)
{
    std::vector<std::string> checkArgs = args;
    checkArgs.insert(checkArgs.begin(), "-C");
    int ret = Utils::executeCommand(ipv6 ? "ip6tables" : "iptables", checkArgs);
    if (ret) {
        std::vector<std::string> insertArgs = args;
        insertArgs.insert(insertArgs.begin(), append ? "-A" : "-I");
        Utils::executeCommand(ipv6 ? "ip6tables" : "iptables", insertArgs);
    }
}

void FirewallController::removeRule(const std::vector<std::string> &args, bool ipv6)
{
    std::vector<std::string> deleteArgs = args;
    deleteArgs.insert(deleteArgs.begin(), "-D");
    Utils::executeCommand(ipv6 ? "ip6tables" : "iptables", deleteArgs);
}
