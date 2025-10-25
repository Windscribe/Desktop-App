#include "backend.h"

#include <QCoreApplication>

#include "engine/engine.h"
#include "persistentstate.h"
#include "locations/locationsmodel_roles.h"
#include "utils/dns_utils/dnsutils.h"
#include "utils/log/categories.h"
#include "utils/network_utils/network_utils.h"

#ifdef Q_OS_WIN
#include "utils/wincryptutils.h"
#endif

#ifdef Q_OS_LINUX
#include "launchonstartup/launchonstartup.h"
#endif

Backend::Backend(QObject *parent) : QObject(parent),
    isSavedApiSettingsExists_(false),
    bLastLoginWithAuthHash_(false),
    engine_(nullptr),
    isCleanupFinished_(false),
    cmdId_(0),
    isCanLoginWithAuthHash_(false),
    isFirewallEnabled_(false),
    isExternalConfigMode_(false),
    loginState_(LOGIN_STATE_LOGGED_OUT)
{

    preferences_.loadGuiSettings();

#ifdef Q_OS_LINUX
    // work around an issue on Linux where after an update in which the autostart file changed
    // the user would not get the updated autostart file until they toggle the feature.  Instead, do it programatically.
    LaunchOnStartup::instance().setLaunchOnStartup(preferences_.isLaunchOnStartup());
#endif

    // if any of the options from engine settings are changed sync with engine settings
    connect(&preferences_, &Preferences::engineSettingsChanged, this, &Backend::onEngineSettingsChangedInPreferences);

    locationsModelManager_ = new gui_locations::LocationsModelManager(this);

    connect(&connectStateHelper_, &ConnectStateHelper::connectStateChanged, this, &Backend::connectStateChanged);
    connect(&emergencyConnectStateHelper_, &ConnectStateHelper::connectStateChanged, this, &Backend::emergencyConnectStateChanged);
    connect(&firewallStateHelper_, &FirewallStateHelper::firewallStateChanged, this, &Backend::firewallStateChanged);
}

Backend::~Backend()
{
    preferences_.saveGuiSettings();
    delete engine_;
}

