#include "wireguardadapter.h"
#include "../../../posix_common/helper_commands.h"
#include "execute_cmd.h"
#include "utils.h"
#include "logger.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <sstream>

namespace
{

bool RunBlockingCommands(const std::vector<std::string> &cmdlist)
{
    std::string output;
    for (const auto &cmd : cmdlist) {
        output.clear();
        const auto status = Utils::executeCommand(cmd, {}, &output);
        if (!output.empty())
            Logger::instance().out("%s", output.c_str());
        if (status != 0) {
            Logger::instance().out("Failed to run command: \"%s\" (exit status %i)", cmd.c_str(), status);
            return false;
        }
    }
    return true;
}
}

WireGuardAdapter::WireGuardAdapter(const std::string &name)
    : name_(name), is_dns_server_set_(false), has_default_route_(false), fwmark_(0)
{
    comment_ = "\"Windscribe daemon rule for " + name_ + "\"";
}

WireGuardAdapter::~WireGuardAdapter()
{
    flushDnsServer();
}

bool WireGuardAdapter::setIpAddress(const std::string &address)
{
    std::vector<std::string> cmdlist;
    cmdlist.push_back("ip -4 address add " + address + " dev " + getName());
    cmdlist.push_back("ip link set up dev " + getName());
    return RunBlockingCommands(cmdlist);
}

bool WireGuardAdapter::setDnsServers(const std::string &addressList, const std::string &scriptName)
{
    dns_script_name_ = scriptName;
    std::vector<std::string> dns_servers_list;
    boost::split(dns_servers_list, addressList, boost::is_any_of(",; "), boost::token_compress_on);
    if (dns_servers_list.empty())
        return true;

    std::string env_block;
    env_block.append("foreign_option_0=\"dhcp-option DOMAIN-ROUTE .\" "); // prevent DNS leakage  and without it doesn't work update-systemd-resolved script
    for (size_t i = 0; i < dns_servers_list.size(); ++i) {
        env_block.append("foreign_option_" + std::to_string(i+1) + "=\"dhcp-option DNS "
                         + dns_servers_list[i] + "\" ");
    }
    is_dns_server_set_ = true;
    std::vector<std::string> cmdlist;
    if (access(dns_script_name_.c_str(), X_OK) != 0)
        cmdlist.push_back("chmod +x \"" + dns_script_name_ + "\"");
    dns_script_command_ = "is_wireguard=\"1\" dev=\"" + getName() + "\" " + env_block + dns_script_name_;
    cmdlist.push_back("script_type=\"up\" " + dns_script_command_);
    Logger::instance().out("%s", cmdlist[0].c_str());
    return RunBlockingCommands(cmdlist);
}

bool WireGuardAdapter::enableRouting(const std::string &ipAddress, const std::vector<std::string> &allowedIps, uint32_t fwmark)
{
    allowedIps_ = allowedIps;
    fwmark_ = fwmark;

    std::vector<std::string> cmdlist;
    for (const auto &ip : allowedIps) {
        if (boost::algorithm::ends_with(ip, "/0")) {
            has_default_route_ = true;
            cmdlist.push_back("ip -4 route add " + ip + " dev " + getName() + " table " + std::to_string(fwmark));
            cmdlist.push_back("ip -4 rule add not fwmark " + std::to_string(fwmark) + " table " + std::to_string(fwmark));
            cmdlist.push_back("ip -4 rule add table main suppress_prefixlength 0");

            if (!RunBlockingCommands(cmdlist))
            {
                return false;
            }
            cmdlist.clear();

            if (!addFirewallRules(ipAddress, fwmark))
            {
                return false;
            }
        } else {

            std::vector<std::string> args;
            args.push_back("-4");
            args.push_back("route");
            args.push_back("show");
            args.push_back("dev");
            args.push_back(getName());
            args.push_back("match");
            args.push_back(ip);

            std::string output;
            Utils::executeCommand("ip", args, &output, false);
            if (output.empty())
            {
                cmdlist.push_back("ip -4 route add " + ip + " dev " + getName());
            }
        }
    }
    return RunBlockingCommands(cmdlist);
}

