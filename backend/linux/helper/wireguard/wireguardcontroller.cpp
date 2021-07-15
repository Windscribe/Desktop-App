#include "wireguardcontroller.h"
#include "wireguardadapter.h"
#include "wireguardcommunicator.h"
#include "../../../posix_common/helper_commands.h"
#include "utils.h"
#include "logger.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

WireGuardController::WireGuardController()
    : comm_(new WireGuardCommunicator), daemonCmdId_(0), is_initialized_(false)
{
}

void WireGuardController::init(const std::string &deviceName, unsigned long daemonCmdId)
{
    daemonCmdId_ = daemonCmdId;
    comm_->setDeviceName(deviceName);
    adapter_.reset(new WireGuardAdapter(deviceName));
    is_initialized_ = true;
}

void WireGuardController::reset()
{
    if (!is_initialized_)
        return;

    adapter_->disableRouting();
    adapter_.reset();
    is_initialized_ = false;
}

bool WireGuardController::configureAdapter(const std::string &ipAddress,
    const std::string &dnsAddressList, const std::string &dnsScriptName,
    const std::vector<std::string> &allowedIps, uint32_t fwmark)
{
    if (!is_initialized_ || !adapter_.get())
        return false;
    return adapter_->setIpAddress(ipAddress)
           //&& adapter_->setDnsServers(dnsAddressList, dnsScriptName)
           && adapter_->enableRouting(allowedIps, fwmark);
}

const std::string WireGuardController::getAdapterName() const
{
    if (!is_initialized_ || !adapter_.get())
        return "";
    return adapter_->getName();
}

bool WireGuardController::configureDaemon(const std::string &clientPrivateKey,
    const std::string &peerPublicKey, const std::string &peerPresharedKey,
    const std::string &peerEndpoint, const std::vector<std::string> &allowedIps, uint32_t fwmark)
{
    return is_initialized_
        && comm_->configure(clientPrivateKey, peerPublicKey, peerPresharedKey, peerEndpoint,
            allowedIps, fwmark);
}

unsigned long WireGuardController::getStatus(unsigned int *errorCode,
    unsigned long long *bytesReceived, unsigned long long *bytesTransmitted) const
{
    if (!is_initialized_)
        return WIREGUARD_STATE_NONE;
    return comm_->getStatus(errorCode, bytesReceived, bytesTransmitted);
}

// static
std::vector<std::string>
WireGuardController::splitAndDeduplicateAllowedIps(const std::string &allowedIps)
{
    std::vector<std::string> result;
    boost::split(result, allowedIps, boost::is_any_of(",; "), boost::token_compress_on);
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

// static
uint32_t WireGuardController::getFwmark()
{
    uint32_t fwmark = 51820;        // initial default fwmark (taken from wireguard tools sources)

    // check for the fwmark busy
    while (true)
    {
        std::vector<std::string> args;
        args.push_back("-4");
        args.push_back("route");
        args.push_back("show");
        args.push_back("table");
        args.push_back(std::to_string(fwmark));

        std::string outputIp4, outputIp6;
        Utils::executeCommand("ip", args, &outputIp4, false);
        // check the same for ipv6
        args[0] = "-6";
        Utils::executeCommand("ip", args, &outputIp6, false);
        if (outputIp4.empty() && outputIp6.empty())
        {
            break;
        }
        else
        {
            fwmark++;
        }
    }

    return fwmark;
}
