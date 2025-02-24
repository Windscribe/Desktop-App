#include "firewallcontroller.h"

#include <fcntl.h>
#include <unistd.h>
#include <spdlog/spdlog.h>

#include "utils.h"

FirewallController::FirewallController() : enabled_(false)
{
    // If firewall on boot is enabled, restore boot rules
    if (Utils::isFileExists("/etc/windscribe/boot_pf.conf")) {
        spdlog::info("Applying boot pfctl rules");
        Utils::executeCommand("/sbin/pfctl", {"-e", "-f", "/etc/windscribe/boot_pf.conf"});
    }
}

FirewallController::~FirewallController()
{
}

bool FirewallController::enable(const std::string &rules, const std::string &table, const std::string &group)
{
    int fd = open("/etc/windscribe/pf.conf", O_CREAT | O_WRONLY | O_TRUNC);
    if (fd < 0) {
        spdlog::error("Could not open firewall rules for writing");
        return false;
    }

    write(fd, rules.c_str(), rules.length());
    close(fd);

    enabled_ = true;

    if (table.empty() && group.empty()) {
        Utils::executeCommand("/sbin/pfctl", {"-v", "-F", "all", "-f", "/etc/windscribe/pf.conf"});
        Utils::executeCommand("/sbin/pfctl", {"-e"});
        updateDns();
    } else if (!table.empty()) {
        Utils::executeCommand("/sbin/pfctl", {"-T", "load", "-f", "/etc/windscribe/pf.conf"});
    } else if (!group.empty()) {
        Utils::executeCommand("/sbin/pfctl", {"-a", group.c_str(), "-f", "/etc/windscribe/pf.conf"});
    }

    return true;
}

void FirewallController::getRules(const std::string &table, const std::string &group, std::string *outRules)
{
    if (table.empty() && group.empty()) {
        Utils::executeCommand("/sbin/pfctl", {"-s", "rules"}, outRules, false);
    } else if (!table.empty()) {
        Utils::executeCommand("/sbin/pfctl", {"-t", table.c_str(), "-T", "show"}, outRules, false);
    } else if (!group.empty()) {
        Utils::executeCommand("/sbin/pfctl", {"-a", group.c_str(), "-s", "rules"}, outRules, false);
    }
}

void FirewallController::disable(bool keepPfEnabled)
{
    Utils::executeCommand("/sbin/pfctl", {"-v", "-F", "all", "-f", "/etc/pf.conf"});
    if (!keepPfEnabled) {
        Utils::executeCommand("/sbin/pfctl", {"-d"});
    }
    enabled_ = false;
}

bool FirewallController::enabled()
{
    std::string output;

    Utils::executeCommand("/sbin/pfctl", {"-si"}, &output);
    return (output.find("Status: Enabled") != std::string::npos);
}

void FirewallController::setVpnDns(const std::string &dns)
{
    spdlog::info("Setting VPN dns: {}", dns.empty() ? "empty" : dns);
    windscribeDns_ = dns;
    updateDns();
}

void FirewallController::updateDns()
{
    if (enabled_ && !windscribeDns_.empty()) {
        std::string rules = "table <windscribe_dns> persist { " + windscribeDns_ + "}\n";
        enable(rules, "windscribe_dns", "");
    } else {
        Utils::executeCommand("/sbin/pfctl", {"-T", "windscribe_dns", "flush"});
    }
}
