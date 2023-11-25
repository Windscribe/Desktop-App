#include "firewallcontroller.h"

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


void FirewallController::setSplitTunnelingEnabled(bool isEnabled, bool isExclude)
{
    splitTunnelEnabled_ = isEnabled;
    splitTunnelExclude_ = isExclude;

    // If firewall is on, apply rules immediately
    if (enabled_) {
        setSplitTunnelExceptions(splitTunnelIps_);
    }
}

void FirewallController::setSplitTunnelExceptions(const std::vector<std::string> &ips)
{
    if (splitTunnelExclude_) {
        for (auto ip : ips) {
            Utils::executeCommand("iptables", {"-I", "windscribe_input", "-s", (ip + "/32").c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", "Windscribe client rule"});
            Utils::executeCommand("iptables", {"-I", "windscribe_output", "-d", (ip + "/32").c_str(), "-j", "ACCEPT", "-m", "comment", "--comment", "Windscribe client rule"});
        }
    } else {
        // For inclusive mode allow any traffic
        Utils::executeCommand("iptables", {"-I", "windscribe_input", "-s", "0/0", "-j", "ACCEPT", "-m", "comment", "--comment", "Windscribe client rule"});
        Utils::executeCommand("iptables", {"-I", "windscribe_output", "-d", "0/0", "-j", "ACCEPT", "-m", "comment", "--comment", "Windscribe client rule"});
    }
}