void Backend::init()
{
    isCleanupFinished_ = false;
    qCDebug(LOG_BASIC) << "Backend::init()";

    threadEngine_ = new QThread(this);
    engine_ = new Engine();
    engine_->moveToThread(threadEngine_);
    connect(threadEngine_, &QThread::started, engine_, &Engine::init);
    connect(threadEngine_, &QThread::finished, this,  &Backend::onEngineCleanupFinished);
    connect(engine_, &Engine::initFinished, this, &Backend::onEngineInitFinished);
    connect(engine_, &Engine::bfeEnableFinished, this, &Backend::onEngineBfeEnableFinished);
    connect(engine_, &Engine::firewallStateChanged, this, &Backend::onEngineFirewallStateChanged);
    connect(engine_, &Engine::loginFinished, this, &Backend::onEngineLoginFinished);
    connect(engine_, &Engine::captchaRequired, this, &Backend::captchaRequired);
    connect(engine_, &Engine::tryingBackupEndpoint, this, &Backend::onEngineTryingBackupEndpoint);
    connect(engine_, &Engine::notificationsUpdated, this, &Backend::onEngineNotificationsUpdated);
    connect(engine_, &Engine::checkUpdateUpdated, this, &Backend::onEngineCheckUpdateUpdated);
    connect(engine_, &Engine::updateDownloaded, this, &Backend::onEngineUpdateDownloaded);
    connect(engine_, &Engine::updateVersionChanged, this, &Backend::onEngineUpdateVersionChanged);
    connect(engine_, &Engine::myIpUpdated, this, &Backend::onEngineMyIpUpdated);
    connect(engine_, &Engine::sessionStatusUpdated, this, &Backend::onEngineUpdateSessionStatus);
    connect(engine_, &Engine::sessionDeleted, this, &Backend::onEngineSessionDeleted);
    connect(engine_->getConnectStateController(), &IConnectStateController::stateChanged, this, &Backend::onEngineConnectStateChanged);
    connect(engine_, &Engine::protocolPortChanged, this, &Backend::onEngineProtocolPortChanged);
    connect(engine_, &Engine::statisticsUpdated, this, &Backend::onEngineStatisticsUpdated);
    connect(engine_, &Engine::emergencyConnected, this, &Backend::onEngineEmergencyConnected);
    connect(engine_, &Engine::emergencyDisconnected, this, &Backend::onEngineEmergencyDisconnected);
    connect(engine_, &Engine::emergencyConnectError, this, &Backend::onEngineEmergencyConnectError);
    connect(engine_, &Engine::testTunnelResult, this, &Backend::onEngineTestTunnelResult);
    connect(engine_, &Engine::lostConnectionToHelper, this, &Backend::onEngineLostConnectionToHelper);
    connect(engine_, &Engine::proxySharingStateChanged, this, &Backend::onEngineProxySharingStateChanged);
    connect(engine_, &Engine::wifiSharingStateChanged, this, &Backend::onEngineWifiSharingStateChanged);
    connect(engine_, &Engine::wifiSharingFailed, this, &Backend::wifiSharingFailed);
    connect(engine_, &Engine::logoutFinished, this, &Backend::onEngineLogoutFinished);
    connect(engine_, &Engine::gotoCustomOvpnConfigModeFinished, this, &Backend::onEngineGotoCustomOvpnConfigModeFinished);
    connect(engine_, &Engine::detectionCpuUsageAfterConnected, this, &Backend::onEngineDetectionCpuUsageAfterConnected);
    connect(engine_, &Engine::requestUsernameAndPassword, this, &Backend::onEngineRequestUsernameAndPassword);
    connect(engine_, &Engine::requestPrivKeyPassword, this, &Backend::onEngineRequestPrivKeyPassword);
    connect(engine_, &Engine::networkChanged, this, &Backend::onEngineNetworkChanged);
    connect(engine_, &Engine::confirmEmailFinished, this, &Backend::onEngineConfirmEmailFinished);
    connect(engine_, &Engine::sendDebugLogFinished, this, &Backend::onEngineSendDebugLogFinished);
    connect(engine_, &Engine::webSessionToken, this, &Backend::onEngineWebSessionToken);
    connect(engine_, &Engine::macAddrSpoofingChanged, this, &Backend::onEngineMacAddrSpoofingChanged);
    connect(engine_, &Engine::sendUserWarning, this, &Backend::onEngineSendUserWarning);
    connect(engine_, &Engine::internetConnectivityChanged, this, &Backend::onEngineInternetConnectivityChanged);
    connect(engine_, &Engine::packetSizeChanged, this, &Backend::onEnginePacketSizeChanged);
    connect(engine_, &Engine::packetSizeDetectionStateChanged, this, &Backend::onEnginePacketSizeDetectionStateChanged);
    connect(engine_, &Engine::hostsFileBecameWritable, this, &Backend::onEngineHostsFileBecameWritable);
    connect(engine_, &Engine::wireGuardAtKeyLimit, this, &Backend::wireGuardAtKeyLimit);
    connect(engine_, &Engine::protocolStatusChanged, this, &Backend::onEngineProtocolStatusChanged);
    connect(this, &Backend::wireGuardKeyLimitUserResponse, engine_, &Engine::onWireGuardKeyLimitUserResponse);
    connect(engine_, &Engine::loginError, this, &Backend::onEngineLoginError);
    connect(engine_, &Engine::robertFiltersUpdated, this, &Backend::onEngineRobertFiltersUpdated);
    connect(engine_, &Engine::setRobertFilterFinished, this, &Backend::onEngineSetRobertFilterFinished);
    connect(engine_, &Engine::syncRobertFinished, this, &Backend::onEngineSyncRobertFinished);
    connect(engine_, &Engine::splitTunnelingStartFailed, this, &Backend::splitTunnelingStartFailed);
    connect(engine_, &Engine::autoEnableAntiCensorship, this, &Backend::onEngineAutoEnableAntiCensorship);
    connect(engine_, &Engine::connectionIdChanged, this, &Backend::connectionIdChanged);
    connect(engine_, &Engine::localDnsServerNotAvailable, this, &Backend::localDnsServerNotAvailable);
    connect(engine_, &Engine::bridgeApiAvailabilityChanged, this, &Backend::onEngineBridgeApiAvailabilityChanged);
    connect(engine_, &Engine::ipRotateFailed, this, &Backend::ipRotateFailed);
    connect(engine_, &Engine::connectingHostnameChanged, this, &Backend::onEngineConnectingHostnameChanged);
    connect(engine_, &Engine::controldDevicesFetched, this, &Backend::controldDevicesFetched);
    threadEngine_->start(QThread::LowPriority);
}

void Backend::cleanup(bool isUpdating)
{
    qCDebug(LOG_BASIC) << "Backend::cleanup()";
    engine_->cleanup(isUpdating);
}

void Backend::enableBFE_win()
{
    engine_->enableBFE_win();
}

void Backend::login(const QString &username, const QString &password, const QString &code2fa)
{
    loginState_ = LOGIN_STATE_LOGGING_IN;
    bLastLoginWithAuthHash_ = false;
    lastUsername_ = username;
    lastPassword_ = password;
    lastCode2fa_ = code2fa;
    engine_->loginWithUsernameAndPassword(username, password, code2fa);
}

void Backend::continueLoginWithCaptcha(const QString &captchaSolution, const std::vector<float> &captchaTrailX, const std::vector<float> &captchaTrailY)
{
     engine_->continueLoginWithCaptcha(captchaSolution, captchaTrailX, captchaTrailY);
}

bool Backend::isCanLoginWithAuthHash() const
{
    return isCanLoginWithAuthHash_;
}

bool Backend::isSavedApiSettingsExists() const
{
    return isSavedApiSettingsExists_;
}

void Backend::loginWithAuthHash()
{
    loginState_ = LOGIN_STATE_LOGGING_IN;
    bLastLoginWithAuthHash_ = true;
    engine_->loginWithAuthHash();
}

