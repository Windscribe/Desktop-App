#pragma once

#include <memory>
#include <vector>

#include "baseconnsettingspolicy.h"
#include "engine/locationsmodel/baselocationinfo.h"
#include "iconnsettingspolicyfactory.h"

namespace wsnet {
class WSNetCancelableCallback;
}

// Attempt sequencing for emergency connect: the hardcoded emergency OpenVPN endpoints from wsnet,
// tried in order until one connects or the list is exhausted. Used by the dedicated emergency
// ConnectionManager instance that Engine constructs; there is no location or user protocol choice.
class EmergencyConnSettingsPolicy : public BaseConnSettingsPolicy
{
    Q_OBJECT
public:
    EmergencyConnSettingsPolicy();
    ~EmergencyConnSettingsPolicy() override;

    void reset() override;
    void debugLocationInfoToLog() const override;
    void putFailedConnection() override;
    bool isFailed() const override;
    CurrentConnectionDescr getCurrentConnectionSettings() const override;
    bool isAutomaticMode() override;
    bool isCustomConfig() override;
    void resolveHostnames() override;
    bool hasProtocolChanged() override;

    // Always the family representative: the connector is created pre-resolve, and the per-endpoint
    // UDP/TCP variant is only known after the endpoint fetch. Emergency connections have always run
    // their connector as OPENVPN_UDP, so the connector-side protocol_ checks (TCP proxy forwarding,
    // DCO) must keep seeing UDP even for TCP endpoints.
    types::Protocol preResolveProtocol() const override;

    // Emergency connect fails fast when there is no network rather than waiting for connectivity.
    bool shouldWaitForNetwork() const override;

    // The endpoint list is the only failover there is; keep walking it on failures the regular
    // policies treat as attempt-fatal.
    bool shouldRetryOnAttemptFailure() const override;

private:
#ifdef WINDSCRIBE_BUILD_TESTS
    friend class TestEmergencyConnSettingsPolicy;
#endif

    struct EmergencyEndpoint
    {
        QString ip;
        uint port = 0;
        bool isTcp = false;
    };

    bool isFetched_ = false;
    size_t currentEndpointIndex_ = 0;
    std::vector<EmergencyEndpoint> endpoints_;
    std::shared_ptr<wsnet::WSNetCancelableCallback> request_;

    void onEndpointsReceived(const std::vector<EmergencyEndpoint> &endpoints);
    void cancelRequest();
};

// Minimal location info satisfying ConnectionManager's non-null bli requirement; the emergency
// policy takes its endpoints from wsnet, not from a location.
class EmergencyLocationInfo : public locationsmodel::BaseLocationInfo
{
    Q_OBJECT
public:
    EmergencyLocationInfo() : BaseLocationInfo(LocationID(), "emergency") {}
    bool isExistSelectedNode() const override { return true; }
    QString getVerifyX509name() const override { return QString(); }
    QString getLogString() const override { return "Emergency connect"; }
};

// Handed to the emergency ConnectionManager instance; every request gets the emergency policy
// regardless of location or settings.
class EmergencyConnSettingsPolicyFactory : public IConnSettingsPolicyFactory
{
public:
    BaseConnSettingsPolicy *createPolicy(QSharedPointer<locationsmodel::BaseLocationInfo> /*bli*/,
                                         const types::ConnectionSettings & /*connectionSettings*/,
                                         const api_responses::PortMap & /*portMap*/,
                                         const types::ProxySettings & /*proxySettings*/,
                                         const QString & /*preferredNodeHostname*/,
                                         bool /*isFirewallAlwaysOnPlusEnabled*/,
                                         bool /*hasUsableCachedWgConfig*/) override
    {
        return new EmergencyConnSettingsPolicy();
    }
};
