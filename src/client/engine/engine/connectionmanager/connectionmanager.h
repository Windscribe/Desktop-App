#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include "ctrldmanager/ictrldmanager.h"
#include "engine/connectionmanager/connectors/openvpn/makeovpnfile.h"
#include "engine/connectionmanager/connectors/openvpn/makeovpnfilefromcustom.h"
#include "engine/connectionmanager/connectors/openvpn/stunnelmanager.h"
#include "engine/connectionmanager/connectors/openvpn/wstunnelmanager.h"

#include "engine/connectionmanager/connectors/iconnection.h"
#include "engine/wireguardconfig/wireguardconfig.h"
#include "connsettingspolicy/baseconnsettingspolicy.h"
#include "engine/customconfigs/customovpnauthcredentialsstorage.h"
#include "api_responses/servercredentials.h"
#include "engine/locationsmodel/baselocationinfo.h"
#include "types/connectionsettings.h"
#include "types/enums.h"
#include "types/packetsize.h"
#include "types/protocol.h"
#include "types/connecteddnsinfo.h"
#include "api_responses/portmap.h"

#ifdef Q_OS_MACOS
    #include "restorednsmanager_mac.h"
#endif

class IConnectionFactory;
class IConnectionPlatformPolicy;
class IConnSettingsPolicyFactory;
class IExtraConfigAccessor;
class INetworkDetectionManager;
class ISleepEvents;
class TestVPNTunnel;
enum class WireGuardConfigRetCode;
class GetWireGuardConfig;

struct ConnectRequest
{
    QString ovpnConfig;
    api_responses::ServerCredentials serverCredentialsOpenVpn;
    api_responses::ServerCredentials serverCredentialsIkev2;
    QSharedPointer<locationsmodel::BaseLocationInfo> bli;
    types::ConnectionSettings connectionSettings;
    api_responses::PortMap portMap;
    types::ProxySettings proxySettings;
    bool bEmitAuthError = false;
    api_responses::AmneziawgUnblockParams amneziawgParams;
    QString amneziawgPreset;
    QString customSni;
    QString preferredNodeHostname;

    // Controls whether IPv6 is enabled for VPN tunnels that can carry dual-stack traffic
    // (currently: WireGuard from API + WireGuard custom configs). kIPv4Only forces v6 to
    // be stripped from the tunnel config; kAuto keeps v6 if the node/config supports it.
    IpStack ipStackEgress = IpStack::kAuto;
};

// manage openvpn connection, reconnects, sleep mode, network change, automatic/manual connection mode

class ConnectionManager : public QObject
{
    Q_OBJECT
public:
    // The seam parameters exist for tests; when null, the production implementations are created.
    // ConnectionManager takes ownership of the seam objects, so they must be heap-allocated.
    explicit ConnectionManager(QObject *parent, Helper *helper, INetworkDetectionManager *networkDetectionManager,
                               CustomOvpnAuthCredentialsStorage *customOvpnAuthCredentialsStorage,
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

    void setConnectedDnsInfo(const types::ConnectedDnsInfo &info);
    const types::ConnectedDnsInfo &connectedDnsInfo() const;

    void removeIkev2ConnectionFromOS();

    void continueWithUsernameAndPassword(const QString &username, const QString &password, bool bNeedReconnect);
    void continueWithPassword(const QString &password, bool bNeedReconnect);
    void continueWithPrivKeyPassword(const QString &password, bool bNeedReconnect);

    bool isCustomOvpnConfigCurrentConnection() const;
    QString getCustomOvpnConfigFileName();

    bool isStaticIpsLocation() const;
    api_responses::StaticIpPortsVector getStatisIps();

    void onWireGuardKeyLimitUserResponse(bool deleteOldestKey);

    void setMss(int mss);
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

    void onConnectionRequestUsername();
    void onConnectionRequestPassword();
    void onConnectionRequestPrivKeyPassword();

    void onSleepMode();
    void onWakeMode();

    void onNetworkOnlineStateChanged(bool isAlive);

    void onTimerReconnection();
    void onConnectTrigger();
    void onConnectingTimeout();

    void onWstunnelStarted();
    void onTunnelTestsFinished(bool bSuccess, const QString &ipAddress);

    void onTimerWaitNetworkConnectivity();

    void onHostnamesResolved();

    void onGetWireGuardConfigAnswer(WireGuardConfigRetCode retCode, const WireGuardConfig &config);

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

    QScopedPointer<IConnectionFactory> connectionFactory_;
    QScopedPointer<IConnectionPlatformPolicy> platformPolicy_;
    QScopedPointer<IConnSettingsPolicyFactory> connSettingsPolicyFactory_;
    QScopedPointer<IExtraConfigAccessor> extraConfig_;

    IConnection *connector_;
    ISleepEvents *sleepEvents_;
    StunnelManager *stunnelManager_;
    WstunnelManager *wstunnelManager_;
    ICtrldManager *ctrldManager_;

#ifdef Q_OS_MACOS
    RestoreDNSManager_mac restoreDnsManager_;
#endif

    QScopedPointer<BaseConnSettingsPolicy> connSettingsPolicy_;

    QString lastIp_;

    ConnectRequest lastRequest_;

    QTimer timerWaitNetworkConnectivity_;

    MakeOVPNFile *makeOVPNFile_;
    MakeOVPNFileFromCustom *makeOVPNFileFromCustom_;
    TestVPNTunnel *testVPNTunnel_;

    bool bIgnoreConnectionErrorsForOpenVpn_;
    bool bWasSuccessfullyConnectionAttempt_;
    CONNECT_ERROR latestConnectionError_;

    QTimer timerReconnection_;
    enum { MAX_RECONNECTION_TIME = 60 * 60 * 1000 };  // 1 hour

    // this timer is used to 'rest' between protocol failovers
    QTimer connectTimer_;
    static constexpr int kConnectionWaitTimeMsec = 10 * 1000;

    // this timer is used to cap the login attempt time
    QTimer connectingTimer_;
    static constexpr int kConnectingTimeoutWireGuard = 20 * 1000;
    static constexpr int kConnectingTimeout = 30 * 1000;

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

    WireGuardConfig wireGuardConfig_;
    GetWireGuardConfig *getWireGuardConfig_ = nullptr;

    AdapterGatewayInfo defaultAdapterInfo_;
    AdapterGatewayInfo vpnAdapterInfo_;

    types::ConnectedDnsInfo connectedDnsInfo_;

    DISCONNECT_REASON userDisconnectReason_ = DISCONNECTED_BY_USER;

    void doConnect();
    void doConnectPart2();
    void doConnectPart3();
    bool checkFails();

    void doMacRestoreProcedures();
    void startReconnectionTimer();
    void waitForNetworkConnectivity();
    void recreateConnector(types::Protocol protocol);
    void restoreConnectionAfterWakeUp();
    void updateConnectionSettingsPolicy(
        const types::ConnectionSettings &connectionSettings,
        const api_responses::PortMap &portMap,
        const types::ProxySettings &proxySettings);
    void connectOrStartConnectTimer();
    void getWireGuardConfig(const QString &serverName, bool deleteOldestKey, bool useCachedConfigOnly = false);
    QString dnsServersFromConnectedDnsInfo() const;

    void disconnect();
};
