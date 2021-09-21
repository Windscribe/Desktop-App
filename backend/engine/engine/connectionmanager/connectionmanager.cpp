#include "connectionmanager.h"
#include "utils/logger.h"
#include <QStandardPaths>
#include <QThread>
#include <QCoreApplication>
#include <QDateTime>
#include "isleepevents.h"
#include "openvpnconnection.h"
#include "wireguardconnection.h"

#include "utils/utils.h"
#include "engine/types/types.h"
#include "engine/types/connectionsettings.h"

#include "engine/networkstatemanager/inetworkstatemanager.h"
#include "utils/extraconfig.h"
#include "utils/ipvalidation.h"

#include "connsettingspolicy/autoconnsettingspolicy.h"
#include "connsettingspolicy/manualconnsettingspolicy.h"
#include "connsettingspolicy/customconfigconnsettingspolicy.h"


#ifdef Q_OS_WIN
    #include "sleepevents_win.h"
    #include "adapterutils_win.h"
    #include "ikev2connection_win.h"
    #include "../openvpnversioncontroller.h"
#elif defined Q_OS_MAC
    #include "sleepevents_mac.h"
    #include "utils/macutils.h"
    #include "ikev2connection_mac.h"
    #include "engine/helper/helper_mac.h"
#elif defined Q_OS_LINUX
    #include "ikev2connection_linux.h"
#endif

const int typeIdProtocol = qRegisterMetaType<ProtoTypes::Protocol>("ProtoTypes::Protocol");

ConnectionManager::ConnectionManager(QObject *parent, IHelper *helper, INetworkStateManager *networkStateManager,
                                             ServerAPI *serverAPI, CustomOvpnAuthCredentialsStorage *customOvpnAuthCredentialsStorage) : QObject(parent),
    helper_(helper),
    networkStateManager_(networkStateManager),
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
    connect(networkStateManager_, SIGNAL(stateChanged(bool, QString)), SLOT(onNetworkStateChanged(bool, QString)));
#elif defined Q_OS_MAC
    sleepEvents_ = new SleepEvents_mac(this);
    connect(networkStateManager_, SIGNAL(stateChanged(bool, QString)), SLOT(onNetworkStateChanged(bool, QString)));
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    connect(sleepEvents_, SIGNAL(gotoSleep()), SLOT(onSleepMode()));
    connect(sleepEvents_, SIGNAL(gotoWake()), SLOT(onWakeMode()));
#endif

    connect(&timerWaitNetworkConnectivity_, SIGNAL(timeout()), SLOT(onTimerWaitNetworkConnectivity()));
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
}

void ConnectionManager::clickConnect(const QString &ovpnConfig, const apiinfo::ServerCredentials &serverCredentials,
                                         QSharedPointer<locationsmodel::BaseLocationInfo> bli,
                                         const ConnectionSettings &connectionSettings,
                                         const apiinfo::PortMap &portMap, const ProxySettings &proxySettings, bool bEmitAuthError, const QString &customConfigPath)
{
    Q_ASSERT(state_ == STATE_DISCONNECTED);

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
        if (connectionSettings.isAutomatic())
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
    Q_ASSERT(state_ == STATE_CONNECTING_FROM_USER_CLICK || state_ == STATE_CONNECTED || state_ == STATE_RECONNECTING ||
             state_ == STATE_DISCONNECTING_FROM_USER_CLICK || state_ == STATE_WAIT_FOR_NETWORK_CONNECTIVITY || state_ == STATE_DISCONNECTED);

    timerWaitNetworkConnectivity_.stop();

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
            emit disconnected(DISCONNECTED_BY_USER);
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
            Q_ASSERT(connector_->isDisconnected());
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
    Q_ASSERT(state_ == STATE_CONNECTED); // make sense only in connected state
    return vpnAdapterInfo_;
}

const ConnectionManager::CustomDnsAdapterGatewayInfo &ConnectionManager::getCustomDnsAdapterGatewayInfo() const
{
    Q_ASSERT(state_ == STATE_CONNECTED); // make sense only in connected state
    return customDnsAdapterGatewayInfo_;
}

QString ConnectionManager::getCustomDnsIp() const
{
    return QString(customDnsAdapterGatewayInfo_.dnsWhileConnectedInfo.ip_address().c_str());
}

