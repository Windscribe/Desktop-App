#include "emergencyconnsettingspolicy.h"

#include <wsnet/WSNet.h>

#include "utils/log/categories.h"

EmergencyConnSettingsPolicy::EmergencyConnSettingsPolicy()
{
}

EmergencyConnSettingsPolicy::~EmergencyConnSettingsPolicy()
{
    cancelRequest();
}

void EmergencyConnSettingsPolicy::reset()
{
    cancelRequest();
    isFetched_ = false;
    currentEndpointIndex_ = 0;
    endpoints_.clear();
}

void EmergencyConnSettingsPolicy::debugLocationInfoToLog() const
{
    qCInfo(LOG_CONNECTION) << "Emergency connect";
}

void EmergencyConnSettingsPolicy::putFailedConnection()
{
    // A failure before the endpoint list arrived (e.g. connecting timeout during a slow fetch)
    // consumed no endpoint; advancing would skip one that was never attempted.
    if (!bStarted_ || !isFetched_) {
        return;
    }

    currentEndpointIndex_++;
}

bool EmergencyConnSettingsPolicy::isFailed() const
{
    return isFetched_ && currentEndpointIndex_ >= endpoints_.size();
}

CurrentConnectionDescr EmergencyConnSettingsPolicy::getCurrentConnectionSettings() const
{
    CurrentConnectionDescr ccd;

    if (isFetched_ && currentEndpointIndex_ < endpoints_.size()) {
        const EmergencyEndpoint &endpoint = endpoints_[currentEndpointIndex_];
        ccd.connectionNodeType = CONNECTION_NODE_DEFAULT;
        ccd.ip = endpoint.ip;
        ccd.port = endpoint.port;
        ccd.protocol = endpoint.isTcp ? types::Protocol::OPENVPN_TCP : types::Protocol::OPENVPN_UDP;
    } else {
        ccd.connectionNodeType = CONNECTION_NODE_ERROR;
        ccd.protocol = types::Protocol::OPENVPN_UDP;
    }

    return ccd;
}

bool EmergencyConnSettingsPolicy::isAutomaticMode()
{
    return false;
}

bool EmergencyConnSettingsPolicy::isCustomConfig()
{
    return false;
}

void EmergencyConnSettingsPolicy::resolveHostnames()
{
    // The endpoint list is fetched once per session; subsequent attempts iterate the cached list.
    if (isFetched_) {
        emit hostnamesResolved();
        return;
    }
    // A retry can re-enter while the fetch is still in flight (connecting timeout during a slow
    // fetch); its completion answers the waiting attempt, a second request would answer twice.
    if (request_) {
        return;
    }

    auto callback = [this](const std::vector<std::shared_ptr<wsnet::WSNetEmergencyConnectEndpoint>> &endpoints) {
        // The callback arrives on a wsnet thread; convert to plain data and marshal to our thread.
        std::vector<EmergencyEndpoint> plainEndpoints;
        for (const auto &endpoint : endpoints) {
            EmergencyEndpoint e;
            e.ip = QString::fromStdString(endpoint->ip());
            e.port = endpoint->port();
            e.isTcp = (endpoint->protocol() == wsnet::Protocol::kTcp);
            plainEndpoints.push_back(e);
        }
        QMetaObject::invokeMethod(this, [this, plainEndpoints] { // NOLINT: false positive for memory leak
            onEndpointsReceived(plainEndpoints);
        });
    };
    request_ = wsnet::WSNet::instance()->emergencyConnect()->getIpEndpoints(callback);
}

bool EmergencyConnSettingsPolicy::hasProtocolChanged()
{
    return false;
}

types::Protocol EmergencyConnSettingsPolicy::preResolveProtocol() const
{
    return types::Protocol::OPENVPN_UDP;
}

bool EmergencyConnSettingsPolicy::shouldWaitForNetwork() const
{
    return false;
}

bool EmergencyConnSettingsPolicy::shouldRetryOnAttemptFailure() const
{
    return true;
}

void EmergencyConnSettingsPolicy::onEndpointsReceived(const std::vector<EmergencyEndpoint> &endpoints)
{
    request_.reset();
    isFetched_ = true;
    currentEndpointIndex_ = 0;
    endpoints_ = endpoints;
    qCInfo(LOG_CONNECTION) << "Emergency connect endpoints received:" << endpoints_.size();
    emit hostnamesResolved();
}

void EmergencyConnSettingsPolicy::cancelRequest()
{
    if (request_) {
        request_->cancel();
        request_.reset();
    }
}
