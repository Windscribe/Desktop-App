#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

#include "engine/connectionmanager/connectors/iconnection.h"
#include "engine/helper/helper.h"
#include "connectrequest.h"
#include "connsettingspolicy/baseconnsettingspolicy.h"
#include "engine/customconfigs/customovpnauthcredentialsstorage.h"
#include "engine/locationsmodel/baselocationinfo.h"
#include "types/connectionsettings.h"
#include "types/enums.h"
#include "types/packetsize.h"
#include "types/protocol.h"
#include "api_responses/portmap.h"

class IConnectionFactory;
class IConnectionPlatformPolicy;
class IConnSettingsPolicyFactory;
class IDnsConfigurator;
class IExtraConfigAccessor;
class INetworkDetectionManager;
class ISleepEvents;
class TestVPNTunnel;

// manage openvpn connection, reconnects, sleep mode, network change, automatic/manual connection mode

class ConnectionManager : public QObject
{
    Q_OBJECT
public:
    // dnsConfigurator is owned by the caller (Engine) and must outlive the ConnectionManager.
    // The seam parameters exist for tests; when null, the production implementations are created.
    // ConnectionManager takes ownership of the seam objects, so they must be heap-allocated.
    explicit ConnectionManager(QObject *parent, Helper *helper, INetworkDetectionManager *networkDetectionManager,
                               CustomOvpnAuthCredentialsStorage *customOvpnAuthCredentialsStorage, IDnsConfigurator *dnsConfigurator,
                               IConnectionFactory *connectionFactory = nullptr,
                               IConnectionPlatformPolicy *platformPolicy = nullptr,
                               IConnSettingsPolicyFactory *connSettingsPolicyFactory = nullptr,
                               ISleepEvents *sleepEvents = nullptr,
                               IExtraConfigAccessor *extraConfig = nullptr);
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

    void continueWithUsernameAndPassword(const QString &username, const QString &password, bool bNeedReconnect);
    void continueWithPassword(const QString &password, bool bNeedReconnect);
    void continueWithPrivKeyPassword(const QString &password, bool bNeedReconnect);

    bool isCustomOvpnConfigCurrentConnection() const;
    QString getCustomOvpnConfigFileName();

    bool isStaticIpsLocation() const;
    api_responses::StaticIpPortsVector getStatisIps();

    void onWireGuardKeyLimitUserResponse(bool deleteOldestKey);

    void setPacketSize(types::PacketSize ps);

    void startTunnelTests();
    bool isAllowFirewallAfterConnection() const;

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
    void errorDuringConnection(CONNECT_ERROR errorCode);
    void reconnecting();
    void statisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes);
    void interfaceUpdated(const QString &interfaceName);  // WireGuard-specific.
    void testTunnelResult(bool success, const QString &ipAddress);
    void internetConnectivityChanged(bool connectivity);
    void protocolPortChanged(const types::Protocol &protocol, const uint port);
    void wireGuardAtKeyLimit();
    void protocolStatusChanged(const QVector<types::ProtocolStatus> &status, bool isAutomaticMode);

    void requestUsername(const QString &pathCustomOvpnConfig);
    void requestPassword(const QString &pathCustomOvpnConfig);
    void requestPrivKeyPassword(const QString &pathCustomOvpnConfig);

    void connectionEnded();

private slots:
    void onConnectionConnected(const AdapterGatewayInfo &connectionAdapterInfo);
    void onConnectionDisconnected();
    void onConnectionReconnecting();
    void onConnectionError(CONNECT_ERROR err);
    void onConnectionStatisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes);
    void onConnectionInterfaceUpdated(const QString &interfaceName);

    void onConnectionPrepared();
    void onConnectionPrepareFailed(CONNECT_ERROR err);
    void onConnectionUserInputRequired(UserInputType type);

    void onSleepMode();
    void onWakeMode();

    void onNetworkOnlineStateChanged(bool isAlive);

    void onTimerReconnection();
    void onConnectTrigger();
    void onConnectingTimeout();

    void onTunnelTestsFinished(bool bSuccess, const QString &ipAddress);

    void onTimerWaitNetworkConnectivity();

    void onHostnamesResolved();

