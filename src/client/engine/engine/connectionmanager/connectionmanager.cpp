#include "utils/log/logger.h"

#include <QCoreApplication>
#include <QThread>

#include "attemptstrategy/iconnectionattemptstrategyfactory.h"
#include "classifyconnecterror.h"
#include "engine/connectionmanager/connectors/iconnectionfactory.h"
#include "engine/connectionmanager/connectors/iconnectionplatformpolicy.h"
#include "engine/dns/idnsconfigurator.h"
#include "engine/networkdetectionmanager/inetworkdetectionmanager.h"
#include "isleepevents.h"
#include "utils/extraconfig.h"
#include "types/connectionsettings.h"
#include "types/enums.h"
#include "utils/utils.h"
#include "utils/ws_assert.h"

// Had to move this here to prevent a compile error with boost already including winsock.h
#include "connectionmanager.h"

using CachedConfigAdvice = IConnectionAttemptStrategy::CachedConfigAdvice;
using FailureAdvice = IConnectionAttemptStrategy::FailureAdvice;

ConnectionManager::ConnectionManager(QObject *parent, INetworkDetectionManager *networkDetectionManager,
                                     IDnsConfigurator *dnsConfigurator,
                                     IConnectionFactory *connectionFactory, IConnectionPlatformPolicy *platformPolicy,
                                     IConnectionAttemptStrategyFactory *attemptStrategyFactory, ISleepEvents *sleepEvents) : QObject(parent),
    networkDetectionManager_(networkDetectionManager),
    dnsConfigurator_(dnsConfigurator),
    connectionFactory_(connectionFactory),
    platformPolicy_(platformPolicy),
    attemptStrategyFactory_(attemptStrategyFactory),
    connector_(nullptr),
    sleepEvents_(sleepEvents),
    bIgnoreConnectionErrors_(false),
    state_(State::kDisconnected),
    bLastIsOnline_(true),
    bWakeSignalReceived_(false),
    currentConnectionDescr_()
{
    connect(&timerReconnection_, &QTimer::timeout, this, &ConnectionManager::onTimerReconnection);
    connect(&connectTimer_, &QTimer::timeout, this, &ConnectionManager::onConnectTrigger);
    connect(&connectingTimer_, &QTimer::timeout, this, &ConnectionManager::onConnectingTimeout);
    connect(&timerWaitNetworkConnectivity_, &QTimer::timeout, this, &ConnectionManager::onTimerWaitNetworkConnectivity);

    connect(networkDetectionManager_, &INetworkDetectionManager::onlineStateChanged, this, &ConnectionManager::onNetworkOnlineStateChanged);

    if (sleepEvents_) {
        sleepEvents_->setParent(this);
        connect(sleepEvents_, &ISleepEvents::gotoSleep, this, &ConnectionManager::onSleepMode);
        connect(sleepEvents_, &ISleepEvents::gotoWake, this, &ConnectionManager::onWakeMode);
    }
}

ConnectionManager::~ConnectionManager()
{
    SAFE_DELETE(connector_);
    SAFE_DELETE(sleepEvents_);
}

void ConnectionManager::clickConnect(const ConnectRequest &req)
{
    WS_ASSERT(state_ == State::kDisconnected);

    lastRequest_ = req;

    usernameForCustomOvpn_.clear();
    passwordForCustomOvpn_.clear();
    bAttemptSucceeded_ = false;

    setState(State::kConnecting);

    // if we had a connector before, get rid of it.  This is because we don't want to receive events from a
    // previous connection if a new connection has started.
    if (connector_) {
        currentProtocol_ = types::Protocol::UNINITIALIZED;
        connector_->teardown();
        SAFE_DELETE_LATER(connector_);
    }

    recreateAttemptStrategy(req.connectionSettings, req.portMap, req.proxySettings);

    attemptStrategy_->debugLocationInfoToLog();
    startAttempt();
}

void ConnectionManager::clickDisconnect(DISCONNECT_REASON reason)
{
    WS_ASSERT(state_ != State::kSleeping);

    if (state_ != State::kStopping || !pendingOutcome_.isUserInitiated) {
        qCDebug(LOG_CONNECTION) << "ConnectionManager::clickDisconnect()";
        if (connector_) {
            stopWithDisconnect(reason, true);
            connector_->startDisconnect();
        } else {
            disconnect();
            if (!attemptStrategy_.isNull()) {
                attemptStrategy_->reset();
            }
            dnsConfigurator_->stopDnsProxy();
            emit disconnected(reason);
        }
    }
}

void ConnectionManager::reconnect()
{
    if (connector_ && isConnectorDialed_) {
        handleReconnecting();
    }
}

void ConnectionManager::blockingDisconnect(bool isSleepEvent)
{
    if (!platformPolicy_->needsSleepEventAwareDisconnect()) {
        isSleepEvent = false;
    }
    if (connector_ && !connector_->isDisconnected()) {
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

        if (!attemptStrategy_.isNull()) {
            attemptStrategy_->reset();
        }

        disconnect();
    } else if (state_ != State::kDisconnected) {
        // No tunnel to wait for -- a pre-dial attempt whose connector already settled, or a
        // between-attempts rest/network wait with no connector -- but wrappers, fetches, ctrld and
        // the reconnection/rest timers must still be quiesced (disconnect() stops them), or the rest
        // timer would fire after the caller expected a full stop and redial. Queued completions are
        // dropped by the state guards.
        if (connector_) {
            connector_->teardown();
        }
        dnsConfigurator_->stopDnsProxy();
        if (!attemptStrategy_.isNull()) {
            attemptStrategy_->reset();
        }
        disconnect();
    }
}