void Backend::loginWithLastLoginSettings()
{
    loginState_ = LOGIN_STATE_LOGGING_IN;
    if (bLastLoginWithAuthHash_)
        loginWithAuthHash();
    else
        login(lastUsername_, lastPassword_, lastCode2fa_);
}

bool Backend::isLastLoginWithAuthHash() const
{
    return bLastLoginWithAuthHash_;
}

void Backend::logout(bool keepFirewallOn)
{
    engine_->logout(keepFirewallOn);
}

void Backend::sendConnect(const LocationID &lid, const types::ConnectionSettings &connectionSettings)
{
    if (isDisconnected()) {
        osDnsServers_ = DnsUtils::getOSDefaultDnsServers();
    }
    connectStateHelper_.connectClickFromUser();

    // Check if this location has a pinned node/IP in favorites
    // Only pin if user is premium and location is an API location
    QPair<QString, QString> pinnedNode; // hostname, ip
					//
    LocationID apiLocationId = lid;

    // If this is Best Location, convert to the actual API location
    if (lid.isBestLocation()) {
        apiLocationId = lid.bestLocationToApiLocation();
    }

    // Only check for pinned data if it's an API location
    if (apiLocationId.isValid() && apiLocationId.type() == 1) { // API_LOCATION = 1
        QModelIndex locationIndex = static_cast<gui_locations::LocationsModel*>(locationsModelManager_->locationsModel())->getIndexByLocationId(apiLocationId);
        if (locationIndex.isValid()) {
            QVariantList pinnedData = locationsModelManager_->locationsModel()->data(locationIndex, gui_locations::kPinnedIp).toList();
            if (pinnedData.size() == 2) {
                pinnedNode.first = pinnedData[0].toString();   // hostname
                pinnedNode.second = pinnedData[1].toString();  // ip
            }
        }
    }

    engine_->connectClick(lid, connectionSettings, pinnedNode);
}

void Backend::sendDisconnect(DISCONNECT_REASON reason)
{
    if (reason == DISCONNECTED_BY_USER) {
        connectStateHelper_.disconnectClickFromUser();
    } else {
        connectStateHelper_.disconnect(reason);
    }
    engine_->disconnectClick(reason);
}

bool Backend::isDisconnected() const
{
    return connectStateHelper_.isDisconnected();
}

LOGIN_STATE Backend::currentLoginState() const
{
    return loginState_;
}

CONNECT_STATE Backend::currentConnectState() const
{
    return connectStateHelper_.currentConnectState().connectState;
}

LocationID Backend::currentLocation() const
{
    return connectStateHelper_.currentConnectState().location;
}

void Backend::firewallOn(bool updateHelperFirst)
{
    if (updateHelperFirst) firewallStateHelper_.firewallOnClickFromGUI();
    engine_->firewallOn();
}

void Backend::firewallOff(bool updateHelperFirst)
{
    if (updateHelperFirst) firewallStateHelper_.firewallOffClickFromGUI();
    engine_->firewallOff();
}

bool Backend::isFirewallEnabled()
{
    return firewallStateHelper_.isEnabled();
}

bool Backend::isFirewallAlwaysOn()
{
    return getPreferences()->firewallSettings().isAlwaysOnMode();
}

void Backend::emergencyConnectClick()
{
    emergencyConnectStateHelper_.connectClickFromUser();
    engine_->emergencyConnectClick();
}

void Backend::emergencyDisconnectClick()
{
    emergencyConnectStateHelper_.disconnectClickFromUser();
    engine_->emergencyDisconnectClick();
}

void Backend::refreshLocations()
{
    engine_->refreshLocations();
}

bool Backend::isEmergencyDisconnected()
{
    return emergencyConnectStateHelper_.isDisconnected();
}

void Backend::startWifiSharing(const QString &ssid, const QString &password)
{
    engine_->startWifiSharing(ssid, password);
}

void Backend::stopWifiSharing()
{
    engine_->stopWifiSharing();
}

void Backend::startProxySharing(PROXY_SHARING_TYPE proxySharingMode, uint port, bool whileConnected)
{
    engine_->startProxySharing(proxySharingMode, port, whileConnected);
}

void Backend::stopProxySharing()
{
    engine_->stopProxySharing();
}

void Backend::gotoCustomOvpnConfigMode()
{
    engine_->gotoCustomOvpnConfigMode();
}

void Backend::recordInstall()
{
    engine_->recordInstall();
}

void Backend::sendConfirmEmail()
{
    engine_->sendConfirmEmail();
}

void Backend::sendDebugLog()
{
    engine_->sendDebugLog();
}

void Backend::getWebSessionTokenForManageAccount()
{
    engine_->getWebSessionToken(WEB_SESSION_PURPOSE_MANAGE_ACCOUNT);
}

void Backend::getWebSessionTokenForAddEmail()
{
    engine_->getWebSessionToken(WEB_SESSION_PURPOSE_ADD_EMAIL);
}

void Backend::getWebSessionTokenForManageRobertRules()
{
    engine_->getWebSessionToken(WEB_SESSION_PURPOSE_MANAGE_ROBERT_RULES);
}

