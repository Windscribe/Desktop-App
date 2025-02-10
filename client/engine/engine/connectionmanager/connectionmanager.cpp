#include "utils/log/logger.h"
#include <QStandardPaths>
#include <QThread>
#include <QCoreApplication>
#include <QDateTime>
#include <QUdpSocket>
#include <QRandomGenerator>

#include "isleepevents.h"
#include "openvpnconnection.h"
#include "engine/crossplatformobjectfactory.h"
#include "testvpntunnel.h"
#include "engine/wireguardconfig/getwireguardconfig.h"

#include "utils/ws_assert.h"
#include "utils/utils.h"
#include "types/enums.h"
#include "types/connectionsettings.h"
#include "engine/getdeviceid.h"

#include "engine/networkdetectionmanager/inetworkdetectionmanager.h"
#include "utils/extraconfig.h"
#include "utils/ipvalidation.h"
#include "types/global_consts.h"

#include "connsettingspolicy/autoconnsettingspolicy.h"
#include "connsettingspolicy/manualconnsettingspolicy.h"
#include "connsettingspolicy/customconfigconnsettingspolicy.h"


// Had to move this here to prevent a compile error with boost already including winsock.h
#include "connectionmanager.h"


#ifdef Q_OS_WIN
    #include "sleepevents_win.h"
    #include "ikev2connection_win.h"
    #include "wireguardconnection_win.h"
    #include "engine/helper/helper_win.h"
    #include "utils/winutils.h"
#elif defined Q_OS_MACOS
    #include "sleepevents_mac.h"
    #include "utils/macutils.h"
    #include "ikev2connection_mac.h"
    #include "engine/helper/helper_mac.h"
    #include "wireguardconnection_posix.h"
#elif defined Q_OS_LINUX
    #include "ikev2connection_linux.h"
    #include "wireguardconnection_posix.h"
#endif

ConnectionManager::ConnectionManager(QObject *parent, IHelper *helper, INetworkDetectionManager *networkDetectionManager, CustomOvpnAuthCredentialsStorage *customOvpnAuthCredentialsStorage) : QObject(parent),
    helper_(helper),
    networkDetectionManager_(networkDetectionManager),
    customOvpnAuthCredentialsStorage_(customOvpnAuthCredentialsStorage),
    connector_(nullptr),
    sleepEvents_(nullptr),
    stunnelManager_(nullptr),
    wstunnelManager_(nullptr),
    ctrldManager_(nullptr),
    bEmitAuthError_(false),
    makeOVPNFile_(nullptr),
    makeOVPNFileFromCustom_(nullptr),
    testVPNTunnel_(nullptr),
    bIgnoreConnectionErrorsForOpenVpn_(false),
    bWasSuccessfullyConnectionAttempt_(false),
    state_(STATE_DISCONNECTED),
    bLastIsOnline_(true),
    bWakeSignalReceived_(false),
    currentConnectionDescr_()
{
    connect(&timerReconnection_, &QTimer::timeout, this, &ConnectionManager::onTimerReconnection);
    connect(&connectTimer_, &QTimer::timeout, this, &ConnectionManager::onConnectTrigger);
    connect(&connectingTimer_, &QTimer::timeout, this, &ConnectionManager::onConnectingTimeout);

    stunnelManager_ = new StunnelManager(this, helper);
    connect(stunnelManager_, &StunnelManager::stunnelStarted, this, &ConnectionManager::onWstunnelStarted);

    wstunnelManager_ = new WstunnelManager(this, helper);
    connect(wstunnelManager_, &WstunnelManager::wstunnelStarted, this, &ConnectionManager::onWstunnelStarted);

    ctrldManager_ = CrossPlatformObjectFactory::createCtrldManager(this, helper, ExtraConfig::instance().getLogCtrld());

    testVPNTunnel_ = new TestVPNTunnel(this);
    connect(testVPNTunnel_, &TestVPNTunnel::testsFinished, this, &ConnectionManager::onTunnelTestsFinished);

    makeOVPNFile_ = new MakeOVPNFile();
    makeOVPNFileFromCustom_ = new MakeOVPNFileFromCustom();

#ifdef Q_OS_WIN
    sleepEvents_ = new SleepEvents_win(this);
#elif defined Q_OS_MACOS
    sleepEvents_ = new SleepEvents_mac(this);
#endif

    connect(networkDetectionManager_, &INetworkDetectionManager::onlineStateChanged, this, &ConnectionManager::onNetworkOnlineStateChanged);

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
    connect(sleepEvents_, &ISleepEvents::gotoSleep, this, &ConnectionManager::onSleepMode);
    connect(sleepEvents_, &ISleepEvents::gotoWake, this, &ConnectionManager::onWakeMode);
#endif

    connect(&timerWaitNetworkConnectivity_, &QTimer::timeout, this, &ConnectionManager::onTimerWaitNetworkConnectivity);
}

ConnectionManager::~ConnectionManager()
{
    SAFE_DELETE(testVPNTunnel_);
    SAFE_DELETE(connector_);
    SAFE_DELETE(stunnelManager_);
    SAFE_DELETE(wstunnelManager_);
    SAFE_DELETE(ctrldManager_);
    SAFE_DELETE(makeOVPNFile_);
    SAFE_DELETE(makeOVPNFileFromCustom_);
    SAFE_DELETE(sleepEvents_);
    SAFE_DELETE(getWireGuardConfig_);
}

QString ConnectionManager::udpStuffingWithNtp(const QString &ip, const quint16 port)
{
    // Special secret sause for Russia ;)
    char simpleBuf[8] = {0};
    simpleBuf[0] = 1;
    simpleBuf[4] = 1;

    char ntpBuf[48] = {0};
    // NTP client behavior as seen in Linux with chrony
    ntpBuf[0] = 0x23; // ntp ver=4, mode=client
    ntpBuf[2] = 0x09; // polling interval=9
    ntpBuf[3] = 0x20; // clock precision
    quint64 *ntpRand = (quint64*)&ntpBuf[40];

    QUdpSocket udpSocket = QUdpSocket();
    udpSocket.bind(QHostAddress::Any, 0);
    const QString localPort = QString::number(udpSocket.localPort());

    // Send "secret" packet first
    udpSocket.writeDatagram(simpleBuf, sizeof(simpleBuf), QHostAddress(ip), port);

    // Send NTP packet, repeat up to 40 times. Bounded argument is exclusive.
    for (int i=0; i<=20+QRandomGenerator::global()->bounded(20); i++) {
        *ntpRand = QRandomGenerator::global()->generate64();
        udpSocket.writeDatagram(ntpBuf, sizeof(ntpBuf), QHostAddress(ip), port);
    }

    udpSocket.close();
    return localPort;
}

void ConnectionManager::clickConnect(const QString &ovpnConfig,
                                     const api_responses::ServerCredentials &serverCredentialsOpenVpn,
                                     const api_responses::ServerCredentials &serverCredentialsIkev2,
                                     QSharedPointer<locationsmodel::BaseLocationInfo> bli,
                                     const types::ConnectionSettings &connectionSettings,
                                     const api_responses::PortMap &portMap, const types::ProxySettings &proxySettings,
                                     bool bEmitAuthError, const QString &customConfigPath, bool isAntiCensorship)
{
    WS_ASSERT(state_ == STATE_DISCONNECTED);

    lastOvpnConfig_ = ovpnConfig;
    lastServerCredentialsOpenVpn_ = serverCredentialsOpenVpn;
    lastServerCredentialsIkev2_ = serverCredentialsIkev2;
    lastProxySettings_ = proxySettings;
    bEmitAuthError_ = bEmitAuthError;
    customConfigPath_ = customConfigPath;
    isAntiCensorship_ = isAntiCensorship;
    bli_ = bli;

    bWasSuccessfullyConnectionAttempt_ = false;

    usernameForCustomOvpn_.clear();
    passwordForCustomOvpn_.clear();

    state_= STATE_CONNECTING_FROM_USER_CLICK;

    // if we had a connector before, get rid of it.  This is because we don't want to receive events from a
    // previous connection if a new connection has started.
    if (connector_) {
        currentProtocol_ = types::Protocol::UNINITIALIZED;
        SAFE_DELETE(connector_);
        connector_ = NULL;
    }

    updateConnectionSettingsPolicy(connectionSettings, portMap, proxySettings);

    connSettingsPolicy_->debugLocationInfoToLog();
    doConnect();
}

