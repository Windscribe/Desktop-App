#include "wireguardadapter.h"
#include "../../common/helper_commands.h"
#include "../../common/io_posix.h"
#include "../utils.h"
#include "types/ipaddress.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <fcntl.h>
#include <filesystem>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <spdlog/spdlog.h>

namespace
{

struct CmdEntry {
    std::string program;
    std::vector<std::string> args;
};

bool RunBlockingCommands(const std::vector<CmdEntry> &cmdlist)
{
    std::string output;
    for (const auto &cmd : cmdlist) {
        output.clear();
        const auto status = Utils::executeCommand(cmd.program, cmd.args, &output);
        if (!output.empty())
            spdlog::debug(output);
        if (status != 0) {
            spdlog::error("Failed to run command: \"{}\" (exit status {})", cmd.program, status);
            return false;
        }
    }
    return true;
}

// Build the parallel *raw / *mangle rule block written to (ip6)tables-restore.
// The same rule shape applies to both families: drop spoofed inbound, save/restore
// the fwmark on UDP for the WG endpoint connection.
std::vector<std::string> buildFirewallRuleLines(const std::string &iface,
                                                const std::string &cidr,
                                                uint32_t fwmark,
                                                const std::string &comment)
{
    // Comment is wrapped in literal double-quotes so iptables-restore -n treats
    // the whole phrase (which contains spaces) as a single argument. Matches
    // the style used by firewallonboot.cpp / firewallcontroller.cpp.
    const std::string quotedComment = "\"" + comment + "\"";

    std::vector<std::string> lines;
    lines.push_back("*raw");
    lines.push_back("-I PREROUTING ! -i " + iface + " -d " + cidr +
                    " -m addrtype ! --src-type LOCAL -j DROP -m comment --comment " + quotedComment);
    lines.push_back("COMMIT");
    lines.push_back("*mangle");
    lines.push_back("-I POSTROUTING -m mark --mark " + std::to_string(fwmark) +
                    " -p udp -j CONNMARK --save-mark -m comment --comment " + quotedComment);
    lines.push_back("-I PREROUTING -p udp -j CONNMARK --restore-mark -m comment --comment " + quotedComment);
    lines.push_back("COMMIT");
    return lines;
}

// Write `lines` to `rulesPath`, feed the file to `restoreCommand` via the
// argv-based Utils::executeCommand, and clean up the temp file regardless of
// outcome. Returns true iff the restore command exited zero.
bool runRestoreFromFile(const char *restoreCommand,
                        const std::string &rulesPath,
                        const std::vector<std::string> &lines)
{
    std::string buf;
    for (const auto &line : lines) {
        spdlog::debug("{} <<< {}", restoreCommand, line);
        buf += line;
        buf += '\n';
    }

    // open()'s mode is ignored on pre-existing files; unlink first to force the intended perms.
    unlink(rulesPath.c_str());
    int fd = open(rulesPath.c_str(), O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        spdlog::error("Failed to open {}: {}", rulesPath, IO::strerror(errno));
        return false;
    }
    if (!IO::writeAll(fd, buf)) {
        spdlog::error("Write to {} failed: {}", rulesPath, IO::strerror(errno));
        close(fd);
        unlink(rulesPath.c_str());
        return false;
    }
    close(fd);

    const int rc = Utils::executeCommand(restoreCommand, {"-n", rulesPath});
    unlink(rulesPath.c_str());
    if (rc != 0) {
        spdlog::error("{} failed: {}", restoreCommand, rc);
        return false;
    }
    return true;
}

// Tear down the per-family fwmark rules previously added by enableRouting.
// Idempotent: delete loops until `ip rule show` no longer matches our markers.
void deleteFwmarkRulesForFamily(const char *family, uint32_t fwmark)
{
    const std::string match_str = "lookup " + std::to_string(fwmark);
    const std::string match_str2 = "from all lookup main suppress_prefixlength 0";

    while (true) {
        std::string output;
        Utils::executeCommand("ip", {family, "rule", "show"}, &output, false);

        bool bContinue = false;
        if (output.find(match_str) != std::string::npos) {
            std::vector<CmdEntry> cmdlist;
            cmdlist.push_back({"ip", {family, "rule", "delete", "table", std::to_string(fwmark)}});
            RunBlockingCommands(cmdlist);
            bContinue = true;
        }
        if (output.find(match_str2) != std::string::npos) {
            std::vector<CmdEntry> cmdlist;
            cmdlist.push_back({"ip", {family, "rule", "delete", "table", "main", "suppress_prefixlength", "0"}});
            RunBlockingCommands(cmdlist);
            bContinue = true;
        }

        if (!bContinue) {
            break;
        }
    }
}

// Walk (ip6)tables-save output, find rules tagged with our comment, and replay
// them as -D via the same family's restore binary fed from a temp file.
bool removeFirewallRulesForFamily(const char *saveCommand,
                                  const char *restoreCommand,
                                  const std::string &rulesPath,
                                  const std::string &comment)
{
    std::string output;
    if (Utils::executeCommand(saveCommand, {}, &output) != 0) {
        spdlog::error("{} failed", saveCommand);
        return false;
    }

    // iptables-save renders --comment values with literal double-quotes around
    // them (matching the form we feed to iptables-restore). Match the same
    // quoted form so removeFirewallRules actually finds our rules on teardown.
    const std::string match = "-m comment --comment \"" + comment + "\"";

    std::vector<std::string> lines;
    bool bFound = false;
    std::stringstream ss(output);
    std::string line;
    while (std::getline(ss, line)) {
        if ((line.rfind("*", 0) == 0) ||
            (line.find("COMMIT") != std::string::npos) ||
            ((line.rfind("-A", 0) == 0) && (line.find(match) != std::string::npos))) {
            if (line.rfind("-A", 0) == 0) {
                line[1] = 'D';
                bFound = true;
            }
            lines.push_back(line);
        }
    }

    if (!bFound) {
        return true;
    }

    return runRestoreFromFile(restoreCommand, rulesPath, lines);
}

}

