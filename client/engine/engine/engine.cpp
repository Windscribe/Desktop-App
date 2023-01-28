#include "engine.h"

#include <QCoreApplication>
#include <QDir>
#include <QCryptographicHash>
#include "utils/utils.h"
#include "utils/logger.h"
#include "utils/mergelog.h"
#include "utils/extraconfig.h"
#include "utils/ipvalidation.h"
#include "utils/executable_signature/executable_signature.h"
#include "connectionmanager/connectionmanager.h"
#include "connectionmanager/finishactiveconnections.h"
#include "proxy/proxyservercontroller.h"
#include "connectstatecontroller/connectstatecontroller.h"
#include "dnsresolver/dnsserversconfiguration.h"
#include "dnsresolver/dnsrequest.h"
#include "dnsresolver/dnsutils.h"
#include "crossplatformobjectfactory.h"
#include "openvpnversioncontroller.h"
#include "openvpnversioncontroller.h"
#include "getdeviceid.h"
#include "names.h"
#include "version/appversion.h"

// For testing merge log functionality
//#include <QStandardPaths>

#ifdef Q_OS_WIN
    #include "utils/bfe_service_win.h"
    #include "utils/winutils.h"
    #include "engine/dnsinfo_win.h"
    #include "engine/taputils/checkadapterenable.h"
    #include "engine/taputils/tapinstall_win.h"
    #include "engine/adaptermetricscontroller_win.h"
    #include "helper/helper_win.h"
#elif defined Q_OS_MAC
    #include "ipv6controller_mac.h"
    #include "utils/macutils.h"
    #include "utils/network_utils/network_utils_mac.h"
    #include "networkdetectionmanager/reachabilityevents.h"
#elif defined Q_OS_LINUX
    #include "helper/helper_linux.h"
    #include "utils/executable_signature/executablesignature_linux.h"
    #include "utils/dnsscripts_linux.h"
    #include "utils/linuxutils.h"
#endif

Engine::Engine(const EngineSettings &engineSettings) : QObject(nullptr),
    engineSettings_(engineSettings),
    helper_(nullptr),
    firewallController_(nullptr),
    networkAccessManager_(nullptr),
    serverAPI_(nullptr),
    connectionManager_(nullptr),
    connectStateController_(nullptr),
    serverApiUserRole_(0),
    serverApiEditAccountDetailsUserRole_(0),
    serverApiAddEmailUserRole_(0),
    getMyIPController_(nullptr),
    vpnShareController_(nullptr),
    emergencyController_(nullptr),
    customConfigs_(nullptr),
    customOvpnAuthCredentialsStorage_(nullptr),
    networkDetectionManager_(nullptr),
    macAddressController_(nullptr),
    keepAliveManager_(nullptr),
    packetSizeController_(nullptr),
#ifdef Q_OS_WIN
    measurementCpuUsage_(nullptr),
#endif
    inititalizeHelper_(nullptr),
    bInitialized_(false),
    loginController_(nullptr),
    loginState_(LOGIN_NONE),
    loginSettingsMutex_(QRecursiveMutex()),
    updateServerResourcesTimer_(nullptr),
    updateSessionStatusTimer_(nullptr),
    notificationsUpdateTimer_(nullptr),
    locationsModel_(nullptr),
    refetchServerCredentialsHelper_(nullptr),
    downloadHelper_(nullptr),
#ifdef Q_OS_MAC
    autoUpdaterHelper_(nullptr),
    robustMacSpoofTimer_(nullptr),
#endif
    isBlockConnect_(false),
    isCleanupFinished_(false),
    isNeedReconnectAfterRequestUsernameAndPassword_(false),
    online_(false),
    packetSizeControllerThread_(nullptr),
    runningPacketDetection_(false),
    lastDownloadProgress_(0),
    installerUrl_(""),
    guiWindowHandle_(0),
    overrideUpdateChannelWithInternal_(false),
    bPrevNetworkInterfaceInitialized_(false)
{
    connectStateController_ = new ConnectStateController(nullptr);
    connect(connectStateController_, SIGNAL(stateChanged(CONNECT_STATE,DISCONNECT_REASON,ProtoTypes::ConnectError,LocationID)), SLOT(onConnectStateChanged(CONNECT_STATE,DISCONNECT_REASON,ProtoTypes::ConnectError,LocationID)));
    emergencyConnectStateController_ = new ConnectStateController(nullptr);
    OpenVpnVersionController::instance().setUseWinTun(engineSettings.isUseWintun());
#ifdef Q_OS_LINUX
    DnsScripts_linux::instance().setDnsManager(engineSettings.getDnsManager());
#endif
}

Engine::~Engine()
{
    SAFE_DELETE(connectStateController_);
    SAFE_DELETE(emergencyConnectStateController_);
    packetSizeControllerThread_->exit();
    packetSizeControllerThread_->wait();
    packetSizeControllerThread_->deleteLater();
    packetSizeController_->deleteLater();
    qCDebug(LOG_BASIC) << "Engine destructor finished";
}

void Engine::setSettings(const EngineSettings &engineSettings)
{
    QMutexLocker locker(&mutex_);
    QMetaObject::invokeMethod(this, "setSettingsImpl", Q_ARG(EngineSettings, engineSettings));
}

void Engine::cleanup(bool isExitWithRestart, bool isFirewallChecked, bool isFirewallAlwaysOn, bool isLaunchOnStart)
{
    // Cannot use invokeMethod("cleanupImpl") here.  Any code called by cleanupImpl causing the message queue
    // to be processed (e.g. qApp->processEvents() in ConnectionManager::blockingDisconnect) would then cause
    // cleanupImpl to be invoked repeatedly before the initial call has completed.  One of the cleanupImpl calls
    // would SAFE_DELETE all the pointers, thereby causing the other pending calls to segfault.
    Q_EMIT initCleanup(isExitWithRestart, isFirewallChecked, isFirewallAlwaysOn, isLaunchOnStart);
}

bool Engine::isCleanupFinished()
{
    return isCleanupFinished_;
}

bool Engine::isInitialized()
{
    QMutexLocker locker(&mutex_);
    return bInitialized_;
}

void Engine::enableBFE_win()
{
    QMutexLocker locker(&mutex_);
    Q_ASSERT(bInitialized_);
    QMetaObject::invokeMethod(this, "enableBFE_winImpl");
}

void Engine::loginWithAuthHash(const QString &authHash)
{
    QMutexLocker locker(&mutex_);
    Q_ASSERT(bInitialized_);
    //Q_ASSERT(loginState_ != LOGIN_IN_PROGRESS);

    {
        QMutexLocker lockerLoginSettings(&loginSettingsMutex_);
        loginSettings_ = LoginSettings(authHash);
    }
    loginState_ = LOGIN_IN_PROGRESS;
    QMetaObject::invokeMethod(this, "loginImpl", Q_ARG(bool, false));
}

void Engine::loginWithUsernameAndPassword(const QString &username, const QString &password, const QString &code2fa)
{
    QMutexLocker locker(&mutex_);
    Q_ASSERT(bInitialized_);
    Q_ASSERT(loginState_ != LOGIN_IN_PROGRESS);

    {
        QMutexLocker lockerLoginSettings(&loginSettingsMutex_);
        loginSettings_ = LoginSettings(username, password, code2fa);
    }
    loginState_ = LOGIN_IN_PROGRESS;
    QMetaObject::invokeMethod(this, "loginImpl", Q_ARG(bool, false));
}

void Engine::loginWithLastLoginSettings()
{
    QMutexLocker locker(&mutex_);
    Q_ASSERT(bInitialized_);
    Q_ASSERT(loginState_ != LOGIN_IN_PROGRESS);

    loginState_ = LOGIN_IN_PROGRESS;
    QMetaObject::invokeMethod(this, "loginImpl", Q_ARG(bool, true));
}

bool Engine::isApiSavedSettingsExists()
{
    if (!apiinfo::ApiInfo::getAuthHash().isEmpty())
    {
        // try load ApiInfo from settings
        apiinfo::ApiInfo apiInfo;
        if (apiInfo.loadFromSettings())
        {
            return true;
        }
    }

    return false;
}

void Engine::signOut(bool keepFirewallOn)
{
    QMetaObject::invokeMethod(this, "signOutImpl", Q_ARG(bool, keepFirewallOn));
}

void Engine::gotoCustomOvpnConfigMode()
{
    QMetaObject::invokeMethod(this, "gotoCustomOvpnConfigModeImpl");
}

void Engine::continueWithUsernameAndPassword(const QString &username, const QString &password, bool bSave)
{
    QMetaObject::invokeMethod(this, "continueWithUsernameAndPasswordImpl", Q_ARG(QString, username),
                              Q_ARG(QString, password), Q_ARG(bool, bSave));
}

void Engine::continueWithPassword(const QString &password, bool bSave)
{
    QMetaObject::invokeMethod(this, "continueWithPasswordImpl", Q_ARG(QString, password), Q_ARG(bool, bSave));
}

void Engine::sendDebugLog()
{
    QMetaObject::invokeMethod(this, "sendDebugLogImpl");
}

void Engine::setIPv6EnabledInOS(bool b)
{
#ifdef Q_OS_WIN
    QMutexLocker locker(&mutex_);
    Helper_win *helper_win = dynamic_cast<Helper_win *>(helper_);
    helper_win->setIPv6EnabledInOS(b);
#else
    Q_UNUSED(b)
#endif
}

bool Engine::IPv6StateInOS()
{
#ifdef Q_OS_WIN
    QMutexLocker locker(&mutex_);
    Helper_win *helper_win = dynamic_cast<Helper_win *>(helper_);
    return helper_win->IPv6StateInOS();
#else
    return true;
#endif
}

void Engine::getWebSessionToken(ProtoTypes::WebSessionPurpose purpose)
{
    QMetaObject::invokeMethod(this, "getWebSessionTokenImpl", Q_ARG(ProtoTypes::WebSessionPurpose, purpose));
}

LoginSettings Engine::getLastLoginSettings()
{
    QMutexLocker lockerLoginSettings(&loginSettingsMutex_);
    return loginSettings_;
}

QString Engine::getAuthHash()
{
    return apiinfo::ApiInfo::getAuthHash();
}

void Engine::clearCredentials()
{
    QMetaObject::invokeMethod(this, "clearCredentialsImpl");
}

locationsmodel::LocationsModel *Engine::getLocationsModel()
{
    Q_ASSERT(locationsModel_ != NULL);
    return locationsModel_;
}

IConnectStateController *Engine::getConnectStateController()
{
    Q_ASSERT(connectStateController_ != NULL);
    return connectStateController_;
}

bool Engine::isFirewallEnabled()
{
    QMutexLocker locker(&mutex_);
    Q_ASSERT(bInitialized_);
    if (bInitialized_)
    {
        return firewallController_->firewallActualState();
    }
    else
    {
        return false;
    }
}

bool Engine::firewallOn()
{
    QMutexLocker locker(&mutex_);
    Q_ASSERT(bInitialized_);
    QMetaObject::invokeMethod(this, "firewallOnImpl");
    return true;
}

bool Engine::firewallOff()
{
    QMutexLocker locker(&mutex_);
    Q_ASSERT(bInitialized_);
    QMetaObject::invokeMethod(this, "firewallOffImpl");
    return true;
}

void Engine::connectClick(const LocationID &locationId)
{
    QMutexLocker locker(&mutex_);
    if (bInitialized_)
    {
        locationId_ = locationId;
        connectStateController_->setConnectingState(locationId_);
        QMetaObject::invokeMethod(this, "connectClickImpl", Q_ARG(LocationID, locationId));
    }
}

void Engine::disconnectClick()
{
    QMutexLocker locker(&mutex_);
    if (bInitialized_)
    {
        if (connectStateController_->currentState() == CONNECT_STATE_CONNECTED || connectStateController_->currentState() == CONNECT_STATE_CONNECTING)
        {
            connectStateController_->setDisconnectingState();
            QMetaObject::invokeMethod(this, "disconnectClickImpl");
        }
    }
}

bool Engine::isBlockConnect() const
{
    return isBlockConnect_;
}

void Engine::setBlockConnect(bool isBlockConnect)
{
    isBlockConnect_ = isBlockConnect;
}

void Engine::recordInstall()
{
    QMutexLocker locker(&mutex_);
    QMetaObject::invokeMethod(this, "recordInstallImpl");
}

void Engine::sendConfirmEmail()
{
    QMutexLocker locker(&mutex_);
    QMetaObject::invokeMethod(this, "sendConfirmEmailImpl");
}

void Engine::speedRating(int rating, const QString &localExternalIp)
{
    QMutexLocker locker(&mutex_);
    if (bInitialized_)
    {
        QMetaObject::invokeMethod(this, "speedRatingImpl", Q_ARG(int, rating), Q_ARG(QString, localExternalIp));
    }
}

void Engine::updateServerConfigs()
{
    QMutexLocker locker(&mutex_);
    if (bInitialized_)
    {
        QMetaObject::invokeMethod(this, "updateServerConfigsImpl");
    }
}

void Engine::emergencyConnectClick()
{
    QMutexLocker locker(&mutex_);
    if (bInitialized_)
    {
        emergencyConnectStateController_->setConnectingState(LocationID());
        QMetaObject::invokeMethod(this, "emergencyConnectClickImpl");
    }
    else
    {
        emergencyConnectStateController_->setDisconnectedState(DISCONNECTED_ITSELF, ProtoTypes::ConnectError::NO_CONNECT_ERROR);
        Q_EMIT emergencyDisconnected();
    }
}

void Engine::emergencyDisconnectClick()
{
    QMutexLocker locker(&mutex_);
    if (bInitialized_)
    {
        emergencyConnectStateController_->setDisconnectingState();
        QMetaObject::invokeMethod(this, "emergencyDisconnectClickImpl");
    }
    else
    {
        emergencyConnectStateController_->setDisconnectedState(DISCONNECTED_ITSELF, ProtoTypes::ConnectError::NO_CONNECT_ERROR);
        Q_EMIT emergencyDisconnected();
    }
}

bool Engine::isEmergencyDisconnected()
{
    QMutexLocker locker(&mutex_);
    return emergencyConnectStateController_->currentState() == CONNECT_STATE_DISCONNECTED;
}

bool Engine::isWifiSharingSupported()
{
    QMutexLocker locker(&mutex_);
    Q_ASSERT(bInitialized_);
    if (bInitialized_)
    {
        return vpnShareController_->isWifiSharingSupported();
    }
    else
    {
        return false;
    }
}