void ConnectionManager::clickDisconnect(DISCONNECT_REASON reason)
{
    WS_ASSERT(state_ == STATE_CONNECTING_FROM_USER_CLICK || state_ == STATE_CONNECTED || state_ == STATE_RECONNECTING ||
              state_ == STATE_WAKEUP_RECONNECTING || state_ == STATE_DISCONNECTING_FROM_USER_CLICK ||
              state_ == STATE_WAIT_FOR_NETWORK_CONNECTIVITY || state_ == STATE_DISCONNECTED);

    timerWaitNetworkConnectivity_.stop();
    connectTimer_.stop();
    connectingTimer_.stop();

    if (state_ != STATE_DISCONNECTING_FROM_USER_CLICK) {
        state_ = STATE_DISCONNECTING_FROM_USER_CLICK;
        qCDebug(LOG_CONNECTION) << "ConnectionManager::clickDisconnect()";
        if (connector_) {
            userDisconnectReason_ = reason;
            connector_->startDisconnect();
        } else {
            disconnect();
            if (!connSettingsPolicy_.isNull())
            {
                connSettingsPolicy_->reset();
            }
            stunnelManager_->killProcess();
            wstunnelManager_->killProcess();
            ctrldManager_->killProcess();
            emit disconnected(reason);
        }
    }
}

void ConnectionManager::reconnect()
{
    if (connector_) {
        onConnectionReconnecting();
    }
}

void ConnectionManager::blockingDisconnect()
{
    if (connector_)
    {
        if (!connector_->isDisconnected())
        {
            testVPNTunnel_->stopTests();
            connector_->blockSignals(true);
            QElapsedTimer elapsedTimer;
            elapsedTimer.start();
            connector_->startDisconnect();
            while (!connector_->isDisconnected())
            {
                QThread::msleep(1);
                qApp->processEvents();

                if (elapsedTimer.elapsed() > 10000)
                {
                    qCWarning(LOG_CONNECTION) << "ConnectionManager::blockingDisconnect() delay more than 10 seconds";
                    connector_->startDisconnect();
                    break;
                }
            }
            connector_->blockSignals(false);
            doMacRestoreProcedures();
            stunnelManager_->killProcess();
            wstunnelManager_->killProcess();
            ctrldManager_->killProcess();

            if (!connSettingsPolicy_.isNull())
            {
                connSettingsPolicy_->reset();
            }

            disconnect();
        }
    }
}

bool ConnectionManager::isDisconnected()
{
    if (state_ == STATE_DISCONNECTED)
    {
        if (connector_)
        {
            WS_ASSERT(connector_->isDisconnected());
        }
    }
    return state_ == STATE_DISCONNECTED;
}

QString ConnectionManager::getLastConnectedIp()
{
    return lastIp_;
}

const AdapterGatewayInfo &ConnectionManager::getDefaultAdapterInfo() const
{
    return defaultAdapterInfo_;
}

const AdapterGatewayInfo &ConnectionManager::getVpnAdapterInfo() const
{
    WS_ASSERT(state_ == STATE_CONNECTED); // make sense only in connected state
    return vpnAdapterInfo_;
}

void ConnectionManager::setConnectedDnsInfo(const types::ConnectedDnsInfo &info)
{
    connectedDnsInfo_ = info;
}

const types::ConnectedDnsInfo &ConnectionManager::connectedDnsInfo() const
{
    return connectedDnsInfo_;
}

void ConnectionManager::removeIkev2ConnectionFromOS()
{
#ifdef Q_OS_WIN
    IKEv2Connection_win::removeIkev2ConnectionFromOS();
#elif defined Q_OS_MACOS
    IKEv2Connection_mac::removeIkev2ConnectionFromOS();
#endif
}

void ConnectionManager::continueWithUsernameAndPassword(const QString &username, const QString &password, bool bNeedReconnect)
{
    WS_ASSERT(connector_ != NULL);
    usernameForCustomOvpn_ = username;
    passwordForCustomOvpn_ = password;

    if (connector_) {
        if (bNeedReconnect) {
            bWasSuccessfullyConnectionAttempt_ = false;
            state_= STATE_CONNECTING_FROM_USER_CLICK;
            doConnect();
        } else {
            connector_->continueWithUsernameAndPassword(username, password);
        }
    }
}

void ConnectionManager::continueWithPassword(const QString &password, bool /*bNeedReconnect*/)
{
    WS_ASSERT(connector_ != NULL);
    if (connector_) {
        connector_->continueWithPassword(password);
    }
}

void ConnectionManager::continueWithPrivKeyPassword(const QString &password, bool bNeedReconnect)
{
    WS_ASSERT(connector_ != NULL && connector_->getConnectionType() == ConnectionType::OPENVPN);

    if (connector_ && connector_->getConnectionType() == ConnectionType::OPENVPN) {

        if (bNeedReconnect) {
            static_cast<OpenVPNConnection *>(connector_)->setPrivKeyPassword(password);

            bWasSuccessfullyConnectionAttempt_ = false;
            state_= STATE_CONNECTING_FROM_USER_CLICK;
            doConnect();
        } else {
            static_cast<OpenVPNConnection *>(connector_)->continueWithPrivKeyPassword(password);
        }
    }
}

void ConnectionManager::onConnectionConnected(const AdapterGatewayInfo &connectionAdapterInfo)
{
    qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionConnected(), state_ =" << state_;

    vpnAdapterInfo_ = connectionAdapterInfo;

#ifdef Q_OS_WIN
    // Set network category
    Helper_win *helper_win = dynamic_cast<Helper_win *>(helper_);
    if (connector_->getConnectionType() == ConnectionType::WIREGUARD) {
        helper_win->setNetworkCategory(QString::fromStdWString(kWireGuardAdapterIdentifier), NETWORK_CATEGORY_PRIVATE);
    } else {
        helper_win->setNetworkCategory(vpnAdapterInfo_.adapterName(), NETWORK_CATEGORY_PRIVATE);
    }
#endif

    qCInfo(LOG_CONNECTION) << "VPN adapter and gateway:" << vpnAdapterInfo_.makeLogString();

    // override the DNS if we are using custom
    if (connectedDnsInfo_.type == CONNECTED_DNS_TYPE_CUSTOM) {
        QString customDnsIp = dnsServersFromConnectedDnsInfo();
        vpnAdapterInfo_.setDnsServers(QStringList() << customDnsIp);
        qCInfo(LOG_CONNECTION) << "Custom DNS detected, will override with: " << customDnsIp;
    }

    if (state_ == STATE_DISCONNECTING_FROM_USER_CLICK) {
        qCDebug(LOG_CONNECTION) << "Already disconnecting -- do not enter connected state";
        return;
    }

    timerReconnection_.stop();
    connectingTimer_.stop();
    state_ = STATE_CONNECTED;
    emit connected();
}