WireGuardAdapter::WireGuardAdapter(const std::string &name)
    : name_(name), is_dns_server_set_(false),
      has_default_route_v4_(false), has_default_route_v6_(false), fwmark_(0)
{
    // Raw value only; rule emitters wrap this in literal double-quotes so
    // iptables-restore parses the whole phrase as a single --comment argument.
    comment_ = WS_PRODUCT_NAME " daemon rule for " + name_;
}

WireGuardAdapter::~WireGuardAdapter()
{
    flushDnsServer();
}

bool WireGuardAdapter::setIpAddress(const std::string &address)
{
    spdlog::debug("WireGuardAdapter::setIpAddress: input=\"{}\"", address);
    ipv4Cidr_.clear();
    ipv6Cidr_.clear();

    // Same split convention as setDnsServers / WireGuardController::splitAndDeduplicateAllowedIps.
    std::vector<std::string> parts;
    boost::split(parts, address, boost::is_any_of(",; "), boost::token_compress_on);

    for (auto &p : parts) {
        boost::trim(p);
        if (p.empty()) {
            continue;
        }

        types::IpAddressRange r(p);
        if (!r.isValid()) {
            spdlog::error("WireGuardAdapter::setIpAddress: invalid CIDR \"{}\"", p);
            return false;
        }

        // Canonicalised CIDR string from the parsed address; safe for shell use.
        const std::string canonical = r.toString();
        if (r.isV4()) {
            if (!ipv4Cidr_.empty()) {
                spdlog::warn("WireGuardAdapter::setIpAddress: multiple IPv4 segments, keeping \"{}\", ignoring \"{}\"",
                             ipv4Cidr_, canonical);
                continue;
            }
            ipv4Cidr_ = canonical;
        } else {
            if (!ipv6Cidr_.empty()) {
                spdlog::warn("WireGuardAdapter::setIpAddress: multiple IPv6 segments, keeping \"{}\", ignoring \"{}\"",
                             ipv6Cidr_, canonical);
                continue;
            }
            ipv6Cidr_ = canonical;
        }
    }

    if (ipv4Cidr_.empty()) {
        // v4 is required for WG control traffic and to satisfy validateWireGuardConfig.
        spdlog::error("WireGuardAdapter::setIpAddress: no IPv4 segment in \"{}\"", address);
        return false;
    }

    spdlog::debug("WireGuardAdapter::setIpAddress: parsed v4=\"{}\", v6=\"{}\"",
                  ipv4Cidr_, ipv6Cidr_.empty() ? std::string("(none)") : ipv6Cidr_);

    std::vector<CmdEntry> cmdlist;
    cmdlist.push_back({"ip", {"-4", "address", "add", ipv4Cidr_, "dev", getName()}});
    if (!ipv6Cidr_.empty()) {
        cmdlist.push_back({"ip", {"-6", "address", "add", ipv6Cidr_, "dev", getName()}});
    }
    cmdlist.push_back({"ip", {"link", "set", "up", "dev", getName()}});
    return RunBlockingCommands(cmdlist);
}

