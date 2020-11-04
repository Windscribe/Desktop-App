#include "wireguardcontroller.h"
#include "wireguardadapter.h"
#include "wireguardcommunicator.h"
#include "defaultroutemonitor.h"
#include "../ipc/helper_commands.h"
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
    drm_.reset();
    adapter_.reset();
    is_initialized_ = false;
}

bool WireGuardController::configureAdapter(const std::string &ipAddress,
    const std::string &dnsAddressList, const std::vector<std::string> &allowedIps)
{
    if (!is_initialized_ || !adapter_.get())
        return false;
    return adapter_->setIpAddress(ipAddress) && adapter_->setDnsServers(dnsAddressList)
           && adapter_->enableRouting(allowedIps);
}

const std::string WireGuardController::getAdapterName() const
{
    if (!is_initialized_ || !adapter_.get())
        return "";
    return adapter_->getName();
}

bool WireGuardController::configureDefaultRouteMonitor(const std::string &peerEndpoint)
{
    if (!is_initialized_ || !adapter_.get())
        return false;
    if (!adapter_->hasDefaultRoute()) {
        if (drm_)
            drm_->stop();
        return true;
    } else {
        if (!drm_)
            drm_.reset(new DefaultRouteMonitor(adapter_->getName()));
        return drm_->start(peerEndpoint);
    }
}

bool WireGuardController::configureDaemon(const std::string &clientPrivateKey,
    const std::string &peerPublicKey, const std::string &peerPresharedKey,
    const std::string &peerEndpoint, const std::vector<std::string> &allowedIps)
{
    return is_initialized_
        && comm_->configure(clientPrivateKey, peerPublicKey, peerPresharedKey, peerEndpoint,
            allowedIps);
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
