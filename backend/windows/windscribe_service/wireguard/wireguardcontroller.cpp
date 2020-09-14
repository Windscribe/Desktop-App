#include "../all_headers.h"
#include "../ipc/servicecommunication.h"
#include "defaultroutemonitor.h"
#include "wireguardadapter.h"
#include "wireguardcontroller.h"
#include "wireguardcommunicator.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

WireGuardController::WireGuardController()
    : comm_(new WireGuardCommunicator), daemonCmdId_(0), is_initialized_(false)
{
}

void WireGuardController::init(const std::wstring &deviceName, UINT daemonCmdId)
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
    bool has_default_allowed_ip = false;
    for (const auto &ip : allowedIps) {
        if (boost::algorithm::ends_with(ip, "/0")) {
            has_default_allowed_ip = true;
            break;
        }
    }
    return adapter_->setIpAddress(ipAddress, has_default_allowed_ip)
            && adapter_->setDnsServers(dnsAddressList)
            && adapter_->enableRouting(allowedIps);
}

bool WireGuardController::configureDefaultRouteMonitor()
{
    if (!is_initialized_ || !adapter_.get())
        return false;
    if (!drm_)
        drm_.reset(new DefaultRouteMonitor(comm_, adapter_->getLuid()));
    return drm_->start();
}

bool WireGuardController::configureDaemon(const std::string &clientPrivateKey,
    const std::string &peerPublicKey, const std::string &peerPresharedKey,
    const std::string &peerEndpoint, const std::vector<std::string> &allowedIps)
{
    return is_initialized_
        && comm_->configure(clientPrivateKey, peerPublicKey, peerPresharedKey, peerEndpoint,
                            allowedIps);
}

UINT WireGuardController::getStatus(UINT32 *errorCode, UINT64 *bytesReceived,
                                    UINT64 *bytesTransmitted) const
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
