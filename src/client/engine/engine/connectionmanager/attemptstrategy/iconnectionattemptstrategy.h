#pragma once

#include <QVector>
#include <QObject>
#include "api_responses/portmap.h"
#include "api_responses/servercredentials.h"
#include "api_responses/staticips.h"
#include "types/protocolstatus.h"
#include "engine/wireguardconfig/wireguardconfig.h"

namespace locationsmodel {
class MutableLocationInfo;
}

enum CONNECTION_NODE_TYPE {
    CONNECTION_NODE_ERROR,
    CONNECTION_NODE_DEFAULT,
    CONNECTION_NODE_CUSTOM_CONFIG,
    CONNECTION_NODE_STATIC_IPS
};

struct CurrentConnectionDescr
{
    CONNECTION_NODE_TYPE connectionNodeType = CONNECTION_NODE_ERROR;

    // Endpoint fields, filled for every node type (custom configs leave verifyX509name and
    // isIpv6Support at their defaults).
    QString ip;
    uint port = 0;
    types::Protocol protocol;
    QString hostname;
    QString verifyX509name;
    bool isIpv6Support = false;

    // OpenVPN payload; filled only for custom-config attempts. The finished .ovpn text: body plus
    // the single selected remote line with the resolved IP substituted, assembled upstream.
    struct OpenVpn {
        QString customConfig;
    } openVpn;

    // WireGuard payload: peerPublicKey for API nodes, the rest for custom-config attempts.
    struct WireGuard {
        QString peerPublicKey;
        QSharedPointer<WireGuardConfig> customConfig;
    } wireGuard;

    // Filled only for CONNECTION_NODE_STATIC_IPS.
    struct StaticIps {
        api_responses::ServerCredentials credentials;
        api_responses::StaticIpPortsVector ports;
    } staticIps;
};
Q_DECLARE_METATYPE(CurrentConnectionDescr)

// helper class for ConnectionManager
class IConnectionAttemptStrategy : public QObject
{
    Q_OBJECT
public:
    enum class FailureAdvice { Retry, GiveUp };

    IConnectionAttemptStrategy() : QObject(nullptr) {}
    virtual ~IConnectionAttemptStrategy() {}

    virtual void reset() = 0;
    virtual void debugLocationInfoToLog() const = 0;
    virtual void putFailedConnection() = 0;
    virtual bool isFailed() const = 0;
    virtual CurrentConnectionDescr getCurrentConnectionSettings() const = 0;
    virtual bool isAutomaticMode() = 0;
    // Nothing to resolve for API-location strategies; custom-config and emergency strategies
    // override with their async resolve/fetch.
    virtual void resolveHostnames() { emit hostnamesResolved(); }
    // Only the automatic walk changes protocol between attempts.
    virtual bool hasProtocolChanged() { return false; }

    // Protocol of the next attempt, valid before resolveHostnames() completes, so the connector can
    // be created at attempt start. Custom configs answer with the family representative (the exact
    // per-remote variant is only known post-resolve).
    virtual types::Protocol preResolveProtocol() const { return getCurrentConnectionSettings().protocol; }

    // Whether an attempt that finds no usable network parks and polls for connectivity. Policies
    // that must fail fast instead (emergency connect) return false.
    virtual bool shouldWaitForNetwork() const { return true; }
    // An endpoint-list strategy walks its list on failures the regular strategies treat as
    // attempt-fatal (local spawn/socket errors, bare process death).
    virtual bool shouldRetryOnAttemptFailure() const { return false; }
    // Custom-config endpoint walks have no connect timeout; a slow dial must not be aborted.
    virtual bool usesConnectTimeout() const { return true; }
    // Whether a failed tunnel test is surfaced to the caller instead of driving failover; a custom
    // config has no alternatives to fail over to.
    virtual bool surfacesTunnelTestFailure() const { return false; }

    // Records the failed attempt and decides whether the caller should try the next attempt or give
    // up. An exhausted walk restarts from the top (with a fresh cached-config budget) instead of
    // giving up when a connection succeeded this session; the caller owns that session fact.
    FailureAdvice recordFailureAndAdvance(bool attemptSucceededThisSession)
    {
        putFailedConnection();
        if (!isFailed()) {
            return FailureAdvice::Retry;
        }
        if (attemptSucceededThisSession) {
            reset();
            resetCachedConfigBudget();
            return FailureAdvice::Retry;
        }
        return FailureAdvice::GiveUp;
    }

    enum class CachedConfigAdvice { UseCachedOnly, Advance, Abort };
    // Under Firewall Always On+ the API is blocked, so a config-fetching protocol may only use its
    // locally cached config, with the attempts capped so a config that never establishes can't loop
    // forever. The caller gates on Always On+ being live; this answers only the budget question.
    // UseCachedOnly consumes an attempt from the budget.
    CachedConfigAdvice takeCachedConfigAdvice()
    {
        if (bHasUsableCachedConfig_ && cachedConfigAttempts_ < kMaxCachedConfigAttempts) {
            cachedConfigAttempts_++;
            return CachedConfigAdvice::UseCachedOnly;
        }
        // Automatic mode advances to a protocol that works without the API; manual mode has no next
        // protocol and must surface the failure.
        return isAutomaticMode() ? CachedConfigAdvice::Advance : CachedConfigAdvice::Abort;
    }

    // Snapshot taken at strategy build (see the factory), so the offer-WireGuard decision and the
    // per-attempt gate stay consistent even if a forced re-registration is flagged mid-connect.
    // Also cleared mid-walk when an exhausted config is discarded.
    void setCachedConfigAvailability(bool hasUsableCachedConfig) { bHasUsableCachedConfig_ = hasUsableCachedConfig; }
    void resetCachedConfigBudget() { cachedConfigAttempts_ = 0; }
    bool isCachedConfigExhausted() const { return bHasUsableCachedConfig_ && cachedConfigAttempts_ >= kMaxCachedConfigAttempts; }
    bool hasUsableCachedConfig() const { return bHasUsableCachedConfig_; }
    int cachedConfigAttempts() const { return cachedConfigAttempts_; }

    static constexpr int kMaxCachedConfigAttempts = 2;

signals:
    void protocolStatusChanged(const QVector<types::ProtocolStatus> &status, bool isAutomaticMode);
    void hostnamesResolved();

protected:
    // Shared node pre-selection for API-location strategies: for a WireGuard attempt a remote-IP
    // override from the advanced parameters wins; otherwise the preferred hostname is tried. On a
    // retry a miss advances to the next node.
    static void selectNodeForAttempt(locationsmodel::MutableLocationInfo *locationInfo, types::Protocol nextProtocol,
                                     const QString &preferredNodeHostname, bool isRetry);

    // Shared descriptor assembly for API-location strategies: endpoint fields from the selected node,
    // plus the static-IPs payload when the location is a static-IPs one. Callers supply only their own
    // protocol/port choice.
    static CurrentConnectionDescr descrForSelectedNode(locationsmodel::MutableLocationInfo *locationInfo,
                                                       const api_responses::PortMap &portMap, types::Protocol protocol, uint port);

private:
    bool bHasUsableCachedConfig_ = false;
    int cachedConfigAttempts_ = 0;
};
