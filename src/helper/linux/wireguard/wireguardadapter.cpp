#include "wireguardadapter.h"
#include "../../common/helper_commands.h"
#include "../../common/io_posix.h"
#include "../nftables/nftablescontroller.h"
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
            spdlog::debug("{}", output);
        if (status != 0) {
            spdlog::error("Failed to run command: \"{}\" (exit status {})", cmd.program, status);
            return false;
        }
    }
    return true;
}

// Tear down the per-family policy-routing rules previously added by enableRouting.
// Idempotent: delete loops until `ip rule show` no longer matches our markers.
void deleteFwmarkRulesForFamily(const char *family, uint32_t routingTable)
{
    const std::string match_str = "lookup " + std::to_string(routingTable);
    const std::string match_str2 = "from all lookup main suppress_prefixlength 0";

    while (true) {
        std::string output;
        Utils::executeCommand("ip", {family, "rule", "show"}, &output, false);

        bool bContinue = false;
        if (output.find(match_str) != std::string::npos) {
            std::vector<CmdEntry> cmdlist;
            cmdlist.push_back({"ip", {family, "rule", "delete", "table", std::to_string(routingTable)}});
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

}

WireGuardAdapter::WireGuardAdapter(const std::string &name)
    : name_(name), is_dns_server_set_(false),
      has_default_route_v4_(false), has_default_route_v6_(false), routingTable_(0)
{
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

bool WireGuardAdapter::enableRouting(const std::vector<std::string> &allowedIps, uint32_t fwmark, uint32_t routingTable)
{
    allowedIps_ = allowedIps;
    routingTable_ = routingTable;

    spdlog::debug("WireGuardAdapter::enableRouting: allowedIps=[{}], fwmark={}, routingTable={}",
                  boost::algorithm::join(allowedIps, ", "), fwmark, routingTable);

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
        // The fwmark is fixed (matches the kill-switch firewall and the WG socket mark); only the
        // routing-table id may vary. The `not fwmark` rule selects the table for unmarked packets.
        const std::string fwmarkStr = std::to_string(fwmark);
        const std::string tableStr = std::to_string(routingTable);

        if (r.prefixLength() == 0) {
            if (r.isV4()) {
                has_default_route_v4_ = true;
            } else {
                has_default_route_v6_ = true;
            }

            spdlog::debug("WireGuardAdapter::enableRouting: \"{}\" -> default-route bypass via fwmark {} table {}",
                          canonical, fwmark, routingTable);

            cmdlist.push_back({"ip", {family, "route", "add", canonical, "dev", getName(), "table", tableStr}});
            cmdlist.push_back({"ip", {family, "rule", "add", "not", "fwmark", fwmarkStr, "table", tableStr}});
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

    // One firewall-rule pass at the end: emits a single nftables inet transaction blocking
    // whichever families ended up with a default route.
    if (has_default_route_v4_ || has_default_route_v6_) {
        if (!addFirewallRules(fwmark)) {
            return false;
        }
    }

    return true;
}

bool WireGuardAdapter::disableRouting()
{
    if (allowedIps_.empty() && routingTable_ == 0)
    {
        return true;
    }

    if (has_default_route_v4_) {
        deleteFwmarkRulesForFamily("-4", routingTable_);
    }
    if (has_default_route_v6_) {
        deleteFwmarkRulesForFamily("-6", routingTable_);
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

    const bool v4 = !ipv4Cidr_.empty() && has_default_route_v4_;
    const bool v6 = !ipv6Cidr_.empty() && has_default_route_v6_;
    if (!v4 && !v6) {
        return true;  // WG isn't the default route for either family; nothing to firewall.
    }

    if (v4) {
        // Required so packets from local sockets marked with our fwmark survive the kernel's
        // reverse-path-filter check (no v6 analogue needed).
        Utils::executeCommand("sysctl", {"-q", "net.ipv4.conf.all.src_valid_mark=1"});
    }

    const std::string iface = getName();
    const std::string fwmarkStr = std::to_string(fwmark);

    std::string buf = nft::addTable();
    // (Re)build our WireGuard chains in the shared table: create-if-absent + flush (see nft::ensureChain
    // for why a base chain is never delete+re-added). wg_mangle_pre stays at prerouting priority -150,
    // ahead of FirewallController's st_pre_cm (-140), so the two connmark chains run in a defined order.
    buf += nft::ensureChain("wg_raw_pre", "type filter hook prerouting priority -300;");
    buf += nft::ensureChain("wg_mangle_post", "type filter hook postrouting priority -150;");
    buf += nft::ensureChain("wg_mangle_pre", "type filter hook prerouting priority -150;");

    // Drop spoofed inbound destined for the tunnel CIDR arriving on a non-tunnel interface
    // (-m addrtype ! --src-type LOCAL -> fib saddr type != local).
    if (v4) {
        buf += nft::rule("wg_raw_pre", "iifname != \"" + iface + "\" ip daddr " + ipv4Cidr_ + " fib saddr type != local drop");
    }
    if (v6) {
        buf += nft::rule("wg_raw_pre", "iifname != \"" + iface + "\" ip6 daddr " + ipv6Cidr_ + " fib saddr type != local drop");
    }

    // Save/restore the fwmark on the WG endpoint UDP connection (family-agnostic, one set suffices in
    // the inet table): CONNMARK --save-mark -> `ct mark set meta mark`; --restore-mark -> the reverse.
    buf += nft::rule("wg_mangle_post", "meta mark " + fwmarkStr + " meta l4proto udp ct mark set meta mark");
    buf += nft::rule("wg_mangle_pre", "meta l4proto udp meta mark set ct mark");

    // One atomic transaction: on failure nothing is applied, so no rollback is needed.
    return NftablesController::instance().run(buf);
}

bool WireGuardAdapter::removeFirewallRules()
{
    std::string buf = nft::addTable();
    for (const char *ch : {"wg_raw_pre", "wg_mangle_post", "wg_mangle_pre"}) {
        buf += nft::deleteChain(ch);
    }
    return NftablesController::instance().run(buf);
}
