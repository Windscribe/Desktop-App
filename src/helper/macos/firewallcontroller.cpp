#include "firewallcontroller.h"

#include <fcntl.h>
#include <unistd.h>
#include <spdlog/spdlog.h>

#include "../common/io_posix.h"
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
    // open()'s mode is ignored on pre-existing files; unlink first to force the intended perms
    unlink(WS_POSIX_CONFIG_DIR "/pf.conf");
    int fd = open(WS_POSIX_CONFIG_DIR "/pf.conf", O_CREAT | O_WRONLY | O_TRUNC, 0600);
    if (fd < 0) {
        spdlog::error("Could not open firewall rules for writing");
        return false;
    }

    if (!IO::writeAll(fd, rules)) {
        spdlog::error("Could not write rules: {}", IO::strerror(errno));
        close(fd);
        unlink(WS_POSIX_CONFIG_DIR "/pf.conf");
        return false;
    }
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

void FirewallController::setVpnDns(const std::vector<types::IpAddress> &dnsList)
{
    // Drop invalid entries (deserialised IpAddress with valid_ == false) so the pf table
    // never ends up with stray spaces or empty tokens. Stay in types::IpAddress form here —
    // string conversion happens once at the pf-config boundary in updateDns(), matching the
    // helper-internal convention from routes_manager / bound_route.
    vpnDns_.clear();
    vpnDns_.reserve(dnsList.size());
    for (const auto &dns : dnsList) {
        if (dns.isValid()) {
            vpnDns_.push_back(dns);
        }
    }

    if (vpnDns_.empty()) {
        spdlog::info("Setting VPN dns: empty");
    } else {
        std::string joined;
        for (const auto &dns : vpnDns_) {
            if (!joined.empty()) joined += ", ";
            joined += dns.toString();
        }
        spdlog::info("Setting VPN dns: {}", joined);
    }

    updateDns();
}

void FirewallController::updateDns()
{
    if (enabled_ && !vpnDns_.empty()) {
        // pf tables accept space-separated dual-stack entries inside `{ ... }`, e.g.
        // `table <windscribe_dns> persist { 10.255.255.1 fd00:abcd::1 }`. The pass/block
        // rules referring to <windscribe_dns> in firewallcontroller_mac.cpp are family-agnostic
        // (no `inet`/`inet6` selector), so a mixed table covers both v4 and v6 DNS lookups.
        std::string entries;
        for (const auto &dns : vpnDns_) {
            if (!entries.empty()) entries += " ";
            entries += dns.toString();
        }
        std::string rules = "table <" WS_PRODUCT_NAME_LOWER "_dns> persist { " + entries + " }\n";
        enable(rules, WS_PRODUCT_NAME_LOWER "_dns", "");
    } else {
        Utils::executeCommand("/sbin/pfctl", {"-T", WS_PRODUCT_NAME_LOWER "_dns", "flush"});
    }
}
