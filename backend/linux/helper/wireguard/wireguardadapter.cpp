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
        if (status != 0) {
            LOG("Failed to run command: \"%s\" (exit status %i)", cmd.c_str(), status);
            return false;
        }
        if (!output.empty())
            LOG("%s", output.c_str());
    }
    return true;
}
}

WireGuardAdapter::WireGuardAdapter(const std::string &name)
    : name_(name), is_dns_server_set_(false), has_default_route_(false), fwmark_(0)
{
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
    for (size_t i = 0; i < dns_servers_list.size(); ++i) {
        env_block.append("foreign_option_" + std::to_string(i) + "=\"dhcp-option DNS "
                         + dns_servers_list[i] + "\" ");
    }
    is_dns_server_set_ = true;
    std::vector<std::string> cmdlist;
    if (access(dns_script_name_.c_str(), X_OK) != 0)
        cmdlist.push_back("chmod +x \"" + dns_script_name_ + "\"");
    cmdlist.push_back(env_block + dns_script_name_ + " -up");
    return RunBlockingCommands(cmdlist);
}

bool WireGuardAdapter::enableRouting(const std::vector<std::string> &allowedIps, uint32_t fwmark)
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
    cmdlist.push_back(dns_script_name_ + " -down");
    return RunBlockingCommands(cmdlist);
}