bool WireGuardAdapter::disableRouting()
{
    if (allowedIps_.empty() && fwmark_ == 0)
    {
        return true;
    }

    while (true)
    {
        std::vector<std::string> args;
        args.push_back("-4");
        args.push_back("rule");
        args.push_back("show");

        std::string output;
        Utils::executeCommand("ip", args, &output, false);

        std::string match_str = "lookup " + std::to_string(fwmark_);
        std::string match_str2 = "from all lookup main suppress_prefixlength 0";
        bool bContinue = false;

        if (output.find(match_str) != std::string::npos)
        {
            std::vector<std::string> cmdlist;
            cmdlist.push_back("ip -4 rule delete table "+ std::to_string(fwmark_));
            RunBlockingCommands(cmdlist);
            bContinue = true;
        }
        if (output.find(match_str2) != std::string::npos)
        {
            std::vector<std::string> cmdlist;
            cmdlist.push_back("ip -4 rule delete table main suppress_prefixlength 0");
            RunBlockingCommands(cmdlist);
            bContinue = true;
        }

        if (!bContinue)
        {
            break;
        }
    }

    removeFirewallRules();

    return true;
}

bool WireGuardAdapter::flushDnsServer()
{
    if (!is_dns_server_set_)
        return true;
    is_dns_server_set_ = false;
    std::vector<std::string> cmdlist;
    if (access(dns_script_name_.c_str(), X_OK) != 0)
        cmdlist.push_back("chmod +x \"" + dns_script_name_ + "\"");
    cmdlist.push_back("script_type=\"down\" " + dns_script_command_);
    return RunBlockingCommands(cmdlist);
}

bool WireGuardAdapter::addFirewallRules(const std::string &ipAddress, uint32_t fwmark)
{
    Utils::executeCommand("sysctl -q net.ipv4.conf.all.src_valid_mark=1");

    FILE *file = popen("iptables-restore -n", "w");
    if(file == NULL)
    {
        Logger::instance().out("iptables-restore not found");
        return false;
    }

    std::vector<std::string> lines;
    lines.push_back("*raw");
    lines.push_back("-I PREROUTING ! -i " +  getName() + " -d " + ipAddress + " -m addrtype ! --src-type LOCAL -j DROP -m comment --comment " + comment_);
    lines.push_back("COMMIT");
    lines.push_back("*mangle");
    lines.push_back("-I POSTROUTING -m mark --mark " + std::to_string(fwmark) + " -p udp -j CONNMARK --save-mark -m comment --comment " + comment_);
    lines.push_back("-I PREROUTING -p udp -j CONNMARK --restore-mark -m comment --comment " + comment_);
    lines.push_back("COMMIT");

    for (auto &line : lines)
    {
        fputs(line.c_str(), file);
        fputs("\n", file);
    }

    pclose(file);
    return true;
}

bool WireGuardAdapter::removeFirewallRules()
{
    FILE *file = popen("iptables-save", "r");
    if(file == NULL)
    {
        Logger::instance().out("iptables-save not found");
        return false;
    }

    std::vector<std::string> lines;
    char szLine[10000];
    bool bFound = false;
    while(fgets(szLine, sizeof(szLine), file) != 0)
    {
        std::string line = szLine;
        if ((line.rfind("*", 0) == 0) || // string starts with "*"
            (line.find("COMMIT") != std::string::npos) ||
            ((line.rfind("-A", 0) == 0) && (line.find("-m comment --comment " + comment_) != std::string::npos)) )
        {
            if (line.rfind("-A", 0) == 0)
            {
                line[1] = 'D';
                bFound = true;
            }
            lines.push_back(line);
        }
    }
    pclose(file);

    if (bFound)
    {
        file = popen("iptables-restore -n", "w");
        if(file == NULL)
        {
            Logger::instance().out("iptables-restore not found");
            return false;
        }

        for (auto &line : lines)
        {
            fputs(line.c_str(), file);
            fputs("\n", file);
        }
        pclose(file);
    }

    return true;
}