void Backend::speedRating(int rating, const QString &localExternalIp)
{
    engine_->speedRating(rating, localExternalIp);
}

void Backend::setBlockConnect(bool isBlockConnect)
{
    if (engine_->isBlockConnect() != isBlockConnect)
        engine_->setBlockConnect(isBlockConnect);
}

void Backend::getRobertFilters()
{
    engine_->getRobertFilters();
}

void Backend::setRobertFilter(const api_responses::RobertFilter &filter)
{
    engine_->setRobertFilter(filter);
}

void Backend::syncRobert()
{
    engine_->syncRobert();
}

bool Backend::isAppCanClose() const
{
    return isCleanupFinished_;
}

void Backend::continueWithCredentialsForOvpnConfig(const QString &username, const QString &password, bool bSave)
{
    engine_->continueWithUsernameAndPassword(username, password, bSave);
}

void Backend::continueWithPrivKeyPasswordForOvpnConfig(const QString &password, bool bSave)
{
    engine_->continueWithPrivKeyPassword(password, bSave);
}

void Backend::sendAdvancedParametersChanged()
{
    engine_->updateAdvancedParams();
}

gui_locations::LocationsModelManager *Backend::locationsModelManager()
{
    return locationsModelManager_;
}

PreferencesHelper *Backend::getPreferencesHelper()
{
    return &preferencesHelper_;
}

Preferences *Backend::getPreferences()
{
    return &preferences_;
}

AccountInfo *Backend::getAccountInfo()
{
    return &accountInfo_;
}

void Backend::applicationActivated()
{
    engine_->applicationActivated();
}

void Backend::applicationDeactivated()
{
    // nothing todo
}

const api_responses::SessionStatus &Backend::getSessionStatus() const
{
    return latestSessionStatus_;
}

void Backend::onEngineSettingsChangedInPreferences()
{
    // sync engine settings with engine
    engine_->setSettings(preferences_.getEngineSettings());
}

void Backend::onEngineCleanupFinished()
{
    isCleanupFinished_ = true;
    delete threadEngine_;
    emit cleanupFinished();
}

void Backend::onEngineInitFinished(ENGINE_INIT_RET_CODE retCode, bool isCanLoginWithAuthHash, const types::EngineSettings &engineSettings)
{
    if (retCode == ENGINE_INIT_SUCCESS) {
        connect(engine_->getLocationsModel(), &locationsmodel::LocationsModel::locationsUpdated, this,  &Backend::onEngineLocationsModelItemsUpdated);
        connect(engine_->getLocationsModel(), &locationsmodel::LocationsModel::bestLocationUpdated, this, &Backend::onEngineLocationsModelBestLocationUpdated);
        connect(engine_->getLocationsModel(), &locationsmodel::LocationsModel::customConfigsLocationsUpdated, this, &Backend::onEngineLocationsModelCustomConfigItemsUpdated);
        connect(engine_->getLocationsModel(), &locationsmodel::LocationsModel::locationPingTimeChanged, this, &Backend::onEngineLocationsModelPingChangedChanged);
        connect(engine_->getLocationsModel(), &locationsmodel::LocationsModel::pingsStarted, this, &Backend::pingsStarted);
        connect(engine_->getLocationsModel(), &locationsmodel::LocationsModel::pingsFinished, this, &Backend::pingsFinished);

        preferences_.setEngineSettings(engineSettings);
        // WiFi sharing supported state
        preferencesHelper_.setWifiSharingSupported(engine_->isWifiSharingSupported());
        isSavedApiSettingsExists_ = engine_->isApiSavedSettingsExists();
        isCanLoginWithAuthHash_ = isCanLoginWithAuthHash;

        emit initFinished(INIT_STATE_SUCCESS);
        engine_->updateCurrentInternetConnectivity();
    } else if (retCode == ENGINE_INIT_HELPER_FAILED) {
        emit initFinished(INIT_STATE_HELPER_FAILED);
    } else if (retCode == ENGINE_INIT_BFE_SERVICE_FAILED) {
        emit initFinished(INIT_STATE_BFE_SERVICE_NOT_STARTED);
    } else if (retCode == ENGINE_INIT_HELPER_USER_CANCELED) {
        emit initFinished(INIT_STATE_HELPER_USER_CANCELED);
    } else {
        WS_ASSERT(false);
    }
}

void Backend::onEngineBfeEnableFinished(ENGINE_INIT_RET_CODE retCode, bool isCanLoginWithAuthHash, const types::EngineSettings &engineSettings)
{
    if (retCode == ENGINE_INIT_SUCCESS)
        onEngineInitFinished(ENGINE_INIT_SUCCESS, isCanLoginWithAuthHash, engineSettings);
    else
        emit initFinished(INIT_STATE_BFE_SERVICE_FAILED_TO_START);
}

void Backend::onEngineFirewallStateChanged(bool isEnabled)
{
    firewallStateHelper_.setFirewallStateFromEngine(isEnabled);
}