void ConnectionManager::setDnsWhileConnectedInfo(const ProtoTypes::DnsWhileConnectedInfo &info)
{
    customDnsAdapterGatewayInfo_.dnsWhileConnectedInfo = info;
#ifdef Q_OS_WIN
    if(helper_) {
        dynamic_cast<Helper_win*>(helper_)->setCustomDnsIp(info.ip_address().c_str());
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
    Q_ASSERT(connector_ != NULL);
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
    Q_ASSERT(connector_ != NULL);
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
    if (customDnsAdapterGatewayInfo_.dnsWhileConnectedInfo.type() == ProtoTypes::DNS_WHILE_CONNECTED_TYPE_CUSTOM)
    {
        QString customDnsIp = QString::fromStdString(customDnsAdapterGatewayInfo_.dnsWhileConnectedInfo.ip_address());
        customDnsAdapterGatewayInfo_.adapterInfo.setDnsServers(QStringList() << customDnsIp);
        qCDebug(LOG_CONNECTION) << "Custom DNS detected, will override with: " << customDnsIp;
    }

    if (state_ == STATE_DISCONNECTING_FROM_USER_CLICK)
    {
        qCDebug(LOG_CONNECTION) << "Already disconnecting -- do not enter connected state";
        return;
    }

    timerReconnection_.stop();
    state_ = STATE_CONNECTED;
    emit connected();
}

void ConnectionManager::onConnectionDisconnected()
{
    qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionDisconnected(), state_ =" << state_;

    testVPNTunnel_->stopTests();
    doMacRestoreProcedures();
    stunnelManager_->killProcess();
    wstunnelManager_->killProcess();
    timerWaitNetworkConnectivity_.stop();

    switch (state_)
    {
        case STATE_DISCONNECTING_FROM_USER_CLICK:
            state_ = STATE_DISCONNECTED;
            connSettingsPolicy_->reset();
            timerReconnection_.stop();
            emit disconnected(DISCONNECTED_BY_USER);
            break;
        case STATE_CONNECTED:
            // goto reconnection state, start reconnection timer and do connection again
            Q_ASSERT(!timerReconnection_.isActive());
            timerReconnection_.start(MAX_RECONNECTION_TIME);
            state_ = STATE_RECONNECTING;
            emit reconnecting();
            doConnect();
            break;
        case STATE_CONNECTING_FROM_USER_CLICK:
        case STATE_AUTO_DISCONNECT:
            state_ = STATE_DISCONNECTED;
            timerReconnection_.stop();
            emit disconnected(DISCONNECTED_ITSELF);
            break;
        case STATE_DISCONNECTED:
        case STATE_WAIT_FOR_NETWORK_CONNECTIVITY:
            // nothing todo
            break;

        case STATE_ERROR_DURING_CONNECTION:
            state_ = STATE_DISCONNECTED;
            emit errorDuringConnection(latestConnectionError_);
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
            Q_ASSERT(false);
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
            Q_ASSERT(!timerReconnection_.isActive());

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
                connector_->startDisconnect();
            }
            else
            {
                state_ = STATE_AUTO_DISCONNECT;
                connector_->startDisconnect();
                emit showFailedAutomaticConnectionMessage();
            }
            break;
        case STATE_SLEEP_MODE_NEED_RECONNECT:
            break;
        case STATE_WAKEUP_RECONNECTING:
            break;
        default:
            Q_ASSERT(false);
    }
}

