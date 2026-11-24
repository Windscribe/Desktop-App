#include "wireguardcontroller.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <spdlog/spdlog.h>

#include "../../common/defaultroutemonitor_posix.h"
#include "../../common/helper_commands.h"
#include "../../common/wireguardcommunicator_posix.h"
#include "utils/log/debug_trace.h"
#include "wireguardadapter.h"

WireGuardController::WireGuardController()
    : is_initialized_(false)
{
}

bool WireGuardController::start(bool verboseLogging)
{
    adapter_.reset(new WireGuardAdapter(kAdapterName));
    comm_.reset(new WireGuardCommunicator());
    if (comm_->start(kAdapterName, verboseLogging))
    {
        is_initialized_ = true;
        return true;
    }
    return false;
}

bool WireGuardController::stop()
{
    if (!is_initialized_) {
        // Controller not initialized, but we still need to clean up (e.g., utun420 device)
        spdlog::info("WireGuardController::stop() called when not initialized - attempting cleanup");
        return WireGuardCommunicator::forceStop(kAdapterName);
    }

    {
        WS_DEBUG_TRACE_SCOPE("WireGuardController::stop restore DNS");
        adapter_.reset();
    }
    bool stopped = true;
    {
        WS_DEBUG_TRACE_SCOPE("WireGuardController::stop daemon");
        stopped = comm_->stop();
        comm_.reset();
    }
    {
        WS_DEBUG_TRACE_SCOPE("WireGuardController::stop route monitor");
        drm_.reset();
    }

    is_initialized_ = false;
    return stopped;
}

bool WireGuardController::configureAdapter(const std::string &ipAddress, const std::string &dnsAddressList,
    const std::string &dnsScriptName, const std::vector<std::string> &allowedIps, const std::string &peerEndpoint)
{
    if (!is_initialized_ || !adapter_.get())
        return false;
    if (!adapter_->setIpAddress(ipAddress)) {
        return false;
    }
    adapter_->addDnsServerRoutes(dnsAddressList, allowedIps, peerEndpoint);
    const bool success = adapter_->setDnsServers(dnsAddressList, dnsScriptName) && adapter_->enableRouting(allowedIps);
    adapter_->removeDnsServerRoutes();
    return success;
}

bool WireGuardController::configureDefaultRouteMonitor(const std::string &peerEndpoint)
{
    if (!is_initialized_ || !adapter_.get())
        return false;
    if (!drm_)
        drm_.reset(new DefaultRouteMonitor());
    return drm_->start(peerEndpoint);
}

bool WireGuardController::configure(const std::string &clientPrivateKey,
                                    const std::string &peerPublicKey, const std::string &peerPresharedKey,
                                    const std::string &peerEndpoint, const std::vector<std::string> &allowedIps,
                                    uint16_t listenPort, const AmneziawgConfig &amneziawgConfig, const std::string &persistentKeepalive,
                                    bool disconnectOnStaleHandshake)
{
    return is_initialized_
        && comm_->configure(clientPrivateKey, peerPublicKey, peerPresharedKey, peerEndpoint,
            allowedIps, 0, listenPort, amneziawgConfig, persistentKeepalive, disconnectOnStaleHandshake);
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
