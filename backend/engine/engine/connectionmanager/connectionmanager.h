#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include "stunnelmanager.h"
#include "wstunnelmanager.h"
#include "makeovpnfile.h"
#include "makeovpnfilefromcustom.h"

#include "iconnectionmanager.h"
#include "iconnection.h"
#include "engine/networkstatemanager/inetworkstatemanager.h"
#include "testvpntunnel.h"
#include "engine/types/protocoltype.h"

#ifdef Q_OS_MAC
    #include "restorednsmanager_mac.h"
#endif


class ISleepEvents;
class IKEv2Connection;

class ConnectionManager : public IConnectionManager
{
    Q_OBJECT
public:
    explicit ConnectionManager(QObject *parent, IHelper *helper, INetworkStateManager *networkStateManager,
                               ServerAPI *serverAPI, CustomOvpnAuthCredentialsStorage *customOvpnAuthCredentialsStorage);
    virtual ~ConnectionManager();

    virtual void clickConnect(const QByteArray &ovpnConfig, const ServerCredentials &serverCredentials,
                              QSharedPointer<MutableLocationInfo> mli,
                              const ConnectionSettings &connectionSettings, const PortMap &portMap, const ProxySettings &proxySettings,
                              bool bEmitAuthError);

    virtual void clickDisconnect();
    virtual void blockingDisconnect();
    virtual bool isDisconnected();
    virtual QString getLastConnectedIp();

    virtual QString getConnectedTapTunAdapter();
    virtual void removeIkev2ConnectionFromOS();

    virtual void continueWithUsernameAndPassword(const QString &username, const QString &password, bool bNeedReconnect);
    virtual void continueWithPassword(const QString &password, bool bNeedReconnect);

    virtual bool isCustomOvpnConfigCurrentConnection();
    virtual QString getCustomOvpnConfigFilePath();

    virtual bool isStaticIpsLocation();
    virtual StaticIpPortsVector getStatisIps();

    void setMss(int mss);

private slots:
    void onConnectionConnected();
    void onConnectionDisconnected();
    void onConnectionReconnecting();
    void onConnectionError(CONNECTION_ERROR err);
    void onConnectionStatisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes);

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

private:
    enum {STATE_DISCONNECTED, STATE_CONNECTING_FROM_USER_CLICK, STATE_CONNECTED, STATE_RECONNECTING,
          STATE_DISCONNECTING_FROM_USER_CLICK, STATE_WAIT_FOR_NETWORK_CONNECTIVITY, STATE_RECONNECTION_TIME_EXCEED,
          STATE_SLEEP_MODE_NEED_RECONNECT, STATE_WAKEUP_RECONNECTING, STATE_AUTO_DISCONNECT, STATE_ERROR_DURING_CONNECTION};

    IConnection *connector_;
    ISleepEvents *sleepEvents_;
    StunnelManager *stunnelManager_;
    WstunnelManager *wstunnelManager_;
#ifdef Q_OS_MAC
    RestoreDNSManager_mac restoreDnsManager_;
#endif

    QString lastDefaultGateway_;
    QString lastIp_;

    QByteArray lastOvpnConfig_;
    ServerCredentials lastServerCredentials_;
    ProxySettings lastProxySettings_;
    bool bEmitAuthError_;

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

    AutoManualConnectionController::CurrentConnectionDescr currentConnectionDescr_;

    QString usernameForCustomOvpn_;     // can be empty
    QString passwordForCustomOvpn_;     // can be empty

    int mss_;

    void doConnect();
    void doConnectPart2();
    bool checkFails();

    void doMacRestoreProcedures();
    void startReconnectionTimer();
    void waitForNetworkConnectivity();
    void recreateConnector(ProtocolType protocol);
};

#endif // CONNECTIONMANAGER_H