void Engine::startWifiSharing(const QString &ssid, const QString &password)
{
    QMutexLocker locker(&mutex_);
    Q_ASSERT(bInitialized_);
    if (bInitialized_)
    {
        QMetaObject::invokeMethod(this, "startWifiSharingImpl", Q_ARG(QString, ssid), Q_ARG(QString, password));
    }
}

void Engine::stopWifiSharing()
{
    QMutexLocker locker(&mutex_);
    Q_ASSERT(bInitialized_);
    if (bInitialized_)
    {
        QMetaObject::invokeMethod(this, "stopWifiSharingImpl");
    }
}

void Engine::startProxySharing(PROXY_SHARING_TYPE proxySharingType)
{
    QMutexLocker locker(&mutex_);
    Q_ASSERT(bInitialized_);
    if (bInitialized_)
    {
        QMetaObject::invokeMethod(this, "startProxySharingImpl", Q_ARG(PROXY_SHARING_TYPE, proxySharingType));
    }
}

void Engine::stopProxySharing()
{
    QMutexLocker locker(&mutex_);
    Q_ASSERT(bInitialized_);
    if (bInitialized_)
    {
        QMetaObject::invokeMethod(this, "stopProxySharingImpl");
    }
}

QString Engine::getProxySharingAddress()
{
    QMutexLocker locker(&mutex_);
    Q_ASSERT(bInitialized_);
    if (bInitialized_)
    {
        return vpnShareController_->getProxySharingAddress();
    }
    else
    {
        return "";
    }
}

QString Engine::getSharingCaption()
{
    QMutexLocker locker(&mutex_);
    Q_ASSERT(bInitialized_);
    if (bInitialized_)
    {
        return vpnShareController_->getCurrentCaption();
    }
    else
    {
        return "";
    }
}

void Engine::applicationActivated()
{
    QMetaObject::invokeMethod(this, "applicationActivatedImpl");
}

void Engine::applicationDeactivated()
{
    QMetaObject::invokeMethod(this, "applicationDeactivatedImpl");
}

void Engine::forceUpdateServerLocations()
{
    QMetaObject::invokeMethod(this, "forceUpdateServerLocationsImpl");
}

void Engine::updateCurrentInternetConnectivity()
{
    QMetaObject::invokeMethod(this, "updateCurrentInternetConnectivityImpl");
}

void Engine::detectAppropriatePacketSize()
{
    QMetaObject::invokeMethod(this, "detectAppropriatePacketSizeImpl");
}

void Engine::setSettingsMacAddressSpoofing(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing)
{
    QMetaObject::invokeMethod(this, "setSettingsMacAddressSpoofingImpl", Q_ARG(ProtoTypes::MacAddrSpoofing, macAddrSpoofing));
}

void Engine::setSplitTunnelingSettings(bool isActive, bool isExclude, const QStringList &files,
                                       const QStringList &ips, const QStringList &hosts)
{
    QMetaObject::invokeMethod(this, "setSplitTunnelingSettingsImpl", Q_ARG(bool, isActive),
                              Q_ARG(bool, isExclude), Q_ARG(QStringList, files),
                              Q_ARG(QStringList, ips), Q_ARG(QStringList, hosts));
}

void Engine::updateWindowInfo(qint32 windowCenterX, qint32 windowCenterY)
{
    QMetaObject::invokeMethod(this, "updateWindowInfoImpl",
                              Q_ARG(qint32, windowCenterX), Q_ARG(qint32, windowCenterY));
}

void Engine::updateVersion(qint32 windowHandle)
{
    QMetaObject::invokeMethod(this, "updateVersionImpl", Q_ARG(qint32, windowHandle));
}

void Engine::updateAdvancedParams()
{
    QMetaObject::invokeMethod(this, "updateAdvancedParamsImpl");
}

void Engine::stopUpdateVersion()
{
    QMetaObject::invokeMethod(this, "stopUpdateVersionImpl");
}

void Engine::makeHostsFileWritableWin()
{
#ifdef Q_OS_WIN
    const auto winHelper = dynamic_cast<Helper_win*>(helper_);
    if(winHelper) {
        if(winHelper->makeHostsFileWritable()) {
            Q_EMIT hostsFileBecameWritable();
        }
        else {
            qCDebug(LOG_BASIC) << "Error: was not able to make 'hosts' file writable.";
        }
    }
#endif
}

void Engine::init()
{
#ifdef Q_OS_WIN
    HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        qCDebug(LOG_BASIC) << "Error: CoInitializeEx failed:" << hr;
    }
#endif

    isCleanupFinished_ = false;
    connect(this, &Engine::initCleanup, this, &Engine::cleanupImpl);

    helper_ = CrossPlatformObjectFactory::createHelper(this);
    connect(helper_, SIGNAL(lostConnectionToHelper()), SLOT(onLostConnectionToHelper()));
    helper_->startInstallHelper();

    inititalizeHelper_ = new InitializeHelper(this, helper_);
    connect(inititalizeHelper_, SIGNAL(finished(INIT_HELPER_RET)), SLOT(onInitializeHelper(INIT_HELPER_RET)));
    inititalizeHelper_->start();
}

// init part2 (after helper initialized)
void Engine::initPart2()
{
#ifdef Q_OS_MAC
    Ipv6Controller_mac::instance().setHelper(helper_);
    ReachAbilityEvents::instance().init();
#endif

    networkDetectionManager_ = CrossPlatformObjectFactory::createNetworkDetectionManager(this, helper_);

    DnsServersConfiguration::instance().setDnsServersPolicy(engineSettings_.getDnsPolicy());
    firewallExceptions_.setDnsPolicy(engineSettings_.getDnsPolicy());

    ProtoTypes::MacAddrSpoofing macAddrSpoofing = engineSettings_.getMacAddrSpoofing();
    //todo refactor
#ifdef Q_OS_MAC
    *macAddrSpoofing.mutable_network_interfaces() = NetworkUtils_mac::currentNetworkInterfaces(true);
#elif defined Q_OS_WIN
    *macAddrSpoofing.mutable_network_interfaces() = WinUtils::currentNetworkInterfaces(true);
#elif define Q_OS_LINUX
    todo
#endif
    setSettingsMacAddressSpoofing(macAddrSpoofing);

    connect(networkDetectionManager_, SIGNAL(onlineStateChanged(bool)), SLOT(onNetworkOnlineStateChange(bool)));
    connect(networkDetectionManager_, SIGNAL(networkChanged(ProtoTypes::NetworkInterface)), SLOT(onNetworkChange(ProtoTypes::NetworkInterface)));

    macAddressController_ = CrossPlatformObjectFactory::createMacAddressController(this, networkDetectionManager_, helper_);
    macAddressController_->initMacAddrSpoofing(macAddrSpoofing);
    connect(macAddressController_, SIGNAL(macAddrSpoofingChanged(ProtoTypes::MacAddrSpoofing)), SLOT(onMacAddressSpoofingChanged(ProtoTypes::MacAddrSpoofing)));
    connect(macAddressController_, SIGNAL(sendUserWarning(ProtoTypes::UserWarningType)), SLOT(onMacAddressControllerSendUserWarning(ProtoTypes::UserWarningType)));
#ifdef Q_OS_MAC
    connect(macAddressController_, SIGNAL(robustMacSpoofApplied()), SLOT(onMacAddressControllerRobustMacSpoofApplied()));
#endif

    packetSizeControllerThread_ = new QThread(this);

    ProtoTypes::PacketSize packetSize = engineSettings_.getPacketSize();
    packetSizeController_ = new PacketSizeController(nullptr);
    packetSizeController_->setPacketSize(packetSize);
    packetSize_ = packetSize;
    connect(packetSizeController_, SIGNAL(packetSizeChanged(bool, int)), SLOT(onPacketSizeControllerPacketSizeChanged(bool, int)));
    connect(packetSizeController_, SIGNAL(finishedPacketSizeDetection(bool)), SLOT(onPacketSizeControllerFinishedSizeDetection(bool)));
    packetSizeController_->moveToThread(packetSizeControllerThread_);
    packetSizeControllerThread_->start(QThread::LowPriority);

    firewallController_ = CrossPlatformObjectFactory::createFirewallController(this, helper_);

    networkAccessManager_ = new NetworkAccessManager(this);
    connect(networkAccessManager_, SIGNAL(whitelistIpsChanged(QSet<QString>)), SLOT(onWhitelistedIPsChanged(QSet<QString>)));

    serverAPI_ = new ServerAPI(this);
    connect(serverAPI_, &ServerAPI::sessionAnswer, this, &Engine::onSessionAnswer, Qt::QueuedConnection);
    connect(serverAPI_, SIGNAL(checkUpdateAnswer(apiinfo::CheckUpdate,bool,uint)),
                        SLOT(onCheckUpdateAnswer(apiinfo::CheckUpdate,bool,uint)), Qt::QueuedConnection);
    connect(serverAPI_, SIGNAL(hostIpsChanged(QStringList)), SLOT(onHostIPsChanged(QStringList)));
    connect(serverAPI_, SIGNAL(notificationsAnswer(SERVER_API_RET_CODE,QVector<apiinfo::Notification>,uint)),
                        SLOT(onNotificationsAnswer(SERVER_API_RET_CODE,QVector<apiinfo::Notification>,uint)));
    connect(serverAPI_, SIGNAL(serverConfigsAnswer(SERVER_API_RET_CODE,QString,uint)), SLOT(onServerConfigsAnswer(SERVER_API_RET_CODE,QString,uint)));
    connect(serverAPI_, SIGNAL(debugLogAnswer(SERVER_API_RET_CODE,uint)), SLOT(onDebugLogAnswer(SERVER_API_RET_CODE,uint)));
    connect(serverAPI_, SIGNAL(confirmEmailAnswer(SERVER_API_RET_CODE,uint)), SLOT(onConfirmEmailAnswer(SERVER_API_RET_CODE,uint)));
    connect(serverAPI_, SIGNAL(webSessionAnswer(SERVER_API_RET_CODE, QString, uint)), SLOT(onWebSessionAnswer(SERVER_API_RET_CODE, QString, uint)));
    connect(serverAPI_, SIGNAL(staticIpsAnswer(SERVER_API_RET_CODE,apiinfo::StaticIps, uint)), SLOT(onStaticIpsAnswer(SERVER_API_RET_CODE,apiinfo::StaticIps, uint)), Qt::QueuedConnection);
    connect(serverAPI_, SIGNAL(serverLocationsAnswer(SERVER_API_RET_CODE, QVector<apiinfo::Location>,QStringList, uint)),
                        SLOT(onServerLocationsAnswer(SERVER_API_RET_CODE,QVector<apiinfo::Location>,QStringList, uint)), Qt::QueuedConnection);
    connect(serverAPI_, SIGNAL(sendUserWarning(ProtoTypes::UserWarningType)), SIGNAL(sendUserWarning(ProtoTypes::UserWarningType)));

    serverAPI_->setIgnoreSslErrors(engineSettings_.isIgnoreSslErrors());
    serverApiUserRole_ = serverAPI_->getAvailableUserRole();
    serverApiEditAccountDetailsUserRole_ = serverAPI_->getAvailableUserRole();
    serverApiAddEmailUserRole_ = serverAPI_->getAvailableUserRole();

    customOvpnAuthCredentialsStorage_ = new CustomOvpnAuthCredentialsStorage();

    connectionManager_ = new ConnectionManager(this, helper_, networkDetectionManager_, serverAPI_, customOvpnAuthCredentialsStorage_);
    connectionManager_->setPacketSize(packetSize_);
    connectionManager_->setDnsWhileConnectedInfo(engineSettings_.getDnsWhileConnectedInfo());
    connect(connectionManager_, SIGNAL(connected()), SLOT(onConnectionManagerConnected()));
    connect(connectionManager_, SIGNAL(disconnected(DISCONNECT_REASON)), SLOT(onConnectionManagerDisconnected(DISCONNECT_REASON)));
    connect(connectionManager_, SIGNAL(reconnecting()), SLOT(onConnectionManagerReconnecting()));
    connect(connectionManager_, SIGNAL(errorDuringConnection(ProtoTypes::ConnectError)), SLOT(onConnectionManagerError(ProtoTypes::ConnectError)));
    connect(connectionManager_, SIGNAL(statisticsUpdated(quint64,quint64, bool)), SLOT(onConnectionManagerStatisticsUpdated(quint64,quint64, bool)));
    connect(connectionManager_, SIGNAL(interfaceUpdated(QString)), SLOT(onConnectionManagerInterfaceUpdated(QString)));
    connect(connectionManager_, SIGNAL(testTunnelResult(bool, QString)), SLOT(onConnectionManagerTestTunnelResult(bool, QString)));
    connect(connectionManager_, SIGNAL(connectingToHostname(QString, QString, QString)), SLOT(onConnectionManagerConnectingToHostname(QString, QString, QString)));
    connect(connectionManager_, SIGNAL(protocolPortChanged(ProtoTypes::Protocol, uint)), SLOT(onConnectionManagerProtocolPortChanged(ProtoTypes::Protocol, uint)));
    connect(connectionManager_, SIGNAL(internetConnectivityChanged(bool)), SLOT(onConnectionManagerInternetConnectivityChanged(bool)));
    connect(connectionManager_, SIGNAL(wireGuardAtKeyLimit()), SLOT(onConnectionManagerWireGuardAtKeyLimit()));
    connect(connectionManager_, SIGNAL(requestUsername(QString)), SLOT(onConnectionManagerRequestUsername(QString)));
    connect(connectionManager_, SIGNAL(requestPassword(QString)), SLOT(onConnectionManagerRequestPassword(QString)));

    locationsModel_ = new locationsmodel::LocationsModel(this, connectStateController_, networkDetectionManager_);
    connect(locationsModel_, SIGNAL(whitelistLocationsIpsChanged(QStringList)), SLOT(onLocationsModelWhitelistIpsChanged(QStringList)));
    connect(locationsModel_, SIGNAL(whitelistCustomConfigsIpsChanged(QStringList)), SLOT(onLocationsModelWhitelistCustomConfigIpsChanged(QStringList)));

    getMyIPController_ = new GetMyIPController(this, serverAPI_, networkDetectionManager_);
    connect(getMyIPController_, SIGNAL(answerMyIP(QString,bool,bool)), SLOT(onMyIpAnswer(QString,bool,bool)));

    vpnShareController_ = new VpnShareController(this, helper_);
    connect(vpnShareController_, SIGNAL(connectedWifiUsersChanged(int)), SIGNAL(vpnSharingConnectedWifiUsersCountChanged(int)));
    connect(vpnShareController_, SIGNAL(connectedProxyUsersChanged(int)), SIGNAL(vpnSharingConnectedProxyUsersCountChanged(int)));

    keepAliveManager_ = new KeepAliveManager(this, connectStateController_);
    keepAliveManager_->setEnabled(engineSettings_.isKeepAliveEnabled());

    emergencyController_ = new EmergencyController(this, helper_);
    emergencyController_->setPacketSize(packetSize_);
    connect(emergencyController_, SIGNAL(connected()), SLOT(onEmergencyControllerConnected()));
    connect(emergencyController_, SIGNAL(disconnected(DISCONNECT_REASON)), SLOT(onEmergencyControllerDisconnected(DISCONNECT_REASON)));
    connect(emergencyController_, SIGNAL(errorDuringConnection(ProtoTypes::ConnectError)), SLOT(onEmergencyControllerError(ProtoTypes::ConnectError)));

    customConfigs_ = new customconfigs::CustomConfigs(this);
    customConfigs_->changeDir(engineSettings_.getCustomOvpnConfigsPath());
    connect(customConfigs_, SIGNAL(changed()), SLOT(onCustomConfigsChanged()));

    updateServerResourcesTimer_ = new QTimer(this);
    connect(updateServerResourcesTimer_, &QTimer::timeout, this, &Engine::onUpdateServerResources);
    updateSessionStatusTimer_ = new SessionStatusTimer(this, connectStateController_);
    connect(updateSessionStatusTimer_, SIGNAL(needUpdateRightNow()), SLOT(onUpdateSessionStatusTimer()));
    notificationsUpdateTimer_ = new QTimer(this);
    connect(notificationsUpdateTimer_, SIGNAL(timeout()), SLOT(getNewNotifications()));

    downloadHelper_ = new DownloadHelper(this, networkAccessManager_, Utils::getPlatformName());
    connect(downloadHelper_, SIGNAL(finished(DownloadHelper::DownloadState)), SLOT(onDownloadHelperFinished(DownloadHelper::DownloadState)));
    connect(downloadHelper_, SIGNAL(progressChanged(uint)), SLOT(onDownloadHelperProgressChanged(uint)));

#ifdef Q_OS_MAC
    autoUpdaterHelper_ = new AutoUpdaterHelper_mac();

    robustMacSpoofTimer_ = new QTimer(this);
    connect(robustMacSpoofTimer_, SIGNAL(timeout()), SLOT(onRobustMacSpoofTimerTick()));
    robustMacSpoofTimer_->setInterval(1000);
#endif

#ifdef Q_OS_WIN
    measurementCpuUsage_ = new MeasurementCpuUsage(this, helper_, connectStateController_);
    connect(measurementCpuUsage_, SIGNAL(detectionCpuUsageAfterConnected(QStringList)), SIGNAL(detectionCpuUsageAfterConnected(QStringList)));
    measurementCpuUsage_->setEnabled(engineSettings_.isCloseTcpSockets());
#endif

    updateProxySettings();
    updateAdvancedParams();
}

