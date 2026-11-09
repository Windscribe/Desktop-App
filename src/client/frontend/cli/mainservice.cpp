#include "mainservice.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QThread>

#include "backend/persistentstate.h"
#include "launchonstartup/launchonstartup.h"
#include "multipleaccountdetection/multipleaccountdetectionfactory.h"
#include "utils/log/categories.h"

MainService::MainService() : QObject(), isExitingAfterUpdate_(false), keyLimitDelete_(false)
{
    backend_ = new Backend(this);
    connect(backend_, &Backend::checkUpdateChanged, this, &MainService::onBackendCheckUpdateChanged);
    connect(backend_, &Backend::initFinished, this, &MainService::onBackendInitFinished);
    connect(backend_, &Backend::sessionStatusChanged, this, &MainService::onBackendSessionStatusChanged);
    connect(backend_, &Backend::updateVersionChanged, this, &MainService::onBackendUpdateVersionChanged);
    connect(backend_, &Backend::wireGuardAtKeyLimit, this, &MainService::onBackendWireGuardAtKeyLimit);
    connect(backend_, &Backend::connectStateChanged, this, &MainService::onBackendConnectStateChanged);
    connect(backend_, &Backend::sessionDeleted, this, &MainService::onBackendSessionDeleted);
    backend_->init();

    // signals from here to Backend
    connect(this, &MainService::wireGuardKeyLimitUserResponse, backend_, &Backend::wireGuardKeyLimitUserResponse);

    // signals from Preferences
    connect(backend_->getPreferences(), &Preferences::firewallSettingsChanged, this, &MainService::onPreferencesFirewallSettingsChanged);
    connect(backend_->getPreferences(), &Preferences::shareProxyGatewayChanged, this, &MainService::onPreferencesShareProxyGatewayChanged);
    connect(backend_->getPreferences(), &Preferences::splitTunnelingChanged, this, &MainService::onPreferencesSplitTunnelingChanged);
    connect(backend_->getPreferences(), &Preferences::isAutoConnectChanged, this, &MainService::onPreferencesAutoConnectChanged);
    connect(backend_->getPreferences(), &Preferences::isAllowLanTrafficChanged, this, &MainService::onPreferencesAllowLanTrafficChanged);
    connect(backend_->getPreferences(), &Preferences::isLaunchOnStartupChanged, this, &MainService::onPreferencesLaunchOnStartupChanged);

    multipleAccountDetection_ = QSharedPointer<IMultipleAccountDetection>(MultipleAccountDetectionFactory::create());

    localIpcServer_ = new LocalIPCServer(backend_, this);
    connect(localIpcServer_, &LocalIPCServer::connectToLocation, this, &MainService::onConnectToLocation);
    connect(localIpcServer_, &LocalIPCServer::attemptLogin, this, &MainService::onLogin);
    connect(localIpcServer_, &LocalIPCServer::setKeyLimitBehavior, this, &MainService::onSetKeyLimitBehavior);
    connect(localIpcServer_, &LocalIPCServer::setIgnoreSslErrors, this, &MainService::onSetIgnoreSslErrors);
}

MainService::~MainService()
{
}

void MainService::onBackendInitFinished(INIT_STATE initState)
{
    if (initState != INIT_STATE_SUCCESS) {
        qCCritical(LOG_BASIC) << "Could not initialize helper";
        qApp->exit();
    }

    // Set initial state
    {
        setInitialFirewallState();
        Preferences *p = backend_->getPreferences();
        p->validateAndUpdateIfNeeded();

        backend_->sendSplitTunneling(p->splitTunneling());
    }

    localIpcServer_->start();

    QString autoLoginUsername;
    QString autoLoginPassword;

    if (backend_->haveAutoLoginCredentials(autoLoginUsername, autoLoginPassword)) {
        onLogin(autoLoginUsername, autoLoginPassword, QString());
    } else if (backend_->isCanLoginWithAuthHash()) {
        backend_->loginWithAuthHash();
    }

    onPreferencesShareProxyGatewayChanged(backend_->getPreferences()->shareProxyGateway());
}

void MainService::setInitialFirewallState()
{
    bool bFirewallStateOn = PersistentState::instance().isFirewallOn();
    qCInfo(LOG_BASIC) << "Firewall state from last app start:" << bFirewallStateOn;

    if (bFirewallStateOn) {
        backend_->firewallOn();
    } else {
        if (backend_->getPreferences()->firewallSettings().isAlwaysOnMode()) {
            backend_->firewallOn();
        } else {
            backend_->firewallOff();
        }
    }
}