void ConnectionManager::onConnectionDisconnected()
{
    if (connector_ == nullptr || static_cast<IConnection *>(sender()) != connector_) {
        // This event came from a connector that we have retired, and we already have a new connection
        // underway. Ignore the event
        qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionDisconnected(), ignored";
        return;
    }

    qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionDisconnected(), state_ =" << state_;

    testVPNTunnel_->stopTests();
    doMacRestoreProcedures();
    stunnelManager_->killProcess();
    wstunnelManager_->killProcess();
    ctrldManager_->killProcess();
    timerWaitNetworkConnectivity_.stop();
    connectingTimer_.stop();

    switch (state_)
    {
        case STATE_DISCONNECTING_FROM_USER_CLICK:
            disconnect();
            connSettingsPolicy_->reset();
            emit disconnected(userDisconnectReason_);
            userDisconnectReason_ = DISCONNECTED_BY_USER; // reset to default value
            break;
        case STATE_CONNECTED:
            // goto reconnection state, start reconnection timer and do connection again
            WS_ASSERT(!timerReconnection_.isActive());
            timerReconnection_.start(MAX_RECONNECTION_TIME);
            state_ = STATE_RECONNECTING;
            emit reconnecting();
            doConnect();
            break;
        case STATE_CONNECTING_FROM_USER_CLICK:
        case STATE_AUTO_DISCONNECT:
            disconnect();
            emit disconnected(DISCONNECTED_ITSELF);
            break;
        case STATE_DISCONNECTED:
        case STATE_WAIT_FOR_NETWORK_CONNECTIVITY:
            // nothing todo
            break;

        case STATE_ERROR_DURING_CONNECTION:
            disconnect();
            emit errorDuringConnection(latestConnectionError_);
            break;

        case STATE_RECONNECTING:
            connectOrStartConnectTimer();
            break;
        case STATE_RECONNECTION_TIME_EXCEED:
            disconnect();
            emit disconnected(DISCONNECTED_BY_RECONNECTION_TIMEOUT_EXCEEDED);
            break;

        case STATE_SLEEP_MODE_NEED_RECONNECT:
            //nothing todo
            break;

        case STATE_WAKEUP_RECONNECTING:
            timerReconnection_.start(MAX_RECONNECTION_TIME);
            state_ = STATE_RECONNECTING;
            doConnect();
            break;

        default:
            WS_ASSERT(false);
    }
}

void ConnectionManager::onConnectionReconnecting()
{
    qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionReconnecting(), state_ =" << state_;

    testVPNTunnel_->stopTests();

    // bIgnoreConnectionErrorsForOpenVpn_ need to prevent handle multiple error messages from openvpn
    if (bIgnoreConnectionErrorsForOpenVpn_)
    {
        return;
    }
    else
    {
        bIgnoreConnectionErrorsForOpenVpn_ = true;
    }

    bool bCheckFailsResult;

    switch (state_)
    {
        case STATE_CONNECTING_FROM_USER_CLICK:
            bCheckFailsResult = checkFails();
            if (!bCheckFailsResult || bWasSuccessfullyConnectionAttempt_)
            {
                if (bCheckFailsResult)
                {
                    connSettingsPolicy_->reset();
                }
                state_ = STATE_RECONNECTING;
                emit reconnecting();
                startReconnectionTimer();
                connector_->startDisconnect();
            }
            else
            {
                state_ = STATE_AUTO_DISCONNECT;
                connector_->startDisconnect();
                emit showFailedAutomaticConnectionMessage();
            }
            break;

        case STATE_CONNECTED:
            WS_ASSERT(!timerReconnection_.isActive());

            if (!connector_->isDisconnected())
            {
                if (state_ != STATE_RECONNECTING)
                {
                    emit reconnecting();
                    state_ = STATE_RECONNECTING;
                    timerReconnection_.start(MAX_RECONNECTION_TIME);
                }
                else
                {
                    if (!timerReconnection_.isActive())
                    {
                        timerReconnection_.start(MAX_RECONNECTION_TIME);
                    }
                }
                connector_->startDisconnect();
            }
            break;
        case STATE_DISCONNECTING_FROM_USER_CLICK:
            break;
        case STATE_WAIT_FOR_NETWORK_CONNECTIVITY:
            break;
        case STATE_RECONNECTING:
            bCheckFailsResult = checkFails();

            if (!bCheckFailsResult || bWasSuccessfullyConnectionAttempt_)
            {
                if (bCheckFailsResult)
                {
                    connSettingsPolicy_->reset();
                }
                if (connector_) {
                    connector_->startDisconnect();
                } else {
                    connectOrStartConnectTimer();
                }
            }
            else
            {
                state_ = STATE_AUTO_DISCONNECT;
                if (connector_) {
                    connector_->startDisconnect();
                }
                emit showFailedAutomaticConnectionMessage();
            }
            break;
        case STATE_SLEEP_MODE_NEED_RECONNECT:
            break;
        case STATE_WAKEUP_RECONNECTING:
            break;
        default:
            WS_ASSERT(false);
    }
}

void ConnectionManager::onConnectionError(CONNECT_ERROR err)
{
    if (state_ == STATE_DISCONNECTING_FROM_USER_CLICK || state_ == STATE_RECONNECTION_TIME_EXCEED ||
        state_ == STATE_AUTO_DISCONNECT || state_ == STATE_ERROR_DURING_CONNECTION)
    {
        return;
    }

    qCInfo(LOG_CONNECTION) << "ConnectionManager::onConnectionError(), state_ =" << state_ << ", error =" << (int)err;
    testVPNTunnel_->stopTests();

    if ((err == CONNECT_ERROR::AUTH_ERROR && bEmitAuthError_)
            || err == CONNECT_ERROR::NO_OPENVPN_SOCKET
            || err == CONNECT_ERROR::NO_INSTALLED_TUN_TAP
            || (!connSettingsPolicy_->isAutomaticMode() && err == CONNECT_ERROR::WIREGUARD_CONNECTION_ERROR)
            || (!connSettingsPolicy_->isAutomaticMode() && err == CONNECT_ERROR::WIREGUARD_ADAPTER_SETUP_FAILED)
            || err == CONNECT_ERROR::WINTUN_FATAL_ERROR
            || err == CONNECT_ERROR::PRIV_KEY_PASSWORD_ERROR)
    {
        // emit error in disconnected event
        latestConnectionError_ = err;
        state_ = STATE_ERROR_DURING_CONNECTION;
        timerReconnection_.stop();
    }
    else if ( (!connSettingsPolicy_->isAutomaticMode() && (err == CONNECT_ERROR::IKEV_NOT_FOUND_WIN
                                                           || err == CONNECT_ERROR::IKEV_FAILED_SET_ENTRY_WIN
                                                           || err == CONNECT_ERROR::IKEV_FAILED_MODIFY_HOSTS_WIN) )
            || (!connSettingsPolicy_->isAutomaticMode() && (err == CONNECT_ERROR::IKEV_NETWORK_EXTENSION_NOT_FOUND_MAC
                                                            || err == CONNECT_ERROR::IKEV_FAILED_SET_KEYCHAIN_MAC
                                                            || err == CONNECT_ERROR::IKEV_FAILED_START_MAC
                                                            || err == CONNECT_ERROR::IKEV_FAILED_LOAD_PREFERENCES_MAC
                                                            || err == CONNECT_ERROR::IKEV_FAILED_SAVE_PREFERENCES_MAC))
            || err == CONNECT_ERROR::EXE_SUBPROCESS_FAILED)
    {
        // immediately stop trying to connect
        disconnect();
        emit errorDuringConnection(err);
    }
    else if (err == CONNECT_ERROR::STATE_TIMEOUT_FOR_AUTOMATIC
             || err == CONNECT_ERROR::UDP_CANT_ASSIGN
             || err == CONNECT_ERROR::UDP_NO_BUFFER_SPACE
             || err == CONNECT_ERROR::UDP_NETWORK_DOWN
             || err == CONNECT_ERROR::WINTUN_OVER_CAPACITY
             || err == CONNECT_ERROR::TCP_ERROR
             || err == CONNECT_ERROR::CONNECTED_ERROR
             || err == CONNECT_ERROR::INITIALIZATION_SEQUENCE_COMPLETED_WITH_ERRORS
             || err == CONNECT_ERROR::IKEV_FAILED_TO_CONNECT
             || err == CONNECT_ERROR::WIREGUARD_CONNECTION_ERROR
             || err == CONNECT_ERROR::WIREGUARD_ADAPTER_SETUP_FAILED
             || (connSettingsPolicy_->isAutomaticMode() && (err == CONNECT_ERROR::IKEV_NOT_FOUND_WIN
                                                            || err == CONNECT_ERROR::IKEV_FAILED_SET_ENTRY_WIN
                                                            || err == CONNECT_ERROR::IKEV_FAILED_MODIFY_HOSTS_WIN))
             || (connSettingsPolicy_->isAutomaticMode() && (err == CONNECT_ERROR::IKEV_NETWORK_EXTENSION_NOT_FOUND_MAC
                                                            || err == CONNECT_ERROR::IKEV_FAILED_SET_KEYCHAIN_MAC
                                                            || err == CONNECT_ERROR::IKEV_FAILED_START_MAC
                                                            || err == CONNECT_ERROR::IKEV_FAILED_LOAD_PREFERENCES_MAC
                                                            || err == CONNECT_ERROR::IKEV_FAILED_SAVE_PREFERENCES_MAC))
             || (err == CONNECT_ERROR::AUTH_ERROR && !bEmitAuthError_))
    {
        // bIgnoreConnectionErrorsForOpenVpn_ need to prevent handle multiple error messages from openvpn
        if (!bIgnoreConnectionErrorsForOpenVpn_)
        {
            bIgnoreConnectionErrorsForOpenVpn_ = true;

            if (state_ == STATE_CONNECTED)
            {
                emit reconnecting();
                state_ = STATE_RECONNECTING;
                startReconnectionTimer();
                // for AUTH_ERROR signal disconnected will be emitted automatically
                if (err != CONNECT_ERROR::AUTH_ERROR)
                {
                    connector_->startDisconnect();
                }
            }
            else
            {
                bool bCheckFailsResult = checkFails();
                if (!bCheckFailsResult || bWasSuccessfullyConnectionAttempt_)
                {
                    if (bCheckFailsResult)
                    {
                        connSettingsPolicy_->reset();
                    }

                    if (state_ != STATE_RECONNECTING)
                    {
                        emit reconnecting();
                        state_ = STATE_RECONNECTING;
                        startReconnectionTimer();
                    }
                    // for AUTH_ERROR signal disconnected will be emitted automatically
                    if (err != CONNECT_ERROR::AUTH_ERROR)
                    {
                        connector_->startDisconnect();
                    }
                }
                else
                {
                    state_ = STATE_AUTO_DISCONNECT;
                    if (err != CONNECT_ERROR::AUTH_ERROR)
                    {
                        connector_->startDisconnect();
                    }
                    emit showFailedAutomaticConnectionMessage();
                }
            }
        }
    }
    else
    {
        qCWarning(LOG_CONNECTION) << "Unknown error from openvpn: " << err;
    }
}