void Engine::onLostConnectionToHelper()
{
    Q_EMIT lostConnectionToHelper();
}

void Engine::onInitializeHelper(INIT_HELPER_RET ret)
{
    if (ret == INIT_HELPER_SUCCESS)
    {
        QMutexLocker locker(&mutex_);
        bInitialized_ = true;

        initPart2();

        FinishActiveConnections::finishAllActiveConnections(helper_);

#ifdef Q_OS_MAC
        QString kextPath = QCoreApplication::applicationDirPath() + "/../Helpers/WindscribeKext.kext";
        kextPath = QDir::cleanPath(kextPath);
        Helper_mac *helper_mac = dynamic_cast<Helper_mac *>(helper_);
        if (helper_mac->setKextPath(kextPath))
        {
            qCDebug(LOG_BASIC) << "Kext path set:" << Utils::cleanSensitiveInfo(kextPath);
        }
        else
        {
            qCDebug(LOG_BASIC) << "Kext path set failed";
            Q_EMIT initFinished(ENGINE_INIT_HELPER_FAILED);
        }
#endif

    // turn off split tunneling (for case the state remains from the last launch)
    helper_->sendConnectStatus(false, engineSettings_.isCloseTcpSockets(), engineSettings_.isAllowLanTraffic(), AdapterGatewayInfo(), AdapterGatewayInfo(), QString(), ProtocolType());
    helper_->setSplitTunnelingSettings(false, false, false, QStringList(), QStringList(), QStringList());


    #ifdef Q_OS_WIN
        // check BFE service status
        if (!BFE_Service_win::instance().isBFEEnabled())
        {
            Q_EMIT initFinished(ENGINE_INIT_BFE_SERVICE_FAILED);
        }
        else
        {
            Q_EMIT initFinished(ENGINE_INIT_SUCCESS);
        }
    #else
        Q_EMIT initFinished(ENGINE_INIT_SUCCESS);
    #endif
    }
    else if (ret == INIT_HELPER_FAILED)
    {
        Q_EMIT initFinished(ENGINE_INIT_HELPER_FAILED);
    }
    else if (ret == INIT_HELPER_USER_CANCELED)
    {
        Q_EMIT initFinished(ENGINE_INIT_HELPER_USER_CANCELED);
    }
    else
    {
        Q_ASSERT(false);
    }
}

void Engine::cleanupImpl(bool isExitWithRestart, bool isFirewallChecked, bool isFirewallAlwaysOn, bool isLaunchOnStart)
{
    // Ensure this slot only gets invoked once.
    disconnect(this, &Engine::initCleanup, nullptr, nullptr);

    if (isCleanupFinished_) {
        qCDebug(LOG_BASIC) << "WARNING - Engine::cleanupImpl called repeatedly. Verify code logic as this should not happen.";
        return;
    }

    qCDebug(LOG_BASIC) << "Cleanup started";

    if (loginController_)
    {
        SAFE_DELETE(loginController_);
        loginState_ = LOGIN_NONE;
    }

    // stop timers
    if (updateServerResourcesTimer_)
    {
        updateServerResourcesTimer_->stop();
    }
    if (updateSessionStatusTimer_)
    {
        updateSessionStatusTimer_->stop();
    }
    if (notificationsUpdateTimer_)
    {
        notificationsUpdateTimer_->stop();
    }

    if (!apiInfo_.isNull())
    {
        apiInfo_->saveToSettings();
    }

    // to skip blocking executeRootCommand() calls
    if (helper_)
    {
        helper_->setNeedFinish();
    }

    if (emergencyController_)
    {
        emergencyController_->blockingDisconnect();
    }

    if (connectionManager_)
    {
        bool bWasIsConnected = !connectionManager_->isDisconnected();
        connectionManager_->blockingDisconnect();
        if (bWasIsConnected)
        {
            #ifdef Q_OS_WIN
                DnsInfo_win::outputDebugDnsInfo();
            #endif
            qCDebug(LOG_BASIC) << "Cleanup, connection manager disconnected";
        }
        else
        {
            qCDebug(LOG_BASIC) << "Cleanup, connection manager no need disconnect";
        }

        connectionManager_->removeIkev2ConnectionFromOS();
    }

    // turn off split tunneling
    if (helper_)
    {
        helper_->sendConnectStatus(false, engineSettings_.isCloseTcpSockets(), engineSettings_.isAllowLanTraffic(), AdapterGatewayInfo(), AdapterGatewayInfo(), QString(), ProtocolType());
        helper_->setSplitTunnelingSettings(false, false, false, QStringList(), QStringList(), QStringList());
    }

#ifdef Q_OS_WIN
    Helper_win *helper_win = dynamic_cast<Helper_win *>(helper_);
    if (helper_win) {
        helper_win->removeWindscribeNetworkProfiles();
    }
#endif

    if (!isExitWithRestart)
    {
        if (vpnShareController_)
        {
            vpnShareController_->stopWifiSharing();
            vpnShareController_->stopProxySharing();
        }
    }

    if (helper_ && firewallController_)
    {
        if (isFirewallChecked)
        {
            if (isExitWithRestart)
            {
                if (isLaunchOnStart)
                {
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
                    firewallController_->enableFirewallOnBoot(true);
#endif
                }
                else
                {
                    if (isFirewallAlwaysOn)
                    {
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
                        firewallController_->enableFirewallOnBoot(true);
#endif
                    }
                    else
                    {
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
                        firewallController_->enableFirewallOnBoot(false);
#endif
                        firewallController_->firewallOff();
                    }
                }
            }
            else  // if exit without restart
            {
                if (isFirewallAlwaysOn)
                {
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
                    firewallController_->enableFirewallOnBoot(true);
#endif
                }
                else
                {
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
                    firewallController_->enableFirewallOnBoot(false);
#endif
                    firewallController_->firewallOff();
                }
            }
        }
        else  // if (!isFirewallChecked)
        {
            firewallController_->firewallOff();
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
            firewallController_->enableFirewallOnBoot(false);
#endif
        }
#ifdef Q_OS_WIN
        Helper_win *helper_win = dynamic_cast<Helper_win *>(helper_);
        helper_win->setIPv6EnabledInFirewall(true);
#endif

#ifdef Q_OS_MAC
        Ipv6Controller_mac::instance().restoreIpv6();
#endif
    }

    SAFE_DELETE(vpnShareController_);
    SAFE_DELETE(emergencyController_);
    SAFE_DELETE(connectionManager_);
    SAFE_DELETE(customConfigs_);
    SAFE_DELETE(customOvpnAuthCredentialsStorage_);
    SAFE_DELETE(firewallController_);
    SAFE_DELETE(keepAliveManager_);
    SAFE_DELETE(inititalizeHelper_);
#ifdef Q_OS_WIN
    SAFE_DELETE(measurementCpuUsage_);
#endif
    SAFE_DELETE(helper_);
    SAFE_DELETE(getMyIPController_);
    SAFE_DELETE(serverAPI_);
    SAFE_DELETE(updateServerResourcesTimer_);
    SAFE_DELETE(updateSessionStatusTimer_);
    SAFE_DELETE(notificationsUpdateTimer_);
    SAFE_DELETE(locationsModel_);
    SAFE_DELETE(networkDetectionManager_);
    SAFE_DELETE(downloadHelper_);
    SAFE_DELETE(networkAccessManager_);
    isCleanupFinished_ = true;
    Q_EMIT cleanupFinished();
    qCDebug(LOG_BASIC) << "Cleanup finished";
}

void Engine::clearCredentialsImpl()
{
    if (!apiInfo_.isNull())
    {
        apiInfo_->setServerCredentials(apiinfo::ServerCredentials());
    }
}

void Engine::enableBFE_winImpl()
{
#ifdef Q_OS_WIN
    bool bSuccess = BFE_Service_win::instance().checkAndEnableBFE(helper_);
    if (bSuccess)
    {
        Q_EMIT bfeEnableFinished(ENGINE_INIT_SUCCESS);
    }
    else
    {
        Q_EMIT bfeEnableFinished(ENGINE_INIT_BFE_SERVICE_FAILED);
    }
#endif
}

void Engine::loginImpl(bool bSkipLoadingFromSettings)
{
    QMutexLocker lockerLoginSettings(&loginSettingsMutex_);

    QString authHash = apiinfo::ApiInfo::getAuthHash();
    if (!bSkipLoadingFromSettings && !authHash.isEmpty())
    {
        apiInfo_.reset(new apiinfo::ApiInfo());

        // try load ApiInfo from settings
        if (apiInfo_->loadFromSettings())
        {
            apiInfo_->setAuthHash(authHash);
            loginSettings_.setServerCredentials(apiInfo_->getServerCredentials());

            qCDebug(LOG_BASIC) << "ApiInfo readed from settings";
            prevSessionStatus_ = apiInfo_->getSessionStatus();

            updateSessionStatus();
            updateServerLocations();
            updateCurrentNetworkInterfaceImpl();
            Q_EMIT loginFinished(true, authHash, apiInfo_->getPortMap());
        }
    }

    startLoginController(loginSettings_, false);
}

void Engine::setIgnoreSslErrorsImlp(bool bIgnoreSslErrors)
{
    serverAPI_->setIgnoreSslErrors(bIgnoreSslErrors);
}

void Engine::recordInstallImpl()
{
    serverAPI_->recordInstall(serverApiUserRole_, true);
}

void Engine::sendConfirmEmailImpl()
{
    if (!apiInfo_.isNull())
    {
        serverAPI_->confirmEmail(serverApiUserRole_, apiInfo_->getAuthHash(), true);
    }
}

void Engine::connectClickImpl(const LocationID &locationId)
{
    locationId_ = locationId;

    // if connected, then first disconnect
    if (!connectionManager_->isDisconnected())
    {
        connectionManager_->setProperty("senderSource", "reconnect");
        connectionManager_->clickDisconnect();
        return;
    }

    if (isBlockConnect_ && !locationId_.isCustomConfigsLocation())
    {
        connectStateController_->setDisconnectedState(DISCONNECTED_WITH_ERROR, ProtoTypes::ConnectError::CONNECTION_BLOCKED);
        getMyIPController_->getIPFromDisconnectedState(1);
        return;
    }

    addCustomRemoteIpToFirewallIfNeed();

#ifdef Q_OS_WIN
    DnsInfo_win::outputDebugDnsInfo();
#elif defined Q_OS_MAC
    Ipv6Controller_mac::instance().disableIpv6();
#endif

    SAFE_DELETE(refetchServerCredentialsHelper_);

    if (engineSettings_.firewallSettings().mode() == ProtoTypes::FIREWALL_MODE_AUTOMATIC && engineSettings_.firewallSettings().when() == ProtoTypes::FIREWALL_WHEN_BEFORE_CONNECTION)
    {
        bool bFirewallStateOn = firewallController_->firewallActualState();
        if (!bFirewallStateOn)
        {
            qCDebug(LOG_BASIC) << "Automatic enable firewall before connection";
            QString ips = firewallExceptions_.getIPAddressesForFirewall();
            firewallController_->firewallOn(ips, engineSettings_.isAllowLanTraffic());
            Q_EMIT firewallStateChanged(true);
        }
    }
    doConnect(true);
}

void Engine::disconnectClickImpl()
{
    SAFE_DELETE(refetchServerCredentialsHelper_);
    connectionManager_->setProperty("senderSource", QVariant());
    connectionManager_->clickDisconnect();
}

