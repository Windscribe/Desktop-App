#include "firewallcontroller.h"

#include <fcntl.h>
#include <unistd.h>
#include <spdlog/spdlog.h>

#include "utils.h"

FirewallController::FirewallController() : enabled_(false)
{
    // If firewall on boot is enabled, restore boot rules
    if (Utils::isFileExists(WS_POSIX_CONFIG_DIR "/boot_pf.conf")) {
        spdlog::info("Applying boot pfctl rules");
        Utils::executeCommand("/sbin/pfctl", {"-e", "-f", WS_POSIX_CONFIG_DIR "/boot_pf.conf"});
    }
}

FirewallController::~FirewallController()
{
}

bool FirewallController::enable(const std::string &rules, const std::string &table, const std::string &group)
{
    int fd = open(WS_POSIX_CONFIG_DIR "/pf.conf", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        spdlog::error("Could not open firewall rules for writing");
        return false;
    }

    write(fd, rules.c_str(), rules.length());
    close(fd);

    enabled_ = true;

    if (table.empty() && group.empty()) {
        Utils::executeCommand("/sbin/pfctl", {"-v", "-F", "all", "-f", WS_POSIX_CONFIG_DIR "/pf.conf"});
        Utils::executeCommand("/sbin/pfctl", {"-e"});
        updateDns();
    } else if (!table.empty()) {
        Utils::executeCommand("/sbin/pfctl", {"-T", "load", "-f", WS_POSIX_CONFIG_DIR "/pf.conf"});
    } else if (!group.empty()) {
        Utils::executeCommand("/sbin/pfctl", {"-a", group.c_str(), "-f", WS_POSIX_CONFIG_DIR "/pf.conf"});
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
    vpnDns_ = dns;
    updateDns();
}

void FirewallController::updateDns()
{
    if (enabled_ && !vpnDns_.empty()) {
        std::string rules = "table <" WS_PRODUCT_NAME_LOWER "_dns> persist { " + vpnDns_ + "}\n";
        enable(rules, WS_PRODUCT_NAME_LOWER "_dns", "");
    } else {
        Utils::executeCommand("/sbin/pfctl", {"-T", WS_PRODUCT_NAME_LOWER "_dns", "flush"});
    }
}