void Backend::onEngineLoginFinished(bool isLoginFromSavedSettings, const api_responses::PortMap &portMap)
{
    loginState_ = LOGIN_STATE_LOGGED_IN;
    preferencesHelper_.setPortMap(portMap);

    triggerAutoConnect(currentNetworkInterface_);

    emit loginFinished(isLoginFromSavedSettings);
}

void Backend::onEngineLoginError(wsnet::LoginResult retCode, const QString &errorMessage)
{
    loginState_ = LOGIN_STATE_LOGIN_ERROR;
    lastLoginError_ = retCode;
    emit loginError(retCode, errorMessage);
}

void Backend::onEngineTryingBackupEndpoint(int num, int cnt)
{
    emit tryingBackupEndpoint(num, cnt);
}

void Backend::onEngineSessionDeleted()
{
    emit sessionDeleted();
}

void Backend::onEngineUpdateSessionStatus(const api_responses::SessionStatus &sessionStatus)
{
    latestSessionStatus_ = sessionStatus;
    locationsModelManager_->setFreeSessionStatus(!latestSessionStatus_.isPremium());
    updateAccountInfo();
    emit sessionStatusChanged(latestSessionStatus_);
}

void Backend::onEngineNotificationsUpdated(const QVector<api_responses::Notification> &notifications)
{
    emit notificationsChanged(notifications);
}

void Backend::onEngineCheckUpdateUpdated(const api_responses::CheckUpdate &checkUpdate)
{
    emit checkUpdateChanged(checkUpdate);
}

void Backend::onEngineUpdateDownloaded(const QString &path)
{
    emit updateDownloaded(path);
}

void Backend::onEngineUpdateVersionChanged(uint progressPercent, const UPDATE_VERSION_STATE &state, const UPDATE_VERSION_ERROR &error)
{
    emit updateVersionChanged(progressPercent, state, error);
}

void Backend::onEngineMyIpUpdated(const QString &ip, bool isDisconnected)
{
    emit myIpChanged(ip, isDisconnected);
}

void Backend::onEngineConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, CONNECT_ERROR err, const LocationID &locationId)
{
    types::ConnectState connectState;
    connectState.connectState = state;
    connectState.disconnectReason = reason;
    connectState.connectError = err;
    connectState.location = locationId;
    connectStateHelper_.setConnectStateFromEngine(connectState);
}

void Backend::onEngineStatisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes)
{
    emit statisticsUpdated(bytesIn, bytesOut, isTotalBytes);
}

void Backend::onEngineProtocolPortChanged(const types::Protocol &protocol, const uint port)
{
    emit protocolPortChanged(protocol, port);
}

void Backend::onEngineRobertFiltersUpdated(bool success, const QVector<api_responses::RobertFilter> &filters)
{
    emit robertFiltersChanged(success, filters);
}

void Backend::onEngineSetRobertFilterFinished(bool success)
{
    emit setRobertFilterResult(success);
}

void Backend::onEngineSyncRobertFinished(bool success)
{
    emit syncRobertResult(success);
}

void Backend::onEngineProtocolStatusChanged(const QVector<types::ProtocolStatus> &status)
{
    emit protocolStatusChanged(status);
}

void Backend::onEngineEmergencyConnected()
{
    types::ConnectState connectState;
    connectState.connectState = CONNECT_STATE_CONNECTED;
    emergencyConnectStateHelper_.setConnectStateFromEngine(connectState);
}

void Backend::onEngineEmergencyDisconnected()
{
    types::ConnectState connectState;
    connectState.connectState = CONNECT_STATE_DISCONNECTED;
    emergencyConnectStateHelper_.setConnectStateFromEngine(connectState);
}

void Backend::onEngineEmergencyConnectError(CONNECT_ERROR err)
{
    types::ConnectState connectState;
    connectState.connectState = CONNECT_STATE_DISCONNECTED;
    connectState.disconnectReason = DISCONNECTED_WITH_ERROR;
    connectState.connectError = err;
    emergencyConnectStateHelper_.setConnectStateFromEngine(connectState);
}

void Backend::onEngineTestTunnelResult(bool bSuccess)
{
    emit testTunnelResult(bSuccess);
}

void Backend::onEngineLostConnectionToHelper()
{
    emit lostConnectionToHelper();
}

void Backend::onEngineProxySharingStateChanged(bool bEnabled, PROXY_SHARING_TYPE proxySharingType, const QString &address, int usersCount)
{
    types::ProxySharingInfo proxySharingInfo;
    proxySharingInfo.isEnabled = bEnabled;
    if (bEnabled) {
        proxySharingInfo.mode = proxySharingType;
        proxySharingInfo.address = address;
        proxySharingInfo.usersCount = usersCount;
    }
    emit proxySharingInfoChanged(proxySharingInfo);
}

void Backend::onEngineWifiSharingStateChanged(bool bEnabled, const QString &ssid, int usersCount)
{
    types::WifiSharingInfo wifiSharingInfo;
    wifiSharingInfo.isEnabled = bEnabled;
    if (bEnabled) {
        wifiSharingInfo.ssid = ssid;
        wifiSharingInfo.usersCount = usersCount;
    }
    emit wifiSharingInfoChanged(wifiSharingInfo);
}

