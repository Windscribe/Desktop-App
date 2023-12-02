#include "firewallcontroller.h"

#include <algorithm>
#include <fcntl.h>
#include <fstream>
#include <sstream>

#include "logger.h"
#include "utils.h"

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
    } else {
        int bytes = write(fd, rules.c_str(), rules.length());
        close(fd);
        if (bytes <= 0) {
            Logger::instance().out("Could not write rules");
            return 1;
        } else {
            if (ipv6) {
                Utils::executeCommand("ip6tables-restore", {"-n", "/etc/windscribe/rules.v6"});
            } else {
                Utils::executeCommand("iptables-restore", {"-n", "/etc/windscribe/rules.v4"});
            }
            enabled_ = true;

            // reapply split tunneling rules if necessary
            setSplitTunnelExceptions(splitTunnelIps_);

            return 0;
        }
    }
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

    enabled_ = false;
}


void FirewallController::setSplitTunnelingEnabled(bool isConnected, bool isEnabled, bool isExclude)
{
    connected_ = isConnected;
    splitTunnelEnabled_ = isEnabled;
    splitTunnelExclude_ = isExclude;

    // If firewall is on, apply rules immediately
    if (enabled_) {
        setSplitTunnelExceptions(splitTunnelIps_);
    }
}

void FirewallController::removeExclusiveRules()
{
    for (auto ip : splitTunnelIps_) {
        Logger::instance().out("Removing exclude rule for %s",  ip.c_str());
        Utils::executeCommand("iptables", {"-D", "windscribe_input", "-s", (ip + "/32").c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", "Windscribe client rule"});
        Utils::executeCommand("iptables", {"-D", "windscribe_output", "-d", (ip + "/32").c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", "Windscribe client rule"});
    }
}

void FirewallController::removeInclusiveRules()
{
    Utils::executeCommand("iptables", {"-D", "windscribe_input", "-s", "0/0", "-j", "ACCEPT", "-m", "comment", "--comment", "Windscribe client rule"});
    Utils::executeCommand("iptables", {"-D", "windscribe_output", "-d", "0/0", "-j", "ACCEPT", "-m", "comment", "--comment", "Windscribe client rule"});
}

void FirewallController::setSplitTunnelExceptions(const std::vector<std::string> &ips)
{
    if (!connected_ || !splitTunnelEnabled_) {
        removeInclusiveRules();
        removeExclusiveRules();
        splitTunnelIps_ = ips;
        return;
    }

    // Otherwise, split tunneling is still enabled.
    if (splitTunnelExclude_) {
        removeInclusiveRules();
        // For exclusive, remove rules for addresses no longer in "ips"
        for (auto ip : splitTunnelIps_) {
            if (std::find(ips.begin(), ips.end(), ip) == ips.end()) {
                Logger::instance().out("Removing old exclude rule for %s", ip.c_str());
                Utils::executeCommand("iptables", {"-D", "windscribe_input", "-s", (ip + "/32").c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", "Windscribe client rule"});
                Utils::executeCommand("iptables", {"-D", "windscribe_output", "-d", (ip + "/32").c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", "Windscribe client rule"});
            }
        }
        // Add rules for new IPs
        for (auto ip : ips) {
			Logger::instance().out("Adding exclude rule for %s", ip.c_str());
			int ret = Utils::executeCommand("iptables", {"-C", "windscribe_input", "-s", (ip + "/32").c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", "Windscribe client rule"});
			if (ret) {
				Utils::executeCommand("iptables", {"-I", "windscribe_input", "-s", (ip + "/32").c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", "Windscribe client rule"});
			}
			ret = Utils::executeCommand("iptables", {"-C", "windscribe_output", "-d", (ip + "/32").c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", "Windscribe client rule"});
			if (ret) {
				ret = Utils::executeCommand("iptables", {"-I", "windscribe_output", "-d", (ip + "/32").c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", "Windscribe client rule"});
            }
        }
    } else {
        removeExclusiveRules();
        // For inclusive, keep the "allow all" rules
        int ret = Utils::executeCommand("iptables", {"-C", "windscribe_input", "-s", "0/0", "-j", "ACCEPT", "-m", "comment", "--comment", "Windscribe client rule"});
        if (ret) {
            Logger::instance().out("Adding inclusive input rule");
            Utils::executeCommand("iptables", {"-I", "windscribe_input", "-s", "0/0", "-j", "ACCEPT", "-m", "comment", "--comment", "Windscribe client rule"});
        }
        ret = Utils::executeCommand("iptables", {"-C", "windscribe_output", "-d", "0/0", "-j", "ACCEPT", "-m", "comment", "--comment", "Windscribe client rule"});
        if (ret) {
            Logger::instance().out("Adding inclusive output rule");
            Utils::executeCommand("iptables", {"-I", "windscribe_output", "-d", "0/0", "-j", "ACCEPT", "-m", "comment", "--comment", "Windscribe client rule"});
        }
    }
    splitTunnelIps_ = ips;
}