bool ConnectionManager::isDisconnected()
{
    if (state_ == State::kDisconnected) {
        if (connector_) {
            WS_ASSERT(connector_->isDisconnected());
        }
    }
    return state_ == State::kDisconnected;
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
    WS_ASSERT(state_ == State::kConnected); // make sense only in connected state
    return vpnAdapterInfo_;
}

void ConnectionManager::removeIkev2ConnectionFromOS()
{
    connectionFactory_->removeIkev2ConnectionFromOS();
}

void ConnectionManager::removeStoredConfig()
{
    connectionFactory_->removeStoredConfig();
}

// On the reconnect path the failed connector was already retired by onConnectionDisconnected, so
// startAttempt() creates a fresh one whose prompts are answered from the cached credentials
// (usernameForCustomOvpn_/passwordForCustomOvpn_) or re-prompted (priv-key password).
void ConnectionManager::forwardUserInputOrReconnect(const UserInputResponse &response, bool bNeedReconnect)
{
    if (bNeedReconnect) {
        bAttemptSucceeded_ = false;
        setState(State::kConnecting);
        startAttempt();
        return;
    }

    WS_ASSERT(connector_ != NULL);
    if (connector_) {
        connector_->continueWithUserInput(response);
    }
}

void ConnectionManager::continueWithUsernameAndPassword(const QString &username, const QString &password, bool bNeedReconnect)
{
    usernameForCustomOvpn_ = username;
    passwordForCustomOvpn_ = password;
    forwardUserInputOrReconnect(UsernameResponse{username, password}, bNeedReconnect);
}

void ConnectionManager::continueWithPassword(const QString &password)
{
    forwardUserInputOrReconnect(PasswordResponse{password}, false);
}

void ConnectionManager::continueWithPrivKeyPassword(const QString &password, bool bNeedReconnect)
{
    forwardUserInputOrReconnect(PrivKeyPasswordResponse{password}, bNeedReconnect);
}

void ConnectionManager::onConnectionConnected(const AdapterGatewayInfo &connectionAdapterInfo)
{
    if (!isSenderCurrentConnector()) {
        qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionConnected(), ignored";
        return;
    }

    qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionConnected(), state_ =" << state_;

    // A queued connected() can land after the attempt settled: behind a stop (whose pending outcome
    // must not be discarded by a resurrect), behind sleep's blockingDisconnect (the connector object
    // stays alive), or while parked offline (our own startDisconnect() completes that recovery).
    if (state_ != State::kConnecting && state_ != State::kReconnecting && state_ != State::kConnected) {
        qCDebug(LOG_CONNECTION) << "Attempt already settled -- do not enter connected state";
        return;
    }

    vpnAdapterInfo_ = connectionAdapterInfo;

    qCInfo(LOG_CONNECTION) << "VPN adapter and gateway:" << vpnAdapterInfo_.makeLogString();

    dnsConfigurator_->overrideAdapterDns(vpnAdapterInfo_);

    timerReconnection_.stop();
    connectingTimer_.stop();
    setState(State::kConnected);

    applyGaiIpv4Priority();

    emit connected();
}

// Prioritize IPv4 in gai.conf only when the tunnel does not carry IPv6 egress, otherwise
// applications that prefer IPv6 stall waiting for blocked v6 traffic.
void ConnectionManager::applyGaiIpv4Priority()
{
    if (!connector_->isDualStackEgress()) {
        platformPolicy_->setGaiIpv4PriorityEnabled(true);
    }
}