void Backend::onEngineLogoutFinished()
{
    loginState_ = LOGIN_STATE_LOGGED_OUT;
    emit logoutFinished();
}

void Backend::onEngineGotoCustomOvpnConfigModeFinished()
{
    emit gotoCustomOvpnConfigModeFinished();
}

void Backend::onEngineNetworkChanged(types::NetworkInterface networkInterface)
{
    handleNetworkChange(networkInterface);
}

void Backend::onEngineDetectionCpuUsageAfterConnected(QStringList list)
{
    emit highCpuUsage(list);
}

void Backend::onEngineRequestUsernameAndPassword(const QString &username)
{
    emit requestCustomOvpnConfigCredentials(username);
}

void Backend::onEngineRequestPrivKeyPassword()
{
    emit requestCustomOvpnConfigPrivKeyPassword();
}

void Backend::onEngineInternetConnectivityChanged(bool connectivity)
{
    emit internetConnectivityChanged(connectivity);
}

void Backend::onEngineSendDebugLogFinished(bool bSuccess)
{
    emit debugLogResult(bSuccess);
}

void Backend::onEngineConfirmEmailFinished(bool bSuccess)
{
    emit confirmEmailResult(bSuccess);
}

void Backend::onEngineWebSessionToken(WEB_SESSION_PURPOSE purpose, const QString &token)
{
    if (purpose == WEB_SESSION_PURPOSE_MANAGE_ACCOUNT)
        emit webSessionTokenForManageAccount(token);
    else if (purpose == WEB_SESSION_PURPOSE_ADD_EMAIL)
        emit webSessionTokenForAddEmail(token);
    else if (purpose == WEB_SESSION_PURPOSE_MANAGE_ROBERT_RULES)
        emit webSessionTokenForManageRobertRules(token);
}

void Backend::onEngineLocationsModelItemsUpdated(const LocationID &bestLocation, const QString &staticIpDeviceName, QSharedPointer<QVector<types::Location> > items)
{
    locationsModelManager_->updateLocations(bestLocation, *items);
    locationsModelManager_->updateDeviceName(staticIpDeviceName);
}

void Backend::onEngineLocationsModelBestLocationUpdated(const LocationID &bestLocation)
{
    locationsModelManager_->updateBestLocation(bestLocation);
}

void Backend::onEngineLocationsModelCustomConfigItemsUpdated(QSharedPointer<types::Location> item)
{
    locationsModelManager_->updateCustomConfigLocation(*item);
}

void Backend::onEngineLocationsModelPingChangedChanged(const LocationID &id, PingTime timeMs)
{
    locationsModelManager_->changeConnectionSpeed(id, timeMs);
}

void Backend::onEngineMacAddrSpoofingChanged(const types::EngineSettings &engineSettings)
{
    preferences_.setEngineSettings(engineSettings);
}

void Backend::onEngineSendUserWarning(USER_WARNING_TYPE userWarningType)
{
    emit userWarning(userWarningType);
}

void Backend::onEnginePacketSizeChanged(const types::EngineSettings &engineSettings)
{
    preferences_.setEngineSettings(engineSettings);
}

void Backend::onEnginePacketSizeDetectionStateChanged(bool on, bool isError)
{
    emit packetSizeDetectionStateChanged(on, isError);
}

void Backend::onEngineHostsFileBecameWritable()
{
    qCInfo(LOG_BASIC) << "Hosts file became writable -- Connecting..";
    sendConnect(PersistentState::instance().lastLocation());
}

void Backend::abortInitialization()
{
    emit initFinished(INIT_STATE_CLEAN);
}