void ConnectionManager::onConnectionError(ProtoTypes::ConnectError err)
{
    if (state_ == STATE_DISCONNECTING_FROM_USER_CLICK || state_ == STATE_RECONNECTION_TIME_EXCEED ||
        state_ == STATE_AUTO_DISCONNECT || state_ == STATE_ERROR_DURING_CONNECTION)
    {
        return;
    }

    qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionError(), state_ =" << state_ << ", error =" << (int)err;
    testVPNTunnel_->stopTests();

    if ((err == ProtoTypes::ConnectError::AUTH_ERROR && bEmitAuthError_)
            || err == ProtoTypes::ConnectError::CANT_RUN_OPENVPN
            || err == ProtoTypes::ConnectError::NO_OPENVPN_SOCKET
            || err == ProtoTypes::ConnectError::NO_INSTALLED_TUN_TAP
            || err == ProtoTypes::ConnectError::ALL_TAP_IN_USE
            || err == ProtoTypes::ConnectError::WIREGUARD_CONNECTION_ERROR
            || err == ProtoTypes::ConnectError::WINTUN_DRIVER_REINSTALLATION_ERROR
            || err == ProtoTypes::ConnectError::TAP_DRIVER_REINSTALLATION_ERROR
            || err == ProtoTypes::ConnectError::WINTUN_FATAL_ERROR)
    {
        // emit error in disconnected event
        latestConnectionError_ = err;
        state_ = STATE_ERROR_DURING_CONNECTION;
        timerReconnection_.stop();
    }
    else if ( (!connSettingsPolicy_->isAutomaticMode() && (err == ProtoTypes::ConnectError::IKEV_NOT_FOUND_WIN
                                                           || err == ProtoTypes::ConnectError::IKEV_FAILED_SET_ENTRY_WIN
                                                           || err == ProtoTypes::ConnectError::IKEV_FAILED_MODIFY_HOSTS_WIN) )
            || (!connSettingsPolicy_->isAutomaticMode() && (err == ProtoTypes::ConnectError::IKEV_NETWORK_EXTENSION_NOT_FOUND_MAC
                                                            || err == ProtoTypes::ConnectError::IKEV_FAILED_SET_KEYCHAIN_MAC
                                                            || err == ProtoTypes::ConnectError::IKEV_FAILED_START_MAC
                                                            || err == ProtoTypes::ConnectError::IKEV_FAILED_LOAD_PREFERENCES_MAC
                                                            || err == ProtoTypes::ConnectError::IKEV_FAILED_SAVE_PREFERENCES_MAC)))
    {
        state_ = STATE_DISCONNECTED;
        timerReconnection_.stop();
        emit errorDuringConnection(err);
    }
    else if (err == ProtoTypes::ConnectError::UDP_CANT_ASSIGN
             || err == ProtoTypes::ConnectError::UDP_NO_BUFFER_SPACE
             || err == ProtoTypes::ConnectError::UDP_NETWORK_DOWN
             || err == ProtoTypes::ConnectError::WINTUN_OVER_CAPACITY
             || err == ProtoTypes::ConnectError::TCP_ERROR
             || err == ProtoTypes::ConnectError::CONNECTED_ERROR
             || err == ProtoTypes::ConnectError::INITIALIZATION_SEQUENCE_COMPLETED_WITH_ERRORS
             || err == ProtoTypes::ConnectError::IKEV_FAILED_TO_CONNECT
             || (connSettingsPolicy_->isAutomaticMode() && (err == ProtoTypes::ConnectError::IKEV_NOT_FOUND_WIN
                                                            || err == ProtoTypes::ConnectError::IKEV_FAILED_SET_ENTRY_WIN
                                                            || err == ProtoTypes::ConnectError::IKEV_FAILED_MODIFY_HOSTS_WIN))
             || (connSettingsPolicy_->isAutomaticMode() && (err == ProtoTypes::ConnectError::IKEV_NETWORK_EXTENSION_NOT_FOUND_MAC
                                                            || err == ProtoTypes::ConnectError::IKEV_FAILED_SET_KEYCHAIN_MAC
                                                            || err == ProtoTypes::ConnectError::IKEV_FAILED_START_MAC
                                                            || err == ProtoTypes::ConnectError::IKEV_FAILED_LOAD_PREFERENCES_MAC
                                                            || err == ProtoTypes::ConnectError::IKEV_FAILED_SAVE_PREFERENCES_MAC))
             || (err == ProtoTypes::ConnectError::AUTH_ERROR && !bEmitAuthError_))
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
                if (err == ProtoTypes::ConnectError::INITIALIZATION_SEQUENCE_COMPLETED_WITH_ERRORS)
                {
                    bNeedResetTap_ = true;
                }
                // for AUTH_ERROR signal disconnected will be emitted automatically
                if (err != ProtoTypes::ConnectError::AUTH_ERROR)
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
                    if (err == ProtoTypes::ConnectError::INITIALIZATION_SEQUENCE_COMPLETED_WITH_ERRORS)
                    {
                        bNeedResetTap_ = true;
                    }
                    // for AUTH_ERROR signal disconnected will be emitted automatically
                    if (err != ProtoTypes::ConnectError::AUTH_ERROR)
                    {
                        connector_->startDisconnect();
                    }
                }
                else
                {
                    state_ = STATE_AUTO_DISCONNECT;
                    if (err != ProtoTypes::ConnectError::AUTH_ERROR)
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
        qCDebug(LOG_CONNECTION) << "Unknown error from openvpn: " << err;
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

void ConnectionManager::onSleepMode()
{
    qCDebug(LOG_CONNECTION) << "ConnectionManager::onSleepMode(), state_ =" << state_;

    timerReconnection_.stop();
    bWakeSignalReceived_ = false;

    switch (state_)
    {
        case STATE_DISCONNECTED:
            break;
        case STATE_CONNECTING_FROM_USER_CLICK:
        case STATE_CONNECTED:
        case STATE_RECONNECTING:
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
            Q_ASSERT(false);
    }
}

void ConnectionManager::onWakeMode()
{
    qCDebug(LOG_CONNECTION) << "ConnectionManager::onWakeMode(), state_ =" << state_;
    timerReconnection_.stop();
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
            Q_ASSERT(false);
    }
}

