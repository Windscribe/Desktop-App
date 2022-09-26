#include "utils/logger.h"
#include <QStandardPaths>
#include <QThread>
#include <QCoreApplication>
#include <QDateTime>
#include "isleepevents.h"
#include "openvpnconnection.h"

#include "utils/ws_assert.h"
#include "utils/utils.h"
#include "types/enums.h"
#include "types/connectionsettings.h"
#include "engine/serverapi/serverapi.h"
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
    #include "adapterutils_win.h"
    #include "ikev2connection_win.h"
    #include "wireguardconnection_win.h"
#elif defined Q_OS_MAC
    #include "sleepevents_mac.h"
    #include "utils/macutils.h"
    #include "ikev2connection_mac.h"
    #include "engine/helper/helper_mac.h"
    #include "wireguardconnection_posix.h"
#elif defined Q_OS_LINUX
    #include "ikev2connection_linux.h"
    #include "wireguardconnection_posix.h"
#endif

ConnectionManager::ConnectionManager(QObject *parent, IHelper *helper, INetworkDetectionManager *networkDetectionManager,
                                             server_api::ServerAPI *serverAPI, CustomOvpnAuthCredentialsStorage *customOvpnAuthCredentialsStorage) : QObject(parent),
    helper_(helper),
    networkDetectionManager_(networkDetectionManager),
    customOvpnAuthCredentialsStorage_(customOvpnAuthCredentialsStorage),
    connector_(NULL),
    sleepEvents_(NULL),
    stunnelManager_(NULL),
    wstunnelManager_(NULL),
    bEmitAuthError_(false),
    makeOVPNFile_(NULL),
    makeOVPNFileFromCustom_(NULL),
    testVPNTunnel_(NULL),
    bNeedResetTap_(false),
    bIgnoreConnectionErrorsForOpenVpn_(false),
    bWasSuccessfullyConnectionAttempt_(false),
    state_(STATE_DISCONNECTED),
    bLastIsOnline_(true),
    bWakeSignalReceived_(false),
    currentConnectionDescr_()
{
    connect(&timerReconnection_, SIGNAL(timeout()), SLOT(onTimerReconnection()));

    stunnelManager_ = new StunnelManager(this);
    connect(stunnelManager_, SIGNAL(stunnelFinished()), SLOT(onStunnelFinishedBeforeConnection()));

    wstunnelManager_ = new WstunnelManager(this);
    connect(wstunnelManager_, SIGNAL(wstunnelStarted()), SLOT(onWstunnelStarted()));
    connect(wstunnelManager_, SIGNAL(wstunnelFinished()), SLOT(onWstunnelFinishedBeforeConnection()));

    testVPNTunnel_ = new TestVPNTunnel(this, serverAPI);
    connect(testVPNTunnel_, SIGNAL(testsFinished(bool, QString)), SLOT(onTunnelTestsFinished(bool, QString)));

    makeOVPNFile_ = new MakeOVPNFile();
    makeOVPNFileFromCustom_ = new MakeOVPNFileFromCustom();

#ifdef Q_OS_WIN
    sleepEvents_ = new SleepEvents_win(this);
#elif defined Q_OS_MAC
    sleepEvents_ = new SleepEvents_mac(this);
#endif

    connect(networkDetectionManager_, SIGNAL(onlineStateChanged(bool)), SLOT(onNetworkOnlineStateChanged(bool)));

#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    connect(sleepEvents_, SIGNAL(gotoSleep()), SLOT(onSleepMode()));
    connect(sleepEvents_, SIGNAL(gotoWake()), SLOT(onWakeMode()));
#endif

    connect(&timerWaitNetworkConnectivity_, SIGNAL(timeout()), SLOT(onTimerWaitNetworkConnectivity()));

    getWireGuardConfigInLoop_ = new GetWireGuardConfigInLoop(this, serverAPI);
    connect(getWireGuardConfigInLoop_, &GetWireGuardConfigInLoop::getWireGuardConfigAnswer, this, &ConnectionManager::onGetWireGuardConfigAnswer);
}

ConnectionManager::~ConnectionManager()
{
    SAFE_DELETE(testVPNTunnel_);
    SAFE_DELETE(connector_);
    SAFE_DELETE(stunnelManager_);
    SAFE_DELETE(wstunnelManager_);
    SAFE_DELETE(makeOVPNFile_);
    SAFE_DELETE(makeOVPNFileFromCustom_);
    SAFE_DELETE(sleepEvents_);
    SAFE_DELETE(getWireGuardConfigInLoop_);
}

