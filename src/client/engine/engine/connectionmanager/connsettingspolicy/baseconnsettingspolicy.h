#pragma once

#include <QVector>
#include <QObject>
#include "api_responses/staticips.h"
#include "types/protocolstatus.h"
#include "engine/wireguardconfig/wireguardconfig.h"

enum CONNECTION_NODE_TYPE {
    CONNECTION_NODE_ERROR,
    CONNECTION_NODE_DEFAULT,
    CONNECTION_NODE_CUSTOM_CONFIG,
    CONNECTION_NODE_STATIC_IPS
};

struct CurrentConnectionDescr
{
    CONNECTION_NODE_TYPE connectionNodeType = CONNECTION_NODE_ERROR;

    // fields for CONNECTION_NODE_DEFAULT
    QString ip;
    uint port = 0;
    types::Protocol protocol;
    QString hostname;
    QString verifyX509name;
    bool isIpv6Support = false;

    // fields for CONNECTION_NODE_CUSTOM_CONFIG
    QString ovpnData;
    QString customConfigFilename;
    QString remoteCmdLine;
    bool isAllowFirewallAfterConnection;

    // fields for WireGuard
    QString wgPeerPublicKey;
    QSharedPointer<WireGuardConfig> wgCustomConfig;

    // fields for static ips
    QString username;
    QString password;
    api_responses::StaticIpPortsVector staticIpPorts;
};

// helper class for ConnectionManager
class BaseConnSettingsPolicy : public QObject
{
    Q_OBJECT
public:
    BaseConnSettingsPolicy() : QObject(nullptr), bStarted_(false) {}
    virtual ~BaseConnSettingsPolicy() {}

    //virtual void startWith(QSharedPointer<const locationsmodel::BaseLocationInfo> mli, const ConnectionSettings &connectionSettings,
    //               const apiinfo::PortMap &portMap, bool isProxyEnabled);
    void start()
    {
        bStarted_ = true;
    }
    void stop()
    {
        bStarted_ = false;
    }

    virtual void reset() = 0;
    virtual void debugLocationInfoToLog() const = 0;
    virtual void putFailedConnection() = 0;
    virtual bool isFailed() const = 0;
    virtual CurrentConnectionDescr getCurrentConnectionSettings() const = 0;
    virtual bool isAutomaticMode() = 0;
    virtual bool isCustomConfig() = 0;
    virtual void resolveHostnames() = 0;
    virtual bool hasProtocolChanged() = 0;

    // Protocol of the next attempt, valid before resolveHostnames() completes, so the connector can
    // be created at attempt start. Custom configs answer with the family representative (the exact
    // per-remote variant is only known post-resolve).
    virtual types::Protocol preResolveProtocol() const { return getCurrentConnectionSettings().protocol; }

    // Whether an attempt that finds no usable network parks and polls for connectivity. Policies
    // that must fail fast instead (emergency connect) return false.
    virtual bool shouldWaitForNetwork() const { return true; }
    // An endpoint-list policy walks its list on failures the regular policies treat as
    // attempt-fatal (local spawn/socket errors, bare process death).
    virtual bool shouldRetryOnAttemptFailure() const { return false; }

signals:
    void protocolStatusChanged(const QVector<types::ProtocolStatus> &status, bool isAutomaticMode);
    void hostnamesResolved();

protected:
    bool bStarted_;
};