// Assumes that duplicate network filtering occurs on Engine side
void Backend::handleNetworkChange(types::NetworkInterface networkInterface, bool manual)
{
    bool newNetwork = true;
    QVector<types::NetworkInterface> networkListOld = PersistentState::instance().networkWhitelist();

    QString friendlyName;
    if (!networkInterface.networkOrSsid.isEmpty()) {
        // find or assign friendly name before checking is network is the same as current network
        friendlyName = networkInterface.networkOrSsid;
        for (int i = 0; i < networkListOld.size(); i++) {
            if (networkListOld[i].networkOrSsid== networkInterface.networkOrSsid) {
                friendlyName = networkListOld[i].friendlyName;
                newNetwork = false;
                break;
            }
        }

        if (friendlyName == "") {
            friendlyName = networkInterface.networkOrSsid;
        }
        networkInterface.friendlyName = friendlyName;
    }

    if (networkInterface.networkOrSsid != "") { // not a disconnect
        // Add a new network as secured
        if (newNetwork) {
#ifdef Q_OS_MACOS
            // generate friendly name for MacOS Ethernet
            if (networkInterface.interfaceType == NETWORK_INTERFACE_ETH) {
                friendlyName = generateNewFriendlyName();
                networkInterface.friendlyName = friendlyName;

            }
#endif
            types::NetworkInterface newEntry;
            newEntry = networkInterface;
            if (preferences_.isAutoSecureNetworks()) {
                newEntry.trustType = NETWORK_TRUST_SECURED;
            } else {
                newEntry.trustType = NETWORK_TRUST_UNSECURED;
            }
            networkListOld << newEntry;
            preferences_.setNetworkWhiteList(networkListOld);
        }

        // GUI-side persistent list holds trustiness
        QVector<types::NetworkInterface> networkList = PersistentState::instance().networkWhitelist();
        types::NetworkInterface foundInterface;
        for (int i = 0; i < networkList.size(); i++) {
            if (networkList[i].networkOrSsid == networkInterface.networkOrSsid) {
                foundInterface = networkList[i];
                break;
            }
        }

        // actual network change or explicit trigger from preference change
        // prevents brief/rare network loss during CONNECTING from triggering network change
        if (!currentNetworkInterface_.sameNetworkInterface(networkInterface) || manual) {
            triggerAutoConnect(foundInterface);
            currentNetworkInterface_ = networkInterface;
        }

        // Even if not a real network change we want to update the UI with current network info.
        types::NetworkInterface protoInterface = networkInterface;
        protoInterface.trustType = foundInterface.trustType;
        emit networkChanged(protoInterface);
    } else {
        currentNetworkInterface_ = networkInterface;

        // inform UI no network
        emit networkChanged(networkInterface);
    }
}

void Backend::triggerAutoConnect(const types::NetworkInterface &interface)
{
    if (!interface.active) {
        return;
    }

    // disconnect VPN on an unsecured network -- connect VPN on a secured network if auto-connect is on
    if (interface.trustType == NETWORK_TRUST_UNSECURED && loginState_ == LOGIN_STATE_LOGGED_IN) {
        if (!connectStateHelper_.isDisconnected()) {
            qCInfo(LOG_BASIC) << "Network Whitelisting detected UNSECURED network -- Disconnecting..";
            sendDisconnect();
        }
    } else if (loginState_ == LOGIN_STATE_LOGGED_IN) { // SECURED
        if (preferences_.isAutoConnect() && connectStateHelper_.isDisconnected()) {
            qCInfo(LOG_BASIC) << "Network Whitelisting detected SECURED network -- Connecting..";
            if (PersistentState::instance().lastLocation().isValid()) {
                qCInfo(LOG_BASIC) << "Using last location: " << PersistentState::instance().lastLocation().getHashString();
                sendConnect(PersistentState::instance().lastLocation());
            } else {
                LocationID bestLocation = locationsModelManager_->getBestLocationId();
                if (bestLocation.isValid()) {
                    qCInfo(LOG_BASIC) << "Using best location: " << locationsModelManager_->getBestLocationId().getHashString();
                    sendConnect(locationsModelManager_->getBestLocationId());
                } else {
                    qCDebug(LOG_BASIC) << "No last location or best location found, not connecting.";
                }
            }
        }
    }
}

types::NetworkInterface Backend::getCurrentNetworkInterface()
{
    return currentNetworkInterface_;
}

void Backend::cycleMacAddress()
{
    QString macAddress = NetworkUtils::generateRandomMacAddress(); // TODO: move generation into Engine (BEWARE: mac change only occurs on preferences closing and when manual MAC cycled)

    types::MacAddrSpoofing mas = preferences_.macAddrSpoofing();
    mas.macAddress = macAddress;
    preferences_.setMacAddrSpoofing(mas);
}

void Backend::sendDetectPacketSize()
{
    engine_->detectAppropriatePacketSize();
}

void Backend::sendSplitTunneling(const types::SplitTunneling &st)
{
    bool isActive = st.settings.active;
    bool isExclude = st.settings.mode == SPLIT_TUNNELING_MODE_EXCLUDE;

    QStringList files;
    for (int i = 0; i < st.apps.size(); ++i) {
        if (st.apps[i].active) {
            files << st.apps[i].fullName;
        }
    }

    QStringList ips;
    QStringList hosts;
    for (int i = 0; i < st.networkRoutes.size(); ++i) {
        if (st.networkRoutes[i].type == SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_IP) {
            ips << st.networkRoutes[i].name;
        } else if (st.networkRoutes[i].type == SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_HOSTNAME) {
            hosts << st.networkRoutes[i].name;
        }
    }

    engine_->setSplitTunnelingSettings(isActive, isExclude, files, ips, hosts);
    emit splitTunnelingStateChanged(st.settings.active);
}

void Backend::sendUpdateWindowInfo(qint32 mainWindowCenterX, qint32 mainWindowCenterY)
{
    engine_->updateWindowInfo(mainWindowCenterX, mainWindowCenterY);
}

void Backend::sendUpdateVersion(qint64 mainWindowHandle)
{
    engine_->updateVersion(mainWindowHandle);
}

void Backend::cancelUpdateVersion()
{
    engine_->stopUpdateVersion();
}

void Backend::sendMakeHostsFilesWritableWin()
{
#ifdef Q_OS_WIN
        engine_->makeHostsFileWritableWin();
#endif
}

