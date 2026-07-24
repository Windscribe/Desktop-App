#pragma once

#include <variant>
#include <QThread>
#include "engine/adaptergatewayinfo.h"
#include "engine/connectionmanager/attemptstrategy/iconnectionattemptstrategy.h"
#include "types/enums.h"
#include "types/packetsize.h"
#include "types/protocol.h"

// Static traits of a connector instance: a pure function of the constructor arguments, fixed for the
// connector's lifetime, never dependent on prepare() results. Runtime-derived facts stay virtual methods.
struct ConnectorCapabilities
{
    int connectTimeoutMs = 30 * 1000;
    bool supportsCachedConfig = false;
    bool needsSystemDnsRestore = false;
};

enum class ConfigFetchMode { Normal, CachedOnly };

// Generic per-attempt data for prepare(). Protocol-specific session data is handed to the connector
// at construction by the factory, not passed here.
struct AttemptEnvironment
{
    types::PacketSize packetSize;
    AdapterGatewayInfo defaultAdapterInfo;
    QString primaryDnsServer;
    ConfigFetchMode configFetchMode = ConfigFetchMode::Normal;
    // Controls whether IPv6 is enabled for tunnels that can carry dual-stack traffic. kIPv4Only
    // forces v6 to be stripped from the tunnel config; kAuto keeps v6 if the node/config supports it.
    IpStack ipStackEgress = IpStack::kAuto;
};

enum class UserInputType { Username, Password, PrivKeyPassword, KeyLimitConsent };

struct UsernameResponse
{
    QString username;
    QString password;
};

struct PasswordResponse
{
    QString password;
};

struct PrivKeyPasswordResponse
{
    QString password;
};

// Arrival of the response is the consent; a decline never reaches the connector.
struct KeyLimitConsentResponse
{
};

using UserInputResponse = std::variant<UsernameResponse, PasswordResponse, PrivKeyPasswordResponse, KeyLimitConsentResponse>;

class IConnection : public QThread
{
    Q_OBJECT

public:
    IConnection(QObject *parent, types::Protocol protocol) : QThread(parent), protocol_(protocol) {}
    virtual ~IConnection() {}

    const ConnectorCapabilities &capabilities() const { return capabilities_; }

    // Async: completes with prepared(), prepareFailed() or userInputRequired(), delivered through a
    // queued connection. startDisconnect() during prepare cancels it and emits disconnected().
    // Refines protocol_ to the attempt's dialed variant: the ctor value may be the pre-resolve
    // family representative (emergency and custom-config connectors are created before the
    // per-endpoint/per-remote variant is known).
    // Non-virtual: the capture below feeds the effective* readbacks (firewall whitelist) and
    // currentProtocol(), so no connector may skip it; per-connector work goes in prepareImpl().
    void prepare(const CurrentConnectionDescr &descr, const AttemptEnvironment &env)
    {
        descr_ = descr;
        env_ = env;
        protocol_ = descr.protocol;
        prepareImpl();
    }

    // Synchronous post-stop cleanup; safe to call on a connector that never prepared or connected.
    virtual void teardown() {}

    virtual void startConnect() = 0;
    virtual void startDisconnect() = 0;
    virtual bool isDisconnected() const = 0;
    virtual void waitForDisconnect() {}
    // Runtime half of the custom-config allow-firewall-after-connect decision; the config-file
    // directive is ANDed in Engine-side.
    virtual bool isAllowFirewallAfterConnectionRuntime() const { return true; }

    // Post-prepare readbacks; overridden where prepare() rewrites the endpoint or knows the tunnel DNS.
    virtual QString effectiveHostname() const { return descr_.hostname; }
    virtual QString effectiveIp() const { return descr_.ip; }
    virtual QString tunnelDefaultDns() const { return QString(); }
    // Whether the tunnel carries IPv6 egress for this attempt.
    virtual bool isDualStackEgress() const { return false; }

    virtual void continueWithUserInput(const UserInputResponse & /*response*/) {}

signals:
    void prepared();
    void prepareFailed(ConnectError err);
    void userInputRequired(UserInputType type);

    void connected(const AdapterGatewayInfo &connectionAdapterInfo);
    void disconnected();
    void reconnecting();
    void error(ConnectError err);
    void statisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes);
    void interfaceUpdated(const QString &interfaceName);  // WireGuard-specific.

protected:
    virtual void prepareImpl() { emit prepared(); }

    types::Protocol protocol_;
    ConnectorCapabilities capabilities_;
    CurrentConnectionDescr descr_;
    AttemptEnvironment env_;
};