void MainService::onConnectToLocation(const LocationID &lid, const types::Protocol &protocol, uint port)
{
    if (!lid.isValid()) {
        qCInfo(LOG_USER) << "Invalid location";
        return;
    }

    qCDebug(LOG_USER) << "Location selected:" << lid.getHashString();
    PersistentState::instance().setLastLocation(lid);

    if (protocol.isValid()) {
        // determine default port for protocol
        QVector<uint> ports = backend_->getPreferencesHelper()->getAvailablePortsForProtocol(protocol);
        if (ports.size() == 0) {
            qCWarning(LOG_BASIC) << "Could not determine port for protocol" << protocol.toLongString();
            port = 0;
        } else if (port == 0) {
            port = ports[0];
        } else {
            // Port specified by caller, check if it is available
            if (!ports.contains(port)) {
                qCWarning(LOG_BASIC) << "Port" << port << "is not available for protocol, using default port" << protocol.toLongString();
                port = ports[0];
            }
        }
    } else {
        port = 0;
    }

    if (port != 0) {
        backend_->sendConnect(lid, types::ConnectionSettings(protocol, port, false));
    } else {
        backend_->sendConnect(lid);
    }
}

void MainService::onLogin(const QString &username, const QString &password, const QString &code2fa)
{
    backend_->login(username, password, code2fa);
}

void MainService::onBackendCheckUpdateChanged(const api_responses::CheckUpdate &info)
{
    if (info.isAvailable() && !info.isSupported()) {
        blockConnect_.setNeedUpgrade();
    }
}

void MainService::onBackendSessionStatusChanged(const api_responses::SessionStatus &sessionStatus)
{
    blockConnect_.setNotBlocking();
    qint32 status = sessionStatus.getStatus();

    // multiple account abuse detection
    QString entryUsername;
    QString entryUserId;
    bool bEntryIsPresent = multipleAccountDetection_->entryIsPresent(entryUsername, entryUserId);
    bool isDifferentAccount = !entryUserId.isEmpty() ? entryUserId != sessionStatus.getUserId() : entryUsername != sessionStatus.getUsername();
    if (bEntryIsPresent && (!sessionStatus.isPremium()) && sessionStatus.getAlc().size() == 0 && sessionStatus.getStatus() == 1 && isDifferentAccount) {
        status = 2;
        if (entryUserId.isEmpty()) {
            qCInfo(LOG_BASIC) << "Multiple account detected - previous username:" << entryUsername << "(userID unavailable from previous version), current username:" << sessionStatus.getUsername() << ", current userID:" << sessionStatus.getUserId();
        } else {
            qCInfo(LOG_BASIC) << "Multiple account detected - previous username:" << entryUsername << ", previous userID:" << entryUserId << ", current username:" << sessionStatus.getUsername() << ", current userID:" << sessionStatus.getUserId();
        }
        blockConnect_.setBlockedMultiAccount(entryUsername, entryUserId);
    } else if (bEntryIsPresent && sessionStatus.getStatus() == 1) {
        bool isSameAccount = !entryUserId.isEmpty() ? entryUserId == sessionStatus.getUserId() : entryUsername == sessionStatus.getUsername();
        if (isSameAccount) {
            multipleAccountDetection_->removeEntry();
        }
    }

    // free account
    if (!sessionStatus.isPremium()) {
        if (status == 2) {
            // write entry into registry expired_user = username
            multipleAccountDetection_->userBecomeExpired(sessionStatus.getUsername(), sessionStatus.getUserId());

            if (backend_->currentConnectState() == CONNECT_STATE_CONNECTED || backend_->currentConnectState() == CONNECT_STATE_CONNECTING) {
                backend_->sendDisconnect(DISCONNECTED_BY_DATA_LIMIT);
            }
            if (!blockConnect_.isBlocked()) {
                blockConnect_.setBlockedExceedTraffic();
            }
        }
    }

    if (status == 3) {
        blockConnect_.setBlockedBannedUser();
        if (backend_->currentConnectState() == CONNECT_STATE_CONNECTED || backend_->currentConnectState() == CONNECT_STATE_CONNECTING) {
            backend_->sendDisconnect();
        }
    }
    backend_->setBlockConnect(blockConnect_.isBlocked());
}