void ConnectionManager::clickConnect(const QString &ovpnConfig, const apiinfo::ServerCredentials &serverCredentials,
                                         QSharedPointer<locationsmodel::BaseLocationInfo> bli,
                                         const types::ConnectionSettings &networkConnectionSettings,
                                         const types::ConnectionSettings &connectionSettings,
                                         const types::PortMap &portMap, const types::ProxySettings &proxySettings,
                                         bool bEmitAuthError, const QString &customConfigPath)
{
    WS_ASSERT(state_ == STATE_DISCONNECTED);

    lastOvpnConfig_ = ovpnConfig;
    lastServerCredentials_ = serverCredentials;
    lastProxySettings_ = proxySettings;
    bEmitAuthError_ = bEmitAuthError;
    customConfigPath_ = customConfigPath;

    bWasSuccessfullyConnectionAttempt_ = false;

    usernameForCustomOvpn_.clear();
    passwordForCustomOvpn_.clear();

    state_= STATE_CONNECTING_FROM_USER_CLICK;

    if (bli->locationId().isCustomConfigsLocation())
    {
        connSettingsPolicy_.reset(new CustomConfigConnSettingsPolicy(bli));
    }
    // API or static ips locations
    else
    {
        if (!networkConnectionSettings.isAutomatic)
        {
            connSettingsPolicy_.reset(new ManualConnSettingsPolicy(bli, networkConnectionSettings, portMap));
        }
        else if (connectionSettings.isAutomatic)
        {
            connSettingsPolicy_.reset(new AutoConnSettingsPolicy(bli, portMap, proxySettings.isProxyEnabled()));
        }
        else
        {
            connSettingsPolicy_.reset(new ManualConnSettingsPolicy(bli, connectionSettings, portMap));
        }
    }
    connSettingsPolicy_->start();

    connect(connSettingsPolicy_.data(), SIGNAL(hostnamesResolved()), SLOT(onHostnamesResolved()));

    connSettingsPolicy_->debugLocationInfoToLog();

    doConnect();
}

void ConnectionManager::clickDisconnect()
{
    WS_ASSERT(state_ == STATE_CONNECTING_FROM_USER_CLICK || state_ == STATE_CONNECTED || state_ == STATE_RECONNECTING ||
             state_ == STATE_DISCONNECTING_FROM_USER_CLICK || state_ == STATE_WAIT_FOR_NETWORK_CONNECTIVITY || state_ == STATE_DISCONNECTED);

    timerWaitNetworkConnectivity_.stop();
    getWireGuardConfigInLoop_->stop();

    if (state_ != STATE_DISCONNECTING_FROM_USER_CLICK)
    {
        state_ = STATE_DISCONNECTING_FROM_USER_CLICK;
        qCDebug(LOG_CONNECTION) << "ConnectionManager::clickDisconnect()";
        if (connector_)
        {
            connector_->startDisconnect();
        }
        else
        {
            state_ = STATE_DISCONNECTED;
            if (!connSettingsPolicy_.isNull())
            {
                connSettingsPolicy_->reset();
            }
            timerReconnection_.stop();
            Q_EMIT disconnected(DISCONNECTED_BY_USER);
        }
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
                    qCDebug(LOG_CONNECTION) << "ConnectionManager::blockingDisconnect() delay more than 10 seconds";
                    connector_->startDisconnect();
                    break;
                }
            }
            connector_->blockSignals(false);
            doMacRestoreProcedures();
            stunnelManager_->killProcess();
            wstunnelManager_->killProcess();

            if (!connSettingsPolicy_.isNull())
            {
                connSettingsPolicy_->reset();
            }

            state_ = STATE_DISCONNECTED;
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

const ConnectionManager::CustomDnsAdapterGatewayInfo &ConnectionManager::getCustomDnsAdapterGatewayInfo() const
{
    WS_ASSERT(state_ == STATE_CONNECTED); // make sense only in connected state
    return customDnsAdapterGatewayInfo_;
}

QString ConnectionManager::getCustomDnsIp() const
{
    return customDnsAdapterGatewayInfo_.connectedDnsInfo.ipAddress();
}

void ConnectionManager::setConnectedDnsInfo(const types::ConnectedDnsInfo &info)
{
    customDnsAdapterGatewayInfo_.connectedDnsInfo = info;
#ifdef Q_OS_WIN
    if(helper_) {
        dynamic_cast<Helper_win*>(helper_)->setCustomDnsIp(info.ipAddress());
    }
#endif
}

void ConnectionManager::removeIkev2ConnectionFromOS()
{
#ifdef Q_OS_WIN
    IKEv2Connection_win::removeIkev2ConnectionFromOS();
#elif defined Q_OS_MAC
    IKEv2Connection_mac::removeIkev2ConnectionFromOS();
#endif
}