bool WireGuardAdapter::setDnsServers(const std::string &addressList, const std::string &scriptName)
{
    dns_script_name_ = scriptName;
    std::vector<std::string> dns_servers_list;
    boost::split(dns_servers_list, addressList, boost::is_any_of(",; "), boost::token_compress_on); // NOLINT: false positive
    if (dns_servers_list.empty())
        return true;

    is_dns_server_set_ = true;
    if (access(dns_script_name_.c_str(), X_OK) != 0) {
        std::error_code ec;
        std::filesystem::permissions(dns_script_name_,
                                     std::filesystem::perms::owner_exec |
                                         std::filesystem::perms::group_exec |
                                         std::filesystem::perms::others_exec,
                                     std::filesystem::perm_options::add, ec);
        if (ec) {
            spdlog::warn("Failed to chmod +x {}: {}", dns_script_name_, ec.message());
        }
    }
    // Build the env vars passed to the dns script. Stored on the adapter so flushDnsServer()
    // can reuse them with a different script_type. Routed via `env` rather than a shell
    // prefix so values are passed exactly without further parsing.
    dns_script_env_.clear();
    dns_script_env_.push_back("is_wireguard=1");
    dns_script_env_.push_back("dev=" + getName());
    // foreign_option_0 prevents DNS leakage; without it, update-systemd-resolved doesn't work
    dns_script_env_.push_back("foreign_option_0=dhcp-option DOMAIN-ROUTE .");
    for (size_t i = 0; i < dns_servers_list.size(); ++i) {
        dns_script_env_.push_back("foreign_option_" + std::to_string(i+1) + "=dhcp-option DNS " + dns_servers_list[i]);
    }
    std::vector<std::string> envArgs = dns_script_env_;
    envArgs.insert(envArgs.begin(), "script_type=up");
    envArgs.push_back(dns_script_name_);
    std::vector<CmdEntry> cmdlist;
    cmdlist.push_back({"env", envArgs});
    spdlog::debug("WireGuardAdapter::setDnsServers: env script_type=up {}", dns_script_name_);
    return RunBlockingCommands(cmdlist);
}

bool WireGuardAdapter::enableRouting(const std::vector<std::string> &allowedIps, uint32_t fwmark)
{
    allowedIps_ = allowedIps;
    fwmark_ = fwmark;

    spdlog::debug("WireGuardAdapter::enableRouting: allowedIps=[{}], fwmark={}",
                  boost::algorithm::join(allowedIps, ", "), fwmark);

    std::vector<CmdEntry> cmdlist;
    for (const auto &ip : allowedIps) {
        types::IpAddressRange r(ip);
        if (!r.isValid()) {
            // process_command::configureWireGuard already validates AllowedIPs; this is defence in depth.
            spdlog::warn("WireGuardAdapter::enableRouting: invalid AllowedIP \"{}\", skipping", ip);
            continue;
        }

        const char *family = r.isV6() ? "-6" : "-4";
        const std::string canonical = r.toString();
        const std::string fwmarkStr = std::to_string(fwmark);

        if (r.prefixLength() == 0) {
            if (r.isV4()) {
                has_default_route_v4_ = true;
            } else {
                has_default_route_v6_ = true;
            }

            spdlog::debug("WireGuardAdapter::enableRouting: \"{}\" -> default-route bypass via fwmark table {}",
                          canonical, fwmark);

            cmdlist.push_back({"ip", {family, "route", "add", canonical, "dev", getName(), "table", fwmarkStr}});
            cmdlist.push_back({"ip", {family, "rule", "add", "not", "fwmark", fwmarkStr, "table", fwmarkStr}});
            cmdlist.push_back({"ip", {family, "rule", "add", "table", "main", "suppress_prefixlength", "0"}});

            if (!RunBlockingCommands(cmdlist)) {
                return false;
            }
            cmdlist.clear();
        } else {
            // Non-default prefix: install a direct route on the WG device only if the
            // kernel doesn't already have one matching this destination.
            std::string output;
            Utils::executeCommand("ip", {family, "route", "show", "dev", getName(), "match", canonical}, &output, false);
            if (output.empty()) {
                spdlog::debug("WireGuardAdapter::enableRouting: \"{}\" -> direct route on {} (no kernel match)",
                              canonical, getName());
                cmdlist.push_back({"ip", {family, "route", "add", canonical, "dev", getName()}});
            } else {
                spdlog::debug("WireGuardAdapter::enableRouting: \"{}\" -> skip direct route, kernel already has: {}",
                              canonical, output);
            }
        }
    }

    if (!RunBlockingCommands(cmdlist)) {
        return false;
    }

    // One firewall-rule pass at the end: emits parallel iptables / ip6tables blocks
    // for whichever families ended up with a default route.
    if (has_default_route_v4_ || has_default_route_v6_) {
        if (!addFirewallRules(fwmark)) {
            return false;
        }
    }

    return true;
}

