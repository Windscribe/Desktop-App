#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

#include "engine/connectionmanager/connectors/iconnection.h"
#include "attemptstrategy/iconnectionattemptstrategy.h"
#include "connectrequest.h"
#include "engine/locationsmodel/baselocationinfo.h"
#include "types/connectionsettings.h"
#include "types/enums.h"
#include "types/packetsize.h"
#include "types/protocol.h"
#include "api_responses/portmap.h"

class IConnectionFactory;
class IConnectionPlatformPolicy;
class IConnectionAttemptStrategyFactory;
class IDnsConfigurator;
class INetworkDetectionManager;
class ISleepEvents;

// manage openvpn connection, reconnects, sleep mode, network change, automatic/manual connection mode

class ConnectionManager : public QObject
{
    Q_OBJECT
public:
    // dnsConfigurator is owned by the caller (Engine) and must outlive the ConnectionManager.
    // The caller supplies the seam implementations (Engine passes the production ones, tests pass
    // fakes); ConnectionManager takes ownership, so they must be heap-allocated. sleepEvents may be
    // null (platforms without sleep handling).
    explicit ConnectionManager(QObject *parent, INetworkDetectionManager *networkDetectionManager,
                               IDnsConfigurator *dnsConfigurator,
                               IConnectionFactory *connectionFactory,
                               IConnectionPlatformPolicy *platformPolicy,
                               IConnectionAttemptStrategyFactory *attemptStrategyFactory,
                               ISleepEvents *sleepEvents);
    ~ConnectionManager() override;

    void clickConnect(const ConnectRequest &req);

    void clickDisconnect(DISCONNECT_REASON reason = DISCONNECTED_BY_USER);
    void reconnect();
    void blockingDisconnect(bool isSleepEvent);
    bool isDisconnected();

    QString getLastConnectedIp();
    const AdapterGatewayInfo &getDefaultAdapterInfo() const;
    const AdapterGatewayInfo &getVpnAdapterInfo() const;

    void removeIkev2ConnectionFromOS();
    void removeStoredConfig();

    void continueWithUsernameAndPassword(const QString &username, const QString &password, bool bNeedReconnect);
    void continueWithPassword(const QString &password);
    void continueWithPrivKeyPassword(const QString &password, bool bNeedReconnect);

    bool isStaticIpsLocation() const;
    api_responses::StaticIpPortsVector getStatisIps();

    void onWireGuardKeyLimitUserResponse(bool deleteOldestKey);

    // Delivers the outcome of the tunnel test the caller (Engine) ran after connect; ignored unless
    // still connected, so a result racing in behind a drop/disconnect can't drive failover.
    void onTunnelTestResult(bool success, const QString &ipAddress);

    void setPacketSize(types::PacketSize ps);

    bool isAllowFirewallAfterConnectionRuntime() const;

    types::Protocol currentProtocol() const;

    void updateConnectionSettings(
        const types::ConnectionSettings &connectionSettings,
        const api_responses::PortMap &portMap,
        const types::ProxySettings &proxySettings);

    void setFirewallAlwaysOnPlusEnabled(bool isEnabled);

signals:
    void connected();
    void connectingToHostname(const QString &hostname, const QString &ip, const QStringList &dnsServers);
    void disconnected(DISCONNECT_REASON reason);
    void errorDuringConnection(ConnectError errorCode, const CurrentConnectionDescr &descr);
    void reconnecting();
    void statisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes);
    void interfaceUpdated(const QString &interfaceName);  // WireGuard-specific.
    void testTunnelResult(bool success, const QString &ipAddress);
    void internetConnectivityChanged(bool connectivity);
    void protocolPortChanged(const types::Protocol &protocol, const uint port);
    void wireGuardAtKeyLimit();
    void protocolStatusChanged(const QVector<types::ProtocolStatus> &status, bool isAutomaticMode);

    void requestUsername();
    void requestPassword();
    void requestPrivKeyPassword();

    void connectionEnded();

private slots:
    void onConnectionConnected(const AdapterGatewayInfo &connectionAdapterInfo);
    void onConnectionDisconnected();
    void onConnectionReconnecting();
    void onConnectionError(ConnectError err);
    void onConnectionStatisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes);
    void onConnectionInterfaceUpdated(const QString &interfaceName);

    void onConnectionPrepared();
    void onConnectionPrepareFailed(ConnectError err);
    void onConnectionUserInputRequired(UserInputType type);

    void onSleepMode();
    void onWakeMode();

    void onNetworkOnlineStateChanged(bool isAlive);

    void onTimerReconnection();
    void onConnectTrigger();
    void onConnectingTimeout();

    void onTimerWaitNetworkConnectivity();

    void onHostnamesResolved();