void ConnectionManager::continueWithUsernameAndPassword(const QString &username, const QString &password, bool bNeedReconnect)
{
    WS_ASSERT(connector_ != NULL);
    usernameForCustomOvpn_ = username;
    passwordForCustomOvpn_ = password;

    if (connector_)
    {
        if (bNeedReconnect)
        {
            bWasSuccessfullyConnectionAttempt_ = false;
            state_= STATE_CONNECTING_FROM_USER_CLICK;
            doConnect();
        }
        else
        {
            connector_->continueWithUsernameAndPassword(username, password);
        }
    }
}

void ConnectionManager::continueWithPassword(const QString &password, bool /*bNeedReconnect*/)
{
    WS_ASSERT(connector_ != NULL);
    if (connector_)
    {
        connector_->continueWithPassword(password);
    }
}

void ConnectionManager::onConnectionConnected(const AdapterGatewayInfo &connectionAdapterInfo)
{
    qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionConnected(), state_ =" << state_;

    vpnAdapterInfo_ = connectionAdapterInfo;
    customDnsAdapterGatewayInfo_.adapterInfo = connectionAdapterInfo;

    qCDebug(LOG_CONNECTION) << "VPN adapter and gateway:" << vpnAdapterInfo_.makeLogString();

    // override the DNS if we are using custom
    if (customDnsAdapterGatewayInfo_.connectedDnsInfo.type() == CONNECTED_DNS_TYPE_CUSTOM)
    {
        QString customDnsIp = customDnsAdapterGatewayInfo_.connectedDnsInfo.ipAddress();
        customDnsAdapterGatewayInfo_.adapterInfo.setDnsServers(QStringList() << customDnsIp);
        qCDebug(LOG_CONNECTION) << "Custom DNS detected, will override with: " << customDnsIp;
    }

    if (state_ == STATE_DISCONNECTING_FROM_USER_CLICK)
    {
        qCDebug(LOG_CONNECTION) << "Already disconnecting -- do not enter connected state";
        return;
    }

    timerReconnection_.stop();
    getWireGuardConfigInLoop_->stop();
    state_ = STATE_CONNECTED;
    Q_EMIT connected();
}