QString Backend::generateNewFriendlyName()
{
    QList<QString> friendlyNames;
    QVector<types::NetworkInterface> whiteList = preferences_.networkWhiteList();
    for (int i = 0; i < whiteList.size(); i++)
    {
        types::NetworkInterface network = whiteList[i];
        friendlyNames.append(network.friendlyName);
    }

    int newIndex = 0;
    QString newFriendlyName = "Network " + QString::number(newIndex);
    while (friendlyNames.contains(newFriendlyName))
    {
        newIndex++;
        newFriendlyName = "Network " + QString::number(newIndex);
    }

    return newFriendlyName;
}

void Backend::updateAccountInfo()
{
    accountInfo_.setEmail(latestSessionStatus_.getEmail());
    accountInfo_.setNeedConfirmEmail(latestSessionStatus_.getEmailStatus() == 0);
    accountInfo_.setUsername(latestSessionStatus_.getUsername());
    accountInfo_.setExpireDate(latestSessionStatus_.getPremiumExpireDate());
    accountInfo_.setLastReset(latestSessionStatus_.getLastResetDate());
    accountInfo_.setPlan(latestSessionStatus_.getTrafficMax());
    accountInfo_.setTrafficUsed(latestSessionStatus_.getTrafficUsed());
    accountInfo_.setIsPremium(latestSessionStatus_.isPremium());
    accountInfo_.setAlcCount(latestSessionStatus_.getAlc().size());
}

QString Backend::getAutoLoginCredential(const QString &key)
{
    QString credential;

#ifdef Q_OS_WIN
    try {
        QSettings settings;
        if (settings.contains(key)) {
            std::string encoded = settings.value(key).toString().toStdString();
            const auto decrypted = wsl::WinCryptUtils::decrypt(encoded, wsl::WinCryptUtils::EncodeHex);
            credential = QString::fromStdWString(decrypted);
        }
    }
    catch (std::system_error& ex) {
        qCCritical(LOG_BASIC) << "ApiInfo::getAutoLoginCredential() -" << ex.what() << ex.code().value();
    }
#endif

    return credential;
}

void Backend::clearAutoLoginCredentials()
{
    QSettings settings;
    if (settings.contains("username")) {
        settings.remove("username");
    }
    if (settings.contains("password")) {
        settings.remove("password");
    }
}

bool Backend::haveAutoLoginCredentials(QString &username, QString &password)
{
#ifdef Q_OS_WIN
    username = getAutoLoginCredential("username");
    password = getAutoLoginCredential("password");

    // Remove the auto-login credentials so we don't attempt to use them again.  If the user entered
    // incorrect auto-login credentials in the installer, we'll bounce them back to the login screen
    // and display the 'bad credentials' error when login is attempted.  If a connectivity/server
    // error prevents login, the login() method will have saved these creds and will retry them.
    clearAutoLoginCredentials();

    return (!username.isEmpty() && !password.isEmpty());
#else
    return false;
#endif
}

void Backend::onEngineAutoEnableAntiCensorship(bool enable)
{
    preferences_.setAntiCensorship(enable);
}

void Backend::onEngineConnectingHostnameChanged(const QString &hostname)
{
    currentConnectingHostname_ = hostname;
}

void Backend::onEngineBridgeApiAvailabilityChanged(bool isAvailable)
{
    bool enableIpUtils = false;

    if (isAvailable) {
        LocationID currentLoc = currentLocation();
        if (currentLoc.isValid() && !currentLoc.isStaticIpsLocation() && !currentLoc.isCustomConfigsLocation()) {
            const api_responses::SessionStatus& sessionStatus = getSessionStatus();

            if (sessionStatus.isPremium()) {
                enableIpUtils = true;
            } else if (!sessionStatus.getAlc().isEmpty()) {
                gui_locations::LocationsModel* locationsModel = static_cast<gui_locations::LocationsModel*>(locationsModelManager_->locationsModel());
                QModelIndex index = locationsModel->getIndexByLocationId(currentLoc);
                QModelIndex parent = locationsModel->parent(index);

                if (parent.isValid()) {
                    QString parentShortName = parent.data(gui_locations::kShortName).toString();
                    if (sessionStatus.getAlc().contains(parentShortName)) {
                        enableIpUtils = true;
                    }
                }
            }
        }
    }

    emit bridgeApiAvailabilityChanged(enableIpUtils);
}

void Backend::updateCurrentNetworkInterface()
{
    engine_->updateCurrentNetworkInterface();
}

void Backend::reconnect()
{
    engine_->reconnect();
}

bool Backend::osDnsServersListContains(const std::wstring &dnsServer)
{
    return std::find(osDnsServers_.begin(), osDnsServers_.end(), dnsServer) != osDnsServers_.end();
}

void Backend::rotateIp()
{
    engine_->rotateIp();
}

QString Backend::getCurrentConnectingHostname() const
{
    return currentConnectingHostname_;
}

void Backend::fetchControldDevices(const QString &apiKey)
{
    engine_->fetchControldDevices(apiKey);
}