void ConnectionManager::onConnectionDisconnected()
{
    if (!isSenderCurrentConnector()) {
        // This event came from a connector that we have retired, and we already have a new connection
        // underway. Ignore the event
        qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionDisconnected(), ignored";
        return;
    }

    qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionDisconnected(), state_ =" << state_;

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

    switch (state_) {
        case State::kStopping:
            disconnect();
            if (pendingOutcome_.type == PendingOutcome::Type::Error) {
                emit errorDuringConnection(pendingOutcome_.error, currentConnectionDescr_);
            } else {
                if (pendingOutcome_.isUserInitiated) {
                    attemptStrategy_->reset();
                }
                emit disconnected(pendingOutcome_.reason);
            }
            break;
        case State::kConnected:
            // goto reconnection state, start reconnection timer and do connection again
            WS_ASSERT(!timerReconnection_.isActive());
            enterReconnecting();
            startAttempt();
            break;
        case State::kConnecting:
            // An endpoint-list strategy keeps walking the list when the process dies without a
            // classified error; regular strategies treat a bare death as attempt-fatal.
            if (attemptStrategy_->shouldRetryOnAttemptFailure() && attemptStrategy_->recordFailureAndAdvance(bAttemptSucceeded_) == FailureAdvice::Retry) {
                enterReconnecting();
                startAttempt();
                break;
            }
            disconnect();
            emit disconnected(DISCONNECTED_ITSELF);
            break;
        case State::kWaitingForNetwork:
            // With the connector gone, the online-state slot can no longer drive the recovery (it
            // requires a dialed connector); poll for connectivity or nothing ever redials.
            // The wake path can enter this state with the reconnection cap stopped, and the poll's
            // give-up check never fires on an inactive cap — arm it or the poll runs forever.
            startReconnectionTimer();
            waitForNetworkConnectivity();
            break;
        case State::kDisconnected:
        case State::kSleeping:
            // nothing todo
            break;

        case State::kReconnecting:
            // A wake-initiated reconnect hasn't failed at anything yet: fresh reconnection budget,
            // no endpoint consumed, redial immediately as a regular reconnect.
            if (reconnectCause_ == ReconnectCause::Wake) {
                setState(State::kReconnecting, ReconnectCause::Regular);
                timerReconnection_.start(MAX_RECONNECTION_TIME);
                startAttempt();
                break;
            }
            // Same bare-death rule mid-walk: no classified error preceded this (the ignore flag is
            // still clear), so the endpoint must be consumed or it would be redialed forever.
            if (attemptStrategy_->shouldRetryOnAttemptFailure() && !bIgnoreConnectionErrors_ &&
                attemptStrategy_->recordFailureAndAdvance(bAttemptSucceeded_) == FailureAdvice::GiveUp) {
                disconnect();
                emit disconnected(DISCONNECTED_ITSELF);
                break;
            }
            connectOrStartConnectTimer();
            break;
    }
}

void ConnectionManager::onConnectionReconnecting()
{
    if (!isSenderCurrentConnector()) {
        qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionReconnecting(), ignored";
        return;
    }
    handleReconnecting();
}

void ConnectionManager::handleReconnecting()
{
    qCDebug(LOG_CONNECTION) << "ConnectionManager::handleReconnecting(), state_ =" << state_;

    dnsConfigurator_->stopDnsProxy(); // If we are reconnecting, we need to kill the ctrld process if it exists, to avoid conflicts with the new connection

    // bIgnoreConnectionErrors_ prevents handling multiple error messages from the connector
    if (bIgnoreConnectionErrors_) {
        return;
    } else {
        bIgnoreConnectionErrors_ = true;
    }

    switch (state_) {
        case State::kConnecting:
        case State::kReconnecting:
            // A wake-initiated attempt's connector-level reconnect isn't a failure; the connector
            // sorts itself out or ends in disconnected()/error().
            if (reconnectCause_ != ReconnectCause::Wake) {
                handleAttemptFailed(true);
            }
            break;

        case State::kConnected:
            WS_ASSERT(!timerReconnection_.isActive());

            if (connector_ && !connector_->isDisconnected()) {
                enterReconnecting();
                connector_->startDisconnect();
            }
            break;
        case State::kDisconnected:
        case State::kStopping:
        case State::kWaitingForNetwork:
        case State::kSleeping:
            break;
    }
}

void ConnectionManager::clearCachedConfigIfExhausted(bool isCachedConfigFailure)
{
    // A cached WireGuard config that can't bring up the tunnel after every attempt is invalid (e.g. a
    // stale key). Clear the stored credentials so the next connect falls back to IKEv2 under Always On+
    // instead of retrying the same bad config; a later API-reachable connect will register fresh
    // credentials. Wait until the attempts are exhausted so a transient error doesn't discard a good config.
    // Only a config-fetching connector's failure says anything about the cached config: a generic
    // tunnel-establishment failure from another protocol must not wipe the stored WG credentials.
    if (!connector_ || !connector_->capabilities().supportsCachedConfig) {
        return;
    }
    if (isCachedConfigFailure && attemptStrategy_->isCachedConfigExhausted()) {
        qCInfo(LOG_CONNECTION) << "Cached WireGuard config failed to connect; clearing stored credentials";
        connectionFactory_->removeStoredConfig();
        attemptStrategy_->setCachedConfigAvailability(false);
    }
}

