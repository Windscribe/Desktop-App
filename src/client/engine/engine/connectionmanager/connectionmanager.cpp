#include "utils/log/logger.h"

#include <QCoreApplication>
#include <QThread>

#include "connsettingspolicy/connsettingspolicyfactory.h"
#include "engine/connectionmanager/connectors/connectionfactory.h"
#include "engine/connectionmanager/connectors/connectionplatformpolicy.h"
#include "engine/crossplatformobjectfactory.h"
#include "engine/dns/idnsconfigurator.h"
#include "engine/helper/helper.h"
#include "engine/networkdetectionmanager/inetworkdetectionmanager.h"
#include "engine/wireguardconfig/getwireguardconfig.h"
#include "extraconfigaccessor.h"
#include "isleepevents.h"
#include "testvpntunnel.h"
#include "types/connectionsettings.h"
#include "types/enums.h"
#include "utils/utils.h"
#include "utils/ws_assert.h"

// Had to move this here to prevent a compile error with boost already including winsock.h
#include "connectionmanager.h"

ConnectionManager::ConnectionManager(QObject *parent, Helper *helper, INetworkDetectionManager *networkDetectionManager,
                                     CustomOvpnAuthCredentialsStorage *customOvpnAuthCredentialsStorage, IDnsConfigurator *dnsConfigurator,
                                     IConnectionFactory *connectionFactory, IConnectionPlatformPolicy *platformPolicy,
                                     IConnSettingsPolicyFactory *connSettingsPolicyFactory, ISleepEvents *sleepEvents,
                                     IExtraConfigAccessor *extraConfig) : QObject(parent),
    helper_(helper),
    networkDetectionManager_(networkDetectionManager),
    customOvpnAuthCredentialsStorage_(customOvpnAuthCredentialsStorage),
    dnsConfigurator_(dnsConfigurator),
    connectionFactory_(connectionFactory ? connectionFactory : new ConnectionFactory()),
    platformPolicy_(platformPolicy ? platformPolicy : new ConnectionPlatformPolicy(helper)),
    connSettingsPolicyFactory_(connSettingsPolicyFactory ? connSettingsPolicyFactory : new ConnSettingsPolicyFactory()),
    extraConfig_(extraConfig ? extraConfig : new ExtraConfigAccessor()),
    connector_(nullptr),
    sleepEvents_(sleepEvents),
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
    connect(&timerWaitNetworkConnectivity_, &QTimer::timeout, this, &ConnectionManager::onTimerWaitNetworkConnectivity);

    testVPNTunnel_ = new TestVPNTunnel(this);
    connect(testVPNTunnel_, &TestVPNTunnel::testsFinished, this, &ConnectionManager::onTunnelTestsFinished);

    connect(networkDetectionManager_, &INetworkDetectionManager::onlineStateChanged, this, &ConnectionManager::onNetworkOnlineStateChanged);

    if (!sleepEvents_) {
        sleepEvents_ = CrossPlatformObjectFactory::createSleepEvents(this);
    } else {
        sleepEvents_->setParent(this);
    }
    if (sleepEvents_) {
        connect(sleepEvents_, &ISleepEvents::gotoSleep, this, &ConnectionManager::onSleepMode);
        connect(sleepEvents_, &ISleepEvents::gotoWake, this, &ConnectionManager::onWakeMode);
    }
}

ConnectionManager::~ConnectionManager()
{
    SAFE_DELETE(testVPNTunnel_);
    SAFE_DELETE(connector_);
    SAFE_DELETE(sleepEvents_);
}

void ConnectionManager::clickConnect(const ConnectRequest &req)
{
    WS_ASSERT(state_ == STATE_DISCONNECTED);

    lastRequest_ = req;

    bWasSuccessfullyConnectionAttempt_ = false;

    usernameForCustomOvpn_.clear();
    passwordForCustomOvpn_.clear();

    state_= STATE_CONNECTING_FROM_USER_CLICK;

    // if we had a connector before, get rid of it.  This is because we don't want to receive events from a
    // previous connection if a new connection has started.
    if (connector_) {
        currentProtocol_ = types::Protocol::UNINITIALIZED;
        connector_->teardown();
        SAFE_DELETE(connector_);
    }

    updateConnectionSettingsPolicy(req.connectionSettings, req.portMap, req.proxySettings);

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
            dnsConfigurator_->stopDnsProxy();
            emit disconnected(reason);
        }
    }
}