void ConnectionManager::onConnectionDisconnected()
{
    qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionDisconnected(), state_ =" << state_;

    testVPNTunnel_->stopTests();
    doMacRestoreProcedures();
    stunnelManager_->killProcess();
    wstunnelManager_->killProcess();
    timerWaitNetworkConnectivity_.stop();
    getWireGuardConfigInLoop_->stop();

    switch (state_)
    {
        case STATE_DISCONNECTING_FROM_USER_CLICK:
            state_ = STATE_DISCONNECTED;
            connSettingsPolicy_->reset();
            timerReconnection_.stop();
            Q_EMIT disconnected(DISCONNECTED_BY_USER);
            break;
        case STATE_CONNECTED:
            // goto reconnection state, start reconnection timer and do connection again
            WS_ASSERT(!timerReconnection_.isActive());
            timerReconnection_.start(MAX_RECONNECTION_TIME);
            state_ = STATE_RECONNECTING;
            Q_EMIT reconnecting();
            doConnect();
            break;
        case STATE_CONNECTING_FROM_USER_CLICK:
        case STATE_AUTO_DISCONNECT:
            state_ = STATE_DISCONNECTED;
            timerReconnection_.stop();
            Q_EMIT disconnected(DISCONNECTED_ITSELF);
            break;
        case STATE_DISCONNECTED:
        case STATE_WAIT_FOR_NETWORK_CONNECTIVITY:
            // nothing todo
            break;

        case STATE_ERROR_DURING_CONNECTION:
            state_ = STATE_DISCONNECTED;
            Q_EMIT errorDuringConnection(latestConnectionError_);
            break;

        case STATE_RECONNECTING:
#ifdef Q_OS_WIN
            if (bNeedResetTap_)
            {
                AdapterUtils_win::resetAdapter(helper_, vpnAdapterInfo_.adapterName());
                bNeedResetTap_ = false;
            }
#endif
            doConnect();
            break;
        case STATE_RECONNECTION_TIME_EXCEED:
            state_ = STATE_DISCONNECTED;
            timerReconnection_.stop();
            Q_EMIT disconnected(DISCONNECTED_BY_RECONNECTION_TIMEOUT_EXCEEDED);
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
                Q_EMIT reconnecting();
                startReconnectionTimer();
                connector_->startDisconnect();
            }
            else
            {
                state_ = STATE_AUTO_DISCONNECT;
                connector_->startDisconnect();
                Q_EMIT showFailedAutomaticConnectionMessage();
            }
            break;

        case STATE_CONNECTED:
            WS_ASSERT(!timerReconnection_.isActive());

            if (!connector_->isDisconnected())
            {
                if (state_ != STATE_RECONNECTING)
                {
                    Q_EMIT reconnecting();
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
                connector_->startDisconnect();
            }
            else
            {
                state_ = STATE_AUTO_DISCONNECT;
                connector_->startDisconnect();
                Q_EMIT showFailedAutomaticConnectionMessage();
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

    qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionError(), state_ =" << state_ << ", error =" << (int)err;
    testVPNTunnel_->stopTests();

    if ((err == CONNECT_ERROR::AUTH_ERROR && bEmitAuthError_)
            || err == CONNECT_ERROR::CANT_RUN_OPENVPN
            || err == CONNECT_ERROR::NO_OPENVPN_SOCKET
            || err == CONNECT_ERROR::NO_INSTALLED_TUN_TAP
            || err == CONNECT_ERROR::ALL_TAP_IN_USE
            || err == CONNECT_ERROR::WIREGUARD_CONNECTION_ERROR
            || err == CONNECT_ERROR::WINTUN_DRIVER_REINSTALLATION_ERROR
            || err == CONNECT_ERROR::TAP_DRIVER_REINSTALLATION_ERROR
            || err == CONNECT_ERROR::WINTUN_FATAL_ERROR)
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
            || (!connSettingsPolicy_->isAutomaticMode() && err == CONNECT_ERROR::EXE_VERIFY_OPENVPN_ERROR)
            || (!connSettingsPolicy_->isAutomaticMode() && err == CONNECT_ERROR::EXE_VERIFY_WIREGUARD_ERROR))
    {
        // immediately stop trying to connect
        state_ = STATE_DISCONNECTED;
        timerReconnection_.stop();
        getWireGuardConfigInLoop_->stop();
        Q_EMIT errorDuringConnection(err);
    }
    else if (err == CONNECT_ERROR::UDP_CANT_ASSIGN
             || err == CONNECT_ERROR::UDP_NO_BUFFER_SPACE
             || err == CONNECT_ERROR::UDP_NETWORK_DOWN
             || err == CONNECT_ERROR::WINTUN_OVER_CAPACITY
             || err == CONNECT_ERROR::TCP_ERROR
             || err == CONNECT_ERROR::CONNECTED_ERROR
             || err == CONNECT_ERROR::INITIALIZATION_SEQUENCE_COMPLETED_WITH_ERRORS
             || err == CONNECT_ERROR::IKEV_FAILED_TO_CONNECT
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
                Q_EMIT reconnecting();
                state_ = STATE_RECONNECTING;
                startReconnectionTimer();
                if (err == CONNECT_ERROR::INITIALIZATION_SEQUENCE_COMPLETED_WITH_ERRORS)
                {
                    bNeedResetTap_ = true;
                }
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
                        Q_EMIT reconnecting();
                        state_ = STATE_RECONNECTING;
                        startReconnectionTimer();
                    }
                    if (err == CONNECT_ERROR::INITIALIZATION_SEQUENCE_COMPLETED_WITH_ERRORS)
                    {
                        bNeedResetTap_ = true;
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
                    Q_EMIT showFailedAutomaticConnectionMessage();
                }
            }
        }
    }
    else
    {
        qCDebug(LOG_CONNECTION) << "Unknown error from openvpn: " << err;
    }
}

void ConnectionManager::onConnectionStatisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes)
{
    Q_EMIT statisticsUpdated(bytesIn, bytesOut, isTotalBytes);
}

void ConnectionManager::onConnectionInterfaceUpdated(const QString &interfaceName)
{
    Q_EMIT interfaceUpdated(interfaceName);
}

void ConnectionManager::onConnectionRequestUsername()
{
    Q_EMIT requestUsername(currentConnectionDescr_.customConfigFilename);
}

void ConnectionManager::onConnectionRequestPassword()
{
    Q_EMIT requestPassword(currentConnectionDescr_.customConfigFilename);
}

