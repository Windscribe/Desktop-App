#include "firewallcontroller.h"

#include <algorithm>
#include <fcntl.h>
#include <fstream>
#include <sstream>

#include "split_tunneling/cgroups.h"
#include "logger.h"
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
        Logger::instance().out("Could not open firewall rules for writing");
        return 1;
    }

    int bytes = write(fd, rules.c_str(), rules.length());
    close(fd);
    if (bytes <= 0) {
        Logger::instance().out("Could not write rules");
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
        Utils::executeCommand("iptables", {"-D", "windscribe_input", "-s", ip.c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
        Utils::executeCommand("iptables", {"-D", "windscribe_output", "-d", ip.c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
    }
}

void FirewallController::removeInclusiveIpRules()
{
    Utils::executeCommand("iptables", {"-D", "windscribe_input", "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
    Utils::executeCommand("iptables", {"-D", "windscribe_output", "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
}

void FirewallController::removeExclusiveAppRules()
{
    Utils::executeCommand("iptables", {"-D", "OUTPUT", "-t", "mangle", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "MARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag});
    if (!prevAdapter_.empty()) {
        Utils::executeCommand("iptables", {"-D", "POSTROUTING", "-t", "nat", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-o", prevAdapter_.c_str(), "-j", "MASQUERADE", "-m", "comment", "--comment", kTag});
    }

    Utils::executeCommand("iptables", {"-D", "windscribe_input", "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
    Utils::executeCommand("iptables", {"-D", "windscribe_output", "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
}

void FirewallController::removeInclusiveAppRules()
{
    Utils::executeCommand("iptables", {"-D", "OUTPUT", "-t", "mangle", "-m", "cgroup", "!", "--cgroup", CGroups::instance().netClassId(), "-j", "MARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag});
    if (!prevAdapter_.empty()) {
        Utils::executeCommand("iptables", {"-D", "POSTROUTING", "-t", "nat", "-m", "cgroup", "!", "--cgroup", CGroups::instance().netClassId(), "-o", prevAdapter_.c_str(), "-j", "MASQUERADE", "-m", "comment", "--comment", kTag});
    }
}

void FirewallController::setSplitTunnelIngressRules(const std::string &defaultAdapterIp)
{
    if (!connected_ || !splitTunnelEnabled_ || splitTunnelExclude_) {
        Logger::instance().out("Deleting ingress rules");
        Utils::executeCommand("iptables", {"-D", "PREROUTING", "-t", "mangle", "-d", defaultAdapterIp.c_str(), "-j", "CONNMARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag});
        Utils::executeCommand("iptables", {"-D", "OUTPUT", "-t", "mangle", "-j", "CONNMARK", "--restore-mark", "-m", "comment", "--comment", kTag});
        return;
    }

	Logger::instance().out("Adding ingress rules");
	addRule({"PREROUTING", "-t", "mangle", "-d", defaultAdapterIp.c_str(), "-j", "CONNMARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag});
	addRule({"OUTPUT", "-t", "mangle", "-j", "CONNMARK", "--restore-mark", "-m", "comment", "--comment", kTag});
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

        addRule({"POSTROUTING",  "-t", "nat", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-o", defaultAdapter_.c_str(), "-j", "MASQUERADE", "-m", "comment", "--comment", kTag});
        addRule({"OUTPUT", "-t", "mangle", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "MARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag});

        // allow packets from excluded apps, if firewall is on
        if (enabled()) {
            addRule({"windscribe_input", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
            addRule({"windscribe_output", "-m", "cgroup", "--cgroup", CGroups::instance().netClassId(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
        }
    } else {
        removeExclusiveAppRules();

        addRule({"POSTROUTING", "-t", "nat", "-m", "cgroup", "!", "--cgroup", CGroups::instance().netClassId(), "-o", defaultAdapter_.c_str(), "-j", "MASQUERADE", "-m", "comment", "--comment", kTag});
        addRule({"OUTPUT", "-t", "mangle", "-m", "cgroup", "!", "--cgroup", CGroups::instance().netClassId(), "-j", "MARK", "--set-mark", CGroups::instance().mark(), "-m", "comment", "--comment", kTag});

        // For inclusive, allow all packets
        if (enabled()) {
            addRule({"windscribe_input", "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
            addRule({"windscribe_output", "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
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

        // For exclusive, remove rules for addresses no longer in "ips"
        for (auto ip : splitTunnelIps_) {
            if (std::find(ips.begin(), ips.end(), ip) == ips.end()) {
                Utils::executeCommand("iptables", {"-D", "windscribe_input", "-s", ip.c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
                Utils::executeCommand("iptables", {"-D", "windscribe_output", "-d", ip.c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
            }
        }

        // Add rules for new IPs
        for (auto ip : ips) {
            addRule({"windscribe_input", "-s", ip.c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
            addRule({"windscribe_output", "-d", ip.c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
        }
    } else {
        removeExclusiveIpRules();

        // For inclusive, keep the "allow all" rules; these rules only apply to non-included apps
        addRule({"windscribe_input", "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
        addRule({"windscribe_output", "-j", "ACCEPT", "-m", "comment", "--comment", kTag});
    }

    splitTunnelIps_ = ips;
}

void FirewallController::addRule(const std::vector<std::string> &args)
{
    std::vector<std::string> checkArgs = args;
    checkArgs.insert(checkArgs.begin(), "-C");
    int ret = Utils::executeCommand("iptables", checkArgs);
    if (ret) {
        std::vector<std::string> insertArgs = args;
        insertArgs.insert(insertArgs.begin(), "-I");
        Utils::executeCommand("iptables", insertArgs);
    }
}