void ConnectionManager::onConnectionStatisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes)
{
    emit statisticsUpdated(bytesIn, bytesOut, isTotalBytes);
}

void ConnectionManager::onConnectionInterfaceUpdated(const QString &interfaceName)
{
    emit interfaceUpdated(interfaceName);
}

void ConnectionManager::onConnectionRequestUsername()
{
    emit requestUsername(currentConnectionDescr_.customConfigFilename);
}

void ConnectionManager::onConnectionRequestPassword()
{
    emit requestPassword(currentConnectionDescr_.customConfigFilename);
}

void ConnectionManager::onConnectionRequestPrivKeyPassword()
{
    emit requestPrivKeyPassword(currentConnectionDescr_.customConfigFilename);
}

void ConnectionManager::onSleepMode()
{
    qCInfo(LOG_CONNECTION) << "ConnectionManager::onSleepMode(), state_ =" << state_;

    timerReconnection_.stop();
    connectTimer_.stop();
    bWakeSignalReceived_ = false;

    switch (state_)
    {
        case STATE_DISCONNECTED:
            break;
        case STATE_CONNECTING_FROM_USER_CLICK:
        case STATE_CONNECTED:
        case STATE_RECONNECTING:
        case STATE_WAKEUP_RECONNECTING:
            emit reconnecting();
            blockingDisconnect();
            qCDebug(LOG_CONNECTION) << "ConnectionManager::onSleepMode(), connection blocking disconnected";
            state_ = STATE_SLEEP_MODE_NEED_RECONNECT;
            if (bWakeSignalReceived_) {
                // If we are already awake (got the wake event during waiting in the blocking
                // disconnect loop), reconnect immediately.
                restoreConnectionAfterWakeUp();
                bWakeSignalReceived_ = false;
            }
            break;
        case STATE_DISCONNECTING_FROM_USER_CLICK:
            break;
        case STATE_WAIT_FOR_NETWORK_CONNECTIVITY:
            state_ = STATE_SLEEP_MODE_NEED_RECONNECT;
            break;
        case STATE_RECONNECTION_TIME_EXCEED:
            break;
        default:
            WS_ASSERT(false);
    }
}

void ConnectionManager::onWakeMode()
{
    qCInfo(LOG_CONNECTION) << "ConnectionManager::onWakeMode(), state_ =" << state_;
    timerReconnection_.stop();
    connectTimer_.stop();
    bWakeSignalReceived_ = true;

    switch (state_)
    {
        case STATE_DISCONNECTED:
            break;
        case STATE_CONNECTING_FROM_USER_CLICK:
        case STATE_CONNECTED:
        case STATE_RECONNECTING:
        case STATE_WAIT_FOR_NETWORK_CONNECTIVITY:
        case STATE_DISCONNECTING_FROM_USER_CLICK:
        case STATE_RECONNECTION_TIME_EXCEED:
        case STATE_SLEEP_MODE_NEED_RECONNECT:
        case STATE_WAKEUP_RECONNECTING:
            // We should not be in some of these above states after wake up, but in some weird cases
            // on Mac it is possible for the network to keep changing after onSleep() which may trigger
            // some state changes.  Regardless of the above state, we should restore connection here.
            restoreConnectionAfterWakeUp();
            break;
        default:
            WS_ASSERT(false);
    }
}

void ConnectionManager::onNetworkOnlineStateChanged(bool isAlive)
{
    qCInfo(LOG_CONNECTION) << "ConnectionManager::onNetworkOnlineStateChanged(), isAlive =" << isAlive << ", state_ =" << state_;

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    emit internetConnectivityChanged(isAlive);
#elif defined Q_OS_MACOS
    emit internetConnectivityChanged(isAlive);

    bLastIsOnline_ = isAlive;
    if (!connector_)
        return;

    switch (state_)
    {
        case STATE_DISCONNECTED:
            //nothing todo
            break;
        case STATE_CONNECTING_FROM_USER_CLICK:
            if (!isAlive)
            {
                state_ = STATE_WAIT_FOR_NETWORK_CONNECTIVITY;
                WS_ASSERT(!timerReconnection_.isActive());
                timerReconnection_.start(MAX_RECONNECTION_TIME);
                connector_->startDisconnect();
            }
            break;
        case STATE_CONNECTED:
            if (!isAlive)
            {
                emit reconnecting();
                state_ = STATE_WAIT_FOR_NETWORK_CONNECTIVITY;
                WS_ASSERT(!timerReconnection_.isActive());
                timerReconnection_.start(MAX_RECONNECTION_TIME);
                connector_->startDisconnect();
            }
            else
            {
                emit reconnecting();
                state_ = STATE_RECONNECTING;
                WS_ASSERT(!timerReconnection_.isActive());
                timerReconnection_.start(MAX_RECONNECTION_TIME);
                connector_->startDisconnect();
            }
            break;
        case STATE_RECONNECTING:
            if (!isAlive)
            {
                state_ = STATE_WAIT_FOR_NETWORK_CONNECTIVITY;
                connector_->startDisconnect();
            }
            break;
        case STATE_DISCONNECTING_FROM_USER_CLICK:
            //nothing todo
            break;
        case STATE_WAIT_FOR_NETWORK_CONNECTIVITY:
            if (isAlive)
            {
                state_ = STATE_RECONNECTING;
                if (!timerReconnection_.isActive())
                {
                    timerReconnection_.start(MAX_RECONNECTION_TIME);
                }
                connector_->startDisconnect();
            }
            break;
        case STATE_RECONNECTION_TIME_EXCEED:
            //nothing todo
            break;

        case STATE_SLEEP_MODE_NEED_RECONNECT:
            break;

        case STATE_WAKEUP_RECONNECTING:
            break;

        default:
            WS_ASSERT(false);
    }
#endif
}