void ConnectionManager::reconnect()
{
    if (connector_ && isConnectorDialed_) {
        onConnectionReconnecting();
    }
}

void ConnectionManager::blockingDisconnect(bool isSleepEvent)
{
#ifndef Q_OS_WIN
    // Ignoring special sleep event handling on non-Windows for now until we have evidence it is required.
    isSleepEvent = false;
#endif
    if (connector_) {
        if (!connector_->isDisconnected()) {
            testVPNTunnel_->stopTests();
            connector_->blockSignals(true);
            QElapsedTimer elapsedTimer;
            elapsedTimer.start();
            connector_->startDisconnect();
            while (!connector_->isDisconnected()) {
                if (isSleepEvent) {
                    // We do not want to processEvents during disconnect due to the PC going to sleep.  Windows may suspend us
                    // while in this loop and resume us when the PC wakes.  If we process events, we will likely start trying
                    // to restoreConnectionAfterWakeUp while we are in here disconnecting.
                    QThread::msleep(50);
                    connector_->waitForDisconnect();
                } else {
                    QThread::msleep(1);
                    qApp->processEvents();
                }

                // The OS may have suspended us while in this loop during a sleep event and thus the elapsed timer will not be accurate.
                if (!isSleepEvent && elapsedTimer.elapsed() > 10000) {
                    qCWarning(LOG_CONNECTION) << "ConnectionManager::blockingDisconnect() wait for disconnect timed out after 10 seconds";
                    connector_->startDisconnect();
                    break;
                }
            }
            connector_->blockSignals(false);
            connector_->teardown();
            if (connector_->capabilities().needsSystemDnsRestore) {
                dnsConfigurator_->restoreSystemDns();
            }
            platformPolicy_->setGaiIpv4PriorityEnabled(false);
            dnsConfigurator_->stopDnsProxy();

            if (!connSettingsPolicy_.isNull()) {
                connSettingsPolicy_->reset();
            }

            disconnect();
        } else if (state_ != STATE_DISCONNECTED) {
            // A pre-dial attempt (prepare in flight) has no tunnel to wait for, but its wrappers,
            // fetches and ctrld must still be quiesced; queued completions are dropped by the state guards.
            connector_->teardown();
            dnsConfigurator_->stopDnsProxy();
            if (!connSettingsPolicy_.isNull()) {
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

void ConnectionManager::removeIkev2ConnectionFromOS()
{
    connectionFactory_->removeIkev2ConnectionFromOS();
}

void ConnectionManager::continueWithUsernameAndPassword(const QString &username, const QString &password, bool bNeedReconnect)
{
    usernameForCustomOvpn_ = username;
    passwordForCustomOvpn_ = password;

    // On the auth-failed retry path the previous connector has already been
    // torn down by onConnectionDisconnected (SAFE_DELETE_LATER), so connector_
    // is null. doConnect() will create a fresh one that picks up the new
    // credentials via usernameForCustomOvpn_/passwordForCustomOvpn_.
    if (bNeedReconnect) {
        bWasSuccessfullyConnectionAttempt_ = false;
        state_= STATE_CONNECTING_FROM_USER_CLICK;
        doConnect();
        return;
    }

    WS_ASSERT(connector_ != NULL);
    if (connector_) {
        connector_->continueWithUserInput(UsernameResponse{username, password});
    }
}

void ConnectionManager::continueWithPassword(const QString &password, bool /*bNeedReconnect*/)
{
    WS_ASSERT(connector_ != NULL);
    if (connector_) {
        connector_->continueWithUserInput(PasswordResponse{password});
    }
}

void ConnectionManager::continueWithPrivKeyPassword(const QString &password, bool bNeedReconnect)
{
    // On the priv-key retry path connector_ is null (already deleted in
    // onConnectionDisconnected). doConnect() will create a fresh connector;
    // OpenVPN will re-prompt for the priv-key password on that new connection.
    if (bNeedReconnect) {
        bWasSuccessfullyConnectionAttempt_ = false;
        state_= STATE_CONNECTING_FROM_USER_CLICK;
        doConnect();
        return;
    }

    WS_ASSERT(connector_ != NULL);
    if (connector_) {
        connector_->continueWithUserInput(PrivKeyPasswordResponse{password});
    }
}

void ConnectionManager::onConnectionConnected(const AdapterGatewayInfo &connectionAdapterInfo)
{
    if (connector_ == nullptr || static_cast<IConnection *>(sender()) != connector_) {
        qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionConnected(), ignored";
        return;
    }

    qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionConnected(), state_ =" << state_;

    vpnAdapterInfo_ = connectionAdapterInfo;

    qCInfo(LOG_CONNECTION) << "VPN adapter and gateway:" << vpnAdapterInfo_.makeLogString();

    dnsConfigurator_->overrideAdapterDns(vpnAdapterInfo_);

    if (state_ == STATE_DISCONNECTING_FROM_USER_CLICK) {
        qCDebug(LOG_CONNECTION) << "Already disconnecting -- do not enter connected state";
        return;
    }

    timerReconnection_.stop();
    connectingTimer_.stop();
    state_ = STATE_CONNECTED;

    // Prioritize IPv4 in gai.conf only when the tunnel does not carry IPv6 egress, otherwise
    // applications that prefer IPv6 stall waiting for blocked v6 traffic. The tunnel egresses
    // IPv6 only for a dual-stack-capable connector on a node that supports it when the user has
    // not forced IPv4 Only; this mirrors the dual-stack decision in WireGuardConnectionBase.
    const bool ipv6Egress = connector_->capabilities().dualStackEgress
                            && currentConnectionDescr_.isIpv6Support
                            && lastRequest_.wireGuard.ipStackEgress == IpStack::kAuto;
    if (!ipv6Egress) {
        platformPolicy_->setGaiIpv4PriorityEnabled(true);
    }

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
    connector_->teardown();
    // A pre-dial connector changed no DNS/gai state; issuing the restores would undo the
    // PREVIOUS session's cleanup work a second time.
    if (isConnectorDialed_) {
        if (connector_->capabilities().needsSystemDnsRestore) {
            dnsConfigurator_->restoreSystemDns();
        }
        platformPolicy_->setGaiIpv4PriorityEnabled(false);
    }

    // Delete the connector to ensure we do not receive any additional events from it, even events
    // already queued to our event queue.  We cannot use connector_->disconnect() here since we connect
    // to the signals with Qt::QueuedConnection and thus any already queued events will still be delivered.
    SAFE_DELETE_LATER(connector_);
    dnsConfigurator_->stopDnsProxy();
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
            // An endpoint-list policy keeps walking the list when the process dies without a
            // classified error; regular policies treat a bare death as attempt-fatal.
            if (connSettingsPolicy_->shouldRetryOnAttemptFailure() && !checkFails()) {
                state_ = STATE_RECONNECTING;
                emit reconnecting();
                startReconnectionTimer();
                doConnect();
                break;
            }
            disconnect();
            emit disconnected(DISCONNECTED_ITSELF);
            break;
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
            // Same bare-death rule mid-walk: no classified error preceded this (the ignore flag is
            // still clear), so the endpoint must be consumed or it would be redialed forever.
            if (connSettingsPolicy_->shouldRetryOnAttemptFailure() && !bIgnoreConnectionErrorsForOpenVpn_ && checkFails()) {
                disconnect();
                emit disconnected(DISCONNECTED_ITSELF);
                break;
            }
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
    // Null-tolerant: reconnect() and startFailoverReconnect() call this directly (no sender); only
    // a queued signal from a retired connector is dropped.
    QObject *s = sender();
    if (s != nullptr && static_cast<IConnection *>(s) != connector_) {
        qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionReconnecting(), ignored";
        return;
    }

    qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionReconnecting(), state_ =" << state_;

    dnsConfigurator_->stopDnsProxy(); // If we are reconnecting, we need to kill the ctrld process if it exists, to avoid conflicts with the new connection
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
                    wgCachedConfigAttempts_ = 0;
                }
                state_ = STATE_RECONNECTING;
                emit reconnecting();
                startReconnectionTimer();
                if (connector_) {
                    connector_->startDisconnect();
                }
            }
            else
            {
                state_ = STATE_AUTO_DISCONNECT;
                if (connector_) {
                    connector_->startDisconnect();
                }
            }
            break;

        case STATE_CONNECTED:
            WS_ASSERT(!timerReconnection_.isActive());

            if (connector_ && !connector_->isDisconnected())
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
                    wgCachedConfigAttempts_ = 0;
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

void ConnectionManager::clearCachedWgConfigIfExhausted(CONNECT_ERROR err)
{
    // A cached WireGuard config that can't bring up the tunnel after every attempt is invalid (e.g. a
    // stale key). Clear the stored credentials so the next connect falls back to IKEv2 under Always On+
    // instead of retrying the same bad config; a later API-reachable connect will register fresh
    // credentials. Wait until the attempts are exhausted so a transient error doesn't discard a good config.
    if (hasUsableCachedWgConfig_ && wgCachedConfigAttempts_ >= kMaxWgCachedConfigAttempts && err == CONNECT_ERROR::WIREGUARD_CONNECTION_ERROR) {
        qCInfo(LOG_CONNECTION) << "Cached WireGuard config failed to connect; clearing stored credentials";
        GetWireGuardConfig::removeWireGuardSettings();
        hasUsableCachedWgConfig_ = false;
    }
}

void ConnectionManager::onConnectionError(CONNECT_ERROR err)
{
    if (connector_ == nullptr || static_cast<IConnection *>(sender()) != connector_) {
        qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionError(), ignored";
        return;
    }
    if (state_ == STATE_DISCONNECTING_FROM_USER_CLICK || state_ == STATE_RECONNECTION_TIME_EXCEED ||
        state_ == STATE_AUTO_DISCONNECT || state_ == STATE_ERROR_DURING_CONNECTION)
    {
        return;
    }

    qCInfo(LOG_CONNECTION) << "ConnectionManager::onConnectionError(), state_ =" << state_ << ", error =" << (int)err;
    testVPNTunnel_->stopTests();

    clearCachedWgConfigIfExhausted(err);

    // An endpoint-list policy treats local fatal errors as walk-to-the-next-endpoint, not attempt-fatal.
    const bool retryOnFailure = connSettingsPolicy_->shouldRetryOnAttemptFailure();

    if ((err == CONNECT_ERROR::AUTH_ERROR && lastRequest_.bEmitAuthError)
            || (!retryOnFailure && err == CONNECT_ERROR::NO_OPENVPN_SOCKET)
            || (!retryOnFailure && err == CONNECT_ERROR::NO_INSTALLED_TUN_TAP)
            || (!connSettingsPolicy_->isAutomaticMode() && err == CONNECT_ERROR::WIREGUARD_CONNECTION_ERROR)
            || (!connSettingsPolicy_->isAutomaticMode() && err == CONNECT_ERROR::WIREGUARD_ADAPTER_SETUP_FAILED)
            || err == CONNECT_ERROR::TAP_FATAL_ERROR
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
            || (!retryOnFailure && err == CONNECT_ERROR::EXE_SUBPROCESS_FAILED))
    {
        // immediately stop trying to connect
        disconnect();
        emit errorDuringConnection(err);
    }
    else if (err == CONNECT_ERROR::UDP_CANT_ASSIGN
             || err == CONNECT_ERROR::UDP_NO_BUFFER_SPACE
             || err == CONNECT_ERROR::UDP_NETWORK_DOWN
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
             || (err == CONNECT_ERROR::AUTH_ERROR && !lastRequest_.bEmitAuthError)
             || (retryOnFailure && (err == CONNECT_ERROR::EXE_SUBPROCESS_FAILED
                                    || err == CONNECT_ERROR::NO_OPENVPN_SOCKET
                                    || err == CONNECT_ERROR::NO_INSTALLED_TUN_TAP)))
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
                    if (connector_) {
                        connector_->startDisconnect();
                    }
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
                        wgCachedConfigAttempts_ = 0;
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
                        if (connector_) {
                            connector_->startDisconnect();
                        }
                    }
                }
                else
                {
                    state_ = STATE_AUTO_DISCONNECT;
                    if (err != CONNECT_ERROR::AUTH_ERROR)
                    {
                        if (connector_) {
                            connector_->startDisconnect();
                        }
                    }
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
    if (connector_ == nullptr || static_cast<IConnection *>(sender()) != connector_) {
        return;
    }
    emit statisticsUpdated(bytesIn, bytesOut, isTotalBytes);
}

void ConnectionManager::onConnectionInterfaceUpdated(const QString &interfaceName)
{
    if (connector_ == nullptr || static_cast<IConnection *>(sender()) != connector_) {
        return;
    }
    emit interfaceUpdated(interfaceName);
}

void ConnectionManager::onConnectionUserInputRequired(UserInputType type)
{
    if (connector_ == nullptr || static_cast<IConnection *>(sender()) != connector_) {
        qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionUserInputRequired(), ignored";
        return;
    }

    switch (type)
    {
        case UserInputType::Username:
            // Credentials the user already supplied this session (e.g. before an auth-failed retry)
            // answer the connector directly without re-prompting.
            if (!usernameForCustomOvpn_.isEmpty()) {
                connector_->continueWithUserInput(UsernameResponse{usernameForCustomOvpn_, passwordForCustomOvpn_});
            } else {
                emit requestUsername(currentConnectionDescr_.customConfigFilename);
            }
            break;
        case UserInputType::Password:
            if (!passwordForCustomOvpn_.isEmpty()) {
                connector_->continueWithUserInput(PasswordResponse{passwordForCustomOvpn_});
            } else {
                emit requestPassword(currentConnectionDescr_.customConfigFilename);
            }
            break;
        case UserInputType::PrivKeyPassword:
            emit requestPrivKeyPassword(currentConnectionDescr_.customConfigFilename);
            break;
        case UserInputType::KeyLimitConsent:
            // A key-limit answer already queued when the user starts disconnecting must not raise
            // the prompt (same state list as onConnectionPrepared).
            if (state_ != STATE_CONNECTING_FROM_USER_CLICK && state_ != STATE_RECONNECTING &&
                state_ != STATE_WAKEUP_RECONNECTING && state_ != STATE_WAIT_FOR_NETWORK_CONNECTIVITY) {
                qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionUserInputRequired(), key limit ignored in state" << state_;
                break;
            }
            // Do not timeout while waiting for user input.
            connectingTimer_.stop();
            emit wireGuardAtKeyLimit();
            break;
    }
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
            blockingDisconnect(true);
            qCDebug(LOG_CONNECTION) << "ConnectionManager::onSleepMode(), blockingDisconnect completed";
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
    if (!connector_ || !isConnectorDialed_) {
        return;
    }

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

void ConnectionManager::doConnect()
{
    // For automatic policy, we would have removed IKEv2 from the list for lockdown mode.
    // There is no custom config for IKEv2, so if we get here it is manual mode.
    // We can get here either by:
    // - User selecting IKEv2 in manual mode and then enabling Lockdown Mode, or
    // - User selecting IKEv2 in manual mode in a previous version of the app, then updating.
    if (!connSettingsPolicy_->isCustomConfig() && connSettingsPolicy_->getCurrentConnectionSettings().protocol == types::Protocol::IKEV2 && platformPolicy_->isLockdownBlockingIkev2()) {
        emit errorDuringConnection(CONNECT_ERROR::LOCKDOWN_MODE_IKEV2);
        return;
    }

    bool isOnline = networkDetectionManager_->isOnline();
    defaultAdapterInfo_.clear();
    if (isOnline) {
        defaultAdapterInfo_ = platformPolicy_->detectDefaultAdapter();
    }

    if (!isOnline || defaultAdapterInfo_.isEmpty()) {
        if (!connSettingsPolicy_->shouldWaitForNetwork()) {
            qCInfo(LOG_CONNECTION) << "No internet connection and the policy does not wait for connectivity, giving up";
            disconnect();
            emit disconnected(DISCONNECTED_ITSELF);
            return;
        }
        startReconnectionTimer();
        waitForNetworkConnectivity();
        return;
    }

    qCInfo(LOG_CONNECTION) << "Default adapter and gateway:" << defaultAdapterInfo_.makeLogString();
    connectTimer_.stop();

    recreateConnector(connSettingsPolicy_->preResolveProtocol());

    connectingTimer_.setSingleShot(true);
    if (!connSettingsPolicy_->isCustomConfig()) {
        // Both automatic and manual mode should timeout the same.
        connectingTimer_.setInterval(connector_->capabilities().connectTimeoutMs);
        connectingTimer_.start();
    }

    connSettingsPolicy_->resolveHostnames();
}

void ConnectionManager::doConnectPart2()
{
    // A late hostnamesResolved from an already-abandoned attempt (e.g. user disconnected during a
    // custom-config DNS resolve) arrives after the connector was retired; drop it.
    if (connector_ == nullptr) {
        qCDebug(LOG_CONNECTION) << "ConnectionManager::doConnectPart2(), no connector, ignored";
        return;
    }

    bIgnoreConnectionErrorsForOpenVpn_ = false;

    currentConnectionDescr_ = connSettingsPolicy_->getCurrentConnectionSettings();

    if (currentConnectionDescr_.connectionNodeType == CONNECTION_NODE_ERROR)
    {
        qCWarning(LOG_CONNECTION) << "connSettingsPolicy_.getCurrentConnectionSettings returned incorrect value";
        connector_->teardown();
        SAFE_DELETE_LATER(connector_);
        disconnect();
        emit errorDuringConnection(CONNECT_ERROR::LOCATION_NO_ACTIVE_NODES);
        return;
    }

    qCInfo(LOG_CONNECTION) << "Connecting to IP:" << currentConnectionDescr_.ip << " protocol:" << currentConnectionDescr_.protocol.toLongString() << " port:" << currentConnectionDescr_.port;
    if (!lastRequest_.openVpn.amneziawgPreset.isEmpty()) {
        qCInfo(LOG_CONNECTION) << "Using protocol tweaks, preset:" << lastRequest_.openVpn.amneziawgPreset;
    }
    emit protocolPortChanged(currentConnectionDescr_.protocol, currentConnectionDescr_.port);

    if (!dnsConfigurator_->prepare()) {
        connector_->teardown();
        SAFE_DELETE_LATER(connector_);
        disconnect();
        emit errorDuringConnection(CONNECT_ERROR::CTRLD_START_FAILED);
        return;
    }

    if (lastRequest_.proxySettings.isProxyEnabled() && currentConnectionDescr_.protocol != types::Protocol::OPENVPN_TCP) {
        qCWarning(LOG_CONNECTION) << "WARNING: LAN proxy setting ignored because the connection protocol is not TCP.";
    }

    AttemptEnvironment env;
    env.packetSize = packetSize_;
    env.defaultAdapterInfo = defaultAdapterInfo_;
    env.primaryDnsServer = dnsConfigurator_->primaryDnsServer();
    env.extraConfig = extraConfig_.data();

    // Always On+ blocks the API, so a config-fetching connector may only use its cached config
    // (attempts capped, see wgCachedConfigAttempts_). Custom configs never fetch.
    if (isFirewallAlwaysOnPlusEnabled_ && connector_->capabilities().supportsCachedConfig &&
        currentConnectionDescr_.connectionNodeType != CONNECTION_NODE_CUSTOM_CONFIG) {
        if (hasUsableCachedWgConfig_ && wgCachedConfigAttempts_ < kMaxWgCachedConfigAttempts) {
            wgCachedConfigAttempts_++;
            qCInfo(LOG_CONNECTION) << "Using cached WireGuard config under Firewall Always On+ mode for hostname =" << currentConnectionDescr_.hostname << "isIpv6Support = " << currentConnectionDescr_.isIpv6Support;
            env.configFetchMode = ConfigFetchMode::CachedOnly;
        } else if (connSettingsPolicy_->isAutomaticMode()) {
            // Let the policy advance to a protocol that works without the API (IKEv2/OpenVPN).
            qCInfo(LOG_CONNECTION) << "Cached WireGuard config exhausted under Firewall Always On+ mode, advancing to next protocol";
            startFailoverReconnect();
            return;
        } else {
            // Manual mode has no next protocol; surface the failure rather than looping.
            qCInfo(LOG_CONNECTION) << "Cached WireGuard config unavailable or exhausted under Firewall Always On+ mode, aborting connection";
            connector_->teardown();
            SAFE_DELETE_LATER(connector_);
            disconnect();
            emit errorDuringConnection(WIREGUARD_COULD_NOT_RETRIEVE_CONFIG);
            return;
        }
    }

    connector_->prepare(currentConnectionDescr_, env);
}

void ConnectionManager::onConnectionPrepared()
{
    if (connector_ == nullptr || static_cast<IConnection *>(sender()) != connector_) {
        qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionPrepared(), ignored";
        return;
    }
    // WAIT_FOR_NETWORK_CONNECTIVITY is included because the connectivity poll re-enters doConnect()
    // without changing state, so a legitimate attempt can reach the dial in that state.
    if (state_ != STATE_CONNECTING_FROM_USER_CLICK && state_ != STATE_RECONNECTING &&
        state_ != STATE_WAKEUP_RECONNECTING && state_ != STATE_WAIT_FOR_NETWORK_CONNECTIVITY) {
        qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionPrepared(), ignored in state" << state_;
        return;
    }
    // A duplicate resolve/prepare of the same attempt (e.g. overlapping custom-config DNS
    // resolutions) must not dial the already-running connector a second time.
    if (isConnectorDialed_) {
        qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionPrepared(), already dialed, ignored";
        return;
    }

    // A non-null tunnel DNS readback means the connector's config carries its own DNS (WireGuard);
    // the emitted list feeds the firewall whitelist before the dial. The endpoint comes from the
    // effective readbacks: prepare() may have rewritten it (IKEv2 ExtraConfig override) and the
    // whitelist must cover what the connector actually dials.
    const QString tunnelDns = connector_->tunnelDefaultDns();
    if (!tunnelDns.isNull()) {
        emit connectingToHostname(connector_->effectiveHostname(), connector_->effectiveIp(),
                                  dnsConfigurator_->tunnelDnsServers(tunnelDns));
    } else {
        emit connectingToHostname(connector_->effectiveHostname(), connector_->effectiveIp(), QStringList());
    }

    connector_->startConnect();

    isConnectorDialed_ = true;
    lastIp_ = connector_->effectiveIp();
}

void ConnectionManager::onConnectionPrepareFailed(CONNECT_ERROR err)
{
    if (connector_ == nullptr || static_cast<IConnection *>(sender()) != connector_) {
        qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionPrepareFailed(), ignored";
        return;
    }
    // Same state list as onConnectionPrepared: a prepareFailed already queued when e.g. a user
    // disconnect lands must not hard-stop; the queued disconnected() completes that path instead.
    if (state_ != STATE_CONNECTING_FROM_USER_CLICK && state_ != STATE_RECONNECTING &&
        state_ != STATE_WAKEUP_RECONNECTING && state_ != STATE_WAIT_FOR_NETWORK_CONNECTIVITY) {
        qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionPrepareFailed(), ignored in state" << state_;
        return;
    }

    qCInfo(LOG_CONNECTION) << "ConnectionManager::onConnectionPrepareFailed(), state_ =" << state_ << ", error =" << (int)err;

    // Config-acquisition failures route to failover rather than a hard stop: CONFIG_FETCH_FAILED in
    // both modes; API-exhausted (WIREGUARD_COULD_NOT_RETRIEVE_CONFIG) only when automatic mode has a
    // next option to advance to.
    if (err == CONNECT_ERROR::CONFIG_FETCH_FAILED ||
        (err == CONNECT_ERROR::WIREGUARD_COULD_NOT_RETRIEVE_CONFIG && connSettingsPolicy_->isAutomaticMode())) {
        startFailoverReconnect();
        return;
    }

    // A live connector exists when prep fails under early creation; retire it before surfacing the error.
    connector_->teardown();
    SAFE_DELETE_LATER(connector_);
    disconnect();
    emit errorDuringConnection(err);
}

void ConnectionManager::startFailoverReconnect()
{
    state_ = STATE_RECONNECTING;
    emit reconnecting();
    startReconnectionTimer();
    onConnectionReconnecting();
}

bool ConnectionManager::checkFails()
{
    // Return true if the connection policy object indicates there are no more attempts to try.
    // TL;DR no more reconnect attempts should be made.
    connSettingsPolicy_->putFailedConnection();
    return connSettingsPolicy_->isFailed();
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
    if (currentProtocol_ == types::Protocol::UNINITIALIZED) {
        WS_ASSERT(connector_ == NULL);
    }

    if (connector_) {
        // teardown() also cancels a superseded attempt's in-flight config fetch, so its answer can
        // never dial the new connector with the old descr.
        connector_->teardown();
    }
    SAFE_DELETE_LATER(connector_);

    connector_ = connectionFactory_->createConnection(protocol, this, helper_, lastRequest_);
    isConnectorDialed_ = false;

    connect(connector_, &IConnection::prepared, this, &ConnectionManager::onConnectionPrepared, Qt::QueuedConnection);
    connect(connector_, &IConnection::prepareFailed, this, &ConnectionManager::onConnectionPrepareFailed, Qt::QueuedConnection);
    connect(connector_, &IConnection::userInputRequired, this, &ConnectionManager::onConnectionUserInputRequired, Qt::QueuedConnection);
    connect(connector_, &IConnection::connected, this, &ConnectionManager::onConnectionConnected, Qt::QueuedConnection);
    connect(connector_, &IConnection::disconnected, this, &ConnectionManager::onConnectionDisconnected, Qt::QueuedConnection);
    connect(connector_, &IConnection::reconnecting, this, &ConnectionManager::onConnectionReconnecting, Qt::QueuedConnection);
    connect(connector_, &IConnection::error, this, &ConnectionManager::onConnectionError, Qt::QueuedConnection);
    connect(connector_, &IConnection::statisticsUpdated, this, &ConnectionManager::onConnectionStatisticsUpdated, Qt::QueuedConnection);
    connect(connector_, &IConnection::interfaceUpdated, this, &ConnectionManager::onConnectionInterfaceUpdated, Qt::QueuedConnection);

    currentProtocol_ = protocol;
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
    int attempts = extraConfig_->tunnelTestAttempts(hasAttempts);
    bool noError = extraConfig_->isTunnelTestNoError();

    if (bSuccess) {
        // A working tunnel refreshes the cached-WG budget so subsequent reconnects can retry it.
        // Reset here rather than on handshake: a WG handshake can succeed while the tunnel is unusable.
        wgCachedConfigAttempts_ = 0;
    }

    if ((hasAttempts && attempts == 0) || (noError && !bSuccess)) {
        emit testTunnelResult(bSuccess, "");
    } else if (!bSuccess) {
        if (connSettingsPolicy_->isCustomConfig()) {
            emit testTunnelResult(false, "");
        } else {
            bool bCheckFailsResult = checkFails();
            if (!bCheckFailsResult || bWasSuccessfullyConnectionAttempt_) {
                if (bCheckFailsResult) {
                    connSettingsPolicy_->reset();
                    wgCachedConfigAttempts_ = 0;
                }
                state_ = STATE_RECONNECTING;
                emit reconnecting();
                startReconnectionTimer();
                if (connector_) {
                    connector_->startDisconnect();
                }
            } else {
                state_= STATE_AUTO_DISCONNECT;
                if (connector_) {
                    connector_->startDisconnect();
                }
            }
        }
    } else {
        emit testTunnelResult(true, ipAddress);

        if (connSettingsPolicy_->isAutomaticMode()) {
            bWasSuccessfullyConnectionAttempt_ = true;
        }
    }
}

void ConnectionManager::onTimerWaitNetworkConnectivity()
{
    if (networkDetectionManager_->isOnline() && !platformPolicy_->detectDefaultAdapter().isEmpty())
    {
        qCInfo(LOG_CONNECTION) << "We're online, making the connection";
        timerWaitNetworkConnectivity_.stop();
        doConnect();
    }
    else
    {
        if (timerReconnection_.remainingTime() == 0)
        {
            qCInfo(LOG_CONNECTION) << "Timed out waiting for network connectivity";
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
    // The answer can arrive after the attempt ended (the prompt round-trips through the UI); resume
    // or disconnect only while an attempt is still active.
    if (state_ != STATE_CONNECTING_FROM_USER_CLICK && state_ != STATE_RECONNECTING &&
        state_ != STATE_WAKEUP_RECONNECTING && state_ != STATE_WAIT_FOR_NETWORK_CONNECTIVITY) {
        qCDebug(LOG_CONNECTION) << "ConnectionManager::onWireGuardKeyLimitUserResponse(), ignored in state" << state_;
        return;
    }

    if (deleteOldestKey) {
        // The consent resumes the connector's paused fetch.
        if (connector_) {
            connector_->continueWithUserInput(KeyLimitConsentResponse{});
            connectingTimer_.start(connector_->capabilities().connectTimeoutMs);
        }
    } else {
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

    if (connector_ == nullptr || !isConnectorDialed_) {
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
    if (!lastRequest_.bli) {
        // no active connection in progress
        return;
    }

    // Only relevant under Always On+; gate the decrypt so it stays off the common connect path.
    hasUsableCachedWgConfig_ = isFirewallAlwaysOnPlusEnabled_ && GetWireGuardConfig::hasUsableStoredConfig();
    wgCachedConfigAttempts_ = 0;

    connSettingsPolicy_.reset(connSettingsPolicyFactory_->createPolicy(lastRequest_.bli, connectionSettings, portMap,
                                                                       proxySettings, lastRequest_.preferredNodeHostname,
                                                                       isFirewallAlwaysOnPlusEnabled_,
                                                                       hasUsableCachedWgConfig_));
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

void ConnectionManager::setFirewallAlwaysOnPlusEnabled(bool isEnabled)
{
    isFirewallAlwaysOnPlusEnabled_ = isEnabled;
}

void ConnectionManager::onConnectingTimeout()
{
    qCInfo(LOG_CONNECTION) << "Connection timed out";
    // A cached config with stale credentials starts and configures fine but never completes the
    // handshake, so the failure lands here rather than in onConnectionError(). Run the exhaustion
    // check before the reconnect logic below can reset the attempt counter.
    if (currentConnectionDescr_.protocol.isWireGuardProtocol()) {
        clearCachedWgConfigIfExhausted(CONNECT_ERROR::WIREGUARD_CONNECTION_ERROR);
    }
    startFailoverReconnect();
}
