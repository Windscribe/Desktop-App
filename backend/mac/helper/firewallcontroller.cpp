#include "firewallcontroller.h"

#include <fcntl.h>
#include <unistd.h>

#include "logger.h"
#include "utils.h"

bool FirewallController::enable(const std::string &rules, const std::string &table, const std::string &group)
{
    int fd = open("/etc/windscribe/pf.conf", O_CREAT | O_WRONLY | O_TRUNC);
    if (fd < 0) {
        LOG("Could not open firewall rules for writing");
        return false;
    }

    write(fd, rules.c_str(), rules.length());
    close(fd);

    if (table.empty() && group.empty()) {
        Utils::executeCommand("pfctl", {"-v", "-F", "all", "-f", "/etc/windscribe/pf.conf"});
        Utils::executeCommand("pfctl", {"-e"});

        // reapply split tunneling rules if necessary
        setSplitTunnelExceptions(splitTunnelIps_);
    } else if (!table.empty()) {
        Utils::executeCommand("pfctl", {"-T", "load", "-f", "/etc/windscribe/pf.conf"});
    } else if (!group.empty()) {
        Utils::executeCommand("pfctl", {"-a", group.c_str(), "-f", "/etc/windscribe/pf.conf"});
    }

    enabled_ = true;
    return true;
}

void FirewallController::getRules(const std::string &table, const std::string &group, std::string *outRules)
{
    if (table.empty() && group.empty()) {
        Utils::executeCommand("pfctl", {"-s", "rules"}, outRules, false);
    } else if (!table.empty()) {
        Utils::executeCommand("pfctl", {"-t", table.c_str(), "-T", "show"}, outRules, false);
    } else if (!group.empty()) {
        Utils::executeCommand("pfctl", {"-a", group.c_str(), "-s", "rules"}, outRules, false);
    }
}
 
void FirewallController::disable(bool keepPfEnabled)
{
    Utils::executeCommand("pfctl", {"-v", "-F", "all", "-f", "/etc/pf.conf"});
    if (!keepPfEnabled) {
        Utils::executeCommand("pfctl", {"-d"});
    }
    enabled_ = false;
}

bool FirewallController::enabled()
{
    std::string output;

    Utils::executeCommand("pfctl", {"-si"}, &output);
    return (output.find("Status: Enabled") != std::string::npos);
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
    splitTunnelIps_ = ips;

    std::string ipStr = "table <windscribe_split_tunnel_ips> persist { ";

    if (!splitTunnelEnabled_) {
        // If split tunneling is disabled, treat this table as if there are no IPs
        ipStr += " }\n";
    } else if (splitTunnelExclude_) {
        for (const auto ip : ips) {
            ipStr += ip + " ";
        } 
        ipStr += " }\n";
    } else {
        // For inclusive tunnel, ignore the ips and apply a rule to allow any traffic
        // NB: pfctl tables do not allow 0/0 so we split it into two
        ipStr += " 0/1 128/1 }\n";
    }

    int fd = open("/etc/windscribe/pf_st.conf", O_CREAT | O_WRONLY | O_TRUNC);
    if (fd < 0) {
        LOG("Could not open firewall rules for writing");
        return;
    }

    write(fd, ipStr.c_str(), ipStr.length());
    close(fd);

    Utils::executeCommand("pfctl", {"-T", "load", "-f", "/etc/windscribe/pf_st.conf"});
    Utils::executeCommand("rm", {"/etc/windscribe/pf_st.conf"});
}