void ConnectionManager::onNetworkStateChanged(bool isAlive, const QString &networkInterface)
{
    qCDebug(LOG_CONNECTION) << "ConnectionManager::onNetworkChanged(), isAlive =" << isAlive << ", primary network interface =" << networkInterface << ", state_ =" << state_;
#ifdef Q_OS_WIN
    emit internetConnectivityChanged(isAlive);
#elif defined Q_OS_MAC
    emit internetConnectivityChanged(isAlive);

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
                Q_ASSERT(!timerReconnection_.isActive());
                timerReconnection_.start(MAX_RECONNECTION_TIME);
                connector_->startDisconnect();
            }
            break;
        case STATE_CONNECTED:
            if (!isAlive)
            {
                emit reconnecting();
                state_ = STATE_WAIT_FOR_NETWORK_CONNECTIVITY;
                Q_ASSERT(!timerReconnection_.isActive());
                timerReconnection_.start(MAX_RECONNECTION_TIME);
                connector_->startDisconnect();
            }
            else
            {
                emit reconnecting();
                state_ = STATE_RECONNECTING;
                Q_ASSERT(!timerReconnection_.isActive());
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
            Q_ASSERT(false);
    }
#elif defined Q_OS_LINUX
        //todo linux
        emit internetConnectivityChanged(isAlive);
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
        emit disconnected(DISCONNECTED_BY_RECONNECTION_TIMEOUT_EXCEEDED);
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
    if (!networkStateManager_->isOnline())
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
    //Q_ASSERT(currentConnectionDescr_.connectionNodeType != CONNECTION_NODE_ERROR);
    if (currentConnectionDescr_.connectionNodeType == CONNECTION_NODE_ERROR)
    {
        qCDebug(LOG_CONNECTION) << "connSettingsPolicy_.getCurrentConnectionSettings returned incorrect value";
        state_ = STATE_DISCONNECTED;
        timerReconnection_.stop();
        emit errorDuringConnection(ProtoTypes::ConnectError::LOCATION_NO_ACTIVE_NODES);
        return;
    }

    if (currentConnectionDescr_.connectionNodeType == CONNECTION_NODE_DEFAULT ||
            currentConnectionDescr_.connectionNodeType == CONNECTION_NODE_STATIC_IPS)
    {
        if (currentConnectionDescr_.protocol.getType() == ProtocolType::PROTOCOL_STUNNEL)
        {
            bool bStunnelConfigSuccess = stunnelManager_->setConfig(currentConnectionDescr_.ip, currentConnectionDescr_.port);
            if (!bStunnelConfigSuccess)
            {
                qCDebug(LOG_CONNECTION) << "Failed create config for stunnel";
                Q_ASSERT(false);
                return;
            }
        }

        if (currentConnectionDescr_.protocol.isOpenVpnProtocol())
        {

            int mss = 0;
            if (!packetSize_.is_automatic())
            {
                bool advParamsOpenVpnExists = false;
                int openVpnOffset = ExtraConfig::instance().getMtuOffsetOpenVpn(advParamsOpenVpnExists);
                if (!advParamsOpenVpnExists) openVpnOffset = MTU_OFFSET_OPENVPN;

                mss = packetSize_.mtu() - openVpnOffset;

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
                        (currentConnectionDescr_.protocol.getType() == ProtocolType::PROTOCOL_STUNNEL ? stunnelManager_->getStunnelPort() : wstunnelManager_->getPort()) : 0;

            const bool blockOutsideDnsOption = !IpValidation::instance().isLocalIp(getCustomDnsIp());
            const bool bOvpnSuccess = makeOVPNFile_->generate(lastOvpnConfig_, currentConnectionDescr_.ip, currentConnectionDescr_.protocol,
                                                        currentConnectionDescr_.port, portForStunnelOrWStunnel, mss, defaultAdapterInfo_.gateway(),
                                                        currentConnectionDescr_.verifyX509name, blockOutsideDnsOption);
            if (!bOvpnSuccess )
            {
                qCDebug(LOG_CONNECTION) << "Failed create ovpn config";
                Q_ASSERT(false);
                return;
            }

            if (currentConnectionDescr_.protocol.getType() == ProtocolType::PROTOCOL_STUNNEL)
            {
                stunnelManager_->runProcess();
            }
            else if (currentConnectionDescr_.protocol.getType() == ProtocolType::PROTOCOL_WSTUNNEL)
            {
                wstunnelManager_->runProcess(currentConnectionDescr_.ip, currentConnectionDescr_.port, false);
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
            // If WireGuard config data don't exist, fetch it now.
            if (!wireGuardConfig_) {
                qCDebug(LOG_CONNECTION) << "Missing WireGuard user config, requesting a new one";
                emit getWireGuardConfig();
                return;
            }
            qCDebug(LOG_CONNECTION) << "Using existing WireGuard user config";
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
                //Q_ASSERT(false);
                state_ = STATE_DISCONNECTED;
                timerReconnection_.stop();
                emit errorDuringConnection(ProtoTypes::ConnectError::CANNOT_OPEN_CUSTOM_CONFIG);
                return;
            }
        } else if (currentConnectionDescr_.protocol.isWireGuardProtocol()) {
            Q_ASSERT(currentConnectionDescr_.wgCustomConfig != nullptr);
            if (currentConnectionDescr_.wgCustomConfig == nullptr) {
                qCDebug(LOG_CONNECTION) << "Failed to get config for custom WG file:"
                                        << currentConnectionDescr_.customConfigFilename;
                state_ = STATE_DISCONNECTED;
                timerReconnection_.stop();
                emit errorDuringConnection(ProtoTypes::ConnectError::CANNOT_OPEN_CUSTOM_CONFIG);
                return;
            }
        }
    }
    else
    {
        Q_ASSERT(false);
    }

    doConnectPart3();
}

