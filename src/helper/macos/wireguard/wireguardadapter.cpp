#include "wireguardadapter.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <sstream>

#include "../utils.h"
#include "types/ipaddress.h"

namespace
{
// Dotted-decimal v4 netmask from a CIDR prefix length. Still needed because the
// macOS `ifconfig <if> inet ... netmask <mask>` invocation expects the dotted form
// (the v6 path uses `inet6 add <cidr>` directly and does not need this).
std::string GetNetMaskFromCidr(int cidr)
{
    std::stringstream netmask;
    for (int byte_index = 0; byte_index < 4; ++byte_index) {
        int byte_value = 0;
        const int test = std::min( 8, cidr - byte_index * 8 );
        for (int bit_index = 0; bit_index < test; ++bit_index)
            byte_value |= 1 << (7-bit_index);
        if (byte_index > 0)
            netmask << ".";
        netmask << byte_value;
    }
    return netmask.str();
}

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
        if (status != 0) {
            spdlog::error("Failed to run command: \"{}\" (exit status {})", cmd.program, status);
            return false;
        }
        if (!output.empty())
            spdlog::info("{}", output);
    }
    return true;
}
}

WireGuardAdapter::WireGuardAdapter(const std::string &name)
    : name_(name), is_dns_server_set_(false),
      has_default_route_v4_(false), has_default_route_v6_(false)
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

    // Reconstruct the bare v4 address (no prefix) and the prefix length for the
    // legacy ifconfig "inet <local-addr> <remote-addr> netmask <mask>" invocation.
    // Matches the master form exactly (bare in both slots, explicit dotted netmask);
    // passing the canonical "/N" form in the local slot can be parsed identically by
    // the kernel when an explicit netmask follows, but the bare form avoids any
    // ambiguity in how `ifconfig` populates the resulting point-to-point route.
    const auto slashPos = ipv4Cidr_.find('/');
    const std::string ipv4Addr = (slashPos == std::string::npos) ? ipv4Cidr_ : ipv4Cidr_.substr(0, slashPos);
    int v4Cidr = 32;
    if (slashPos != std::string::npos) {
        v4Cidr = static_cast<int>(strtol(ipv4Cidr_.c_str() + slashPos + 1, nullptr, 10));
    }

    std::vector<CmdEntry> cmdlist;
    cmdlist.push_back({"ifconfig", {getName(), "inet", ipv4Addr, ipv4Addr,
                                    "netmask", GetNetMaskFromCidr(v4Cidr)}});
    if (!ipv6Cidr_.empty()) {
        // macOS `ifconfig <utunN> inet6 add <addr>/<prefix>` documented in `man 8 ifconfig`
        // on 13+; the kernel infers prefixlen from the slash form so no `prefixlen` keyword.
        cmdlist.push_back({"ifconfig", {getName(), "inet6", "add", ipv6Cidr_}});
    }
    cmdlist.push_back({"ifconfig", {getName(), "up"}});
    return RunBlockingCommands(cmdlist);
}

bool WireGuardAdapter::setDnsServers(const std::string &addressList, const std::string &scriptName)
{
    dns_script_name_ = scriptName;
    std::vector<std::string> dns_servers_list;
    boost::split(dns_servers_list, addressList, boost::is_any_of(",; "), boost::token_compress_on);
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
    // Build the env-prefixed command via `env`, which sets KEY=VALUE for the next exec
    // without going through a shell. The script reads these via getenv().
    std::vector<std::string> envArgs;
    for (size_t i = 0; i < dns_servers_list.size(); ++i) {
        envArgs.push_back("foreign_option_" + std::to_string(i) + "=dhcp-option DNS " + dns_servers_list[i]);
    }
    envArgs.push_back("dev=" + getName());
    envArgs.push_back(dns_script_name_);
    envArgs.push_back("-up");
    std::vector<CmdEntry> cmdlist;
    cmdlist.push_back({"env", envArgs});
    return RunBlockingCommands(cmdlist);
}

bool WireGuardAdapter::enableRouting(const std::vector<std::string> &allowedIps)
{
    spdlog::debug("WireGuardAdapter::enableRouting: allowedIps=[{}]",
                  boost::algorithm::join(allowedIps, ", "));

    std::vector<CmdEntry> cmdlist;
    for (const auto &ip : allowedIps) {
        types::IpAddressRange r(ip);
        if (!r.isValid()) {
            // process_command::configureWireGuard already validates AllowedIPs; this is defence in depth.
            spdlog::warn("WireGuardAdapter::enableRouting: invalid AllowedIP \"{}\", skipping", ip);
            continue;
        }

        const char *family = r.isV6() ? "-inet6" : "-inet";
        const std::string canonical = r.toString();

        if (r.prefixLength() == 0) {
            // Default-route override via the "two /1 routes + delete /0" trick. Both
            // /1 halves cover the entire address space and outrank the system /0,
            // and the /0 delete then drops the original default so the WG iface owns
            // everything. v4 and v6 each do this independently.
            if (r.isV4()) {
                has_default_route_v4_ = true;
                cmdlist.push_back({"route", {"-q", "-n", "add", family, "0.0.0.0/1", "-interface", getName()}});
                cmdlist.push_back({"route", {"-q", "-n", "add", family, "128.0.0.0/1", "-interface", getName()}});
                cmdlist.push_back({"route", {"-q", "-n", "delete", family, "0.0.0.0/0", "-interface", getName()}});
            } else {
                has_default_route_v6_ = true;
                cmdlist.push_back({"route", {"-q", "-n", "add", family, "::/1", "-interface", getName()}});
                cmdlist.push_back({"route", {"-q", "-n", "add", family, "8000::/1", "-interface", getName()}});
                cmdlist.push_back({"route", {"-q", "-n", "delete", family, "::/0", "-interface", getName()}});
            }
        } else {
            cmdlist.push_back({"route", {"-q", "-n", "add", family, canonical, "-interface", getName()}});
        }
    }
    return RunBlockingCommands(cmdlist);
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
    std::vector<CmdEntry> cmdlist;
    cmdlist.push_back({dns_script_name_, {"-down"}});
    return RunBlockingCommands(cmdlist);
}