void Engine::sendDebugLogImpl()
{
    QString userName;
    if (!apiInfo_.isNull())
    {
        userName = apiInfo_->getSessionStatus().getUsername();
    }

#ifdef Q_OS_WIN
    if (!MergeLog::canMerge())
    {
        Q_EMIT sendUserWarning(ProtoTypes::USER_WARNING_SEND_LOG_FILE_TOO_BIG);
        return;
    }
#endif

    QString log = MergeLog::mergePrevLogs(true);
    log += "================================================================================================================================================================================================\n";
    log += "================================================================================================================================================================================================\n";
    log += MergeLog::mergeLogs(true);

    /*
    // For testing merge log functionality
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    path += "/merged_logs.txt";
    QFile file(path);
    if (file.open(QIODeviceBase::WriteOnly | QIODevice::Truncate))
    {
        file.write(log.toLatin1());
        file.close();
    }
    */

    serverAPI_->debugLog(userName, log, serverApiUserRole_, true);
}

void Engine::getWebSessionTokenImpl(ProtoTypes::WebSessionPurpose purpose)
{
    uint userRole = serverApiEditAccountDetailsUserRole_;
    if (purpose == ProtoTypes::WEB_SESSION_PURPOSE_ADD_EMAIL) userRole = serverApiAddEmailUserRole_;
    serverAPI_->webSession(apiInfo_->getAuthHash(), userRole, true);
}

// function consists of two parts (first - disconnect if need, second - do other signout stuff)
void Engine::signOutImpl(bool keepFirewallOn)
{
    if (!connectionManager_->isDisconnected())
    {
        connectionManager_->setProperty("senderSource", (keepFirewallOn ? "signOutImplKeepFirewallOn" : "signOutImpl"));
        connectionManager_->clickDisconnect();
    }
    else
    {
        signOutImplAfterDisconnect(keepFirewallOn);
    }
}

void Engine::signOutImplAfterDisconnect(bool keepFirewallOn)
{
    if (loginController_)
    {
        SAFE_DELETE(loginController_);
        loginState_ = LOGIN_NONE;
    }
    updateServerResourcesTimer_->stop();
    updateSessionStatusTimer_->stop();
    notificationsUpdateTimer_->stop();

    locationsModel_->clear();
    prevSessionStatus_.clear();
    prevSessionForLogging_.clear();

#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
    firewallController_->enableFirewallOnBoot(false);
#endif

    if (!apiInfo_.isNull())
    {
        serverAPI_->deleteSession(apiInfo_->getAuthHash(), serverApiUserRole_, true);
        apiInfo_.reset();
    }
    apiinfo::ApiInfo::removeFromSettings();
    GetWireGuardConfig::removeWireGuardSettings();

    if (!keepFirewallOn)
    {
        firewallController_->firewallOff();
        Q_EMIT firewallStateChanged(false);
    }

    Q_EMIT signOutFinished();
}

void Engine::continueWithUsernameAndPasswordImpl(const QString &username, const QString &password, bool bSave)
{
    // if username and password is empty, then disconnect
    if (username.isEmpty() && password.isEmpty())
    {
        connectionManager_->clickDisconnect();
    }
    else
    {
        if (bSave)
        {
            customOvpnAuthCredentialsStorage_->setAuthCredentials(connectionManager_->getCustomOvpnConfigFileName(), username, password);
        }
        connectionManager_->continueWithUsernameAndPassword(username, password, isNeedReconnectAfterRequestUsernameAndPassword_);
    }
}

void Engine::continueWithPasswordImpl(const QString &password, bool bSave)
{
    // if password is empty, then disconnect
    if (password.isEmpty())
    {
        connectionManager_->clickDisconnect();
    }
    else
    {
        if (bSave)
        {
            customOvpnAuthCredentialsStorage_->setAuthCredentials(connectionManager_->getCustomOvpnConfigFileName(), "", password);
        }
        connectionManager_->continueWithPassword(password, isNeedReconnectAfterRequestUsernameAndPassword_);
    }
}

void Engine::gotoCustomOvpnConfigModeImpl()
{
    updateServerLocations();
    Q_EMIT gotoCustomOvpnConfigModeFinished();
}

void Engine::updateCurrentInternetConnectivityImpl()
{
    online_ = networkDetectionManager_->isOnline();
    Q_EMIT internetConnectivityChanged(online_);
}

void Engine::updateCurrentNetworkInterfaceImpl()
{
    ProtoTypes::NetworkInterface networkInterface;
    networkDetectionManager_->getCurrentNetworkInterface(networkInterface);

    if (!bPrevNetworkInterfaceInitialized_ || !google::protobuf::util::MessageDifferencer::Equals(networkInterface, prevNetworkInterface_))
    {
        prevNetworkInterface_ = networkInterface;
        bPrevNetworkInterfaceInitialized_ = true;
        Q_EMIT networkChanged(networkInterface);
    }
}

void Engine::firewallOnImpl()
{
    if (connectStateController_->currentState() != CONNECT_STATE_CONNECTED)
    {
        firewallController_->firewallOn(firewallExceptions_.getIPAddressesForFirewall(), engineSettings_.isAllowLanTraffic());
    }
    else
    {
        firewallController_->firewallOn(firewallExceptions_.getIPAddressesForFirewallForConnectedState(connectionManager_->getLastConnectedIp()), engineSettings_.isAllowLanTraffic());
    }
    Q_EMIT firewallStateChanged(true);
}

void Engine::firewallOffImpl()
{
    firewallController_->firewallOff();
    Q_EMIT firewallStateChanged(false);
}

void Engine::speedRatingImpl(int rating, const QString &localExternalIp)
{
    serverAPI_->speedRating(apiInfo_->getAuthHash(), lastConnectingHostname_, localExternalIp, rating, serverApiUserRole_, true);
}

void Engine::setSettingsImpl(const EngineSettings &engineSettings)
{
    qCDebug(LOG_BASIC) << "Engine::";

    bool isAllowLanTrafficChanged = engineSettings_.isAllowLanTraffic() != engineSettings.isAllowLanTraffic();
    bool isUpdateChannelChanged = engineSettings_.getUpdateChannel() != engineSettings.getUpdateChannel();
    bool isLanguageChanged = engineSettings_.language() != engineSettings.language();
    bool isProtocolChanged = !engineSettings_.connectionSettings().protocol().isEqual(engineSettings.connectionSettings().protocol());
    bool isCloseTcpSocketsChanged = engineSettings_.isCloseTcpSockets() != engineSettings.isCloseTcpSockets();
    bool isDnsPolicyChanged = engineSettings_.getDnsPolicy() != engineSettings.getDnsPolicy();
    bool isCustomOvpnConfigsPathChanged = engineSettings_.getCustomOvpnConfigsPath() != engineSettings.getCustomOvpnConfigsPath();
    bool isMACSpoofingChanged = !google::protobuf::util::MessageDifferencer::Equals(engineSettings_.getMacAddrSpoofing(), engineSettings.getMacAddrSpoofing());
    bool isPacketSizeChanged =  !google::protobuf::util::MessageDifferencer::Equals(engineSettings_.getPacketSize(),      engineSettings.getPacketSize());
    bool isDnsWhileConnectedChanged = !google::protobuf::util::MessageDifferencer::Equals(engineSettings_.getDnsWhileConnectedInfo(), engineSettings.getDnsWhileConnectedInfo());
    engineSettings_ = engineSettings;

#ifdef Q_OS_LINUX
    DnsScripts_linux::instance().setDnsManager(engineSettings.getDnsManager());
#endif

    if (isDnsPolicyChanged)
    {
        firewallExceptions_.setDnsPolicy(engineSettings_.getDnsPolicy());
        if (connectStateController_->currentState() != CONNECT_STATE_CONNECTED && emergencyConnectStateController_->currentState() != CONNECT_STATE_CONNECTED)
        {
            DnsServersConfiguration::instance().setDnsServersPolicy(engineSettings_.getDnsPolicy());
        }
    }

    if (isDnsWhileConnectedChanged)
    {
        // tell connection manager about new settings (it will use them onConnect)
        connectionManager_->setDnsWhileConnectedInfo(engineSettings.getDnsWhileConnectedInfo());
    }

    if (isAllowLanTrafficChanged || isDnsPolicyChanged)
    {
        updateFirewallSettings();
    }

    if (isUpdateChannelChanged)
    {
        ProtoTypes::UpdateChannel channel =   engineSettings_.getUpdateChannel();
        if (overrideUpdateChannelWithInternal_)
        {
            qCDebug(LOG_BASIC) << "Overriding update channel: internal";
            channel = ProtoTypes::UPDATE_CHANNEL_INTERNAL;
        }
        serverAPI_->checkUpdate(channel, serverApiUserRole_, true);
    }
    if (isLanguageChanged || isProtocolChanged)
    {
        if (!apiInfo_.isNull())
        {
            apiinfo::SessionStatus ss = apiInfo_->getSessionStatus();
            if (isLanguageChanged)
            {
                qCDebug(LOG_BASIC) << "Language changed -> update server locations";
            }
            else if (isProtocolChanged)
            {
                if (engineSettings_.connectionSettings().protocol().isOpenVpnProtocol())
                {
                    qCDebug(LOG_BASIC) << "Protocol changed to openvpn -> update server locations";
                }
                else if (engineSettings_.connectionSettings().protocol().isIkev2Protocol())
                {
                    qCDebug(LOG_BASIC) << "Protocol changed to ikev -> update server locations";
                }
                else if (engineSettings_.connectionSettings().protocol().isWireGuardProtocol())
                {
                    qCDebug(LOG_BASIC) << "Protocol changed to wireguard -> update server locations";
                }
                else
                {
                    Q_ASSERT(false);
                }
            }
            serverAPI_->serverLocations(apiInfo_->getAuthHash(), engineSettings_.language(), serverApiUserRole_, true, ss.getRevisionHash(),
                                        ss.isPro(), engineSettings_.connectionSettings().protocol(), ss.getAlc());
        }
    }

    if (isCloseTcpSocketsChanged)
    {
    #ifdef Q_OS_WIN
        measurementCpuUsage_->setEnabled(engineSettings_.isCloseTcpSockets());
    #endif
    }

    if (isMACSpoofingChanged)
    {
        qCDebug(LOG_BASIC) << "Set MAC Spoofing (Engine)";
        macAddressController_->setMacAddrSpoofing(engineSettings_.getMacAddrSpoofing());
    }

    if (isPacketSizeChanged)
    {
        qCDebug(LOG_BASIC) << "Engine updating packet size controller";
        packetSizeController_->setPacketSize(engineSettings_.getPacketSize());
    }

    serverAPI_->setIgnoreSslErrors(engineSettings_.isIgnoreSslErrors());

    if (isCustomOvpnConfigsPathChanged)
    {
        customConfigs_->changeDir(engineSettings_.getCustomOvpnConfigsPath());
    }

    keepAliveManager_->setEnabled(engineSettings_.isKeepAliveEnabled());

    updateProxySettings();

    OpenVpnVersionController::instance().setUseWinTun(engineSettings_.isUseWintun());
}

void Engine::onLoginControllerFinished(LOGIN_RET retCode, const apiinfo::ApiInfo &apiInfo, bool bFromConnectedToVPNState, const QString &errorMessage)
{
    qCDebug(LOG_BASIC) << "onLoginControllerFinished, retCode =" << loginRetToString(retCode) << ";bFromConnectedToVPNState ="
                       << bFromConnectedToVPNState << ";errorMessage =" << errorMessage;

    Q_ASSERT(loginState_ != LOGIN_FINISHED);
    if (retCode == LOGIN_SUCCESS)
    {
        if (!emergencyController_->isDisconnected())
        {
            emergencyController_->blockingDisconnect();
            emergencyConnectStateController_->setDisconnectedState(DISCONNECTED_ITSELF, ProtoTypes::ConnectError::NO_CONNECT_ERROR);
            Q_EMIT emergencyDisconnected();

            qCDebug(LOG_BASIC) << "Update ip after emergency connect";
            getMyIPController_->getIPFromDisconnectedState(1);
        }

        apiInfo_.reset(new apiinfo::ApiInfo);
        *apiInfo_ = apiInfo;
        QString curRevisionHash = apiInfo_->getSessionStatus().getRevisionHash();

        // if updateServerLocation not called in loginImpl
        if (!prevSessionStatus_.isInitialized())
        {
            prevSessionStatus_ = apiInfo_->getSessionStatus();
        }
        else
        {
            Q_ASSERT(!curRevisionHash.isEmpty());
            if (curRevisionHash != prevSessionStatus_.getRevisionHash())
            {
                prevSessionStatus_.setRevisionHash(curRevisionHash);
            }
        }

        updateServerLocations();
        updateSessionStatus();
        getNewNotifications();
        notificationsUpdateTimer_->start(NOTIFICATIONS_UPDATE_PERIOD);
        updateSessionStatusTimer_->start();

        if (!bFromConnectedToVPNState)
        {
            loginState_ = LOGIN_FINISHED;
        }
        updateCurrentNetworkInterfaceImpl();
        Q_EMIT loginFinished(false, apiInfo_->getAuthHash(), apiInfo_->getPortMap());
    }
    else if (retCode == LOGIN_NO_CONNECTIVITY)
    {
        loginState_ = LOGIN_NONE;
        Q_EMIT loginError(LOGIN_NO_CONNECTIVITY, QString());
    }
    else if (retCode == LOGIN_NO_API_CONNECTIVITY)
    {
        loginState_ = LOGIN_NONE;
        Q_EMIT loginError(LOGIN_NO_API_CONNECTIVITY, QString());

        if (bFromConnectedToVPNState)
        {
            qCDebug(LOG_BASIC) << "No API connectivity from connected state. Using stale API data from settings.";
        }
        else
        {
            qCDebug(LOG_BASIC) << "No API connectivity from disconnected state. Using stale API data from settings.";
        }
    }
    else if (retCode == LOGIN_PROXY_AUTH_NEED)
    {
        loginState_ = LOGIN_NONE;
        Q_EMIT loginError(LOGIN_PROXY_AUTH_NEED, QString());
    }
    else if (retCode == LOGIN_INCORRECT_JSON)
    {
        loginState_ = LOGIN_NONE;
        Q_EMIT loginError(LOGIN_INCORRECT_JSON, QString());
    }
    else if (retCode == LOGIN_SSL_ERROR)
    {
        if (bFromConnectedToVPNState)
        {
            QMutexLocker lockerLoginSettings(&loginSettingsMutex_);
            loginController_->deleteLater(); // delete LoginController object
            loginController_ = NULL;

            loginState_ = LOGIN_IN_PROGRESS;
            startLoginController(loginSettings_, bFromConnectedToVPNState);
            return;
        }
        else
        {
            loginState_ = LOGIN_NONE;
            Q_EMIT loginError(LOGIN_SSL_ERROR, QString());
        }
    }
    else if (retCode == LOGIN_BAD_USERNAME || retCode == LOGIN_BAD_CODE2FA ||
             retCode == LOGIN_MISSING_CODE2FA || retCode == LOGIN_ACCOUNT_DISABLED ||
             retCode == LOGIN_SESSION_INVALID)
    {
        loginState_ = LOGIN_NONE;
        Q_EMIT loginError(retCode, errorMessage);
    }
    else
    {
        Q_ASSERT(false);
    }

    loginController_->deleteLater(); // delete LoginController object
    loginController_ = NULL;
}