void ConnectionManager::onSleepMode()
{
    qCDebug(LOG_CONNECTION) << "ConnectionManager::onSleepMode(), state_ =" << state_;

    timerReconnection_.stop();
    getWireGuardConfigInLoop_->stop();
    bWakeSignalReceived_ = false;

    switch (state_)
    {
        case STATE_DISCONNECTED:
            break;
        case STATE_CONNECTING_FROM_USER_CLICK:
        case STATE_CONNECTED:
        case STATE_RECONNECTING:
            Q_EMIT reconnecting();
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
    qCDebug(LOG_CONNECTION) << "ConnectionManager::onWakeMode(), state_ =" << state_;
    timerReconnection_.stop();
    getWireGuardConfigInLoop_->stop();
    bWakeSignalReceived_ = true;

    switch (state_)
    {
        case STATE_DISCONNECTED:
        case STATE_CONNECTING_FROM_USER_CLICK:
        case STATE_CONNECTED:
        case STATE_RECONNECTING:
        case STATE_WAIT_FOR_NETWORK_CONNECTIVITY:
        case STATE_DISCONNECTING_FROM_USER_CLICK:
        case STATE_RECONNECTION_TIME_EXCEED:
            break;
        case STATE_SLEEP_MODE_NEED_RECONNECT:
            restoreConnectionAfterWakeUp();
            break;
        default:
            WS_ASSERT(false);
    }
}

void ConnectionManager::onNetworkOnlineStateChanged(bool isAlive)
{
    qCDebug(LOG_CONNECTION) << "ConnectionManager::onNetworkOnlineStateChanged(), isAlive =" << isAlive << ", state_ =" << state_;
#ifdef Q_OS_WIN
    Q_EMIT internetConnectivityChanged(isAlive);
#elif defined Q_OS_MAC
    Q_EMIT internetConnectivityChanged(isAlive);

    bLastIsOnline_ = isAlive;

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
                Q_EMIT reconnecting();
                state_ = STATE_WAIT_FOR_NETWORK_CONNECTIVITY;
                WS_ASSERT(!timerReconnection_.isActive());
                timerReconnection_.start(MAX_RECONNECTION_TIME);
                connector_->startDisconnect();
            }
            else
            {
                Q_EMIT reconnecting();
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
#elif defined Q_OS_LINUX
        Q_EMIT internetConnectivityChanged(isAlive);
#endif
}

void ConnectionManager::onTimerReconnection()
{
    qCDebug(LOG_CONNECTION) << "Time for reconnection exceed";
    state_ = STATE_RECONNECTION_TIME_EXCEED;
    if (connector_)
    {
        connector_->startDisconnect();
    }
    else
    {
        state_ = STATE_DISCONNECTED;
        Q_EMIT disconnected(DISCONNECTED_BY_RECONNECTION_TIMEOUT_EXCEEDED);
    }
    timerReconnection_.stop();
}

void ConnectionManager::onStunnelFinishedBeforeConnection()
{
    state_ = STATE_AUTO_DISCONNECT;
    if (connector_)
    {
        connector_->startDisconnect();
    }
    else
    {
        onConnectionDisconnected();
    }
}

void ConnectionManager::onWstunnelFinishedBeforeConnection()
{
    state_ = STATE_AUTO_DISCONNECT;
    if (connector_)
    {
        connector_->startDisconnect();
    }
    else
    {
        onConnectionDisconnected();
    }
}

void ConnectionManager::onWstunnelStarted()
{
    doConnectPart3();
}

void ConnectionManager::doConnect()
{
    if (!networkDetectionManager_->isOnline())
    {
        startReconnectionTimer();
        waitForNetworkConnectivity();
        return;
    }
    defaultAdapterInfo_ = AdapterGatewayInfo::detectAndCreateDefaultAdaperInfo();
    qCDebug(LOG_CONNECTION) << "Default adapter and gateway:" << defaultAdapterInfo_.makeLogString();

    connSettingsPolicy_->resolveHostnames();
}

void ConnectionManager::doConnectPart2()
{

    bIgnoreConnectionErrorsForOpenVpn_ = false;

    currentConnectionDescr_ = connSettingsPolicy_->getCurrentConnectionSettings();
    //WS_ASSERT(currentConnectionDescr_.connectionNodeType != CONNECTION_NODE_ERROR);
    if (currentConnectionDescr_.connectionNodeType == CONNECTION_NODE_ERROR)
    {
        qCDebug(LOG_CONNECTION) << "connSettingsPolicy_.getCurrentConnectionSettings returned incorrect value";
        state_ = STATE_DISCONNECTED;
        timerReconnection_.stop();
        getWireGuardConfigInLoop_->stop();
        Q_EMIT errorDuringConnection(CONNECT_ERROR::LOCATION_NO_ACTIVE_NODES);
        return;
    }

    if (currentConnectionDescr_.connectionNodeType == CONNECTION_NODE_DEFAULT ||
            currentConnectionDescr_.connectionNodeType == CONNECTION_NODE_STATIC_IPS)
    {
        if (currentConnectionDescr_.protocol == PROTOCOL::STUNNEL)
        {
            bool bStunnelConfigSuccess = stunnelManager_->setConfig(currentConnectionDescr_.ip, currentConnectionDescr_.port);
            if (!bStunnelConfigSuccess)
            {
                qCDebug(LOG_CONNECTION) << "Failed create config for stunnel";
                WS_ASSERT(false);
                return;
            }
        }

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
                    qCDebug(LOG_PACKET_SIZE) << "Using default MSS - OpenVpn ConnectionManager MSS too low: " << mss;
                }
                else
                {
                    qCDebug(LOG_PACKET_SIZE) << "OpenVpn connection MSS: " << mss;
                }
            }
            else
            {
                qCDebug(LOG_PACKET_SIZE) << "Packet size mode auto - using default MSS (ConnectionManager)";
            }

            uint portForStunnelOrWStunnel = currentConnectionDescr_.protocol.isStunnelOrWStunnelProtocol() ?
                        (currentConnectionDescr_.protocol == PROTOCOL::STUNNEL ? stunnelManager_->getStunnelPort() : wstunnelManager_->getPort()) : 0;

            const bool blockOutsideDnsOption = !IpValidation::instance().isLocalIp(getCustomDnsIp());
            const bool bOvpnSuccess = makeOVPNFile_->generate(lastOvpnConfig_, currentConnectionDescr_.ip, currentConnectionDescr_.protocol,
                                                        currentConnectionDescr_.port, portForStunnelOrWStunnel, mss, defaultAdapterInfo_.gateway(),
                                                        currentConnectionDescr_.verifyX509name, blockOutsideDnsOption);
            if (!bOvpnSuccess )
            {
                qCDebug(LOG_CONNECTION) << "Failed create ovpn config";
                WS_ASSERT(false);
                return;
            }

            if (currentConnectionDescr_.protocol == PROTOCOL::STUNNEL)
            {
                if(!stunnelManager_->runProcess())
                {
                    state_ = STATE_DISCONNECTED;
                    timerReconnection_.stop();
                    getWireGuardConfigInLoop_->stop();
                    Q_EMIT errorDuringConnection(CONNECT_ERROR::EXE_VERIFY_STUNNEL_ERROR);
                    return;
                }
            }
            else if (currentConnectionDescr_.protocol == PROTOCOL::WSTUNNEL)
            {
                if (!wstunnelManager_->runProcess(currentConnectionDescr_.ip, currentConnectionDescr_.port, false))
                {
                    state_ = STATE_DISCONNECTED;
                    timerReconnection_.stop();
                    getWireGuardConfigInLoop_->stop();
                    Q_EMIT errorDuringConnection(CONNECT_ERROR::EXE_VERIFY_WSTUNNEL_ERROR);
                    return;
                }
                // call doConnectPart2 in onWstunnelStarted slot
                return;
            }
        }
        else if (currentConnectionDescr_.protocol.isIkev2Protocol())
        {
            QString remoteHostname = ExtraConfig::instance().getRemoteIpFromExtraConfig();
            if (IpValidation::instance().isDomain(remoteHostname))
            {
                currentConnectionDescr_.hostname = remoteHostname;
                currentConnectionDescr_.ip = ExtraConfig::instance().getDetectedIp();
                currentConnectionDescr_.dnsHostName = IpValidation::instance().getRemoteIdFromDomain(remoteHostname);
                qCDebug(LOG_CONNECTION) << "Use data from extra config: hostname=" << currentConnectionDescr_.hostname << ", ip=" << currentConnectionDescr_.ip << ", remoteId=" << currentConnectionDescr_.dnsHostName;

            }
        }
        else if (currentConnectionDescr_.protocol.isWireGuardProtocol())
        {
            qCDebug(LOG_CONNECTION) << "Requesting WireGuard config for hostname =" << currentConnectionDescr_.hostname;
            QString deviceId = (isStaticIpsLocation() ? GetDeviceId::instance().getDeviceId() : QString());
            getWireGuardConfigInLoop_->getWireGuardConfig(currentConnectionDescr_.hostname, false, deviceId);
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
                qCDebug(LOG_CONNECTION) << "Failed create ovpn config for custom ovpn file:"
                                        << currentConnectionDescr_.customConfigFilename;
                //WS_ASSERT(false);
                state_ = STATE_DISCONNECTED;
                timerReconnection_.stop();
                getWireGuardConfigInLoop_->stop();
                Q_EMIT errorDuringConnection(CONNECT_ERROR::CANNOT_OPEN_CUSTOM_CONFIG);
                return;
            }
        } else if (currentConnectionDescr_.protocol.isWireGuardProtocol()) {
            WS_ASSERT(currentConnectionDescr_.wgCustomConfig != nullptr);
            if (currentConnectionDescr_.wgCustomConfig == nullptr) {
                qCDebug(LOG_CONNECTION) << "Failed to get config for custom WG file:"
                                        << currentConnectionDescr_.customConfigFilename;
                state_ = STATE_DISCONNECTED;
                timerReconnection_.stop();
                getWireGuardConfigInLoop_->stop();
                Q_EMIT errorDuringConnection(CONNECT_ERROR::CANNOT_OPEN_CUSTOM_CONFIG);
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
    qCDebug(LOG_CONNECTION) << "Connecting to IP:" << currentConnectionDescr_.ip << " protocol:" << currentConnectionDescr_.protocol.toLongString() << " port:" << currentConnectionDescr_.port;
    Q_EMIT protocolPortChanged(currentConnectionDescr_.protocol, currentConnectionDescr_.port);

    if (currentConnectionDescr_.protocol.isWireGuardProtocol())
    {
        WireGuardConfig* pConfig = (currentConnectionDescr_.connectionNodeType == CONNECTION_NODE_CUSTOM_CONFIG ? currentConnectionDescr_.wgCustomConfig.get() : &wireGuardConfig_);
        WS_ASSERT(pConfig != nullptr);
        Q_EMIT connectingToHostname(currentConnectionDescr_.hostname, currentConnectionDescr_.ip, pConfig->clientDnsAddress());
    }
    else
    {
        Q_EMIT connectingToHostname(currentConnectionDescr_.hostname, currentConnectionDescr_.ip, "");
    }

    if (currentConnectionDescr_.connectionNodeType == CONNECTION_NODE_CUSTOM_CONFIG)
    {
        if (currentConnectionDescr_.protocol.isWireGuardProtocol())
            recreateConnector(PROTOCOL::WIREGUARD);
        else
            recreateConnector(PROTOCOL::OPENVPN_UDP);

        connector_->startConnect(makeOVPNFileFromCustom_->path(), "", "", usernameForCustomOvpn_,
                                 passwordForCustomOvpn_, lastProxySettings_,
                                 currentConnectionDescr_.wgCustomConfig.get(), false, false);
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
                username = lastServerCredentials_.usernameForOpenVpn();
                password = lastServerCredentials_.passwordForOpenVpn();
            }

            recreateConnector(PROTOCOL::OPENVPN_UDP);
            connector_->startConnect(makeOVPNFile_->path(), "", "", username, password, lastProxySettings_, nullptr, false, connSettingsPolicy_->isAutomaticMode());
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
                username = lastServerCredentials_.usernameForIkev2();
                password = lastServerCredentials_.passwordForIkev2();
            }

            recreateConnector(PROTOCOL::IKEV2);
            connector_->startConnect(currentConnectionDescr_.hostname, currentConnectionDescr_.ip, currentConnectionDescr_.hostname, username, password, lastProxySettings_,
                                     nullptr, ExtraConfig::instance().isUseIkev2Compression(), connSettingsPolicy_->isAutomaticMode());
        }
        else if (currentConnectionDescr_.protocol.isWireGuardProtocol())
        {
            QString endpointAndPort = QString("%1:%2").arg(currentConnectionDescr_.ip).arg(currentConnectionDescr_.port);
            wireGuardConfig_.setPeerPublicKey(currentConnectionDescr_.wgPeerPublicKey);
            wireGuardConfig_.setPeerEndpoint(endpointAndPort);
            recreateConnector(PROTOCOL::WIREGUARD);
            connector_->startConnect(QString(), currentConnectionDescr_.ip,
                currentConnectionDescr_.dnsHostName, QString(), QString(), lastProxySettings_,
                &wireGuardConfig_, false, connSettingsPolicy_->isAutomaticMode());
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
#ifdef Q_OS_MAC
    if (!connector_)
        return;
    const auto connection_type = connector_->getConnectionType();
    if (connection_type == ConnectionType::OPENVPN)
    {
        QString delRouteCommand = "route -n delete " + lastIp_ + "/32 " + defaultAdapterInfo_.gateway();
        qCDebug(LOG_CONNECTION) << "Execute command: " << delRouteCommand;
        Helper_mac *helper_mac = dynamic_cast<Helper_mac *>(helper_);
        QString cmdAnswer = helper_mac->executeRootCommand(delRouteCommand);
        qCDebug(LOG_CONNECTION) << "Output from route delete command: " << cmdAnswer;
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
    qCDebug(LOG_CONNECTION) << "No internet connection, waiting maximum: " << MAX_RECONNECTION_TIME << "ms";
    timerWaitNetworkConnectivity_.start(1000);
}

void ConnectionManager::recreateConnector(PROTOCOL protocol)
{
    if (currentProtocol_ == PROTOCOL::UNINITIALIZED)
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
#elif defined Q_OS_MAC
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

        connect(connector_, SIGNAL(connected(AdapterGatewayInfo)), SLOT(onConnectionConnected(AdapterGatewayInfo)), Qt::QueuedConnection);
        connect(connector_, SIGNAL(disconnected()), SLOT(onConnectionDisconnected()), Qt::QueuedConnection);
        connect(connector_, SIGNAL(reconnecting()), SLOT(onConnectionReconnecting()), Qt::QueuedConnection);
        connect(connector_, SIGNAL(error(CONNECT_ERROR)), SLOT(onConnectionError(CONNECT_ERROR)), Qt::QueuedConnection);
        connect(connector_, SIGNAL(statisticsUpdated(quint64,quint64, bool)), SLOT(onConnectionStatisticsUpdated(quint64,quint64, bool)), Qt::QueuedConnection);
        connect(connector_, SIGNAL(interfaceUpdated(QString)), SLOT(onConnectionInterfaceUpdated(QString)), Qt::QueuedConnection);

        connect(connector_, SIGNAL(requestUsername()), SLOT(onConnectionRequestUsername()), Qt::QueuedConnection);
        connect(connector_, SIGNAL(requestPassword()), SLOT(onConnectionRequestPassword()), Qt::QueuedConnection);

        currentProtocol_ = protocol;
    }
}