void ConnectionManager::doConnectPart3()
{
    qCDebug(LOG_CONNECTION) << "Connecting to IP:" << currentConnectionDescr_.ip << " protocol:" << currentConnectionDescr_.protocol.toLongString() << " port:" << currentConnectionDescr_.port;
    emit protocolPortChanged(currentConnectionDescr_.protocol.convertToProtobuf(), currentConnectionDescr_.port);
    emit connectingToHostname(currentConnectionDescr_.hostname, currentConnectionDescr_.ip);

    if (currentConnectionDescr_.connectionNodeType == CONNECTION_NODE_CUSTOM_CONFIG)
    {
        if (currentConnectionDescr_.protocol.isWireGuardProtocol())
            recreateConnector(ProtocolType(ProtocolType::PROTOCOL_WIREGUARD));
        else
            recreateConnector(ProtocolType(ProtocolType::PROTOCOL_OPENVPN_UDP));
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

            recreateConnector(ProtocolType(ProtocolType::PROTOCOL_OPENVPN_UDP));
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

            recreateConnector(ProtocolType(ProtocolType::PROTOCOL_IKEV2));
            connector_->startConnect(currentConnectionDescr_.hostname, currentConnectionDescr_.ip, currentConnectionDescr_.hostname, username, password, lastProxySettings_,
                                     nullptr, ExtraConfig::instance().isUseIkev2Compression(), connSettingsPolicy_->isAutomaticMode());
        }
        else if (currentConnectionDescr_.protocol.isWireGuardProtocol())
        {
            Q_ASSERT(wireGuardConfig_ != nullptr);
            QString endpointAndPort = QString("%1:%2")
                .arg(currentConnectionDescr_.ip)
                .arg(currentConnectionDescr_.port);
            wireGuardConfig_->updatePeerInfo(currentConnectionDescr_.wgPublicKey, endpointAndPort);
            recreateConnector(ProtocolType(ProtocolType::PROTOCOL_WIREGUARD));
            connector_->startConnect(QString(), currentConnectionDescr_.ip,
                currentConnectionDescr_.dnsHostName, QString(), QString(), lastProxySettings_,
                wireGuardConfig_.get(), false, connSettingsPolicy_->isAutomaticMode());
        }
        else
        {
            Q_ASSERT(false);
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

void ConnectionManager::recreateConnector(ProtocolType protocol)
{
    if (!currentProtocol_.isInitialized())
    {
        Q_ASSERT(connector_ == NULL);
    }

    if (!currentProtocol_.isEqual(protocol))
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
            Q_ASSERT(false);
        }

        connect(connector_, SIGNAL(connected(AdapterGatewayInfo)), SLOT(onConnectionConnected(AdapterGatewayInfo)), Qt::QueuedConnection);
        connect(connector_, SIGNAL(disconnected()), SLOT(onConnectionDisconnected()), Qt::QueuedConnection);
        connect(connector_, SIGNAL(reconnecting()), SLOT(onConnectionReconnecting()), Qt::QueuedConnection);
        connect(connector_, SIGNAL(error(ProtoTypes::ConnectError)), SLOT(onConnectionError(ProtoTypes::ConnectError)), Qt::QueuedConnection);
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
    if (!bSuccess)
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

        // if connection mode is automatic, save last successfully connection settings
        bWasSuccessfullyConnectionAttempt_ = true;
        connSettingsPolicy_->saveCurrentSuccessfullConnectionSettings();
    }
}