void Engine::onReadyForNetworkRequests()
{
    qCDebug(LOG_BASIC) << "Engine::onReadyForNetworkRequests()";
    serverAPI_->setRequestsEnabled(true);
    updateServerResourcesTimer_->start(UPDATE_SERVER_RESOURCES_PERIOD);
    checkForAppUpdate();

    if (connectionManager_->isDisconnected())
    {
        getMyIPController_->getIPFromDisconnectedState(1);
    }
    else
    {
        getMyIPController_->getIPFromConnectedState(1);
    }
}

void Engine::onLoginControllerStepMessage(LOGIN_MESSAGE msg)
{
    Q_EMIT loginStepMessage(msg);
}

void Engine::onServerLocationsAnswer(SERVER_API_RET_CODE retCode, const QVector<apiinfo::Location> &serverLocations, QStringList forceDisconnectNodes, uint userRole)
{
    if (userRole == serverApiUserRole_)
    {
        if (retCode == SERVER_RETURN_SUCCESS)
        {
            if (!serverLocations.isEmpty())
            {
                apiInfo_->setLocations(serverLocations);
                apiInfo_->setForceDisconnectNodes(forceDisconnectNodes);
                updateServerLocations();
            }
        }
        else if (retCode == SERVER_RETURN_API_NOT_READY)
        {
            qCDebug(LOG_BASIC) << "Request server locations failed. API not ready";
        }
        else
        {
            qCDebug(LOG_BASIC) << "Request server locations failed. Seems no internet connectivity";
        }
    }
}

void Engine::onSessionAnswer(SERVER_API_RET_CODE retCode, const apiinfo::SessionStatus &sessionStatus, uint userRole)
{
    if (userRole == serverApiUserRole_)
    {
        if (retCode == SERVER_RETURN_SUCCESS)
        {
            apiInfo_->setSessionStatus(sessionStatus);
            updateSessionStatus();
            // updateCurrentNetworkInterface();
        }
        else if (retCode == SERVER_RETURN_SESSION_INVALID)
        {
            Q_EMIT sessionDeleted();
        }
     }
}

void Engine::onNotificationsAnswer(SERVER_API_RET_CODE retCode, const QVector<apiinfo::Notification> &notifications, uint userRole)
{
    if (userRole == serverApiUserRole_)
    {
        if (retCode == SERVER_RETURN_SUCCESS)
        {
            Q_EMIT notificationsUpdated(notifications);
        }
    }
}

void Engine::onServerConfigsAnswer(SERVER_API_RET_CODE retCode, const QString &config, uint userRole)
{
    if (userRole == serverApiUserRole_)
    {
        if (retCode == SERVER_RETURN_SUCCESS)
        {
            apiInfo_->setOvpnConfig(config);
        }
        else
        {
            qCDebug(LOG_BASIC) << "Failed update server configs for openvpn version change";
            // try again with 3 sec
            QTimer::singleShot(3000, this, SLOT(updateServerConfigs()));
        }
    }
}

void Engine::onCheckUpdateAnswer(const apiinfo::CheckUpdate &checkUpdate, bool bNetworkErrorOccured, uint userRole)
{
    qCDebug(LOG_BASIC) << "Received Check Update Answer";

    if (userRole == serverApiUserRole_)
    {
        if (bNetworkErrorOccured)
        {
            QTimer::singleShot(60000, this, &Engine::checkForAppUpdate);
            return;
        }

        if (checkUpdate.isInitialized())
        {
            installerUrl_ = checkUpdate.getUrl();
            installerHash_ = checkUpdate.getSha256();

            // testing only
//#ifdef Q_OS_LINUX
//            if(LinuxUtils::isDeb()) {
//                installerUrl_ = "https://nexus.int.windscribe.com/repository/client-desktop-beta/windscribe_2.3.11_beta_amd64.deb";
//             }
//             else {
//                installerUrl_ = "https://nexus.int.windscribe.com/repository/client-desktop-beta/windscribe_2.3.11_beta_x86_64.rpm";
//             }
//#elif defined Q_OS_MAC
//             installerUrl_ = "https://nexus.int.windscribe.com/repository/client-desktop-beta/Windscribe_2.3.11_beta.dmg";
//#else
//             installerUrl_ = "https://nexus.int.windscribe.com/repository/client-desktop-beta/Windscribe_2.3.11_beta.exe";
//#endif
            qCDebug(LOG_BASIC) << "Installer URL: " << installerUrl_;
            qCDebug(LOG_BASIC) << "Installer Hash: " << installerHash_;
            Q_EMIT checkUpdateUpdated(checkUpdate);
        }
    }
}

void Engine::onWhitelistedIPsChanged(const QSet<QString> &ips)
{
    qCDebug(LOG_BASIC) << "on whitelisted ips changed event:" << ips;
    firewallExceptions_.setWhiteListedIPs(ips);
    updateFirewallSettings();
}

void Engine::onHostIPsChanged(const QStringList &hostIps)
{
    qCDebug(LOG_BASIC) << "on host ips changed event:" << hostIps;
    firewallExceptions_.setHostIPs(hostIps);
    updateFirewallSettings();
}

void Engine::onMyIpAnswer(const QString &ip, bool success, bool isDisconnected)
{
    Q_EMIT myIpUpdated(ip, success, isDisconnected);
}

void Engine::onDebugLogAnswer(SERVER_API_RET_CODE retCode, uint userRole)
{
    if (userRole == serverApiUserRole_)
    {
        Q_EMIT sendDebugLogFinished(retCode == SERVER_RETURN_SUCCESS);
    }
}

void Engine::onConfirmEmailAnswer(SERVER_API_RET_CODE retCode, uint userRole)
{
    if (userRole == serverApiUserRole_)
    {
        Q_EMIT confirmEmailFinished(retCode == SERVER_RETURN_SUCCESS);
    }
}

void Engine::onStaticIpsAnswer(SERVER_API_RET_CODE retCode, const apiinfo::StaticIps &staticIps, uint userRole)
{
    if (userRole == serverApiUserRole_)
    {
        if (retCode == SERVER_RETURN_SUCCESS)
        {
            apiInfo_->setStaticIps(staticIps);
            updateServerLocations();
        }
        else
        {
            qCDebug(LOG_BASIC) << "Failed get static ips";
            QTimer::singleShot(3000, this, [this]() {
                if (!apiInfo_.isNull()) {
                    serverAPI_->staticIps(apiInfo_->getAuthHash(), GetDeviceId::instance().getDeviceId(), serverApiUserRole_, true);
                }
            });
        }
    }
}

void Engine::onWebSessionAnswer(SERVER_API_RET_CODE retCode, const QString &token, uint userRole)
{
    if (retCode == SERVER_RETURN_SUCCESS)
    {
        if (userRole == serverApiEditAccountDetailsUserRole_)
        {
            Q_EMIT webSessionToken(ProtoTypes::WEB_SESSION_PURPOSE_EDIT_ACCOUNT_DETAILS, token);
        }
        if (userRole == serverApiAddEmailUserRole_)
        {
            Q_EMIT webSessionToken(ProtoTypes::WEB_SESSION_PURPOSE_ADD_EMAIL, token);
        }
    }
}

void Engine::onUpdateServerResources()
{
    // Refresh all server resources, and check for a new app version, every 24 hours.

    if (!apiInfo_.isNull() && apiInfo_->getSessionStatus().getStatus() == 1)
    {
        qCDebug(LOG_BASIC) << "24 hours have past, refreshing all server API resources.";

        const apiinfo::SessionStatus ss = apiInfo_->getSessionStatus();
        const QString authHash = apiInfo_->getAuthHash();
        const bool isConnected = (connectStateController_->currentState() == CONNECT_STATE_CONNECTED);
        const bool isDisconnected = (connectStateController_->currentState() == CONNECT_STATE_DISCONNECTED);

        if (isDisconnected || (isConnected && !connectionManager_->currentProtocol().isOpenVpnProtocol()))
        {
            serverAPI_->serverConfigs(authHash, serverApiUserRole_, true);
            serverAPI_->serverCredentials(authHash, serverApiUserRole_, ProtocolType(ProtocolType::PROTOCOL_OPENVPN_UDP), true);
        }

        if (isDisconnected || (isConnected && !connectionManager_->currentProtocol().isIkev2Protocol())) {
            serverAPI_->serverCredentials(authHash, serverApiUserRole_, ProtocolType(ProtocolType::PROTOCOL_IKEV2), true);
        }

        serverAPI_->serverLocations(authHash, engineSettings_.language(), serverApiUserRole_, true, ss.getRevisionHash(), ss.isPro(),
                                    connectionManager_->currentProtocol(), ss.getAlc());

        serverAPI_->portMap(authHash, serverApiUserRole_, true);

        if (ss.getStaticIpsCount() > 0) {
            serverAPI_->staticIps(authHash, GetDeviceId::instance().getDeviceId(), serverApiUserRole_, true);
        }
    }

    checkForAppUpdate();
}

void Engine::checkForAppUpdate()
{
    ProtoTypes::UpdateChannel channel = engineSettings_.getUpdateChannel();
    if (overrideUpdateChannelWithInternal_)
    {
        qCDebug(LOG_BASIC) << "Overriding update channel: internal";
        channel = ProtoTypes::UPDATE_CHANNEL_INTERNAL;
    }
    serverAPI_->checkUpdate(channel, serverApiUserRole_, true);
}

void Engine::onUpdateSessionStatusTimer()
{
    serverAPI_->session(apiInfo_->getAuthHash(), serverApiUserRole_, true);
}

void Engine::onConnectionManagerConnected()
{
    QString adapterName = connectionManager_->getVpnAdapterInfo().adapterName();

#ifdef Q_OS_WIN
    // wireguard-nt driver monitors metrics itself.
    if (!connectionManager_->currentProtocol().isWireGuardProtocol()) {
        AdapterMetricsController_win::updateMetrics(connectionManager_->getVpnAdapterInfo().adapterName(), helper_);
    }
#elif defined (Q_OS_MAC) || defined (Q_OS_LINUX)
    firewallController_->setInterfaceToSkip_posix(adapterName);
#endif

    bool isFirewallAlreadyEnabled = false;
    if (engineSettings_.firewallSettings().mode() == ProtoTypes::FIREWALL_MODE_AUTOMATIC) {
        const bool isAllowFirewallAfterConnection =
            connectionManager_->isAllowFirewallAfterConnection();

        if (isAllowFirewallAfterConnection &&
            engineSettings_.firewallSettings().when() == ProtoTypes::FIREWALL_WHEN_AFTER_CONNECTION)
        {
            if (!firewallController_->firewallActualState())
            {
                qCDebug(LOG_BASIC) << "Automatic enable firewall after connection";
                QString ips = firewallExceptions_.getIPAddressesForFirewallForConnectedState(connectionManager_->getLastConnectedIp());
                firewallController_->firewallOn(ips, engineSettings_.isAllowLanTraffic());
                Q_EMIT firewallStateChanged(true);
                isFirewallAlreadyEnabled = true;
            }
        }
        else if (!isAllowFirewallAfterConnection &&
            engineSettings_.firewallSettings().when() == ProtoTypes::FIREWALL_WHEN_BEFORE_CONNECTION)
        {
            if (firewallController_->firewallActualState())
            {
                qCDebug(LOG_BASIC) << "Automatic disable firewall after connection";
                firewallController_->firewallOff();
                Q_EMIT firewallStateChanged(false);
            }
        }
    }

    helper_->sendConnectStatus(true, engineSettings_.isCloseTcpSockets(), engineSettings_.isAllowLanTraffic(),
                               connectionManager_->getDefaultAdapterInfo(), connectionManager_->getCustomDnsAdapterGatewayInfo().adapterInfo,
                               connectionManager_->getLastConnectedIp(), lastConnectingProtocol_);

    if (firewallController_->firewallActualState() && !isFirewallAlreadyEnabled)
    {
        firewallController_->firewallOn(firewallExceptions_.getIPAddressesForFirewallForConnectedState(connectionManager_->getLastConnectedIp()), engineSettings_.isAllowLanTraffic());
    }

    if (connectionManager_->getCustomDnsAdapterGatewayInfo().dnsWhileConnectedInfo.type() == ProtoTypes::DNS_WHILE_CONNECTED_TYPE_CUSTOM)
    {
         if (!helper_->setCustomDnsWhileConnected(connectionManager_->currentProtocol().isIkev2Protocol(),
                                                  connectionManager_->getVpnAdapterInfo().ifIndex(),
                                                  QString::fromStdString(connectionManager_->getCustomDnsAdapterGatewayInfo().dnsWhileConnectedInfo.ip_address())))
         {
             qCDebug(LOG_CONNECTED_DNS) << "Failed to set Custom 'while connected' DNS";
         }
    }

#ifdef Q_OS_WIN
    Helper_win *helper_win = dynamic_cast<Helper_win *>(helper_);
    helper_win->setIPv6EnabledInFirewall(false);
#endif

    if (connectionManager_->currentProtocol().isIkev2Protocol() || connectionManager_->currentProtocol().isWireGuardProtocol())
    {
        if (!packetSize_.is_automatic())
        {
            int mtuForProtocol = 0;
            if (connectionManager_->currentProtocol().isWireGuardProtocol())
            {
                bool advParamWireguardMtuOffset = false;
                int wgoffset = ExtraConfig::instance().getMtuOffsetWireguard(advParamWireguardMtuOffset);
                if (!advParamWireguardMtuOffset) wgoffset = MTU_OFFSET_WG;

                mtuForProtocol = packetSize_.mtu() - wgoffset;
            }
            else
            {
                bool advParamIkevMtuOffset = false;
                int ikev2offset = ExtraConfig::instance().getMtuOffsetIkev2(advParamIkevMtuOffset);
                if (!advParamIkevMtuOffset) ikev2offset = MTU_OFFSET_IKEV2;

                mtuForProtocol = packetSize_.mtu() - ikev2offset;
            }

            if (mtuForProtocol > 0)
            {
                qCDebug(LOG_PACKET_SIZE) << "Applying MTU on " << adapterName << ": " << mtuForProtocol;
    #ifdef Q_OS_MAC
                const QString setIkev2MtuCmd = QString("ifconfig %1 mtu %2").arg(adapterName).arg(mtuForProtocol);
                Helper_mac *helper_mac = dynamic_cast<Helper_mac *>(helper_);
                helper_mac->executeRootCommand(setIkev2MtuCmd);
    #elif defined(Q_OS_WIN)
                Helper_win *helper_win = dynamic_cast<Helper_win *>(helper_);
                helper_win->executeChangeMtu(adapterName, mtuForProtocol);
    #elif defined(Q_OS_LINUX)
                //todo
    #endif
            }
            else
            {
                qCDebug(LOG_PACKET_SIZE) << "Using default MTU, mtu minus overhead is too low: " << mtuForProtocol;
            }
        }
        else
        {
            qCDebug(LOG_PACKET_SIZE) << "Packet size mode auto - using default MTU (Engine)";
        }
    }

    if (connectionManager_->isStaticIpsLocation())
    {
        firewallController_->whitelistPorts(connectionManager_->getStatisIps());
        qCDebug(LOG_BASIC) << "the firewall rules are added for static IPs location, ports:" << connectionManager_->getStatisIps().getAsStringWithDelimiters();
    }

    serverAPI_->disableProxy();
    locationsModel_->disableProxy();

    DnsServersConfiguration::instance().setDnsServersPolicy(DNS_TYPE_OS_DEFAULT);


    if (loginState_ == LOGIN_IN_PROGRESS)
    {
        QMutexLocker lockerLoginSettings(&loginSettingsMutex_);
        serverAPI_->setRequestsEnabled(false);
        if (loginController_)
        {
            SAFE_DELETE(loginController_);
        }
        startLoginController(loginSettings_, true);
    }

    if (engineSettings_.isCloseTcpSockets())
    {
#ifdef Q_OS_WIN
        Helper_win *helper_win = dynamic_cast<Helper_win *>(helper_);
        helper_win->closeAllTcpConnections(engineSettings_.isAllowLanTraffic());
#endif
    }

    connectionManager_->startTunnelTests();

    connectStateController_->setConnectedState(locationId_);
}