void ConnectionManager::onTimerReconnection()
{
    qCInfo(LOG_CONNECTION) << "Time for reconnection exceed";
    state_ = STATE_RECONNECTION_TIME_EXCEED;
    if (connector_)
    {
        connector_->startDisconnect();
    }
    else
    {
        disconnect();
        emit disconnected(DISCONNECTED_BY_RECONNECTION_TIMEOUT_EXCEEDED);
    }
    timerReconnection_.stop();
}

void ConnectionManager::onWstunnelStarted()
{
    doConnectPart3();
}

void ConnectionManager::doConnect()
{
#ifdef Q_OS_MACOS
    // For automatic policy, we would have removed IKEv2 from the list for lockdown mode.
    // There is no custom config for IKEv2, so if we get here it is manual mode.
    // We can get here either by:
    // - User selecting IKEv2 in manual mode and then enabling Lockdown Mode, or
    // - User selecting IKEv2 in manual mode in a previous version of Windscribe, then updating.
    if (!connSettingsPolicy_->isCustomConfig() && connSettingsPolicy_->getCurrentConnectionSettings().protocol == types::Protocol::IKEV2 && MacUtils::isLockdownMode()) {
        emit errorDuringConnection(CONNECT_ERROR::LOCKDOWN_MODE_IKEV2);
        return;
    }
#endif

    bool isOnline = networkDetectionManager_->isOnline();
    defaultAdapterInfo_.clear();
    if (isOnline) {
        defaultAdapterInfo_ = AdapterGatewayInfo::detectAndCreateDefaultAdapterInfo();
    }

    if (!isOnline || defaultAdapterInfo_.isEmpty()) {
        startReconnectionTimer();
        waitForNetworkConnectivity();
        return;
    }

    qCInfo(LOG_CONNECTION) << "Default adapter and gateway:" << defaultAdapterInfo_.makeLogString();
    connectTimer_.stop();

    connectingTimer_.setSingleShot(true);
    if (connSettingsPolicy_->isAutomaticMode()) {
        CurrentConnectionDescr settings = connSettingsPolicy_->getCurrentConnectionSettings();
        if (settings.protocol == types::Protocol::WIREGUARD) {
            connectingTimer_.setInterval(kConnectingTimeoutWireGuard);
        } else {
            connectingTimer_.setInterval(kConnectingTimeout);
        }
        connectingTimer_.start();
    }

    connSettingsPolicy_->resolveHostnames();
}

void ConnectionManager::doConnectPart2()
{
    bIgnoreConnectionErrorsForOpenVpn_ = false;

    currentConnectionDescr_ = connSettingsPolicy_->getCurrentConnectionSettings();

    if (currentConnectionDescr_.connectionNodeType == CONNECTION_NODE_ERROR)
    {
        qCWarning(LOG_CONNECTION) << "connSettingsPolicy_.getCurrentConnectionSettings returned incorrect value";
        disconnect();
        emit errorDuringConnection(CONNECT_ERROR::LOCATION_NO_ACTIVE_NODES);
        return;
    }

    qCInfo(LOG_CONNECTION) << "Connecting to IP:" << currentConnectionDescr_.ip << " protocol:" << currentConnectionDescr_.protocol.toLongString() << " port:" << currentConnectionDescr_.port;
    emit protocolPortChanged(currentConnectionDescr_.protocol, currentConnectionDescr_.port);

#if defined(Q_OS_WIN)
    if (WinUtils::isDohSupported() && connectedDnsInfo_.type == CONNECTED_DNS_TYPE_FORCED) {
        auto helperWin = dynamic_cast<Helper_win *>(helper_);
        WS_ASSERT(helperWin);
        helperWin->disableDohSettings();
    }
#endif

    // start ctrld utility
    if (connectedDnsInfo_.type == CONNECTED_DNS_TYPE_CUSTOM && !connectedDnsInfo_.isCustomIPv4Address()) {
        bool bStarted = false;
        if (connectedDnsInfo_.isSplitDns)
            bStarted = ctrldManager_->runProcess(connectedDnsInfo_.upStream1, connectedDnsInfo_.upStream2, connectedDnsInfo_.hostnames);
        else
            bStarted = ctrldManager_->runProcess(connectedDnsInfo_.upStream1, QString(), QStringList());

        if (!bStarted) {
            qCCritical(LOG_BASIC) << "connection manager ctrld start failed";
            disconnect();
            emit errorDuringConnection(CONNECT_ERROR::CTRLD_START_FAILED);
            return;
        }
    #ifdef Q_OS_WIN
        // we need to exclude these DNS-addresses from DNS leak protection on Windows
        QStringList dnsIps;
        dnsIps << ctrldManager_->listenIp();
        if (IpValidation::isIp(connectedDnsInfo_.upStream1)) {
            dnsIps << connectedDnsInfo_.upStream1;
        }
        dynamic_cast<Helper_win*>(helper_)->setCustomDnsIps(dnsIps);
    #endif
    } else if (connectedDnsInfo_.isCustomIPv4Address())  {
#ifdef Q_OS_WIN
        dynamic_cast<Helper_win*>(helper_)->setCustomDnsIps(QStringList() << connectedDnsInfo_.upStream1);
#endif
    } else {
#ifdef Q_OS_WIN
        dynamic_cast<Helper_win*>(helper_)->setCustomDnsIps(QStringList());
#endif
    }

    if (currentConnectionDescr_.connectionNodeType == CONNECTION_NODE_DEFAULT ||
            currentConnectionDescr_.connectionNodeType == CONNECTION_NODE_STATIC_IPS)
    {
        if (currentConnectionDescr_.protocol.isOpenVpnProtocol())
        {

            int mss = 0;
            if (!packetSize_.isAutomatic)
            {
                bool advParamsOpenVpnExists = false;
                int openVpnOffset = ExtraConfig::instance().getMtuOffsetOpenVpn(advParamsOpenVpnExists);
                if (!advParamsOpenVpnExists) openVpnOffset = MTU_OFFSET_OPENVPN;

                mss = packetSize_.mtu - openVpnOffset;

                if (mss <= 0)
                {
                    mss = 0;
                    qCInfo(LOG_PACKET_SIZE) << "Using default MSS - OpenVpn ConnectionManager MSS too low: " << mss;
                }
                else
                {
                    qCInfo(LOG_PACKET_SIZE) << "OpenVpn connection MSS: " << mss;
                }
            }
            else
            {
                qCInfo(LOG_PACKET_SIZE) << "Packet size mode auto - using default MSS (ConnectionManager)";
            }

            uint localPort = 0;
            if (currentConnectionDescr_.protocol.isStunnelOrWStunnelProtocol()) {
                if (currentConnectionDescr_.protocol == types::Protocol::STUNNEL) {
                    localPort = stunnelManager_->getPort();
                } else {
                    localPort = wstunnelManager_->getPort();
                }
            }

            const bool bOvpnSuccess = makeOVPNFile_->generate(
                lastOvpnConfig_, currentConnectionDescr_.ip, currentConnectionDescr_.protocol,
                currentConnectionDescr_.port, localPort, mss, defaultAdapterInfo_.gateway(),
                currentConnectionDescr_.verifyX509name,
                dnsServersFromConnectedDnsInfo(), isAntiCensorship_, false);
            if (!bOvpnSuccess) {
                qCCritical(LOG_CONNECTION) << "Failed create ovpn config";
                WS_ASSERT(false);
                return;
            }

            if (currentConnectionDescr_.protocol == types::Protocol::STUNNEL) {
                if (!stunnelManager_->runProcess(currentConnectionDescr_.ip, currentConnectionDescr_.port,
                                                 ExtraConfig::instance().getStealthExtraTLSPadding() || isAntiCensorship_)) {
                    disconnect();
                    emit errorDuringConnection(CONNECT_ERROR::EXE_SUBPROCESS_FAILED);
                    return;
                }
                // call doConnectPart2 in onWstunnelStarted slot
                return;
            } else if (currentConnectionDescr_.protocol == types::Protocol::WSTUNNEL) {
                if (!wstunnelManager_->runProcess(currentConnectionDescr_.ip, currentConnectionDescr_.port)) {
                    disconnect();
                    emit errorDuringConnection(CONNECT_ERROR::EXE_SUBPROCESS_FAILED);
                    return;
                }
                // call doConnectPart2 in onWstunnelStarted slot
                return;
            }
        }
        else if (currentConnectionDescr_.protocol.isIkev2Protocol())
        {
            QString remoteHostname = ExtraConfig::instance().getRemoteIpFromExtraConfig();
            if (IpValidation::isDomain(remoteHostname))
            {
                currentConnectionDescr_.hostname = remoteHostname;
                currentConnectionDescr_.ip = ExtraConfig::instance().getDetectedIp();
                currentConnectionDescr_.dnsHostName = IpValidation::getRemoteIdFromDomain(remoteHostname);
                qCInfo(LOG_CONNECTION) << "Use data from extra config: hostname=" << currentConnectionDescr_.hostname << ", ip=" << currentConnectionDescr_.ip << ", remoteId=" << currentConnectionDescr_.dnsHostName;

            }
        }
        else if (currentConnectionDescr_.protocol.isWireGuardProtocol())
        {
            qCInfo(LOG_CONNECTION) << "Requesting WireGuard config for hostname =" << currentConnectionDescr_.hostname;
            QString deviceId = (isStaticIpsLocation() ? GetDeviceId::instance().getDeviceId() : QString());
            getWireGuardConfig(currentConnectionDescr_.hostname, false, deviceId);
            return;
        }
    }
    else if (currentConnectionDescr_.connectionNodeType == CONNECTION_NODE_CUSTOM_CONFIG)
    {
        if (currentConnectionDescr_.protocol.isOpenVpnProtocol()) {
            bool bOvpnSuccess = makeOVPNFileFromCustom_->generate(customConfigPath_,
                currentConnectionDescr_.ovpnData, currentConnectionDescr_.ip,
                currentConnectionDescr_.remoteCmdLine);
            if (!bOvpnSuccess)
            {
                qCCritical(LOG_CONNECTION) << "Failed create ovpn config for custom ovpn file:"
                                        << currentConnectionDescr_.customConfigFilename;
                //WS_ASSERT(false);
                disconnect();
                emit errorDuringConnection(CONNECT_ERROR::CANNOT_OPEN_CUSTOM_CONFIG);
                return;
            }
        } else if (currentConnectionDescr_.protocol.isWireGuardProtocol()) {
            WS_ASSERT(currentConnectionDescr_.wgCustomConfig != nullptr);
            if (currentConnectionDescr_.wgCustomConfig == nullptr) {
                qCCritical(LOG_CONNECTION) << "Failed to get config for custom WG file:"
                                        << currentConnectionDescr_.customConfigFilename;
                disconnect();
                emit errorDuringConnection(CONNECT_ERROR::CANNOT_OPEN_CUSTOM_CONFIG);
                return;
            }
        }
    }
    else
    {
        WS_ASSERT(false);
    }

    doConnectPart3();
}