void ConnectionManager::onConnectionError(ConnectError err)
{
    if (!isSenderCurrentConnector()) {
        qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionError(), ignored";
        return;
    }
    // kSleeping/kDisconnected: blockingDisconnect keeps the connector object alive and pumps no
    // events on the sleep path, so a queued error can land after the teardown settled; acting on it
    // would redial from a settled state.
    if (state_ == State::kStopping || state_ == State::kSleeping || state_ == State::kDisconnected) {
        return;
    }

    qCInfo(LOG_CONNECTION) << "ConnectionManager::onConnectionError(), state_ =" << state_ << ", error =" << (int)err;

    clearCachedConfigIfExhausted(err == ConnectError::kTunnelEstablishmentFailure);

    switch (classifyConnectError(err, attemptStrategy_->isAutomaticMode(), lastRequest_.bEmitAuthError,
                                 attemptStrategy_->shouldRetryOnAttemptFailure())) {
        case ConnectErrorClassification::ErrorAfterDisconnect:
            // emit error in disconnected event
            stopWithError(err);
            // for ConnectError::kAuthFailure signal disconnected will be emitted automatically; other
            // emitters can be settled already and never emit it, wedging kStopping — force the teardown
            // (startDisconnect() on a settled connector emits disconnected() immediately).
            if (err != ConnectError::kAuthFailure && connector_) {
                connector_->startDisconnect();
            }
            break;
        case ConnectErrorClassification::ErrorImmediately:
            // immediately stop trying to connect
            disconnect();
            emit errorDuringConnection(err, currentConnectionDescr_);
            break;
        case ConnectErrorClassification::Retry:
            // bIgnoreConnectionErrors_ prevents handling multiple error messages from the connector
            if (!bIgnoreConnectionErrors_) {
                bIgnoreConnectionErrors_ = true;

                if (state_ == State::kConnected) {
                    // A drop out of an established session isn't an attempt failure: reconnect
                    // without consuming a strategy endpoint (unlike handleAttemptFailed).
                    enterReconnecting();
                    // for ConnectError::kAuthFailure signal disconnected will be emitted automatically
                    if (err != ConnectError::kAuthFailure && connector_) {
                        connector_->startDisconnect();
                    }
                } else {
                    // for ConnectError::kAuthFailure signal disconnected will be emitted automatically
                    handleAttemptFailed(err != ConnectError::kAuthFailure);
                }
            }
            break;
        case ConnectErrorClassification::Unknown:
            qCWarning(LOG_CONNECTION) << "Unknown error from openvpn: " << static_cast<int>(err);
            break;
    }
}

void ConnectionManager::onConnectionStatisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes)
{
    if (!isSenderCurrentConnector()) {
        return;
    }
    // Same late-delivery window as onConnectionError(): don't feed Engine stale data for a dead tunnel.
    if (state_ == State::kStopping || state_ == State::kSleeping || state_ == State::kDisconnected) {
        return;
    }
    emit statisticsUpdated(bytesIn, bytesOut, isTotalBytes);
}

void ConnectionManager::onConnectionInterfaceUpdated(const QString &interfaceName)
{
    if (!isSenderCurrentConnector()) {
        return;
    }
    // Same late-delivery window as onConnectionError(): don't feed Engine stale data for a dead tunnel.
    if (state_ == State::kStopping || state_ == State::kSleeping || state_ == State::kDisconnected) {
        return;
    }
    emit interfaceUpdated(interfaceName);
}