void Engine::onConnectionManagerDisconnected(DISCONNECT_REASON reason)
{
    qCDebug(LOG_BASIC) << "on disconnected event";

    if (connectionManager_->isStaticIpsLocation())
    {
        qCDebug(LOG_BASIC) << "the firewall rules are removed for static IPs location";
        firewallController_->deleteWhitelistPorts();
    }

    // get sender source for additional actions in this handler
    QString senderSource;
    if (connectionManager_->property("senderSource").isValid())
    {
        senderSource = connectionManager_->property("senderSource").toString();
        connectionManager_->setProperty("senderSource", QVariant());
    }

    doDisconnectRestoreStuff();

    if (loginState_ == LOGIN_IN_PROGRESS)
    {
        QMutexLocker lockerLoginSettings(&loginSettingsMutex_);
        serverAPI_->setRequestsEnabled(false);
        if (loginController_)
        {
            SAFE_DELETE(loginController_);
        }

        startLoginController(loginSettings_, false);
    }

#ifdef Q_OS_WIN
    DnsInfo_win::outputDebugDnsInfo();
#endif

    if (senderSource == "signOutImpl")
    {
        signOutImplAfterDisconnect(false);
    }
    else if (senderSource == "signOutImplKeepFirewallOn")
    {
        signOutImplAfterDisconnect(true);
    }
    else if (senderSource == "reconnect")
    {
        connectClickImpl(locationId_);
        return;
    }
    else
    {
        getMyIPController_->getIPFromDisconnectedState(1);

        if (reason == DISCONNECTED_BY_USER && engineSettings_.firewallSettings().mode() == ProtoTypes::FIREWALL_MODE_AUTOMATIC &&
            firewallController_->firewallActualState())
        {
            firewallController_->firewallOff();
            Q_EMIT firewallStateChanged(false);
        }
    }

    connectStateController_->setDisconnectedState(reason, ProtoTypes::ConnectError::NO_CONNECT_ERROR);
}

void Engine::onConnectionManagerReconnecting()
{
    qCDebug(LOG_BASIC) << "on reconnecting event";

    DnsServersConfiguration::instance().setDnsServersPolicy(engineSettings_.getDnsPolicy());

    if (firewallController_->firewallActualState())
    {
        firewallController_->firewallOn(firewallExceptions_.getIPAddressesForFirewall(), engineSettings_.isAllowLanTraffic());
    }

    connectStateController_->setConnectingState(LocationID());
}

void Engine::onConnectionManagerError(ProtoTypes::ConnectError err)
{
    if (err == ProtoTypes::ConnectError::AUTH_ERROR)
    {
        if (connectionManager_->isCustomOvpnConfigCurrentConnection())
        {
            qCDebug(LOG_BASIC) << "Incorrect username or password for custom ovpn config";
        }
        else
        {
            qCDebug(LOG_BASIC) << "Incorrect username or password, refetch server credentials";
        }

        doDisconnectRestoreStuff();

        if (connectionManager_->isCustomOvpnConfigCurrentConnection())
        {
            customOvpnAuthCredentialsStorage_->removeCredentials(connectionManager_->getCustomOvpnConfigFileName());

            isNeedReconnectAfterRequestUsernameAndPassword_ = true;
            Q_EMIT requestUsername();
        }
        else
        {
            // goto update server credentials and try connect again
            if (refetchServerCredentialsHelper_ == NULL)
            {
                // force update session status (for check blocked, banned account state)
                serverAPI_->session(apiInfo_->getAuthHash(), serverApiUserRole_, true);

                refetchServerCredentialsHelper_ = new RefetchServerCredentialsHelper(this, apiInfo_->getAuthHash(), serverAPI_);
                connect(refetchServerCredentialsHelper_, &RefetchServerCredentialsHelper::finished, this, &Engine::onRefetchServerCredentialsFinished);
                refetchServerCredentialsHelper_->setProperty("fromAuthError", true);
                refetchServerCredentialsHelper_->startRefetch();
            }
            else
            {
                Q_ASSERT(false);
            }
        }

        return;
    }
    /*else if (err == IKEV_FAILED_REINSTALL_WAN_WIN)
    {
        qCDebug(LOG_BASIC) << "RAS error other than ERROR_AUTHENTICATION_FAILURE (691)";
        getMyIPController_->getIPFromDisconnectedState(1);
        connectStateController_->setDisconnectedState();
        Q_EMIT connectError(IKEV_FAILED_REINSTALL_WAN_WIN);
    }*/
#ifdef Q_OS_WIN
    else if (err == ProtoTypes::ConnectError::NO_INSTALLED_TUN_TAP)
    {
        //Q_EMIT connectError(NO_INSTALLED_TUN_TAP);
    }
    else if (err == ProtoTypes::ConnectError::ALL_TAP_IN_USE)
    {
        if(dynamic_cast<Helper_win*>(helper_)->reinstallTapDriver(QCoreApplication::applicationDirPath() + "/tap")) {
            qCDebug(LOG_BASIC) << "Tap driver was re-installed successfully.";
            doConnect(true);
        }
        else {
            qCDebug(LOG_BASIC) << "Failed to re-install tap driver";
            connectStateController_->setDisconnectedState(DISCONNECTED_WITH_ERROR, ProtoTypes::ConnectError::TAP_DRIVER_REINSTALLATION_ERROR);
        }
        return;
    }
    else if (err == ProtoTypes::ConnectError::WINTUN_FATAL_ERROR)
    {
        if(dynamic_cast<Helper_win*>(helper_)->reinstallWintunDriver(QCoreApplication::applicationDirPath() + "/wintun")) {
            qCDebug(LOG_BASIC) << "Wintun driver was re-installed successfully.";
            doConnect(true);
        }
        else {
            qCDebug(LOG_BASIC) << "Failed to re-install tap driver";
            connectStateController_->setDisconnectedState(DISCONNECTED_WITH_ERROR, ProtoTypes::ConnectError::WINTUN_DRIVER_REINSTALLATION_ERROR);
        }
        return;
    }
#endif
    else
    {
        //Q_EMIT connectError(err);
    }

#ifdef Q_OS_MAC
    Ipv6Controller_mac::instance().restoreIpv6();
#endif
    connectStateController_->setDisconnectedState(DISCONNECTED_WITH_ERROR, err);
}

void Engine::onConnectionManagerInternetConnectivityChanged(bool connectivity)
{
    online_ = connectivity;
    Q_EMIT internetConnectivityChanged(connectivity);
}

void Engine::onConnectionManagerStatisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes)
{
    Q_EMIT statisticsUpdated(bytesIn, bytesOut, isTotalBytes);
}

void Engine::onConnectionManagerInterfaceUpdated(const QString &interfaceName)
{
#if defined (Q_OS_MAC) || defined(Q_OS_LINUX)
    firewallController_->setInterfaceToSkip_posix(interfaceName);
    updateFirewallSettings();
#else
    Q_UNUSED(interfaceName);
#endif
}

void Engine::onConnectionManagerConnectingToHostname(const QString &hostname, const QString &ip, const QString &dnsServer)
{
    lastConnectingHostname_ = hostname;
    connectStateController_->setConnectingState(locationId_);

    qCDebug(LOG_BASIC) << "Whitelist connecting ip:" << ip;
    if (!dnsServer.isEmpty())
    {
        qCDebug(LOG_BASIC) << "Whitelist DNS-server ip:" << dnsServer;
    }

    bool bChanged1 = false;
    firewallExceptions_.setConnectingIp(ip, bChanged1);
    bool bChanged2 = false;
    firewallExceptions_.setDNSServerIp(dnsServer, bChanged2);
    if (bChanged1 || bChanged2)
    {
        updateFirewallSettings();
    }
}

void Engine::onConnectionManagerProtocolPortChanged(const ProtoTypes::Protocol &protocol, const uint port)
{
    lastConnectingProtocol_ = protocol;
    Q_EMIT protocolPortChanged(protocol, port);
}

void Engine::onConnectionManagerTestTunnelResult(bool success, const QString &ipAddress)
{
    Q_EMIT testTunnelResult(success); // stops protocol/port flashing
    Q_EMIT myIpUpdated(ipAddress, success, false); // sends IP address to UI // test should only occur in connected state
}

void Engine::onConnectionManagerWireGuardAtKeyLimit()
{
    Q_EMIT wireGuardAtKeyLimit();
}

#ifdef Q_OS_MAC
void Engine::onRobustMacSpoofTimerTick()
{
    QDateTime now = QDateTime::currentDateTime();

    // When using MAC spoofing robust method the WindscribeNetworkListener may not trigger when the network comes back up
    // So force a connectivity check for 15 seconds after the spoof
    // Not elegant, but lower risk as additional changes to the networkdetection module may affect network whitelisting
    if (robustTimerStart_.secsTo(now) > 15)
    {
        robustMacSpoofTimer_->stop();
        return;
    }

    updateCurrentInternetConnectivity();
}
#endif

void Engine::onConnectionManagerRequestUsername(const QString &pathCustomOvpnConfig)
{
    CustomOvpnAuthCredentialsStorage::Credentials c = customOvpnAuthCredentialsStorage_->getAuthCredentials(pathCustomOvpnConfig);

    if (!c.username.isEmpty() && !c.password.isEmpty())
    {
        connectionManager_->continueWithUsernameAndPassword(c.username, c.password, false);
    }
    else
    {
        isNeedReconnectAfterRequestUsernameAndPassword_ = false;
        Q_EMIT requestUsername();
    }
}

void Engine::onConnectionManagerRequestPassword(const QString &pathCustomOvpnConfig)
{
    CustomOvpnAuthCredentialsStorage::Credentials c = customOvpnAuthCredentialsStorage_->getAuthCredentials(pathCustomOvpnConfig);

    if (!c.password.isEmpty())
    {
        connectionManager_->continueWithPassword(c.password, false);
    }
    else
    {
        isNeedReconnectAfterRequestUsernameAndPassword_ = false;
        Q_EMIT requestPassword();
    }
}

void Engine::emergencyConnectClickImpl()
{
    emergencyController_->clickConnect(ProxyServerController::instance().getCurrentProxySettings());
}

void Engine::emergencyDisconnectClickImpl()
{
    emergencyController_->clickDisconnect();
}

void Engine::detectAppropriatePacketSizeImpl()
{
    if (networkDetectionManager_->isOnline())
    {
        if (serverAPI_->isRequestsEnabled())
        {
            qCDebug(LOG_PACKET_SIZE) << "Detecting appropriate packet size";
            runningPacketDetection_ = true;
            Q_EMIT packetSizeDetectionStateChanged(true, false);
            packetSizeController_->detectAppropriatePacketSize(serverAPI_->getHostname());
        }
        else
        {
            qCDebug(LOG_PACKET_SIZE) << "ServerAPI not enabled for requests (working hostname not detected). Using: " << QString::number(packetSize_.mtu());
        }
    }
    else
    {
         qCDebug(LOG_PACKET_SIZE) << "No internet, cannot detect appropriate packet size. Using: " << QString::number(packetSize_.mtu());
    }
}

void Engine::updateWindowInfoImpl(qint32 windowCenterX, qint32 windowCenterY)
{
    if (installerPath_ != "" && lastDownloadProgress_ == 100)
    {
        lastDownloadProgress_ = 0;
        updateRunInstaller(windowCenterX, windowCenterY);
    }
}

void Engine::updateVersionImpl(qint32 windowHandle)
{
    guiWindowHandle_ = windowHandle;

    if (installerUrl_ != "")
    {
        QMap<QString, QString> downloads;
        downloads.insert(installerUrl_, downloadHelper_->downloadInstallerPath());
        downloadHelper_->get(downloads);
    }
}

void Engine::stopUpdateVersionImpl()
{
    downloadHelper_->stop();
}

void Engine::updateAdvancedParamsImpl()
{
    bool newOverrideUpdateChannel = ExtraConfig::instance().getOverrideUpdateChannelToInternal();

    // only trigger the check update if override changed
    if (overrideUpdateChannelWithInternal_ != newOverrideUpdateChannel)
    {
        overrideUpdateChannelWithInternal_ = newOverrideUpdateChannel;

        ProtoTypes::UpdateChannel channel =   engineSettings_.getUpdateChannel();
        if (overrideUpdateChannelWithInternal_)
        {
            qCDebug(LOG_BASIC) << "Overriding update channel: internal";
            channel = ProtoTypes::UPDATE_CHANNEL_INTERNAL;
        }
        serverAPI_->checkUpdate(channel, serverApiUserRole_, true);
    }
}