void ConnectionManager::doConnectPart3()
{
    if (currentConnectionDescr_.protocol.isWireGuardProtocol())
    {
        WireGuardConfig* pConfig = (currentConnectionDescr_.connectionNodeType == CONNECTION_NODE_CUSTOM_CONFIG ? currentConnectionDescr_.wgCustomConfig.get() : &wireGuardConfig_);
        WS_ASSERT(pConfig != nullptr);

        // For WG protocol we need to add upStream1 adrress if it's custom ip. Otherwise on Windows WG may not connect.
        QStringList dnsIps;
        if (connectedDnsTypeAuto()) {
            dnsIps << pConfig->clientDnsAddress();
        } else if (connectedDnsInfo_.isCustomIPv4Address()) {
            dnsIps << connectedDnsInfo_.upStream1;
        } else {
            if (IpValidation::isIp(connectedDnsInfo_.upStream1)) {
                dnsIps << connectedDnsInfo_.upStream1;
            }
            dnsIps << ctrldManager_->listenIp();
        }
        emit connectingToHostname(currentConnectionDescr_.hostname, currentConnectionDescr_.ip, dnsIps);
    }
    else
    {
        emit connectingToHostname(currentConnectionDescr_.hostname, currentConnectionDescr_.ip, QStringList());
    }

    if (currentConnectionDescr_.connectionNodeType == CONNECTION_NODE_CUSTOM_CONFIG)
    {
        if (currentConnectionDescr_.protocol.isWireGuardProtocol())
            recreateConnector(types::Protocol::WIREGUARD);
        else
            recreateConnector(types::Protocol::OPENVPN_UDP);

        connector_->startConnect(makeOVPNFileFromCustom_->config(), "", "", usernameForCustomOvpn_,
                                 passwordForCustomOvpn_, lastProxySettings_,
                                 currentConnectionDescr_.wgCustomConfig.get(), false, false, true,
                                 dnsServersFromConnectedDnsInfo());
    }
    else
    {
        if (currentConnectionDescr_.protocol.isOpenVpnProtocol())
        {
            QString username, password;
            if (currentConnectionDescr_.connectionNodeType == CONNECTION_NODE_STATIC_IPS)
            {
                username = currentConnectionDescr_.username;
                password = currentConnectionDescr_.password;
            }
            else
            {
                username = lastServerCredentialsOpenVpn_.username();
                password = lastServerCredentialsOpenVpn_.password();
            }

            recreateConnector(types::Protocol::OPENVPN_UDP);
            connector_->startConnect(makeOVPNFile_->config(), "", "", username, password, lastProxySettings_, nullptr,
                                     false, connSettingsPolicy_->isAutomaticMode(), false,
                                     dnsServersFromConnectedDnsInfo());
        }
        else if (currentConnectionDescr_.protocol.isIkev2Protocol())
        {
            QString username, password;
            if (currentConnectionDescr_.connectionNodeType == CONNECTION_NODE_STATIC_IPS)
            {
                username = currentConnectionDescr_.username;
                password = currentConnectionDescr_.password;
            }
            else
            {
                username = lastServerCredentialsIkev2_.username();
                password = lastServerCredentialsIkev2_.password();
            }

            recreateConnector(types::Protocol::IKEV2);
            connector_->startConnect(currentConnectionDescr_.hostname, currentConnectionDescr_.ip, currentConnectionDescr_.hostname, username, password, lastProxySettings_,
                                     nullptr, ExtraConfig::instance().isUseIkev2Compression(), connSettingsPolicy_->isAutomaticMode(), false,
                                     dnsServersFromConnectedDnsInfo());
        }
        else if (currentConnectionDescr_.protocol.isWireGuardProtocol())
        {
            QString endpointAndPort = QString("%1:%2").arg(currentConnectionDescr_.ip).arg(currentConnectionDescr_.port);
            wireGuardConfig_.setPeerPublicKey(currentConnectionDescr_.wgPeerPublicKey);
            wireGuardConfig_.setPeerEndpoint(endpointAndPort);

            if (ExtraConfig::instance().getWireGuardUdpStuffing() || isAntiCensorship_) {
                QString localPort = udpStuffingWithNtp(currentConnectionDescr_.ip, currentConnectionDescr_.port);
                wireGuardConfig_.setClientListenPort(localPort);
            }

            recreateConnector(types::Protocol::WIREGUARD);
            connector_->startConnect(QString(), currentConnectionDescr_.ip,
                currentConnectionDescr_.dnsHostName, QString(), QString(), lastProxySettings_,
                &wireGuardConfig_, false, connSettingsPolicy_->isAutomaticMode(), false,
                                     dnsServersFromConnectedDnsInfo());
        }
        else
        {
            WS_ASSERT(false);
        }
    }

    lastIp_ = currentConnectionDescr_.ip;
}