private:
#ifdef WINDSCRIBE_BUILD_TESTS
    // Tests use real QTimers: they read timer state directly and simulate expiry by invoking the
    // timeout slots (stopping single-shot timers first, matching what a real expiry leaves behind).
    friend class TestConnectionManager;
#endif

    enum {STATE_DISCONNECTED, STATE_CONNECTING_FROM_USER_CLICK, STATE_CONNECTED, STATE_RECONNECTING,
          STATE_DISCONNECTING_FROM_USER_CLICK, STATE_WAIT_FOR_NETWORK_CONNECTIVITY, STATE_RECONNECTION_TIME_EXCEED,
          STATE_SLEEP_MODE_NEED_RECONNECT, STATE_WAKEUP_RECONNECTING, STATE_AUTO_DISCONNECT, STATE_ERROR_DURING_CONNECTION};

    Helper *helper_;
    INetworkDetectionManager *networkDetectionManager_;
    CustomOvpnAuthCredentialsStorage *customOvpnAuthCredentialsStorage_;
    IDnsConfigurator *dnsConfigurator_;

    QScopedPointer<IConnectionFactory> connectionFactory_;
    QScopedPointer<IConnectionPlatformPolicy> platformPolicy_;
    QScopedPointer<IConnSettingsPolicyFactory> connSettingsPolicyFactory_;
    QScopedPointer<IExtraConfigAccessor> extraConfig_;

    IConnection *connector_;
    // Under early creation the connector exists before the dial; external entry points that used
    // "connector_ != nullptr" to mean "attempt has dialed" must check this instead.
    bool isConnectorDialed_ = false;
    ISleepEvents *sleepEvents_;

    QScopedPointer<BaseConnSettingsPolicy> connSettingsPolicy_;

    QString lastIp_;

    ConnectRequest lastRequest_;

    QTimer timerWaitNetworkConnectivity_;

    TestVPNTunnel *testVPNTunnel_;

    bool bIgnoreConnectionErrorsForOpenVpn_;
    bool bWasSuccessfullyConnectionAttempt_;
    CONNECT_ERROR latestConnectionError_;

    QTimer timerReconnection_;
    enum { MAX_RECONNECTION_TIME = 60 * 60 * 1000 };  // 1 hour

    // this timer is used to 'rest' between protocol failovers
    QTimer connectTimer_;
    static constexpr int kConnectionWaitTimeMsec = 10 * 1000;

    // this timer is used to cap the login attempt time; the interval comes from the connector's
    // capabilities
    QTimer connectingTimer_;

    int state_;
    bool bLastIsOnline_;
    bool bWakeSignalReceived_;
    bool isFirewallAlwaysOnPlusEnabled_ = false;

    // Snapshot taken when the connection policy is built, so the policy decision and doConnectPart2
    // stay consistent even if a forced re-registration is flagged asynchronously mid-connect.
    bool hasUsableCachedWgConfig_ = false;
    // Under Always On+ a stale cached config can't be refreshed (API blocked); cap the attempts so a
    // config that never establishes can't loop forever. Reset on tunnel success and rotation restart.
    int wgCachedConfigAttempts_ = 0;
    static constexpr int kMaxWgCachedConfigAttempts = 2;

    types::Protocol currentProtocol_;

    CurrentConnectionDescr currentConnectionDescr_;

    QString usernameForCustomOvpn_;     // can be empty
    QString passwordForCustomOvpn_;     // can be empty

    types::PacketSize packetSize_;

    AdapterGatewayInfo defaultAdapterInfo_;
    AdapterGatewayInfo vpnAdapterInfo_;

    DISCONNECT_REASON userDisconnectReason_ = DISCONNECTED_BY_USER;

    void doConnect();
    void doConnectPart2();
    bool checkFails();

    void startReconnectionTimer();
    void waitForNetworkConnectivity();
    void recreateConnector(types::Protocol protocol);
    void restoreConnectionAfterWakeUp();
    void updateConnectionSettingsPolicy(
        const types::ConnectionSettings &connectionSettings,
        const api_responses::PortMap &portMap,
        const types::ProxySettings &proxySettings);
    void connectOrStartConnectTimer();
    void clearCachedWgConfigIfExhausted(CONNECT_ERROR err);
    void startFailoverReconnect();

    void disconnect();
};