void Engine::onDownloadHelperProgressChanged(uint progressPercent)
{
    if (lastDownloadProgress_ != progressPercent)
    {
        lastDownloadProgress_ = progressPercent;
        Q_EMIT updateVersionChanged(progressPercent, ProtoTypes::UPDATE_VERSION_STATE_DOWNLOADING, ProtoTypes::UPDATE_VERSION_ERROR_NO_ERROR);
    }
}

void Engine::onDownloadHelperFinished(const DownloadHelper::DownloadState &state)
{
    lastDownloadProgress_ = 100;
    installerPath_ = downloadHelper_->downloadInstallerPath();

    if (state != DownloadHelper::DOWNLOAD_STATE_SUCCESS)
    {
        qCDebug(LOG_DOWNLOADER) << "Removing incomplete installer";
        QFile::remove(installerPath_);
        Q_EMIT updateVersionChanged(0, ProtoTypes::UPDATE_VERSION_STATE_DONE, ProtoTypes::UPDATE_VERSION_ERROR_DL_FAIL);
        return;
    }
    qCDebug(LOG_DOWNLOADER) << "Successful download to: " << installerPath_;

#ifdef Q_OS_WIN

    ExecutableSignature sigCheck;
    if (!sigCheck.verify(installerPath_.toStdWString()))
    {
        qCDebug(LOG_AUTO_UPDATER) << "Incorrect signature, removing unsigned installer: " << QString::fromStdString(sigCheck.lastError());
        QFile::remove(installerPath_);
        Q_EMIT updateVersionChanged(0, ProtoTypes::UPDATE_VERSION_STATE_DONE, ProtoTypes::UPDATE_VERSION_ERROR_SIGN_FAIL);
        return;
    }
    qCDebug(LOG_AUTO_UPDATER) << "Installer signature valid";
#elif defined Q_OS_MAC

    const QString tempInstallerFilename = autoUpdaterHelper_->copyInternalInstallerToTempFromDmg(installerPath_);
    QFile::remove(installerPath_);

    if (tempInstallerFilename == "")
    {
        Q_EMIT updateVersionChanged(0, ProtoTypes::UPDATE_VERSION_STATE_DONE, autoUpdaterHelper_->error());
        return;
    }
    installerPath_ = tempInstallerFilename;
#elif defined Q_OS_LINUX

    // if api for some reason doesn't return sha256 field
    if (installerHash_ == "")
    {
        qCDebug(LOG_BASIC) << "Hash from API is empty -- cannot verify";
        if (QFile::exists(installerPath_)) QFile::remove(installerPath_);
        emit updateVersionChanged(0, ProtoTypes::UPDATE_VERSION_STATE_DONE, ProtoTypes::UPDATE_VERSION_ERROR_API_HASH_INVALID);
        return;
    }

    if (!verifyContentsSha256(installerPath_, installerHash_)) // installerPath_
    {
        qCDebug(LOG_AUTO_UPDATER) << "Incorrect hash, removing installer";
        if (QFile::exists(installerPath_)) QFile::remove(installerPath_);
        emit updateVersionChanged(0, ProtoTypes::UPDATE_VERSION_STATE_DONE, ProtoTypes::UPDATE_VERSION_ERROR_COMPARE_HASH_FAIL);
        return;
    }
#endif

    Q_EMIT updateVersionChanged(0, ProtoTypes::UPDATE_VERSION_STATE_RUNNING, ProtoTypes::UPDATE_VERSION_ERROR_NO_ERROR);
}

void Engine::updateRunInstaller(qint32 windowCenterX, qint32 windowCenterY)
{
#ifdef Q_OS_WIN
    std::wstring installerPath = installerPath_.toStdWString();

    QString installerArgString{ "-update" };
    if (windowCenterX != INT_MAX && windowCenterY != INT_MAX)
        installerArgString.append(QString(" -center %1 %2").arg(windowCenterX).arg(windowCenterY));
    std::wstring installerArgs = installerArgString.toStdWString();

    SHELLEXECUTEINFO shExInfo;
    memset(&shExInfo, 0, sizeof(shExInfo));
    shExInfo.cbSize = sizeof(shExInfo);
    shExInfo.fMask = SEE_MASK_DEFAULT;
    shExInfo.lpVerb = L"runas";                // Operation to perform
    shExInfo.lpFile = installerPath.c_str();       // Application to start
    shExInfo.lpParameters = installerArgs.c_str();  // Additional parameters
    shExInfo.nShow = SW_SHOW;
    if (guiWindowHandle_ != 0)
    {
        shExInfo.hwnd = (HWND)guiWindowHandle_;
    }

    if (!ShellExecuteEx(&shExInfo))
    {
        DWORD lastError = GetLastError();
        qCDebug(LOG_AUTO_UPDATER) << "Can't start installer: errorCode = " << lastError;
        QFile::remove(installerPath_);
        Q_EMIT updateVersionChanged(0, ProtoTypes::UPDATE_VERSION_STATE_DONE, ProtoTypes::UPDATE_VERSION_ERROR_START_INSTALLER_FAIL);
        return;
    }

#elif defined Q_OS_MAC
    QString additionalArgs;
    if (windowCenterX != INT_MAX && windowCenterY != INT_MAX)
        additionalArgs.append(QString("-center %1 %2").arg(windowCenterX).arg(windowCenterY));

    bool verifiedAndRan = autoUpdaterHelper_->verifyAndRun(installerPath_, additionalArgs);
    if (!verifiedAndRan)
    {
        Q_EMIT updateVersionChanged(0, ProtoTypes::UPDATE_VERSION_STATE_DONE, autoUpdaterHelper_->error());
        return;
    }
#else // Linux
    Q_UNUSED(windowCenterX);
    Q_UNUSED(windowCenterY);

    if(helper_) {
        if(!dynamic_cast<Helper_linux*>(helper_)->installUpdate(installerPath_)) {
            emit updateVersionChanged(0, ProtoTypes::UPDATE_VERSION_STATE_DONE, ProtoTypes::UPDATE_VERSION_ERROR_START_INSTALLER_FAIL);
            return;
        }
    }
#endif

    qCDebug(LOG_AUTO_UPDATER) << "Installer valid and executed";
    installerPath_.clear();

    Q_EMIT updateVersionChanged(0, ProtoTypes::UPDATE_VERSION_STATE_DONE, ProtoTypes::UPDATE_VERSION_ERROR_NO_ERROR);
}

void Engine::onEmergencyControllerConnected()
{
    qCDebug(LOG_BASIC) << "Engine::onEmergencyControllerConnected()";

#ifdef Q_OS_WIN
    AdapterMetricsController_win::updateMetrics(emergencyController_->getVpnAdapterInfo().adapterName(), helper_);
#endif

    serverAPI_->disableProxy();
    DnsServersConfiguration::instance().setDnsServersPolicy(DNS_TYPE_OS_DEFAULT);

    emergencyConnectStateController_->setConnectedState(LocationID());
    Q_EMIT emergencyConnected();
}

void Engine::onEmergencyControllerDisconnected(DISCONNECT_REASON reason)
{
    qCDebug(LOG_BASIC) << "Engine::onEmergencyControllerDisconnected(), reason =" << reason;

    serverAPI_->enableProxy();
    DnsServersConfiguration::instance().setDnsServersPolicy(engineSettings_.getDnsPolicy());

    emergencyConnectStateController_->setDisconnectedState(reason, ProtoTypes::ConnectError::NO_CONNECT_ERROR);
    Q_EMIT emergencyDisconnected();
}

void Engine::onEmergencyControllerError(ProtoTypes::ConnectError err)
{
    qCDebug(LOG_BASIC) << "Engine::onEmergencyControllerError(), err =" << err;
    emergencyConnectStateController_->setDisconnectedState(DISCONNECTED_WITH_ERROR, err);
    Q_EMIT emergencyConnectError(err);
}

void Engine::onRefetchServerCredentialsFinished(bool success, const apiinfo::ServerCredentials &serverCredentials, const QString &serverConfig)
{
    bool bFromAuthError = refetchServerCredentialsHelper_->property("fromAuthError").isValid();
    refetchServerCredentialsHelper_->deleteLater();
    refetchServerCredentialsHelper_ = NULL;

    if (success)
    {
        qCDebug(LOG_BASIC) << "Engine::onRefetchServerCredentialsFinished, successfully";
        apiInfo_->setServerCredentials(serverCredentials);
        apiInfo_->setOvpnConfig(serverConfig);
        doConnect(!bFromAuthError);
    }
    else
    {
        qCDebug(LOG_BASIC) << "Engine::onRefetchServerCredentialsFinished, failed";
        getMyIPController_->getIPFromDisconnectedState(1);
        connectStateController_->setDisconnectedState(DISCONNECTED_WITH_ERROR, ProtoTypes::ConnectError::COULD_NOT_FETCH_CREDENTAILS);
    }
}

void Engine::getNewNotifications()
{
    serverAPI_->notifications(apiInfo_->getAuthHash(), serverApiUserRole_, true);
}

void Engine::onCustomConfigsChanged()
{
    qCDebug(LOG_BASIC) << "Custom configs changed";
    updateServerLocations();
}

void Engine::onLocationsModelWhitelistIpsChanged(const QStringList &ips)
{
    firewallExceptions_.setLocationsPingIps(ips);
    updateFirewallSettings();
}

void Engine::onLocationsModelWhitelistCustomConfigIpsChanged(const QStringList &ips)
{
    firewallExceptions_.setCustomConfigPingIps(ips);
    updateFirewallSettings();
}

void Engine::onNetworkOnlineStateChange(bool isOnline)
{
    if (!isOnline && runningPacketDetection_)
    {
        qCDebug(LOG_BASIC) << "Internet lost during packet size detection -- stopping";
        stopPacketDetection();
    }
}

void Engine::onNetworkChange(const ProtoTypes::NetworkInterface &networkInterface)
{
    Q_EMIT networkChanged(networkInterface);
}

void Engine::stopPacketDetection()
{
    QMetaObject::invokeMethod(this, "stopPacketDetectionImpl");
}

void Engine::onMacAddressSpoofingChanged(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing)
{
    qCDebug(LOG_BASIC) << "Engine::onMacAddressSpoofingChanged";
    setSettingsMacAddressSpoofing(macAddrSpoofing);
}

void Engine::onPacketSizeControllerPacketSizeChanged(bool isAuto, int mtu)
{
    ProtoTypes::PacketSize packetSize;
    packetSize.set_is_automatic(isAuto);
    packetSize.set_mtu(mtu);

    packetSize_ = packetSize;
    connectionManager_->setPacketSize(packetSize);
    emergencyController_->setPacketSize(packetSize);

    // update gui
    if (mtu    != engineSettings_.getPacketSize().mtu() ||
        isAuto != engineSettings_.getPacketSize().is_automatic())
    {

        // qDebug() << "Updating gui with mtu: " << mtu;
        engineSettings_.setPacketSize(packetSize);

        // Connection to EngineServer is chewing the parameters to garbage when passed as ProtoTypes::PacketSize
        // Probably has something to do with EngineThread
        Q_EMIT packetSizeChanged(packetSize.is_automatic(), packetSize.mtu());
    }
}

void Engine::onPacketSizeControllerFinishedSizeDetection(bool isError)
{
    runningPacketDetection_ = false;
    Q_EMIT packetSizeDetectionStateChanged(false, isError);
}

void Engine::onMacAddressControllerSendUserWarning(ProtoTypes::UserWarningType userWarningType)
{
    Q_EMIT sendUserWarning(userWarningType);
}

#ifdef Q_OS_MAC
void Engine::onMacAddressControllerRobustMacSpoofApplied()
{
    // Robust MAC-spoofing can confuse the app into thinking it is offline
    // Update the connectivity check to fix this
    robustTimerStart_ = QDateTime::currentDateTime();
    robustMacSpoofTimer_->start();
}
#endif

void Engine::updateServerConfigsImpl()
{
    if (!apiInfo_.isNull())
    {
        serverAPI_->serverConfigs(apiInfo_->getAuthHash(), serverApiUserRole_, true);
    }
}

void Engine::checkForceDisconnectNode(const QStringList & /*forceDisconnectNodes*/)
{
    if (!connectionManager_->isDisconnected())
    {
        // check for force_disconnect nodes if we connected
       /* bool bNeedDisconnect = false;
        for (const QString &sn : forceDisconnectNodes)
        {
            if (lastConnectingHostname_ == sn)
            {
                qCDebug(LOG_BASIC) << "Force disconnect for connected node:" << lastConnectingHostname_;
                bNeedDisconnect = true;
                break;
            }
        }

        if (bNeedDisconnect)
        {
            //reconnect
            connectStateController_->setConnectingState();
            connectClickImpl(newLocationId);
        }*/
        /*else
        {
            // check if current connected nodes changed
            ServerModelLocationInfo sml = serversModel_->getLocationInfoById(newLocationId);
            QVector<ServerNode> curServerNodes = connectionManager_->getCurrentServerNodes();

            if (!ServerNode::isEqualIpsServerNodes(sml.nodes, curServerNodes))
            {
                //reconnect
                connectStateController_->setConnectingState();
                connectClickImpl(newLocationId, connectionSettings, isAutoEnableFirewall);
            }
        }*/
    }
}

// currently needed for cli state update -- might be a better way
void Engine::forceUpdateServerLocationsImpl()
{
    locationsModel_->forceSendLocationsToCli();
}

void Engine::startProxySharingImpl(PROXY_SHARING_TYPE proxySharingType)
{
    vpnShareController_->startProxySharing(proxySharingType);
    Q_EMIT proxySharingStateChanged(true, proxySharingType);
}

void Engine::stopProxySharingImpl()
{
    vpnShareController_->stopProxySharing();
    Q_EMIT proxySharingStateChanged(false, PROXY_SHARING_HTTP);
}

void Engine::startWifiSharingImpl(const QString &ssid, const QString &password)
{
    vpnShareController_->stopWifiSharing(); //  need to stop it first
    vpnShareController_->startWifiSharing(ssid, password);
    Q_EMIT wifiSharingStateChanged(true, ssid);
}