void ConnectionManager::onConnectionUserInputRequired(UserInputType type)
{
    if (!isSenderCurrentConnector()) {
        qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionUserInputRequired(), ignored";
        return;
    }
    // Same late-delivery window as onConnectionError(): a prompt queued behind a stop or sleep's
    // blockingDisconnect must not raise UI for a settled attempt.
    if (state_ == State::kStopping || state_ == State::kSleeping || state_ == State::kDisconnected) {
        qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionUserInputRequired(), ignored in state" << state_;
        return;
    }

    switch (type) {
        case UserInputType::Username:
            // Credentials the user already supplied this session (e.g. before an auth-failed retry)
            // answer the connector directly without re-prompting.
            if (!usernameForCustomOvpn_.isEmpty()) {
                connector_->continueWithUserInput(UsernameResponse{usernameForCustomOvpn_, passwordForCustomOvpn_});
            } else {
                emit requestUsername();
            }
            break;
        case UserInputType::Password:
            if (!passwordForCustomOvpn_.isEmpty()) {
                connector_->continueWithUserInput(PasswordResponse{passwordForCustomOvpn_});
            } else {
                emit requestPassword();
            }
            break;
        case UserInputType::PrivKeyPassword:
            emit requestPrivKeyPassword();
            break;
        case UserInputType::KeyLimitConsent:
            // A key-limit answer already queued when the user starts disconnecting must not raise
            // the prompt.
            if (!isAttemptActive()) {
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
    connectingTimer_.stop();
    // A running connectivity poll would start an attempt mid-sleep when the network returns;
    // restoreConnectionAfterWakeUp re-arms it if the wake still has no network.
    timerWaitNetworkConnectivity_.stop();
    bWakeSignalReceived_ = false;

    switch (state_) {
        case State::kDisconnected:
        case State::kStopping:
        case State::kSleeping:
            break;
        case State::kConnecting:
        case State::kConnected:
        case State::kReconnecting:
            emit reconnecting();
            blockingDisconnect(true);
            qCDebug(LOG_CONNECTION) << "ConnectionManager::onSleepMode(), blockingDisconnect completed";
            setState(State::kSleeping);
            if (bWakeSignalReceived_) {
                // If we are already awake (got the wake event during waiting in the blocking
                // disconnect loop), reconnect immediately.
                restoreConnectionAfterWakeUp();
                bWakeSignalReceived_ = false;
            }
            break;
        case State::kWaitingForNetwork:
            setState(State::kSleeping);
            break;
    }
}

void ConnectionManager::onWakeMode()
{
    qCInfo(LOG_CONNECTION) << "ConnectionManager::onWakeMode(), state_ =" << state_;
    timerReconnection_.stop();
    connectTimer_.stop();
    bWakeSignalReceived_ = true;

    // We should not be in some of these states after wake up, but in some weird cases on Mac it is
    // possible for the network to keep changing after onSleep() which may trigger some state
    // changes.  Regardless of the state, we should restore connection here -- except a teardown in
    // flight, whose pending outcome (a fatal error or the user's own disconnect) must be left to
    // settle rather than papered over with a reconnect.
    if (state_ != State::kDisconnected && state_ != State::kStopping) {
        restoreConnectionAfterWakeUp();
    }
}

void ConnectionManager::onNetworkOnlineStateChanged(bool isAlive)
{
    qCInfo(LOG_CONNECTION) << "ConnectionManager::onNetworkOnlineStateChanged(), isAlive =" << isAlive << ", state_ =" << state_;

    emit internetConnectivityChanged(isAlive);

    if (!platformPolicy_->shouldReconnectOnOnlineStateChange()) {
        return;
    }

    bLastIsOnline_ = isAlive;
    if (!connector_ || !isConnectorDialed_) {
        return;
    }

    switch (state_) {
        case State::kDisconnected:
        case State::kStopping:
        case State::kSleeping:
            //nothing todo
            break;
        case State::kConnecting:
            if (!isAlive) {
                setState(State::kWaitingForNetwork);
                WS_ASSERT(!timerReconnection_.isActive());
                timerReconnection_.start(MAX_RECONNECTION_TIME);
                connector_->startDisconnect();
            }
            break;
        case State::kConnected:
            emit reconnecting();
            setState(isAlive ? State::kReconnecting : State::kWaitingForNetwork);
            WS_ASSERT(!timerReconnection_.isActive());
            timerReconnection_.start(MAX_RECONNECTION_TIME);
            connector_->startDisconnect();
            break;
        case State::kReconnecting:
            // A wake-initiated reconnect already has its own disconnect/redial in flight.
            if (!isAlive && reconnectCause_ != ReconnectCause::Wake) {
                setState(State::kWaitingForNetwork);
                connector_->startDisconnect();
            }
            break;
        case State::kWaitingForNetwork:
            if (isAlive) {
                setState(State::kReconnecting);
                startReconnectionTimer();
                connector_->startDisconnect();
            }
            break;
    }
}

void ConnectionManager::onTimerReconnection()
{
    timerReconnection_.stop();
    // A stop already in flight carries its own outcome; overwriting it here would emit the wrong
    // disconnect reason and skip the user-stop strategy reset.
    if (state_ == State::kStopping) {
        return;
    }
    qCInfo(LOG_CONNECTION) << "Time for reconnection exceed";
    if (connector_) {
        stopWithDisconnect(DISCONNECTED_BY_RECONNECTION_TIMEOUT_EXCEEDED, false);
        connector_->startDisconnect();
    } else {
        disconnect();
        emit disconnected(DISCONNECTED_BY_RECONNECTION_TIMEOUT_EXCEEDED);
    }
}

void ConnectionManager::startAttempt()
{
    // For automatic strategy, we would have removed IKEv2 from the list for lockdown mode.
    // There is no custom config for IKEv2, so if we get here it is manual mode.
    // We can get here either by:
    // - User selecting IKEv2 in manual mode and then enabling Lockdown Mode, or
    // - User selecting IKEv2 in manual mode in a previous version of the app, then updating.
    // The protocol pre-check guards the OS read: isLockdownMode() forks a blocking `defaults read`
    // on macOS (once per process — the policy caches it), so only IKEv2 attempts pay for it.
    const types::Protocol protocol = attemptStrategy_->preResolveProtocol();
    if (protocol.isIkev2Protocol() && lockdownBlocksProtocol(platformPolicy_->isLockdownMode(), protocol)) {
        // failWithError settles the state machine (state, timers, connectionEnded); emitting the
        // error alone leaves it parked in Connecting/Reconnecting with the reconnection cap running.
        failWithError(ConnectError::kBlockedByOsPolicy);
        return;
    }

    // An automatic walk can be empty when every protocol was filtered out (proxy, lockdown, Always
    // On+ without a cached config); the factory cannot build a connector for it.
    if (protocol == types::Protocol::UNINITIALIZED) {
        failWithError(ConnectError::kLocationUnavailable);
        return;
    }

    bool isOnline = networkDetectionManager_->isOnline();
    defaultAdapterInfo_.clear();
    if (isOnline) {
        defaultAdapterInfo_ = platformPolicy_->detectDefaultAdapter();
    }

    if (!isOnline || defaultAdapterInfo_.isEmpty()) {
        if (!attemptStrategy_->shouldWaitForNetwork()) {
            qCInfo(LOG_CONNECTION) << "No internet connection and the strategy does not wait for connectivity, giving up";
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

    recreateConnector(protocol);

    connectingTimer_.setSingleShot(true);
    if (attemptStrategy_->usesConnectTimeout()) {
        // Both automatic and manual mode should timeout the same.
        connectingTimer_.setInterval(connector_->capabilities().connectTimeoutMs);
        connectingTimer_.start();
    }

    attemptStrategy_->resolveHostnames();
}

void ConnectionManager::onHostnamesResolved()
{
    // A late hostnamesResolved from an already-abandoned attempt (e.g. user disconnected during a
    // custom-config DNS resolve) arrives after the connector was retired; drop it.
    if (connector_ == nullptr) {
        qCDebug(LOG_CONNECTION) << "ConnectionManager::onHostnamesResolved(), no connector, ignored";
        return;
    }
    // A resolve already queued when a stop lands (connector alive, disconnected() still queued) must
    // not run attempt setup mid-teardown; the queued disconnected() completes the stop.
    if (!isAttemptActive()) {
        qCDebug(LOG_CONNECTION) << "ConnectionManager::onHostnamesResolved(), ignored in state" << state_;
        return;
    }

    bIgnoreConnectionErrors_ = false;

    currentConnectionDescr_ = attemptStrategy_->getCurrentConnectionSettings();

    if (currentConnectionDescr_.connectionNodeType == CONNECTION_NODE_ERROR) {
        qCWarning(LOG_CONNECTION) << "attemptStrategy_.getCurrentConnectionSettings returned incorrect value";
        failWithError(ConnectError::kLocationUnavailable);
        return;
    }

    qCInfo(LOG_CONNECTION) << "Connecting to IP:" << currentConnectionDescr_.ip << " protocol:" << currentConnectionDescr_.protocol.toLongString() << " port:" << currentConnectionDescr_.port;
    if (!lastRequest_.openVpn.amneziawgPreset.isEmpty()) {
        qCInfo(LOG_CONNECTION) << "Using protocol tweaks, preset:" << lastRequest_.openVpn.amneziawgPreset;
    }
    emit protocolPortChanged(currentConnectionDescr_.protocol, currentConnectionDescr_.port);

    if (!dnsConfigurator_->prepare()) {
        failWithError(ConnectError::kDnsServiceStartFailure);
        return;
    }

    if (lastRequest_.proxySettings.isProxyEnabled() && currentConnectionDescr_.protocol != types::Protocol::OPENVPN_TCP) {
        qCWarning(LOG_CONNECTION) << "WARNING: LAN proxy setting ignored because the connection protocol is not TCP.";
    }

    AttemptEnvironment env;
    env.packetSize = packetSize_;
    env.defaultAdapterInfo = defaultAdapterInfo_;
    env.primaryDnsServer = dnsConfigurator_->primaryDnsServer();
    env.ipStackEgress = lastRequest_.ipStackEgress;

    // The strategy owns the cached-config budget; CM contributes its own facts (Always On+ read
    // live so a mid-session mode change takes effect on the next attempt, a config-fetching
    // connector, not a custom config — custom configs never fetch).
    if (connector_->capabilities().supportsCachedConfig &&
        currentConnectionDescr_.connectionNodeType != CONNECTION_NODE_CUSTOM_CONFIG) {
        if (isFirewallAlwaysOnPlusEnabled_) {
            switch (attemptStrategy_->takeCachedConfigAdvice()) {
                case CachedConfigAdvice::UseCachedOnly:
                    qCInfo(LOG_CONNECTION) << "Using cached WireGuard config under Firewall Always On+ mode for hostname =" << currentConnectionDescr_.hostname << "isIpv6Support = " << currentConnectionDescr_.isIpv6Support;
                    env.configFetchMode = ConfigFetchMode::CachedOnly;
                    break;
                case CachedConfigAdvice::Advance:
                    qCInfo(LOG_CONNECTION) << "Cached WireGuard config exhausted under Firewall Always On+ mode, advancing to next protocol";
                    startFailoverReconnect();
                    return;
                case CachedConfigAdvice::Abort:
                    qCInfo(LOG_CONNECTION) << "Cached WireGuard config unavailable or exhausted under Firewall Always On+ mode, aborting connection";
                    failWithError(ConnectError::kConfigFetchFailure);
                    return;
            }
        } else {
            // The budget counts consecutive cached attempts only. A fresh API fetch supersedes that
            // history; without the reset, a stale exhausted counter would let this attempt's connect
            // timeout wipe the config the fetch just stored.
            attemptStrategy_->resetCachedConfigBudget();
        }
    }

    connector_->prepare(currentConnectionDescr_, env);
}

void ConnectionManager::onConnectionPrepared()
{
    if (!isSenderCurrentConnector()) {
        qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionPrepared(), ignored";
        return;
    }
    if (!isAttemptActive()) {
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

void ConnectionManager::onConnectionPrepareFailed(ConnectError err)
{
    if (!isSenderCurrentConnector()) {
        qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionPrepareFailed(), ignored";
        return;
    }
    // A prepareFailed already queued when e.g. a user disconnect lands must not hard-stop; the
    // queued disconnected() completes that path instead.
    if (!isAttemptActive()) {
        qCDebug(LOG_CONNECTION) << "ConnectionManager::onConnectionPrepareFailed(), ignored in state" << state_;
        return;
    }

    qCInfo(LOG_CONNECTION) << "ConnectionManager::onConnectionPrepareFailed(), state_ =" << state_ << ", error =" << (int)err;

    if (classifyPrepareError(err, attemptStrategy_->isAutomaticMode()) == PrepareErrorRouting::Failover) {
        startFailoverReconnect();
        return;
    }

    // A live connector exists when prep fails under early creation; retire it before surfacing the error.
    failWithError(err);
}

void ConnectionManager::startFailoverReconnect()
{
    enterReconnecting();
    handleReconnecting();
}

void ConnectionManager::handleAttemptFailed(bool bDisconnectConnector)
{
    switch (attemptStrategy_->recordFailureAndAdvance(bAttemptSucceeded_)) {
        case FailureAdvice::Retry:
            // A wake-initiated reconnect recording its first failure becomes a regular one: it
            // (re)enters Reconnecting so the announcement and the reconnection cap start now.
            if (state_ != State::kReconnecting || reconnectCause_ == ReconnectCause::Wake) {
                enterReconnecting();
            }
            if (bDisconnectConnector) {
                if (connector_) {
                    connector_->startDisconnect();
                } else {
                    connectOrStartConnectTimer();
                }
            }
            break;
        case FailureAdvice::GiveUp:
            stopWithDisconnect(DISCONNECTED_ITSELF, false);
            if (bDisconnectConnector && connector_) {
                connector_->startDisconnect();
            }
            break;
    }
}

// Unified entry into Reconnecting: transition, announce, and arm the reconnection cap (guarded — an
// already-running cap keeps its deadline).
void ConnectionManager::enterReconnecting()
{
    setState(State::kReconnecting);
    emit reconnecting();
    startReconnectionTimer();
}

void ConnectionManager::startReconnectionTimer()
{
    if (!timerReconnection_.isActive()) {
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

    connector_ = connectionFactory_->createConnection(protocol, this, lastRequest_);
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
    if (bLastIsOnline_) {
        qCInfo(LOG_CONNECTION) <<
            "ConnectionManager::restoreConnectionAfterWakeUp(), reconnecting";
        setState(State::kReconnecting, ReconnectCause::Wake);
        if (connector_) {
            connector_->startDisconnect();
        } else {
            emit reconnecting();
            startAttempt();
        }
    } else {
        qCInfo(LOG_CONNECTION) <<
            "ConnectionManager::restoreConnectionAfterWakeUp(), waiting for network connectivity";
        setState(State::kWaitingForNetwork);
        // Without a dialed connector (sleep's blockingDisconnect tears down but keeps the object),
        // the online-state slot can't drive the recovery; poll under the reconnection cap or nothing
        // ever redials.
        if (!connector_ || !isConnectorDialed_) {
            startReconnectionTimer();
            waitForNetworkConnectivity();
        }
    }
}

void ConnectionManager::onTunnelTestResult(bool bSuccess, const QString &ipAddress)
{
    // A result racing in behind a drop/disconnect must not drive failover or bookkeeping; the tester
    // (Engine-owned) is stopped on those transitions, but the stop and this call can cross.
    if (state_ != State::kConnected) {
        return;
    }

    bool hasAttempts = false;
    int attempts = ExtraConfig::instance().getTunnelTestAttempts(hasAttempts);
    bool noError = ExtraConfig::instance().getIsTunnelTestNoError();

    if (bSuccess) {
        // A working tunnel refreshes the cached-config budget so subsequent reconnects can retry it.
        // Reset here rather than on handshake: a WG handshake can succeed while the tunnel is unusable.
        attemptStrategy_->resetCachedConfigBudget();
    }

    if ((hasAttempts && attempts == 0) || (noError && !bSuccess)) {
        emit testTunnelResult(bSuccess, "");
    } else if (!bSuccess) {
        if (attemptStrategy_->surfacesTunnelTestFailure()) {
            emit testTunnelResult(false, "");
        } else {
            handleAttemptFailed(true);
        }
    } else {
        emit testTunnelResult(true, ipAddress);

        // Only automatic mode restarts an exhausted walk on the strength of an earlier success.
        if (attemptStrategy_->isAutomaticMode()) {
            bAttemptSucceeded_ = true;
        }
    }
}

void ConnectionManager::onTimerWaitNetworkConnectivity()
{
    if (networkDetectionManager_->isOnline() && !platformPolicy_->detectDefaultAdapter().isEmpty()) {
        qCInfo(LOG_CONNECTION) << "We're online, making the connection";
        timerWaitNetworkConnectivity_.stop();
        // Leave the park before redialing: onConnectionConnected() drops connected() in
        // kWaitingForNetwork, so a redial left parked would discard its own success.
        if (state_ == State::kWaitingForNetwork) {
            setState(State::kReconnecting);
        }
        startAttempt();
    } else {
        if (timerReconnection_.remainingTime() == 0) {
            qCInfo(LOG_CONNECTION) << "Timed out waiting for network connectivity";
            timerWaitNetworkConnectivity_.stop();
            disconnect();
            emit disconnected(DISCONNECTED_BY_RECONNECTION_TIMEOUT_EXCEEDED);
        }
    }
}

bool ConnectionManager::isStaticIpsLocation() const
{
    return currentConnectionDescr_.connectionNodeType == CONNECTION_NODE_STATIC_IPS;
}

api_responses::StaticIpPortsVector ConnectionManager::getStatisIps()
{
    WS_ASSERT(isStaticIpsLocation());
    return currentConnectionDescr_.staticIps.ports;
}

void ConnectionManager::onWireGuardKeyLimitUserResponse(bool deleteOldestKey)
{
    // The answer can arrive after the attempt ended (the prompt round-trips through the UI); resume
    // or disconnect only while an attempt is still active.
    if (!isAttemptActive()) {
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

bool ConnectionManager::isAllowFirewallAfterConnectionRuntime() const
{
    WS_ASSERT(connector_);
    if (!connector_ || currentConnectionDescr_.connectionNodeType != CONNECTION_NODE_CUSTOM_CONFIG) {
        return true;
    }

    return connector_->isAllowFirewallAfterConnectionRuntime();
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

    recreateAttemptStrategy(connectionSettings, portMap, proxySettings);

    if (connector_ == nullptr || !isConnectorDialed_) {
        return;
    }

    switch (state_) {
        case State::kDisconnected:
        case State::kStopping:
        case State::kWaitingForNetwork:
        case State::kSleeping:
        case State::kReconnecting:
            break;
        case State::kConnecting:
        case State::kConnected:
            enterReconnecting();
            connector_->startDisconnect();
            break;
    }
}

void ConnectionManager::recreateAttemptStrategy(const types::ConnectionSettings &connectionSettings,
                                                       const api_responses::PortMap &portMap,
                                                       const types::ProxySettings &proxySettings)
{
    if (!lastRequest_.bli) {
        // no active connection in progress
        return;
    }

    // Raw availability fact, not fused with Always On+: the conjunction happens at attempt time
    // against the live flag, so a mid-session mode change is honored on the next attempt.
    // The fresh strategy owns the cached-config budget from here on.
    const bool hasUsableCachedConfig = connectionFactory_->hasUsableStoredConfig();

    attemptStrategy_.reset(attemptStrategyFactory_->createStrategy(lastRequest_.bli, connectionSettings, portMap,
                                                                       proxySettings, lastRequest_.preferredNodeHostname,
                                                                       isFirewallAlwaysOnPlusEnabled_,
                                                                       hasUsableCachedConfig,
                                                                       platformPolicy_->isLockdownMode()));
    connect(attemptStrategy_.data(), &IConnectionAttemptStrategy::hostnamesResolved, this, &ConnectionManager::onHostnamesResolved);
    connect(attemptStrategy_.data(), &IConnectionAttemptStrategy::protocolStatusChanged, this, &ConnectionManager::protocolStatusChanged);
}

void ConnectionManager::connectOrStartConnectTimer()
{
    if (attemptStrategy_->hasProtocolChanged()) {
        connectTimer_.setSingleShot(true);
        connectTimer_.setInterval(kConnectionWaitTimeMsec);
        connectTimer_.start();
    } else {
        startAttempt();
    }
}

bool ConnectionManager::isSenderCurrentConnector() const
{
    return connector_ != nullptr && static_cast<IConnection *>(sender()) == connector_;
}

// WaitingForNetwork counts as active: the connectivity poll re-enters startAttempt() without
// changing state, so a legitimate attempt can progress in that state.
bool ConnectionManager::isAttemptActive() const
{
    return state_ == State::kConnecting || state_ == State::kReconnecting || state_ == State::kWaitingForNetwork;
}

void ConnectionManager::setState(State state, ReconnectCause cause)
{
    // No same-state short-circuit: re-entering Reconnecting is how a wake reconnect converts to a
    // regular one.
    state_ = state;
    reconnectCause_ = cause;
}

void ConnectionManager::enterStopping()
{
    // The connect timeout must not fire during the teardown and convert the pending outcome
    // into a failover reconnect.
    connectingTimer_.stop();
    // A poll tick during the teardown would startAttempt() in kStopping, retiring the connector
    // whose disconnected() carries the pending outcome and wedging the machine.
    timerWaitNetworkConnectivity_.stop();
    timerReconnection_.stop();
    connectTimer_.stop();
    setState(State::kStopping);
}

void ConnectionManager::stopWithDisconnect(DISCONNECT_REASON reason, bool isUserInitiated)
{
    pendingOutcome_.type = PendingOutcome::Type::Disconnect;
    pendingOutcome_.reason = reason;
    pendingOutcome_.isUserInitiated = isUserInitiated;
    enterStopping();
}

void ConnectionManager::stopWithError(ConnectError err)
{
    pendingOutcome_.type = PendingOutcome::Type::Error;
    pendingOutcome_.error = err;
    pendingOutcome_.isUserInitiated = false;
    enterStopping();
}

void ConnectionManager::failWithError(ConnectError err)
{
    if (connector_) {
        connector_->teardown();
        SAFE_DELETE_LATER(connector_);
    }
    disconnect();
    emit errorDuringConnection(err, currentConnectionDescr_);
}

void ConnectionManager::disconnect()
{
    log_utils::Logger::instance().endConnectionMode();
    timerReconnection_.stop();
    connectTimer_.stop();
    connectingTimer_.stop();
    // A surviving connectivity poll would redial from kDisconnected once the network returns.
    timerWaitNetworkConnectivity_.stop();
    setState(State::kDisconnected);
    emit connectionEnded();
}

void ConnectionManager::onConnectTrigger()
{
    startAttempt();
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
    clearCachedConfigIfExhausted(true);
    startFailoverReconnect();
}
