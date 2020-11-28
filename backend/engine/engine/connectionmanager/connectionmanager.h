#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include "stunnelmanager.h"
#include "wstunnelmanager.h"
#include "makeovpnfile.h"
#include "makeovpnfilefromcustom.h"

#include "iconnection.h"
#include "engine/networkstatemanager/inetworkstatemanager.h"
#include "testvpntunnel.h"
#include "engine/types/protocoltype.h"
#include "engine/types/wireguardconfig.h"
#include "connsettingspolicy/baseconnsettingspolicy.h"
#include "engine/customconfigs/customovpnauthcredentialsstorage.h"
#include "engine/apiinfo/servercredentials.h"
#include "engine/locationsmodel/baselocationinfo.h"
#include "engine/types/connectionsettings.h"

#ifdef Q_OS_MAC
    #include "restorednsmanager_mac.h"
#endif


class ISleepEvents;
class IKEv2Connection;

// manage openvpn connection, reconnects, sleep mode, network change, automatic/manual connection mode

class ConnectionManager : public QObject
{
    Q_OBJECT
public:
    explicit ConnectionManager(QObject *parent, IHelper *helper, INetworkStateManager *networkStateManager,
                               ServerAPI *serverAPI, CustomOvpnAuthCredentialsStorage *customOvpnAuthCredentialsStorage);
    ~ConnectionManager() override;

    void clickConnect(const QString &ovpnConfig, const apiinfo::ServerCredentials &serverCredentials,
                      QSharedPointer<locationsmodel::BaseLocationInfo> bli,
                      const ConnectionSettings &connectionSettings, const apiinfo::PortMap &portMap, const ProxySettings &proxySettings,
                      bool bEmitAuthError, const QString &customConfigPath);

    void clickDisconnect();
    void blockingDisconnect();
    bool isDisconnected();
    QString getLastConnectedIp();

    QString getConnectedTapTunAdapter();
    void removeIkev2ConnectionFromOS();

    void continueWithUsernameAndPassword(const QString &username, const QString &password, bool bNeedReconnect);
    void continueWithPassword(const QString &password, bool bNeedReconnect);

    bool isCustomOvpnConfigCurrentConnection() const;
    QString getCustomOvpnConfigFileName();

    bool isStaticIpsLocation() const;
    apiinfo::StaticIpPortsVector getStatisIps();

    void setWireGuardConfig(QSharedPointer<WireGuardConfig> config);

    void setMss(int mss);
    void setPacketSize(ProtoTypes::PacketSize ps);

    void startTunnelTests();

signals:
    void connected();
    void connectingToHostname(const QString &hostname, const QString &ip);
    void disconnected(DISCONNECT_REASON reason);
    void errorDuringConnection(CONNECTION_ERROR errorCode);
    void reconnecting();
    void statisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes);
    void interfaceUpdated(const QString &interfaceName);  // WireGuard-specific.
    void testTunnelResult(bool success, const QString &ipAddress);
    void showFailedAutomaticConnectionMessage();
    void internetConnectivityChanged(bool connectivity);
    void protocolPortChanged(const ProtoTypes::Protocol &protocol, const uint port);
    void getWireGuardConfig();

    void requestUsername(const QString &pathCustomOvpnConfig);
    void requestPassword(const QString &pathCustomOvpnConfig);

private slots:
    void onConnectionConnected();
    void onConnectionDisconnected();
    void onConnectionReconnecting();
    void onConnectionError(CONNECTION_ERROR err);
    void onConnectionStatisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes);
    void onConnectionInterfaceUpdated(const QString &interfaceName);

    void onConnectionRequestUsername();
    void onConnectionRequestPassword();

    void onSleepMode();
    void onWakeMode();

    void onNetworkStateChanged(bool isAlive, const QString &networkInterface);

    void onTimerReconnection();

    void onStunnelFinishedBeforeConnection();
    void onWstunnelFinishedBeforeConnection();
    void onWstunnelStarted();
    void onTunnelTestsFinished(bool bSuccess, const QString &ipAddress);

    void onTimerWaitNetworkConnectivity();

    void onHostnamesResolved();

private:
    enum {STATE_DISCONNECTED, STATE_CONNECTING_FROM_USER_CLICK, STATE_CONNECTED, STATE_RECONNECTING,
          STATE_DISCONNECTING_FROM_USER_CLICK, STATE_WAIT_FOR_NETWORK_CONNECTIVITY, STATE_RECONNECTION_TIME_EXCEED,
          STATE_SLEEP_MODE_NEED_RECONNECT, STATE_WAKEUP_RECONNECTING, STATE_AUTO_DISCONNECT, STATE_ERROR_DURING_CONNECTION};

    IHelper *helper_;
    INetworkStateManager *networkStateManager_;
    CustomOvpnAuthCredentialsStorage *customOvpnAuthCredentialsStorage_;

    IConnection *connector_;
    ISleepEvents *sleepEvents_;
    StunnelManager *stunnelManager_;
    WstunnelManager *wstunnelManager_;
#ifdef Q_OS_MAC
    RestoreDNSManager_mac restoreDnsManager_;
#endif

    QScopedPointer<BaseConnSettingsPolicy> connSettingsPolicy_;

    QString lastDefaultGateway_;
    QString lastIp_;

    QString lastOvpnConfig_;
    apiinfo::ServerCredentials lastServerCredentials_;
    ProxySettings lastProxySettings_;
    bool bEmitAuthError_;

    QString customConfigPath_;

    QTimer timerWaitNetworkConnectivity_;

    MakeOVPNFile *makeOVPNFile_;
    MakeOVPNFileFromCustom *makeOVPNFileFromCustom_;
    TestVPNTunnel *testVPNTunnel_;

    bool bNeedResetTap_;
    bool bIgnoreConnectionErrorsForOpenVpn_;
    bool bWasSuccessfullyConnectionAttempt_;
    CONNECTION_ERROR latestConnectionError_;

    QTimer timerReconnection_;
    enum { MAX_RECONNECTION_TIME = 60 * 60 * 1000 };  // 1 hour

    int state_;
    bool bLastIsOnline_;

    ProtocolType currentProtocol_;

    CurrentConnectionDescr currentConnectionDescr_;

    QString usernameForCustomOvpn_;     // can be empty
    QString passwordForCustomOvpn_;     // can be empty

    ProtoTypes::PacketSize packetSize_;

    QSharedPointer<WireGuardConfig> wireGuardConfig_;

    void doConnect();
    void doConnectPart2();
    void doConnectPart3();
    bool checkFails();

    void doMacRestoreProcedures();
    void startReconnectionTimer();
    void waitForNetworkConnectivity();
    void recreateConnector(ProtocolType protocol);
};

#endif // CONNECTIONMANAGER_H
