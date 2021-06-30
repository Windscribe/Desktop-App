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
    : name_(name), is_dns_server_set_(false), has_default_route_(false)
{
}

WireGuardAdapter::~WireGuardAdapter()
{
    flushDnsServer();
}

bool WireGuardAdapter::setIpAddress(const std::string &address)
{
    std::vector<std::string> address_and_cidr;
    boost::split(address_and_cidr, address, boost::is_any_of("/"), boost::token_compress_on);
    int cidr = 32;
    if (address_and_cidr.size() > 1)
        cidr = static_cast<int>(strtol(address_and_cidr[1].c_str(), nullptr, 10));
    std::vector<std::string> cmdlist;
    cmdlist.push_back("ifconfig " + getName() + " inet " + address + " " + address_and_cidr[0]
                      + " netmask " + GetNetMaskFromCidr(cidr));
    cmdlist.push_back("ifconfig " + getName() + " up");
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

bool WireGuardAdapter::enableRouting(const std::vector<std::string> &allowedIps)
{
    std::vector<std::string> cmdlist;
    for (const auto &ip : allowedIps) {
        if (boost::algorithm::ends_with(ip, "/0")) {
            has_default_route_ = true;
            cmdlist.push_back("route -n add 0.0.0.0/1 dev " + getName());
            cmdlist.push_back("route -n add 128.0.0.0/1 dev " + getName());
        } else {
            cmdlist.push_back("route -n add \"" + ip + "\" dev " + getName());
        }
    }
    return RunBlockingCommands(cmdlist);
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
