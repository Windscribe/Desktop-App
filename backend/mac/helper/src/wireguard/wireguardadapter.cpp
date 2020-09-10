#include "wireguardadapter.h"
#include "../ipc/helper_commands.h"
#include "execute_cmd.h"
#include "utils.h"
#include "logger.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>

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
    : name_(name), is_adapter_initialized_(false), has_default_route_(false)
{
}

WireGuardAdapter::~WireGuardAdapter()
{
    flushDnsServer();
}

void WireGuardAdapter::initializeOnce()
{
    if (is_adapter_initialized_)
        return;
    std::string output;
    auto status = Utils::executeCommand("networksetup -listallnetworkservices", {}, &output);
    if (status == 0) {
        std::vector<std::string> services;
        boost::trim(output);
        boost::split(services, output, boost::is_any_of("\n"), boost::token_compress_on);
        for (const auto &service : services) {
            if (service.find_first_of('*') != std::string::npos)
                continue;
            status = Utils::executeCommand("networksetup -getdnsservers \"" + service + "\"",
                {}, &output);
            if (status || output.find_first_of(' ') != std::string::npos)
                output = "empty";
            boost::replace_all(output, "\n", " ");
            dns_info_[service] = output;
        }
    }
    is_adapter_initialized_ = true;
}

bool WireGuardAdapter::setIpAddress(const std::string &address)
{
    initializeOnce();
    std::vector<std::string> address_and_cidr;
    boost::split(address_and_cidr, address, boost::is_any_of("/"), boost::token_compress_on);
    int cidr = 0;
    if (address_and_cidr.size() > 1)
        cidr = static_cast<int>(strtol(address_and_cidr[1].c_str(), nullptr, 10));
    std::vector<std::string> cmdlist;
    cmdlist.push_back("ifconfig " + getName() + " inet " + address + " " + address_and_cidr[0]
                      + " netmask " + GetNetMaskFromCidr(cidr));
    cmdlist.push_back("ifconfig " + getName() + " up");
    return RunBlockingCommands(cmdlist);
}

bool WireGuardAdapter::setDnsServers(const std::string &addressList)
{
    initializeOnce();
    std::string dns_servers(addressList);
    boost::replace_all(dns_servers, ",", " ");
    boost::replace_all(dns_servers, ";", " ");
    std::vector<std::string> cmdlist;
    for (const auto &info : dns_info_)
        cmdlist.push_back("networksetup -setdnsservers \"" + info.first + "\" " + dns_servers);
    return RunBlockingCommands(cmdlist);
}

bool WireGuardAdapter::enableRouting(const std::vector<std::string> &allowedIps)
{
    initializeOnce();
    std::vector<std::string> cmdlist;
    for (const auto &ip : allowedIps) {
        if (boost::algorithm::ends_with(ip, "/0")) {
            has_default_route_ = true;
            cmdlist.push_back("route -q -n add -inet 0.0.0.0/1 -interface " + getName());
            cmdlist.push_back("route -q -n add -inet 128.0.0.0/1 -interface " + getName());
        } else {
            cmdlist.push_back("route -q -n add -inet \"" + ip + "\" -interface " + getName());
        }
    }
    return RunBlockingCommands(cmdlist);
}

bool WireGuardAdapter::flushDnsServer()
{
    if (!is_adapter_initialized_)
        return true;
    std::vector<std::string> cmdlist;
    for (const auto &info : dns_info_)
        cmdlist.push_back("networksetup -setdnsservers \"" + info.first + "\" " + info.second);
    return RunBlockingCommands(cmdlist);
}