void Engine::stopWifiSharingImpl()
{
    bool bInitialState = vpnShareController_->isWifiSharingEnabled();
    vpnShareController_->stopWifiSharing();
    if (bInitialState == true)  // Q_EMIT signal if state changed
    {
        Q_EMIT wifiSharingStateChanged(false, "");
    }
}

void Engine::applicationActivatedImpl()
{
    if (updateSessionStatusTimer_)
    {
        updateSessionStatusTimer_->applicationActivated();
    }
}

void Engine::applicationDeactivatedImpl()
{
    if (updateSessionStatusTimer_)
    {
        updateSessionStatusTimer_->applicationDeactivated();
    }
}

void Engine::setSettingsMacAddressSpoofingImpl(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing)
{
    engineSettings_.setMacAddrSpoofing(macAddrSpoofing);
    Q_EMIT macAddrSpoofingChanged(macAddrSpoofing);
}

void Engine::setSplitTunnelingSettingsImpl(bool isActive, bool isExclude, const QStringList &files, const QStringList &ips, const QStringList &hosts)
{
    Q_ASSERT(helper_ != NULL);
    helper_->setSplitTunnelingSettings(isActive, isExclude, engineSettings_.isAllowLanTraffic(),
                                       files, ips, hosts);
}

void Engine::startLoginController(const LoginSettings &loginSettings, bool bFromConnectedState)
{
    Q_ASSERT(loginController_ == NULL);
    Q_ASSERT(loginState_ == LOGIN_IN_PROGRESS);
    loginController_ = new LoginController(this, helper_, networkDetectionManager_, serverAPI_, engineSettings_.language(), engineSettings_.connectionSettings().protocol());
    connect(loginController_, &LoginController::finished, this, &Engine::onLoginControllerFinished);
    connect(loginController_, &LoginController::readyForNetworkRequests, this, &Engine::onReadyForNetworkRequests);
    connect(loginController_, &LoginController::stepMessage, this, &Engine::onLoginControllerStepMessage);
    loginController_->startLoginProcess(loginSettings, engineSettings_.dnsResolutionSettings(), bFromConnectedState);
}

void Engine::updateSessionStatus()
{
    if (!apiInfo_.isNull())
    {
        apiinfo::SessionStatus ss = apiInfo_->getSessionStatus();

        if (ss.isChangedForLogging(prevSessionForLogging_))
        {
            std::string strLog =  "[" + ss.getProtoBuf().descriptor()->name() + "] {\n"
                   + Utils::cleanSensitiveInfo(ss.getProtoBuf().DebugString()) + "}";

            qCDebug(LOG_BASIC) << "update session status (changed since last call)";
            qCDebugMultiline(LOG_BASIC) << QString::fromStdString(strLog);
            prevSessionForLogging_ = ss;
        }
        else
        {
            // Commented debug entry out as this method is called every minute and we don't
            // need to flood the log with this info.
            //qCDebug(LOG_BASIC) << "update session status, no changes since last call";
        }


        Q_EMIT sessionStatusUpdated(ss);

        if (prevSessionStatus_.getRevisionHash() != ss.getRevisionHash() || prevSessionStatus_.getStaticIpsCount() != ss.getStaticIpsCount() ||
                ss.isContainsStaticDeviceId(GetDeviceId::instance().getDeviceId()))
        {
            if (ss.getStaticIpsCount() > 0)
            {
                serverAPI_->staticIps(apiInfo_->getAuthHash(), GetDeviceId::instance().getDeviceId(), serverApiUserRole_, true);
            }
            else
            {
                // set empty list of static ips
                apiInfo_->setStaticIps(apiinfo::StaticIps());
                updateServerLocations();
            }
        }

        if (prevSessionStatus_.getRevisionHash() != ss.getRevisionHash() || prevSessionStatus_.isPro() != ss.isPro() ||
            prevSessionStatus_.getAlc() != ss.getAlc() || (prevSessionStatus_.getStatus() != 1 && ss.getStatus() == 1))
        {
            serverAPI_->serverLocations(apiInfo_->getAuthHash(), engineSettings_.language(), serverApiUserRole_, true,
                                        ss.getRevisionHash(), ss.isPro(), engineSettings_.connectionSettings().protocol(), ss.getAlc());
        }

        if (prevSessionStatus_.getBillingPlanId() != ss.getBillingPlanId())
        {
            serverAPI_->notifications(apiInfo_->getAuthHash(), serverApiUserRole_, true);
            notificationsUpdateTimer_->start(NOTIFICATIONS_UPDATE_PERIOD);
        }

        prevSessionStatus_ = ss;
    }
}

void Engine::updateServerLocations()
{
    qCDebug(LOG_BASIC) << "Servers locations changed";
    if (!apiInfo_.isNull())
    {
        locationsModel_->setApiLocations(apiInfo_->getLocations(), apiInfo_->getStaticIps());
    }
    locationsModel_->setCustomConfigLocations(customConfigs_->getConfigs());

    if (!apiInfo_.isNull() && !apiInfo_->getForceDisconnectNodes().isEmpty())
    {
        checkForceDisconnectNode(apiInfo_->getForceDisconnectNodes());
    }
}

void Engine::updateFirewallSettings()
{
    if (firewallController_->firewallActualState())
    {
        if (connectStateController_->currentState() != CONNECT_STATE_CONNECTED)
        {
            firewallController_->firewallOn(firewallExceptions_.getIPAddressesForFirewall(), engineSettings_.isAllowLanTraffic());
        }
        else
        {
            firewallController_->firewallOn(firewallExceptions_.getIPAddressesForFirewallForConnectedState(connectionManager_->getLastConnectedIp()), engineSettings_.isAllowLanTraffic());
        }
    }
}

void Engine::addCustomRemoteIpToFirewallIfNeed()
{
    QString ip;
    QString strHost = ExtraConfig::instance().getRemoteIpFromExtraConfig();
    if (!strHost.isEmpty())
    {
        if (IpValidation::instance().isIp(strHost))
        {
            ip = strHost;
        }
        else if (IpValidation::instance().isDomain(strHost))
        {
            // make DNS-resolution for add IP to firewall exceptions
            qCDebug(LOG_BASIC) << "Make DNS-resolution for" << strHost;

            DnsRequest dnsRequest(this, strHost, DnsServersConfiguration::instance().getCurrentDnsServers());
            dnsRequest.lookupBlocked();
            if (!dnsRequest.isError())
            {
                qCDebug(LOG_BASIC) << "Resolved IP address for" << strHost << ":" << dnsRequest.ips()[0];
                ip = dnsRequest.ips()[0];
                ExtraConfig::instance().setDetectedIp(ip);
            }
            else
            {
                qCDebug(LOG_BASIC) << "Failed to resolve IP for" << strHost;
                ExtraConfig::instance().setDetectedIp("");
            }
        }
        else
        {
            ExtraConfig::instance().setDetectedIp("");
        }

        if (!ip.isEmpty())
        {
            bool bChanged = false;
            firewallExceptions_.setCustomRemoteIp(ip, bChanged);
            if (bChanged)
            {
                updateFirewallSettings();
            }
        }
    }
}

void Engine::doConnect(bool bEmitAuthError)
{
    // before connect, update ICS sharing and wait for update ICS finished
    vpnShareController_->onConnectingOrConnectedToVPNEvent(OpenVpnVersionController::instance().isUseWinTun() ? "Windscribe Windtun420" : "Windscribe VPN");
    while (vpnShareController_->isUpdateIcsInProgress())
    {
        QThread::msleep(1);
    }

    locationId_ = checkLocationIdExistingAndReturnNewIfNeed(locationId_);

    QSharedPointer<locationsmodel::BaseLocationInfo> bli = locationsModel_->getMutableLocationInfoById(locationId_);
    if (bli.isNull())
    {
        connectStateController_->setDisconnectedState(DISCONNECTED_WITH_ERROR, ProtoTypes::ConnectError::LOCATION_NOT_EXIST);
        getMyIPController_->getIPFromDisconnectedState(1);
        qCDebug(LOG_BASIC) << "Engine::connectError(LOCATION_NOT_EXIST)";
        return;
    }
    if (!bli->isExistSelectedNode())
    {
        connectStateController_->setDisconnectedState(DISCONNECTED_WITH_ERROR, ProtoTypes::ConnectError::LOCATION_NO_ACTIVE_NODES);
        getMyIPController_->getIPFromDisconnectedState(1);
        qCDebug(LOG_BASIC) << "Engine::connectError(LOCATION_NO_ACTIVE_NODES)";
        return;
    }

    locationName_ = bli->getName();

#ifdef Q_OS_WIN
    Helper_win *helper_win = dynamic_cast<Helper_win *>(helper_);
    helper_win->clearDnsOnTap();
    CheckAdapterEnable::enableIfNeed(helper_, "Windscribe VPN");
#endif

    if (!apiInfo_.isNull())
    {
        if ((!apiInfo_->getServerCredentials().isInitialized() || apiInfo_->ovpnConfigRefetchRequired()) && !locationId_.isCustomConfigsLocation())
        {
            qCDebug(LOG_BASIC) << "radius username/password empty or ovpn refetch config required, refetch server config and credentials";

            if (refetchServerCredentialsHelper_ == NULL)
            {
                refetchServerCredentialsHelper_ = new RefetchServerCredentialsHelper(this, apiInfo_->getAuthHash(), serverAPI_);
                connect(refetchServerCredentialsHelper_, &RefetchServerCredentialsHelper::finished, this, &Engine::onRefetchServerCredentialsFinished);
                refetchServerCredentialsHelper_->startRefetch();
            }
        }
        else
        {
            if (!bli->locationId().isCustomConfigsLocation() && !bli->locationId().isStaticIpsLocation())
            {
                if (apiInfo_->getServerCredentials().isInitialized())
                {
                    qCDebug(LOG_BASIC) << "radiusUsername openvpn: " << apiInfo_->getServerCredentials().usernameForOpenVpn();
                    qCDebug(LOG_BASIC) << "radiusUsername ikev2: " << apiInfo_->getServerCredentials().usernameForIkev2();
                }
            }
            qCDebug(LOG_BASIC) << "Connecting to" << locationName_;

            connectionManager_->clickConnect(apiInfo_->getOvpnConfig(), apiInfo_->getServerCredentials(), bli,
                engineSettings_.connectionSettings(), apiInfo_->getPortMap(),
                ProxyServerController::instance().getCurrentProxySettings(), bEmitAuthError, engineSettings_.getCustomOvpnConfigsPath());
        }
    }
    // for custom configs without login
    else
    {
        qCDebug(LOG_BASIC) << "Connecting to" << locationName_;

        connectionManager_->clickConnect("", apiinfo::ServerCredentials(), bli,
            engineSettings_.connectionSettings(), apiinfo::PortMap(),
            ProxyServerController::instance().getCurrentProxySettings(), bEmitAuthError, engineSettings_.getCustomOvpnConfigsPath());
    }
}

LocationID Engine::checkLocationIdExistingAndReturnNewIfNeed(const LocationID &locationId)
{
    //todo
    return locationId;
    /*QSharedPointer<MutableLocationInfo> mli = serversModel_->getMutableLocationInfoById(locationId);
    if (mli.isNull() || !mli->isExistSelectedNode())
    {
        // this is city
        if (!locationId.getCity().isEmpty())
        {
            // try select another city for this location id
            QStringList cities = serversModel_->getCitiesForLocationId(locationId.getId());
            if (!cities.empty())
            {
                return LocationID(locationId.getId(), cities[0]);
            }
            else
            {
                return LocationID(LocationID::BEST_LOCATION_ID);
            }
        }
        //this is country
        else
        {
            return LocationID(LocationID::BEST_LOCATION_ID);
        }
    }
    else
    {
        return locationId;
    }*/
}

void Engine::doDisconnectRestoreStuff()
{
    vpnShareController_->onDisconnectedFromVPNEvent();

    serverAPI_->enableProxy();
    locationsModel_->enableProxy();
    DnsServersConfiguration::instance().setDnsServersPolicy(engineSettings_.getDnsPolicy());

#if defined (Q_OS_MAC) || defined(Q_OS_LINUX)
    firewallController_->setInterfaceToSkip_posix("");
#endif

    bool bChanged;
    firewallExceptions_.setConnectingIp("", bChanged);
    firewallExceptions_.setDNSServerIp("", bChanged);

    if (firewallController_->firewallActualState())
    {
        QString ips = firewallExceptions_.getIPAddressesForFirewall();
        firewallController_->firewallOn(ips, engineSettings_.isAllowLanTraffic());
    }

#ifdef Q_OS_WIN
    Helper_win *helper_win = dynamic_cast<Helper_win *>(helper_);
    helper_win->setIPv6EnabledInFirewall(true);
#endif

#ifdef Q_OS_MAC
    Ipv6Controller_mac::instance().restoreIpv6();
#endif
}

void Engine::stopPacketDetectionImpl()
{
    packetSizeController_->earlyStop();
}

void Engine::onConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON /*reason*/, ProtoTypes::ConnectError /*err*/, const LocationID & /*location*/)
{
    if (helper_)
    {
        if (state != CONNECT_STATE_CONNECTED)
        {
            helper_->sendConnectStatus(false, engineSettings_.isCloseTcpSockets(), engineSettings_.isAllowLanTraffic(), AdapterGatewayInfo(), AdapterGatewayInfo(), QString(), ProtocolType());
        }
    }
}

void Engine::updateProxySettings()
{
    if (ProxyServerController::instance().updateProxySettings(engineSettings_.proxySettings())) {
        const auto &proxySettings = ProxyServerController::instance().getCurrentProxySettings();
        serverAPI_->setProxySettings(proxySettings);
        locationsModel_->setProxySettings(proxySettings);
        firewallExceptions_.setProxyIP(proxySettings);
        updateFirewallSettings();
        if (connectStateController_->currentState() == CONNECT_STATE_DISCONNECTED)
            getMyIPController_->getIPFromDisconnectedState(500);
    }
}

bool Engine::verifyContentsSha256(const QString &filename, const QString &compareHash)
{
    QFile file(filename);
    if (!file.open(QIODeviceBase::ReadOnly))
    {
        qCDebug(LOG_BASIC) << "Failed to open installer for reading";
        return false;
    }
    QByteArray contentsBytes = file.readAll();
    QString sha256Hash = QCryptographicHash::hash(contentsBytes, QCryptographicHash::Sha256).toHex();
    if (sha256Hash == compareHash)
    {
        return true;
    }
    return false;
}

void Engine::onWireGuardKeyLimitUserResponse(bool deleteOldestKey)
{
    connectionManager_->onWireGuardKeyLimitUserResponse(deleteOldestKey);
}