// return true, if need finish reconnecting
bool ConnectionManager::checkFails()
{
    connSettingsPolicy_->putFailedConnection();
    return connSettingsPolicy_->isFailed();
}

void ConnectionManager::doMacRestoreProcedures()
{
#ifdef Q_OS_MACOS
    if (!connector_)
        return;
    const auto connection_type = connector_->getConnectionType();
    if (connection_type == ConnectionType::OPENVPN)
    {
        Helper_mac *helper_mac = dynamic_cast<Helper_mac *>(helper_);
        helper_mac->deleteRoute(lastIp_, 32, defaultAdapterInfo_.gateway());
    }
    if (connection_type == ConnectionType::OPENVPN || connection_type == ConnectionType::WIREGUARD)
    {
        RestoreDNSManager_mac::restoreState(helper_);
    }
#endif
}

void ConnectionManager::startReconnectionTimer()
{
    if (!timerReconnection_.isActive())
    {
        timerReconnection_.start(MAX_RECONNECTION_TIME);
    }
}

void ConnectionManager::waitForNetworkConnectivity()
{
    qCInfo(LOG_CONNECTION) << "No internet connection, waiting maximum: " << MAX_RECONNECTION_TIME << "ms";
    timerWaitNetworkConnectivity_.start(1000);
}

void ConnectionManager::recreateConnector(types::Protocol protocol)
{
    if (currentProtocol_ == types::Protocol::UNINITIALIZED)
    {
        WS_ASSERT(connector_ == NULL);
    }

    if (currentProtocol_ != protocol)
    {
        SAFE_DELETE_LATER(connector_);

        if (protocol.isOpenVpnProtocol())
        {
            connector_ = new OpenVPNConnection(this, helper_);
        }
        else if (protocol.isIkev2Protocol())
        {
#ifdef Q_OS_WIN
            connector_ = new IKEv2Connection_win(this, helper_);
#elif defined Q_OS_MACOS
            connector_ = new IKEv2Connection_mac(this, helper_);
#elif defined Q_OS_LINUX
            connector_ = new IKEv2Connection_linux(this, helper_);
#endif
        }
        else if (protocol.isWireGuardProtocol())
        {
            connector_ = new WireGuardConnection(this, helper_);
        }
        else
        {
            WS_ASSERT(false);
        }

        connect(connector_, &IConnection::connected, this, &ConnectionManager::onConnectionConnected, Qt::QueuedConnection);
        connect(connector_, &IConnection::disconnected, this, &ConnectionManager::onConnectionDisconnected, Qt::QueuedConnection);
        connect(connector_, &IConnection::reconnecting, this, &ConnectionManager::onConnectionReconnecting, Qt::QueuedConnection);
        connect(connector_, &IConnection::error, this, &ConnectionManager::onConnectionError, Qt::QueuedConnection);
        connect(connector_, &IConnection::statisticsUpdated, this, &ConnectionManager::onConnectionStatisticsUpdated, Qt::QueuedConnection);
        connect(connector_, &IConnection::interfaceUpdated, this, &ConnectionManager::onConnectionInterfaceUpdated, Qt::QueuedConnection);

        connect(connector_, &IConnection::requestUsername, this, &ConnectionManager::onConnectionRequestUsername, Qt::QueuedConnection);
        connect(connector_, &IConnection::requestPassword, this, &ConnectionManager::onConnectionRequestPassword, Qt::QueuedConnection);
        if (protocol.isOpenVpnProtocol()) {
            connect(static_cast<OpenVPNConnection *>(connector_), &OpenVPNConnection::requestPrivKeyPassword, this, &ConnectionManager::onConnectionRequestPrivKeyPassword, Qt::QueuedConnection);
        }

        currentProtocol_ = protocol;
    }
}

void ConnectionManager::restoreConnectionAfterWakeUp()
{
    if (bLastIsOnline_)
    {
        qCInfo(LOG_CONNECTION) <<
            "ConnectionManager::restoreConnectionAfterWakeUp(), reconnecting";
        state_ = STATE_WAKEUP_RECONNECTING;
        if (connector_) {
            connector_->startDisconnect();
        } else {
            emit reconnecting();
            doConnect();
        }
    }
    else
    {
        qCInfo(LOG_CONNECTION) <<
            "ConnectionManager::restoreConnectionAfterWakeUp(), waiting for network connectivity";
        state_ = STATE_WAIT_FOR_NETWORK_CONNECTIVITY;
    }
}

void ConnectionManager::onTunnelTestsFinished(bool bSuccess, const QString &ipAddress)
{
    bool hasAttempts = false;
    int attempts = ExtraConfig::instance().getTunnelTestAttempts(hasAttempts);
    bool noError = ExtraConfig::instance().getIsTunnelTestNoError();

    if ((hasAttempts && attempts == 0) || (noError && !bSuccess))
    {
        emit testTunnelResult(bSuccess, "");
    }
    else if (!bSuccess)
    {
        if (connSettingsPolicy_->isAutomaticMode())
        {
            bool bCheckFailsResult = checkFails();
            if (!bCheckFailsResult || bWasSuccessfullyConnectionAttempt_)
            {
                if (bCheckFailsResult)
                {
                    connSettingsPolicy_->reset();
                }
                state_ = STATE_RECONNECTING;
                emit reconnecting();
                startReconnectionTimer();
                connector_->startDisconnect();
             }
             else
             {
                state_= STATE_AUTO_DISCONNECT;
                connector_->startDisconnect();
                emit showFailedAutomaticConnectionMessage();
             }
        }
        else
        {
            emit testTunnelResult(false, "");
        }
    }
    else
    {
        emit testTunnelResult(true, ipAddress);

        // if connection mode is automatic
        bWasSuccessfullyConnectionAttempt_ = true;
    }
}

void ConnectionManager::onTimerWaitNetworkConnectivity()
{
    if (networkDetectionManager_->isOnline() && !AdapterGatewayInfo::detectAndCreateDefaultAdapterInfo().isEmpty())
    {
        qCInfo(LOG_CONNECTION) << "We online, make the connection";
        timerWaitNetworkConnectivity_.stop();
        doConnect();
    }
    else
    {
        if (timerReconnection_.remainingTime() == 0)
        {
            qCInfo(LOG_CONNECTION) << "Time for wait network connection exceed";
            timerWaitNetworkConnectivity_.stop();
            disconnect();
            emit disconnected(DISCONNECTED_BY_RECONNECTION_TIMEOUT_EXCEEDED);
        }
    }
}

void ConnectionManager::onHostnamesResolved()
{
    doConnectPart2();
}

void ConnectionManager::onGetWireGuardConfigAnswer(WireGuardConfigRetCode retCode, const WireGuardConfig &config)
{
    // if we got an answer after we've timed out or disconnected, ignore this event
    CurrentConnectionDescr settings = connSettingsPolicy_->getCurrentConnectionSettings();
    if ((state_ != STATE_CONNECTING_FROM_USER_CLICK && state_ != STATE_WAKEUP_RECONNECTING && state_ != STATE_RECONNECTING)
        || settings.protocol != types::Protocol::WIREGUARD)
    {
        return;
    }

    if (retCode == WireGuardConfigRetCode::kKeyLimit)
    {
        // Do not timeout while waiting for user input
        connectingTimer_.stop();
        emit wireGuardAtKeyLimit();
    }
    else if (retCode == WireGuardConfigRetCode::kFailoverFailed && !connSettingsPolicy_->isAutomaticMode())
    {
        // All options for accessing the API have been exhausted, there is no point in trying again
        // immediately stop trying to connect
        disconnect();
        emit errorDuringConnection(WIREGUARD_COULD_NOT_RETRIEVE_CONFIG);
        return;
    }
    else if (retCode == WireGuardConfigRetCode::kSuccess)
    {
        wireGuardConfig_ = config;
        // If the protocol has been changed, do nothing.
        if (!currentConnectionDescr_.protocol.isWireGuardProtocol())
        {
            WS_ASSERT(false);  // this should not happen logically?
            return;
        }

        doConnectPart3();
    }
    else
    {
        state_ = STATE_RECONNECTING;
        emit reconnecting();
        startReconnectionTimer();
        onConnectionReconnecting();
    }
}