bool WireGuardAdapter::disableRouting()
{
    if (allowedIps_.empty() && fwmark_ == 0)
    {
        return true;
    }

    if (has_default_route_v4_) {
        deleteFwmarkRulesForFamily("-4", fwmark_);
    }
    if (has_default_route_v6_) {
        deleteFwmarkRulesForFamily("-6", fwmark_);
    }

    removeFirewallRules();

    return true;
}

bool WireGuardAdapter::flushDnsServer()
{
    if (!is_dns_server_set_)
        return true;
    is_dns_server_set_ = false;
    if (access(dns_script_name_.c_str(), X_OK) != 0) {
        std::error_code ec;
        std::filesystem::permissions(dns_script_name_,
                                     std::filesystem::perms::owner_exec |
                                         std::filesystem::perms::group_exec |
                                         std::filesystem::perms::others_exec,
                                     std::filesystem::perm_options::add, ec);
        if (ec) {
            spdlog::warn("Failed to chmod +x {}: {}", dns_script_name_, ec.message());
        }
    }
    std::vector<std::string> envArgs = dns_script_env_;
    envArgs.insert(envArgs.begin(), "script_type=down");
    envArgs.push_back(dns_script_name_);
    std::vector<CmdEntry> cmdlist;
    cmdlist.push_back({"env", envArgs});
    return RunBlockingCommands(cmdlist);
}

bool WireGuardAdapter::addFirewallRules(uint32_t fwmark)
{
    spdlog::debug("WireGuardAdapter::addFirewallRules: v4={} (default={}), v6={} (default={}), fwmark={}",
                  ipv4Cidr_.empty() ? std::string("(none)") : ipv4Cidr_, has_default_route_v4_,
                  ipv6Cidr_.empty() ? std::string("(none)") : ipv6Cidr_, has_default_route_v6_, fwmark);

    if (!ipv4Cidr_.empty() && has_default_route_v4_) {
        // Required so packets from local sockets marked with our fwmark survive
        // the kernel's reverse-path-filter check (no v6 analogue needed).
        Utils::executeCommand("sysctl", {"-q", "net.ipv4.conf.all.src_valid_mark=1"});

        const auto lines = buildFirewallRuleLines(getName(), ipv4Cidr_, fwmark, comment_);
        if (!runRestoreFromFile("iptables-restore", WS_LINUX_TMP_DIR "/wg_firewall.v4", lines)) {
            return false;
        }
    }

    if (!ipv6Cidr_.empty() && has_default_route_v6_) {
        const auto lines = buildFirewallRuleLines(getName(), ipv6Cidr_, fwmark, comment_);
        if (!runRestoreFromFile("ip6tables-restore", WS_LINUX_TMP_DIR "/wg_firewall.v6", lines)) {
            // Best-effort rollback of the v4 install so we don't leave half-state.
            removeFirewallRules();
            return false;
        }
    }

    return true;
}

bool WireGuardAdapter::removeFirewallRules()
{
    bool v4_ok = removeFirewallRulesForFamily("iptables-save", "iptables-restore",
                                              WS_LINUX_TMP_DIR "/wg_firewall.v4", comment_);
    bool v6_ok = removeFirewallRulesForFamily("ip6tables-save", "ip6tables-restore",
                                              WS_LINUX_TMP_DIR "/wg_firewall.v6", comment_);
    return v4_ok && v6_ok;
}