void MainService::onPreferencesLaunchOnStartupChanged(bool enabled)
{
    LaunchOnStartup::instance().setLaunchOnStartup(enabled);
}

void MainService::stop()
{
    backend_->cleanup(isExitingAfterUpdate_);

    // Backend handles setting firewall state after app closes
    // This block handles initializing the firewall state on next run
    if (PersistentState::instance().isFirewallOn() && backend_->getPreferences()->firewallSettings().mode == FIREWALL_MODE_AUTOMATIC) {
        PersistentState::instance().setFirewallState(false);
    } else if (backend_->getPreferences()->firewallSettings().isAlwaysOnMode()) {
        PersistentState::instance().setFirewallState(true);
    }

    PersistentState::instance().save();
    backend_->locationsModelManager()->saveFavoriteLocations();

    // Wait for backend to complete its operations before returning
    while (!backend_->isAppCanClose()) {
        QThread::msleep(1);
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    }
}

void MainService::onBackendUpdateVersionChanged(uint progressPercent, UPDATE_VERSION_STATE state, UPDATE_VERSION_ERROR error)
{
    if (state == UPDATE_VERSION_STATE_DONE && error == UPDATE_VERSION_ERROR_NO_ERROR) {
        isExitingAfterUpdate_ = true;
    }
}

void MainService::onBackendWireGuardAtKeyLimit()
{
    if (!keyLimitDelete_) {
        localIpcServer_->setDisconnectedByKeyLimit();
    }

    qCInfo(LOG_BASIC) << "WireGuard key limit response:" << keyLimitDelete_;
    emit wireGuardKeyLimitUserResponse(keyLimitDelete_);
    keyLimitDelete_ = false;
}

void MainService::onPreferencesSplitTunnelingChanged(const types::SplitTunneling &st)
{
    backend_->sendSplitTunneling(st);
}

void MainService::onPreferencesFirewallSettingsChanged(const types::FirewallSettings &fm)
{
    if (fm.isAlwaysOnMode()) {
        if (!PersistentState::instance().isFirewallOn()) {
            backend_->firewallOn();
        }
    }
}

void MainService::onPreferencesShareProxyGatewayChanged(const types::ShareProxyGateway &sp)
{
    if (sp.isEnabled) {
        backend_->startProxySharing(sp);
    } else {
        backend_->stopProxySharing();
    }
}

void MainService::onPreferencesAutoConnectChanged(bool /*on*/)
{
    // Re-evaluate auto-connect against the current network (manual=true forces the check even though
    // the network itself didn't change), so toggling Auto-Connect on connects immediately when the
    // current network is secured. Mirrors MainWindow::onAutoConnectUpdated in the GUI.
    backend_->handleNetworkChange(backend_->getCurrentNetworkInterface(), true);
}

void MainService::onPreferencesAllowLanTrafficChanged(bool /*allowLanTraffic*/)
{
    // Changing Allow Lan Traffic may affect split tunnel behavior
    onPreferencesSplitTunnelingChanged(backend_->getPreferences()->splitTunneling());
}

void MainService::onSetKeyLimitBehavior(bool deleteKey)
{
    keyLimitDelete_ = deleteKey;
}

void MainService::onSetIgnoreSslErrors(bool ignore)
{
    if (backend_->setIgnoreSslErrors(ignore) && ignore) {
        backend_->loginWithLastLoginSettings();
    }
}

void MainService::onBackendConnectStateChanged(const types::ConnectState &connectState)
{
    // The user picked a local DNS server that turned out to be unavailable; revert Connected DNS to
    // Auto so the next connect isn't stuck on it.
    if (connectState.disconnectReason == DISCONNECTED_WITH_ERROR &&
        connectState.connectError == ConnectError::kLocalDnsServerNotAvailable) {
        types::ConnectedDnsInfo connectedDnsInfo = backend_->getPreferences()->connectedDnsInfo();
        connectedDnsInfo.type = CONNECTED_DNS_TYPE::CONNECTED_DNS_TYPE_AUTO;
        backend_->getPreferences()->setConnectedDnsInfo(connectedDnsInfo);
    }
}

void MainService::onBackendSessionDeleted()
{
    qCInfo(LOG_BASIC) << "Handle deleted session";
    backend_->logout(true);
}