bool ConnectionManager::isCustomOvpnConfigCurrentConnection() const
{
    return currentConnectionDescr_.connectionNodeType == CONNECTION_NODE_CUSTOM_CONFIG &&
           currentConnectionDescr_.protocol.isOpenVpnProtocol();
}

QString ConnectionManager::getCustomOvpnConfigFileName()
{
    WS_ASSERT(isCustomOvpnConfigCurrentConnection());
    return currentConnectionDescr_.customConfigFilename;
}

bool ConnectionManager::isStaticIpsLocation() const
{
    return currentConnectionDescr_.connectionNodeType == CONNECTION_NODE_STATIC_IPS;
}

api_responses::StaticIpPortsVector ConnectionManager::getStatisIps()
{
    WS_ASSERT(isStaticIpsLocation());
    return currentConnectionDescr_.staticIpPorts;
}

void ConnectionManager::onWireGuardKeyLimitUserResponse(bool deleteOldestKey)
{
    if (deleteOldestKey)
    {
        QString deviceId = (isStaticIpsLocation() ? GetDeviceId::instance().getDeviceId() : QString());
        getWireGuardConfig(currentConnectionDescr_.hostname, true, deviceId);
        // restart connecting timeout
        connectingTimer_.start(kConnectingTimeoutWireGuard);
    }
    else
    {
        clickDisconnect();
    }
}

void ConnectionManager::setPacketSize(types::PacketSize ps)
{
    packetSize_ = ps;
}

void ConnectionManager::startTunnelTests()
{
    testVPNTunnel_->startTests(currentConnectionDescr_.protocol);
}

bool ConnectionManager::isAllowFirewallAfterConnection() const
{
    WS_ASSERT(connector_);
    if (!connector_ || currentConnectionDescr_.connectionNodeType != CONNECTION_NODE_CUSTOM_CONFIG)
        return true;

    return currentConnectionDescr_.isAllowFirewallAfterConnection
        && connector_->isAllowFirewallAfterCustomConfigConnection();
}

types::Protocol ConnectionManager::currentProtocol() const
{
    return currentProtocol_;
}

void ConnectionManager::updateConnectionSettings(const types::ConnectionSettings &connectionSettings,
        const api_responses::PortMap &portMap,
        const types::ProxySettings &proxySettings)
{
    qCDebug(LOG_CONNECTION) << "ConnectionManager::updateConnectionSettings(), state_ =" << state_;

    updateConnectionSettingsPolicy(connectionSettings, portMap, proxySettings);

    if (connector_ == nullptr) {
        return;
    }

    switch (state_)
    {
        case STATE_DISCONNECTED:
        case STATE_DISCONNECTING_FROM_USER_CLICK:
        case STATE_WAIT_FOR_NETWORK_CONNECTIVITY:
        case STATE_RECONNECTION_TIME_EXCEED:
        case STATE_SLEEP_MODE_NEED_RECONNECT:
        case STATE_WAKEUP_RECONNECTING:
        case STATE_AUTO_DISCONNECT:
        case STATE_ERROR_DURING_CONNECTION:
        case STATE_RECONNECTING:
            break;
        case STATE_CONNECTING_FROM_USER_CLICK:
        case STATE_CONNECTED:
            emit reconnecting();
            state_ = STATE_RECONNECTING;
            if (!timerReconnection_.isActive()) {
                timerReconnection_.start(MAX_RECONNECTION_TIME);
            }
            connector_->startDisconnect();
            break;
        default:
            WS_ASSERT(false);
    }
}

void ConnectionManager::updateConnectionSettingsPolicy(const types::ConnectionSettings &connectionSettings,
        const api_responses::PortMap &portMap,
        const types::ProxySettings &proxySettings)
{
    if (!bli_) {
        // no active connection in progress
        return;
    }

    if (bli_->locationId().isCustomConfigsLocation()) {
        connSettingsPolicy_.reset(new CustomConfigConnSettingsPolicy(bli_));
    } else if (connectionSettings.isAutomatic()) {
#ifdef Q_OS_MACOS
        connSettingsPolicy_.reset(new AutoConnSettingsPolicy(bli_, portMap, proxySettings.isProxyEnabled(), lastKnownGoodProtocol_, MacUtils::isLockdownMode()));
#else
        connSettingsPolicy_.reset(new AutoConnSettingsPolicy(bli_, portMap, proxySettings.isProxyEnabled(), lastKnownGoodProtocol_, false));
#endif
    } else {
        connSettingsPolicy_.reset(new ManualConnSettingsPolicy(bli_, connectionSettings, portMap));
    }
    connSettingsPolicy_->start();
    connect(connSettingsPolicy_.data(), &BaseConnSettingsPolicy::hostnamesResolved, this, &ConnectionManager::onHostnamesResolved);
    connect(connSettingsPolicy_.data(), &BaseConnSettingsPolicy::protocolStatusChanged, this, &ConnectionManager::protocolStatusChanged);
}

void ConnectionManager::connectOrStartConnectTimer()
{
    if (connSettingsPolicy_->hasProtocolChanged()) {
        connectTimer_.setSingleShot(true);
        connectTimer_.setInterval(kConnectionWaitTimeMsec);
        connectTimer_.start();
    } else {
        doConnect();
    }
}

void ConnectionManager::getWireGuardConfig(const QString &serverName, bool deleteOldestKey, const QString &deviceId)
{
    SAFE_DELETE(getWireGuardConfig_);
    getWireGuardConfig_ = new GetWireGuardConfig(this);
    connect(getWireGuardConfig_, &GetWireGuardConfig::getWireGuardConfigAnswer, this, &ConnectionManager::onGetWireGuardConfigAnswer);
    getWireGuardConfig_->getWireGuardConfig(serverName, deleteOldestKey, deviceId);
}

bool ConnectionManager::connectedDnsTypeAuto() const
{
    return connectedDnsInfo_.type == CONNECTED_DNS_TYPE_AUTO || connectedDnsInfo_.type == CONNECTED_DNS_TYPE_FORCED;
}

QString ConnectionManager::dnsServersFromConnectedDnsInfo() const
{
    if (connectedDnsInfo_.type == CONNECTED_DNS_TYPE_AUTO || connectedDnsInfo_.type == CONNECTED_DNS_TYPE_FORCED)
        return QString();
    else if (connectedDnsInfo_.isCustomIPv4Address())
        return connectedDnsInfo_.upStream1;
    else
        return ctrldManager_->listenIp();
}

void ConnectionManager::disconnect()
{
    log_utils::Logger::instance().endConnectionMode();
    timerReconnection_.stop();
    connectTimer_.stop();
    connectingTimer_.stop();
    state_ = STATE_DISCONNECTED;
    emit connectionEnded();
}

void ConnectionManager::onConnectTrigger()
{
    doConnect();
}

void ConnectionManager::setLastKnownGoodProtocol(const types::Protocol protocol) {
    lastKnownGoodProtocol_ = protocol;
}

void ConnectionManager::onConnectingTimeout()
{
    qCInfo(LOG_CONNECTION) << "Connection timed out";
    state_ = STATE_RECONNECTING;
    emit reconnecting();
    startReconnectionTimer();
    onConnectionReconnecting();
}
