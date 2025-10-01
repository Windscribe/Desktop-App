#include "wireguardcontroller.h"
#include "wireguardadapter.h"
#include "wireguardcommunicator.h"
#include "defaultroutemonitor.h"
#include "../../common/helper_commands.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <spdlog/spdlog.h>

WireGuardController::WireGuardController()
    : daemonCmdId_(0), is_initialized_(false)
{
}

bool WireGuardController::start()
{
    adapter_.reset(new WireGuardAdapter(kAdapterName));
    comm_.reset(new WireGuardCommunicator());
    if (comm_->start(kAdapterName))
    {
        is_initialized_ = true;
        return true;
    }
    return false;
}

bool WireGuardController::stop()
{
    if (!is_initialized_)
        return false;

    adapter_.reset();
    comm_->stop();
    comm_.reset();
    drm_.reset();

    is_initialized_ = false;
    return true;
}

bool WireGuardController::configureAdapter(const std::string &ipAddress, const std::string &dnsAddressList,
    const std::string &dnsScriptName, const std::vector<std::string> &allowedIps)
{
    if (!is_initialized_ || !adapter_.get())
        return false;
    return adapter_->setIpAddress(ipAddress)
        && adapter_->setDnsServers(dnsAddressList, dnsScriptName)
        && adapter_->enableRouting(allowedIps);
}

bool WireGuardController::configureDefaultRouteMonitor(const std::string &peerEndpoint)
{
    if (!is_initialized_ || !adapter_.get())
        return false;
    if (!drm_)
        drm_.reset(new DefaultRouteMonitor(adapter_->getName()));
    return drm_->start(peerEndpoint);
}

bool WireGuardController::configure(const std::string &clientPrivateKey,
    const std::string &peerPublicKey, const std::string &peerPresharedKey,
    const std::string &peerEndpoint, const std::vector<std::string> &allowedIps,
    uint16_t listenPort)
{
    return is_initialized_
        && comm_->configure(clientPrivateKey, peerPublicKey, peerPresharedKey, peerEndpoint,
            allowedIps, listenPort);
}

unsigned long WireGuardController::getStatus(unsigned int *errorCode,
    unsigned long long *bytesReceived, unsigned long long *bytesTransmitted) const
{
    if (!is_initialized_)
        return kWgStateNone;
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