private:
#ifdef WINDSCRIBE_BUILD_TESTS
    // Tests use real QTimers: they read timer state directly and simulate expiry by invoking the
    // timeout slots (stopping single-shot timers first, matching what a real expiry leaves behind).
    friend class TestConnectionManager;
#endif

    enum class State {
        kDisconnected,
        kConnecting, // an attempt is live (prepare or dial)
        kConnected,
        kReconnecting, // between attempts / failover; reconnectCause_ distinguishes a wake-initiated one
        kWaitingForNetwork,
        kSleeping,
        kStopping // connector teardown in progress; the outcome to surface is in pendingOutcome_
    };
    Q_ENUM(State)

    // How Reconnecting was entered; meaningful only while state_ is Reconnecting. setState() is the
    // only writer.
    enum class ReconnectCause { Regular, Wake };

    // What to emit once the connector confirms it stopped (consumed in the Stopping case of
    // onConnectionDisconnected).
    struct PendingOutcome {
        enum class Type { Disconnect, Error };
        Type type = Type::Disconnect;
        DISCONNECT_REASON reason = DISCONNECTED_ITSELF;
        ConnectError error = ConnectError::kNoError;
        // A user-initiated stop resets the strategy walk on completion and wins over a repeat click.
        bool isUserInitiated = false;
    };

    INetworkDetectionManager *networkDetectionManager_;
    IDnsConfigurator *dnsConfigurator_;

    QScopedPointer<IConnectionFactory> connectionFactory_;
    QScopedPointer<IConnectionPlatformPolicy> platformPolicy_;
    QScopedPointer<IConnectionAttemptStrategyFactory> attemptStrategyFactory_;

    IConnection *connector_;
    // Under early creation the connector exists before the dial; external entry points that used
    // "connector_ != nullptr" to mean "attempt has dialed" must check this instead.
    bool isConnectorDialed_ = false;
    ISleepEvents *sleepEvents_;

    QScopedPointer<IConnectionAttemptStrategy> attemptStrategy_;

    QString lastIp_;

    ConnectRequest lastRequest_;

    QTimer timerWaitNetworkConnectivity_;

    bool bIgnoreConnectionErrors_;

    QTimer timerReconnection_;
    enum { MAX_RECONNECTION_TIME = 60 * 60 * 1000 };  // 1 hour

    // this timer is used to 'rest' between protocol failovers
    QTimer connectTimer_;
    static constexpr int kConnectionWaitTimeMsec = 10 * 1000;

    // this timer is used to cap the login attempt time; the interval comes from the connector's
    // capabilities
    QTimer connectingTimer_;

    State state_;
    PendingOutcome pendingOutcome_;
    // Wake: the reconnect was initiated by wake-restore and hasn't recorded a failure yet — the
    // attempt gets a fresh reconnection budget and connector-level failures don't consume an endpoint.
    ReconnectCause reconnectCause_ = ReconnectCause::Regular;
    bool bLastIsOnline_;
    bool bWakeSignalReceived_;
    bool isFirewallAlwaysOnPlusEnabled_ = false;
    // Session fact, not walk state: survives strategy recreation, cleared on a fresh connect and on
    // a credential-reprompt reconnect. An exhausted walk restarts instead of giving up when set.
    bool bAttemptSucceeded_ = false;

    types::Protocol currentProtocol_;

    CurrentConnectionDescr currentConnectionDescr_;

    QString usernameForCustomOvpn_;     // can be empty
    QString passwordForCustomOvpn_;     // can be empty

    types::PacketSize packetSize_;

    AdapterGatewayInfo defaultAdapterInfo_;
    AdapterGatewayInfo vpnAdapterInfo_;

    bool isSenderCurrentConnector() const;
    bool isAttemptActive() const;
    void forwardUserInputOrReconnect(const UserInputResponse &response, bool bNeedReconnect);
    void setState(State state, ReconnectCause cause = ReconnectCause::Regular);
    void enterStopping();
    void stopWithDisconnect(DISCONNECT_REASON reason, bool isUserInitiated);
    void stopWithError(ConnectError err);

    void startAttempt();
    void handleAttemptFailed(bool bDisconnectConnector);
    void handleReconnecting();

    void enterReconnecting();
    void applyGaiIpv4Priority();
    void startReconnectionTimer();
    void waitForNetworkConnectivity();
    void recreateConnector(types::Protocol protocol);
    void restoreConnectionAfterWakeUp();
    void recreateAttemptStrategy(
        const types::ConnectionSettings &connectionSettings,
        const api_responses::PortMap &portMap,
        const types::ProxySettings &proxySettings);
    void connectOrStartConnectTimer();
    void clearCachedConfigIfExhausted(bool isCachedConfigFailure);
    void startFailoverReconnect();
    void failWithError(ConnectError err);

    void disconnect();
};