void ConnectionManager::restoreConnectionAfterWakeUp()
{
    if (bLastIsOnline_)
    {
        qCDebug(LOG_CONNECTION) <<
            "ConnectionManager::restoreConnectionAfterWakeUp(), reconnecting";
        state_ = STATE_WAKEUP_RECONNECTING;
        Q_EMIT reconnecting();
        doConnect();
    }
    else
    {
        qCDebug(LOG_CONNECTION) <<
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
        Q_EMIT testTunnelResult(bSuccess, "");
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
                Q_EMIT reconnecting();
                startReconnectionTimer();
                connector_->startDisconnect();
             }
             else
             {
                state_= STATE_AUTO_DISCONNECT;
                connector_->startDisconnect();
                Q_EMIT showFailedAutomaticConnectionMessage();
             }
        }
        else
        {
            Q_EMIT testTunnelResult(false, "");
        }
    }
    else
    {
        Q_EMIT testTunnelResult(true, ipAddress);

        // if connection mode is automatic, save last successfully connection settings
        bWasSuccessfullyConnectionAttempt_ = true;
        connSettingsPolicy_->saveCurrentSuccessfullConnectionSettings();
    }
}

void ConnectionManager::onTimerWaitNetworkConnectivity()
{
    if (networkDetectionManager_->isOnline())
    {
        qCDebug(LOG_CONNECTION) << "We online, make the connection";
        timerWaitNetworkConnectivity_.stop();
        doConnect();
    }
    else
    {
        if (timerReconnection_.remainingTime() == 0)
        {
            qCDebug(LOG_CONNECTION) << "Time for wait network connection exceed";
            timerWaitNetworkConnectivity_.stop();
            timerReconnection_.stop();
            getWireGuardConfigInLoop_->stop();
            state_ = STATE_DISCONNECTED;
            Q_EMIT disconnected(DISCONNECTED_BY_RECONNECTION_TIMEOUT_EXCEEDED);
        }
    }
}

void ConnectionManager::onHostnamesResolved()
{
    doConnectPart2();
}

void ConnectionManager::onGetWireGuardConfigAnswer(SERVER_API_RET_CODE retCode, const WireGuardConfig &config)
{
    if (retCode == SERVER_RETURN_WIREGUARD_KEY_LIMIT)
    {
        Q_EMIT wireGuardAtKeyLimit();
    }
    else if (retCode == SERVER_RETURN_SUCCESS)
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
        // this should not happen logically
        WS_ASSERT(false);
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

apiinfo::StaticIpPortsVector ConnectionManager::getStatisIps()
{
    WS_ASSERT(isStaticIpsLocation());
    return currentConnectionDescr_.staticIpPorts;
}

void ConnectionManager::onWireGuardKeyLimitUserResponse(bool deleteOldestKey)
{
    if (deleteOldestKey)
    {
        QString deviceId = (isStaticIpsLocation() ? GetDeviceId::instance().getDeviceId() : QString());
        getWireGuardConfigInLoop_->getWireGuardConfig(currentConnectionDescr_.hostname, true, deviceId);
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

PROTOCOL ConnectionManager::currentProtocol() const
{
    return currentProtocol_;
}