void ConnectionManager::onTimerWaitNetworkConnectivity()
{
    if (networkStateManager_->isOnline())
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
            state_ = STATE_DISCONNECTED;
            emit disconnected(DISCONNECTED_BY_RECONNECTION_TIMEOUT_EXCEEDED);
        }
    }
}

void ConnectionManager::onHostnamesResolved()
{
    doConnectPart2();
}

void ConnectionManager::setWireGuardConfig(QSharedPointer<WireGuardConfig> config)
{
    if (config) {
        // Config fetched successfully.
        wireGuardConfig_ = config;
    }

    // If the protocol has been changed, do nothing.
    if (!currentConnectionDescr_.protocol.isWireGuardProtocol())
        return;

    if (config) {
        doConnectPart3();
    } else {
        // Failed to fetch a config, stop connection.
        state_ = STATE_AUTO_DISCONNECT;
        if (connector_)
            connector_->startDisconnect();
        else
            onConnectionDisconnected();
    }
}

void ConnectionManager::resetWireGuardConfig()
{
    wireGuardConfig_.reset();
}

bool ConnectionManager::isCustomOvpnConfigCurrentConnection() const
{
    return currentConnectionDescr_.connectionNodeType == CONNECTION_NODE_CUSTOM_CONFIG &&
           currentConnectionDescr_.protocol.isOpenVpnProtocol();
}

QString ConnectionManager::getCustomOvpnConfigFileName()
{
    Q_ASSERT(isCustomOvpnConfigCurrentConnection());
    return currentConnectionDescr_.customConfigFilename;
}

bool ConnectionManager::isStaticIpsLocation() const
{
    return currentConnectionDescr_.connectionNodeType == CONNECTION_NODE_STATIC_IPS;
}

apiinfo::StaticIpPortsVector ConnectionManager::getStatisIps()
{
    Q_ASSERT(isStaticIpsLocation());
    return currentConnectionDescr_.staticIpPorts;
}

void ConnectionManager::setPacketSize(ProtoTypes::PacketSize ps)
{
    packetSize_ = ps;
}

void ConnectionManager::startTunnelTests()
{
    #if defined(Q_OS_WINDOWS)
    // Issue 516.  Need to delay the start of the tunnel tests when using OpenVPN and Wintun on Windows.
    if (currentConnectionDescr_.protocol.isOpenVpnProtocol() && OpenVpnVersionController::instance().isUseWinTun())
    {
        bool advParamExists = false;
        int delay = ExtraConfig::instance().getTunnelTestStartDelay(advParamExists);

        if (!advParamExists) {
            delay = 3000;
        }

        qCDebug(LOG_CONNECTION) << "Delaying tunnel test start for" << delay << "ms";
        QTimer::singleShot(delay, testVPNTunnel_, &TestVPNTunnel::startTests);
        return;
    }
    #endif

    testVPNTunnel_->startTests();
}

bool ConnectionManager::isAllowFirewallAfterConnection() const
{
    Q_ASSERT(connector_);
    if (!connector_ || currentConnectionDescr_.connectionNodeType != CONNECTION_NODE_CUSTOM_CONFIG)
        return true;

    return currentConnectionDescr_.isAllowFirewallAfterConnection
        && connector_->isAllowFirewallAfterCustomConfigConnection();
}
