#include "engine.h"
#include "utils/utils.h"
#include "utils/logger.h"
#include "utils/mergelog.h"
#include "crossplatformobjectfactory.h"
#include "connectionmanager/connectionmanager.h"
#include "connectionmanager/finishactiveconnections.h"
#include "proxy/proxyservercontroller.h"
#include "openvpnversioncontroller.h"
#include "connectstatecontroller/connectstatecontroller.h"
#include "dnsresolver/dnsresolver.h"
#include "openvpnversioncontroller.h"
#include "utils/extraconfig.h"
#include "utils/ipvalidation.h"
#include "getdeviceid.h"
#include <QCoreApplication>
#include <QDir>
#include <google/protobuf/util/message_differencer.h>

#ifdef Q_OS_WIN
    #include "utils/bfe_service_win.h"
    #include "engine/dnsinfo_win.h"
    #include "engine/taputils/checkadapterenable.h"
    #include "engine/taputils/tapinstall_win.h"
    #include "engine/adaptermetricscontroller_win.h"
#elif defined Q_OS_MAC
    #include "ipv6controller_mac.h"
    #include "utils/macutils.h"
    #include "networkstatemanager/reachabilityevents.h"
#endif

Engine::Engine(const EngineSettings &engineSettings) : QObject(NULL),
    engineSettings_(engineSettings),
    helper_(NULL),
    networkStateManager_(NULL),
    firewallController_(NULL),
    serverAPI_(NULL),
    serverLocationsApiWrapper_(NULL),
    connectionManager_(NULL),
    connectStateController_(NULL),
    serverApiUserRole_(0),
    getMyIPController_(NULL),
    vpnShareController_(NULL),
    emergencyController_(NULL),
    customOvpnConfigs_(NULL),
    customOvpnAuthCredentialsStorage_(NULL),
    networkDetectionManager_(NULL),
    macAddressController_(NULL),
    keepAliveManager_(NULL),
    packetSizeController_(NULL),
#ifdef Q_OS_WIN
    measurementCpuUsage_(NULL),
#endif
    inititalizeHelper_(NULL),
    bInitialized_(false),
    loginController_(NULL),
    loginState_(LOGIN_NONE),
    loginSettingsMutex_(QMutex::Recursive),
    checkUpdateTimer_(NULL),
    updateSessionStatusTimer_(NULL),
    notificationsUpdateTimer_(NULL),
    nodesSpeedRatings_(NULL),
    serversModel_(NULL),
    nodesSpeedStore_(NULL),
    refetchServerCredentialsHelper_(NULL),
    mutexApiInfo_(QMutex::Recursive),
    isBlockConnect_(false),
    isCleanupFinished_(false),
    isNeedReconnectAfterRequestUsernameAndPassword_(false),
    online_(false),
    mss_(-1),
    packetSizeControllerThread_(NULL),
    runningPacketDetection_(false)
{
    connectStateController_ = new ConnectStateController(NULL);
    connect(connectStateController_, SIGNAL(stateChanged(CONNECT_STATE,DISCONNECT_REASON,CONNECTION_ERROR,LocationID)), SLOT(onConnectStateChanged(CONNECT_STATE,DISCONNECT_REASON,CONNECTION_ERROR,LocationID)));
    emergencyConnectStateController_ = new ConnectStateController(NULL);
    nodesSpeedRatings_ = new NodesSpeedRatings();
    OpenVpnVersionController::instance().setUseWinTun(engineSettings.isUseWintun());
}

Engine::~Engine()
{
    SAFE_DELETE(connectStateController_);
    SAFE_DELETE(emergencyConnectStateController_);
    SAFE_DELETE(nodesSpeedRatings_);
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
    QMutexLocker locker(&mutex_);
    QMetaObject::invokeMethod(this, "cleanupImpl", Q_ARG(bool, isExitWithRestart), Q_ARG(bool, isFirewallChecked),
                                                   Q_ARG(bool, isFirewallAlwaysOn), Q_ARG(bool, isLaunchOnStart));
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

bool Engine::isCanLoginWithAuthHash()
{
    QMutexLocker locker(&mutex_);
    QSettings settings;
    return settings.contains("authHash");
}

void Engine::loginWithAuthHash()
{
    QMutexLocker locker(&mutex_);
    Q_ASSERT(bInitialized_);
    Q_ASSERT(loginState_ != LOGIN_IN_PROGRESS);

    QSettings settings;
    {
        QMutexLocker lockerLoginSettings(&loginSettingsMutex_);
        loginSettings_ = LoginSettings(settings.value("authHash").toString());
    }
    loginState_ = LOGIN_IN_PROGRESS;
    QMetaObject::invokeMethod(this, "loginImpl", Q_ARG(bool, false));
}

void Engine::loginWithCustomAuthHash(const QString &authHash)
{
    QMutexLocker locker(&mutex_);
    Q_ASSERT(bInitialized_);
    //Q_ASSERT(loginState_ != LOGIN_IN_PROGRESS);

    //QSettings settings;
    //settings.remove("authHash");
    {
        QMutexLocker lockerLoginSettings(&loginSettingsMutex_);
        loginSettings_ = LoginSettings(authHash);
    }
    loginState_ = LOGIN_IN_PROGRESS;
    QMetaObject::invokeMethod(this, "loginImpl", Q_ARG(bool, false));
}

void Engine::loginWithUsernameAndPassword(const QString &username, const QString &password)
{
    QMutexLocker locker(&mutex_);
    Q_ASSERT(bInitialized_);
    Q_ASSERT(loginState_ != LOGIN_IN_PROGRESS);

    {
        QMutexLocker lockerLoginSettings(&loginSettingsMutex_);
        loginSettings_ = LoginSettings(username, password);
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
    QSettings settings;
    if (settings.contains("authHash"))
    {
        // try load ApiInfo from settings
        QSharedPointer<ApiInfo> apiInfo(new ApiInfo());
        if (apiInfo->loadFromSettings())
        {
            return true;
        }
    }
    return false;
}

void Engine::signOut()
{
    QMetaObject::invokeMethod(this, "signOutImpl");
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
    QMutexLocker locker(&mutex_);
    helper_->setIPv6EnabledInOS(b);
}

bool Engine::IPv6StateInOS()
{
    QMutexLocker locker(&mutex_);
    return helper_->IPv6StateInOS();
}

LoginSettings Engine::getLastLoginSettings()
{
    QMutexLocker lockerLoginSettings(&loginSettingsMutex_);
    return loginSettings_;
}

QSharedPointer<PortMap> Engine::getCurrentPortMap()
{
    QMutexLocker locker(&mutexApiInfo_);
    Q_ASSERT(!apiInfo_.isNull());
    QSharedPointer<PortMap> src = apiInfo_->getPortMap();
    QSharedPointer<PortMap> portMap(new PortMap);
    *portMap = *src;
    return portMap;
}

QString Engine::getCurrentUserId()
{
    QMutexLocker locker(&mutexApiInfo_);
    Q_ASSERT(!apiInfo_.isNull());
    return apiInfo_->getSessionStatus()->userId;
}

// return empty string, if not logged
QString Engine::getCurrentUserName()
{
    QMutexLocker locker(&mutexApiInfo_);
    if (!apiInfo_.isNull())
    {
        return apiInfo_->getSessionStatus()->username;
    }
    else
    {
        return "";
    }
}

QString Engine::getAuthHash()
{
    QMutexLocker locker(&mutexApiInfo_);
    if (!apiInfo_.isNull())
    {
        return apiInfo_->getAuthHash();
    }
    else
    {
        QSettings settings;
        return settings.value("authHash", "").toString();
    }
}

LocationID Engine::getLocationIdByName(const QString &location)
{
    QMutexLocker locker(&mutex_);
    if (bInitialized_)
    {
        return serversModel_->getLocationIdByName(location);
    }
    else
    {
        return LocationID();
    }
}

void Engine::clearCredentials()
{
    QMutexLocker locker(&mutexApiInfo_);
    if (!apiInfo_.isNull())
    {
        return apiInfo_->setServerCredentials(ServerCredentials());
    }
}

IServersModel *Engine::getServersModel()
{
    Q_ASSERT(serversModel_ != NULL);
    return serversModel_;
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

void Engine::clearSpeedRatings()
{
    nodesSpeedRatings_->clear();
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
        emergencyConnectStateController_->setDisconnectedState(DISCONNECTED_ITSELF, NO_CONNECT_ERROR);
        emit emergencyDisconnected();
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
        emergencyConnectStateController_->setDisconnectedState(DISCONNECTED_ITSELF, NO_CONNECT_ERROR);
        emit emergencyDisconnected();
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

void Engine::updateCurrentNetworkInterface(bool requested)
{
    QMetaObject::invokeMethod(this, "updateCurrentNetworkInterfaceImpl", Q_ARG(bool, requested));
}

void Engine::updateCurrentInternetConnectivity()
{
    QMetaObject::invokeMethod(this, "updateCurrentInternetConnectivityImpl");
}

void Engine::forceUpdateSessionStatus()
{
    QTimer::singleShot(1, this, SLOT(onUpdateSessionStatusTimer()));
}

void Engine::forceUpdateServerLocations()
{
    QMetaObject::invokeMethod(this, "forceUpdateServerLocationsImpl");
}

void Engine::detectPacketSizeMss()
{
    QMetaObject::invokeMethod(this, "detectPacketSizeMssImpl");
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

void Engine::init()
{
    isCleanupFinished_ = false;

    DnsResolver::instance().init();
    DnsResolver::instance().setDnsPolicy(engineSettings_.getDnsPolicy());
    firewallExceptions_.setDnsPolicy(engineSettings_.getDnsPolicy());

    helper_ = CrossPlatformObjectFactory::createHelper(this);
    connect(helper_, SIGNAL(lostConnectionToHelper()), SLOT(onLostConnectionToHelper()));
    helper_->startInstallHelper();

#ifdef Q_OS_MAC
    Ipv6Controller_mac::instance().setHelper(helper_);
    ReachAbilityEvents::instance().init();
#endif

    networkDetectionManager_ = CrossPlatformObjectFactory::createNetworkDetectionManager(this, helper_);
    networkStateManager_ = CrossPlatformObjectFactory::createNetworkStateManager(this, networkDetectionManager_);

    ProtoTypes::MacAddrSpoofing macAddrSpoofing = engineSettings_.getMacAddrSpoofing();
    *macAddrSpoofing.mutable_network_interfaces() = Utils::currentNetworkInterfaces(true);
    setSettingsMacAddressSpoofing(macAddrSpoofing);

    connect(networkDetectionManager_, SIGNAL(networkChanged(ProtoTypes::NetworkInterface)), SLOT(onNetworkChange(ProtoTypes::NetworkInterface)));
    connect(networkStateManager_, SIGNAL(stateChanged(bool,QString)), SLOT(onNetworkStateManagerStateChanged(bool, QString)));

    macAddressController_ = CrossPlatformObjectFactory::createMacAddressController(this, networkDetectionManager_, helper_);
    macAddressController_->initMacAddrSpoofing(macAddrSpoofing);
    connect(macAddressController_, SIGNAL(macAddrSpoofingChanged(ProtoTypes::MacAddrSpoofing)), SLOT(onMacAddressSpoofingChanged(ProtoTypes::MacAddrSpoofing)));
    connect(macAddressController_, SIGNAL(sendUserWarning(ProtoTypes::UserWarningType)), SLOT(onMacAddressControllerSendUserWarning(ProtoTypes::UserWarningType)));

    packetSizeControllerThread_ = new QThread(this);

    ProtoTypes::PacketSize packetSize = engineSettings_.getPacketSize();
    packetSizeController_ = new PacketSizeController(nullptr);
    packetSizeController_->setPacketSize(packetSize);
    mss_ = packetSize.mss();
    connect(packetSizeController_, SIGNAL(packetSizeChanged(bool, int)), SLOT(onPacketSizeControllerPacketSizeChanged(bool, int)));
    connect(packetSizeController_, SIGNAL(finishedPacketSizeDetection()), SLOT(onPacketSizeControllerFinishedSizeDetection()));
    packetSizeController_->moveToThread(packetSizeControllerThread_);
    packetSizeControllerThread_->start(QThread::LowPriority);

    if (packetSize.is_automatic())
    {
        qCDebug(LOG_BASIC) << "Packet Size Auto ON";
        detectPacketSizeMssImpl();
    }

    firewallController_ = CrossPlatformObjectFactory::createFirewallController(this, helper_);

    serverAPI_ = new ServerAPI(this, networkStateManager_);
    connect(serverAPI_, SIGNAL(sessionAnswer(SERVER_API_RET_CODE,QSharedPointer<SessionStatus>, uint)),
                        SLOT(onSessionAnswer(SERVER_API_RET_CODE,QSharedPointer<SessionStatus>, uint)), Qt::QueuedConnection);
    connect(serverAPI_, SIGNAL(checkUpdateAnswer(bool,QString,bool,int,QString,bool,bool,uint)), SLOT(onCheckUpdateAnswer(bool,QString,bool,int,QString,bool,bool,uint)), Qt::QueuedConnection);
    connect(serverAPI_, SIGNAL(hostIpsChanged(QStringList)), SLOT(onHostIPsChanged(QStringList)));
    connect(serverAPI_, SIGNAL(notificationsAnswer(SERVER_API_RET_CODE,QSharedPointer<ApiNotifications>,uint)),
                        SLOT(onNotificationsAnswer(SERVER_API_RET_CODE,QSharedPointer<ApiNotifications>,uint)));
    connect(serverAPI_, SIGNAL(serverConfigsAnswer(SERVER_API_RET_CODE,QByteArray,uint)), SLOT(onServerConfigsAnswer(SERVER_API_RET_CODE,QByteArray,uint)));
    connect(serverAPI_, SIGNAL(debugLogAnswer(SERVER_API_RET_CODE,uint)), SLOT(onDebugLogAnswer(SERVER_API_RET_CODE,uint)));
    connect(serverAPI_, SIGNAL(confirmEmailAnswer(SERVER_API_RET_CODE,uint)), SLOT(onConfirmEmailAnswer(SERVER_API_RET_CODE,uint)));
    connect(serverAPI_, SIGNAL(staticIpsAnswer(SERVER_API_RET_CODE,QSharedPointer<StaticIpsLocation>, uint)), SLOT(onStaticIpsAnswer(SERVER_API_RET_CODE,QSharedPointer<StaticIpsLocation>, uint)), Qt::QueuedConnection);

    nodesSpeedStore_ = new NodesSpeedStore(this);

    serverAPI_->setIgnoreSslErrors(engineSettings_.isIgnoreSslErrors());
    serverApiUserRole_ = serverAPI_->getAvailableUserRole();

    serverLocationsApiWrapper_ = new ServerLocationsApiWrapper(this, nodesSpeedStore_, serverAPI_);
    connect(serverLocationsApiWrapper_, SIGNAL(serverLocationsAnswer(SERVER_API_RET_CODE,QVector<QSharedPointer<ServerLocation> >,QStringList, uint)),
                        SLOT(onServerLocationsAnswer(SERVER_API_RET_CODE,QVector<QSharedPointer<ServerLocation> >,QStringList, uint)), Qt::QueuedConnection);
    connect(serverLocationsApiWrapper_, SIGNAL(updatedBestLocation(QVector<QSharedPointer<ServerLocation> > &)), SLOT(onBestLocationChanged(QVector<QSharedPointer<ServerLocation> > &)));
    connect(serverLocationsApiWrapper_, SIGNAL(updateFirewallIpsForLocations(QVector<QSharedPointer<ServerLocation> >&)), SLOT(onUpdateFirewallIpsForLocations(QVector<QSharedPointer<ServerLocation> >&)));

    customOvpnAuthCredentialsStorage_ = new CustomOvpnAuthCredentialsStorage();

    connectionManager_ = new ConnectionManager(this, helper_, networkStateManager_, serverAPI_, customOvpnAuthCredentialsStorage_);
    connectionManager_->setMss(mss_);
    connect(connectionManager_, SIGNAL(connected()), SLOT(onConnectionManagerConnected()));
    connect(connectionManager_, SIGNAL(disconnected(DISCONNECT_REASON)), SLOT(onConnectionManagerDisconnected(DISCONNECT_REASON)));
    connect(connectionManager_, SIGNAL(reconnecting()), SLOT(onConnectionManagerReconnecting()));
    connect(connectionManager_, SIGNAL(errorDuringConnection(CONNECTION_ERROR)), SLOT(onConnectionManagerError(CONNECTION_ERROR)));
    connect(connectionManager_, SIGNAL(statisticsUpdated(quint64,quint64, bool)), SLOT(onConnectionManagerStatisticsUpdated(quint64,quint64, bool)));
    connect(connectionManager_, SIGNAL(testTunnelResult(bool, QString)), SLOT(onConnectionManagerTestTunnelResult(bool, QString)));
    connect(connectionManager_, SIGNAL(connectingToHostname(QString)), SLOT(onConnectionManagerConnectingToHostname(QString)));
    connect(connectionManager_, SIGNAL(protocolPortChanged(ProtoTypes::Protocol, uint)), SLOT(onConnectionManagerProtocolPortChanged(ProtoTypes::Protocol, uint)));
    connect(connectionManager_, SIGNAL(internetConnectivityChanged(bool)), SLOT(onConnectionManagerInternetConnectivityChanged(bool)));
    connect(connectionManager_, SIGNAL(requestUsername(QString)), SLOT(onConnectionManagerRequestUsername(QString)));
    connect(connectionManager_, SIGNAL(requestPassword(QString)), SLOT(onConnectionManagerRequestPassword(QString)));

    serversModel_ = new ServersModel(this, connectStateController_, networkStateManager_, nodesSpeedRatings_, nodesSpeedStore_);
    connect(serversModel_, SIGNAL(customOvpnConfgsIpsChanged(QStringList)), SLOT(onCustomOvpnConfgsIpsChanged(QStringList)));

    getMyIPController_ = new GetMyIPController(this, serverAPI_, networkStateManager_);
    connect(getMyIPController_, SIGNAL(answerMyIP(QString,bool,bool)), SLOT(onMyIpAnswer(QString,bool,bool)));

    vpnShareController_ = new VpnShareController(this, helper_);
    connect(vpnShareController_, SIGNAL(connectedWifiUsersChanged(int)), SIGNAL(vpnSharingConnectedWifiUsersCountChanged(int)));
    connect(vpnShareController_, SIGNAL(connectedProxyUsersChanged(int)), SIGNAL(vpnSharingConnectedProxyUsersCountChanged(int)));

    keepAliveManager_ = new KeepAliveManager(this, connectStateController_);
    keepAliveManager_->setEnabled(engineSettings_.isKeepAliveEnabled());

    emergencyController_ = new EmergencyController(this, helper_);
    emergencyController_->setMss(mss_);
    connect(emergencyController_, SIGNAL(connected()), SLOT(onEmergencyControllerConnected()));
    connect(emergencyController_, SIGNAL(disconnected(DISCONNECT_REASON)), SLOT(onEmergencyControllerDisconnected(DISCONNECT_REASON)));
    connect(emergencyController_, SIGNAL(errorDuringConnection(CONNECTION_ERROR)), SLOT(onEmergencyControllerError(CONNECTION_ERROR)));

    customOvpnConfigs_ = new CustomOvpnConfigs(this);
    customOvpnConfigs_->changeDir(engineSettings_.getCustomOvpnConfigsPath());
    connect(customOvpnConfigs_, SIGNAL(changed()), SLOT(onCustomOvpnConfigsChanged()));

    checkUpdateTimer_ = new QTimer(this);
    connect(checkUpdateTimer_, SIGNAL(timeout()), SLOT(onStartCheckUpdate()));
    updateSessionStatusTimer_ = new SessionStatusTimer(this, connectStateController_);
    connect(updateSessionStatusTimer_, SIGNAL(needUpdateRightNow()), SLOT(onUpdateSessionStatusTimer()));
    notificationsUpdateTimer_ = new QTimer(this);
    connect(notificationsUpdateTimer_, SIGNAL(timeout()), SLOT(getNewNotifications()));

#ifdef Q_OS_WIN
    measurementCpuUsage_ = new MeasurementCpuUsage(this, helper_, connectStateController_);
    connect(measurementCpuUsage_, SIGNAL(detectionCpuUsageAfterConnected(QStringList)), SIGNAL(detectionCpuUsageAfterConnected(QStringList)));
    measurementCpuUsage_->setEnabled(engineSettings_.isCloseTcpSockets());
#endif

    updateProxySettings();

    inititalizeHelper_ = new InitializeHelper(this, helper_);
    connect(inititalizeHelper_, SIGNAL(finished(INIT_HELPER_RET)), SLOT(onInitializeHelper(INIT_HELPER_RET)));
    inititalizeHelper_->start();
}

void Engine::onLostConnectionToHelper()
{
    emit lostConnectionToHelper();
}

void Engine::onInitializeHelper(INIT_HELPER_RET ret)
{
    if (ret == INIT_HELPER_SUCCESS)
    {
        QMutexLocker locker(&mutex_);
        bInitialized_ = true;

#ifdef Q_OS_WIN
        FinishActiveConnections::finishAllActiveConnections_win(helper_);
#else
        QString kextPath = QCoreApplication::applicationDirPath() + "/../Helpers/WindscribeKext.kext";
        kextPath = QDir::cleanPath(kextPath);
        if (helper_->setKextPath(kextPath))
        {
            qCDebug(LOG_BASIC) << "Kext path set:" << kextPath;
        }
        else
        {
            qCDebug(LOG_BASIC) << "Kext path set failed";
            emit initFinished(ENGINE_INIT_HELPER_FAILED);
        }

        // turn off split tunneling (for case the state remains from the last launch)
        helper_->sendConnectStatus(false, SplitTunnelingNetworkInfo());
        helper_->setSplitTunnelingSettings(false, false, false, QStringList(), QStringList(), QStringList());

        //todo: Mac finish active connections
#endif

    #ifdef Q_OS_WIN
        // check BFE service status
        if (!BFE_Service_win::instance().isBFEEnabled())
        {
            emit initFinished(ENGINE_INIT_BFE_SERVICE_FAILED);
        }
        else
        {
            emit initFinished(ENGINE_INIT_SUCCESS);
        }
    #else
        emit initFinished(ENGINE_INIT_SUCCESS);
    #endif
    }
    else if (ret == INIT_HELPER_FAILED)
    {
        emit initFinished(ENGINE_INIT_HELPER_FAILED);
    }
    else
    {
        Q_ASSERT(false);
    }
}

void Engine::cleanupImpl(bool isExitWithRestart, bool isFirewallChecked, bool isFirewallAlwaysOn, bool isLaunchOnStart)
{
    qCDebug(LOG_BASIC) << "Cleanup started";

    if (loginController_)
    {
        SAFE_DELETE(loginController_);
        loginState_ = LOGIN_NONE;
    }

    // stop timers
    if (checkUpdateTimer_)
    {
        checkUpdateTimer_->stop();
    }
    if (updateSessionStatusTimer_)
    {
        updateSessionStatusTimer_->stop();
    }
    if (notificationsUpdateTimer_)
    {
        notificationsUpdateTimer_->stop();
    }

    {
        QMutexLocker locker(&mutexApiInfo_);
        if (!apiInfo_.isNull())
        {
            apiInfo_->saveToSettings();
        }
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
#ifdef Q_OS_MAC
    helper_->sendConnectStatus(false, SplitTunnelingNetworkInfo());
#endif
    helper_->setSplitTunnelingSettings(false, false, false, QStringList(), QStringList(), QStringList());

#ifdef Q_OS_WIN
    if (helper_)
    {
        helper_->removeWindscribeNetworkProfiles();
    }
#endif

    if (!isExitWithRestart)
    {
        vpnShareController_->stopWifiSharing();
        vpnShareController_->stopProxySharing();
    }

    if (helper_ && firewallController_)
    {
        if (isFirewallChecked)
        {
            if (isExitWithRestart)
            {
                if (isLaunchOnStart)
                {
                    helper_->enableFirewallOnBoot(true);
                }
                else
                {
                    if (isFirewallAlwaysOn)
                    {
                        helper_->enableFirewallOnBoot(true);
                    }
                    else
                    {
                        helper_->enableFirewallOnBoot(false);
                        firewallController_->firewallOff();
                    }
                }
            }
            else  // if exit without restart
            {
                if (isLaunchOnStart)
                {
                    if (isFirewallAlwaysOn)
                    {
                        helper_->enableFirewallOnBoot(true);
                    }
                    else
                    {
                        helper_->enableFirewallOnBoot(false);
                        firewallController_->firewallOff();
                    }
                }
                else
                {
                    if (isFirewallAlwaysOn)
                    {
                        helper_->enableFirewallOnBoot(true);
                    }
                    else
                    {
                        helper_->enableFirewallOnBoot(false);
                        firewallController_->firewallOff();
                    }
                }
            }
        }
        else  // if (!isFirewallChecked)
        {
            firewallController_->firewallOff();
            helper_->enableFirewallOnBoot(false);
        }

        helper_->setIPv6EnabledInFirewall(true);
    #ifdef Q_OS_MAC
        Ipv6Controller_mac::instance().restoreIpv6();
    #endif
    }

    SAFE_DELETE(vpnShareController_);
    SAFE_DELETE(emergencyController_);
    SAFE_DELETE(connectionManager_);
    SAFE_DELETE(customOvpnConfigs_);
    SAFE_DELETE(customOvpnAuthCredentialsStorage_);
    SAFE_DELETE(firewallController_);
    SAFE_DELETE(keepAliveManager_);
    SAFE_DELETE(inititalizeHelper_);
#ifdef Q_OS_WIN
    SAFE_DELETE(measurementCpuUsage_);
#endif
    SAFE_DELETE(helper_);
    SAFE_DELETE(getMyIPController_);
    SAFE_DELETE(serverLocationsApiWrapper_);
    SAFE_DELETE(serverAPI_);
    SAFE_DELETE(checkUpdateTimer_);
    SAFE_DELETE(updateSessionStatusTimer_);
    SAFE_DELETE(notificationsUpdateTimer_);
    SAFE_DELETE(serversModel_);
    SAFE_DELETE(nodesSpeedStore_);
    SAFE_DELETE(networkStateManager_);
    SAFE_DELETE(networkDetectionManager_);
    DnsResolver::instance().stop();
    isCleanupFinished_ = true;
    emit cleanupFinished();
    qCDebug(LOG_BASIC) << "Cleanup finished";
}

void Engine::enableBFE_winImpl()
{
#ifdef Q_OS_WIN
    bool bSuccess = BFE_Service_win::instance().checkAndEnableBFE(helper_);
    if (bSuccess)
    {
        emit bfeEnableFinished(ENGINE_INIT_SUCCESS);
    }
    else
    {
        emit bfeEnableFinished(ENGINE_INIT_BFE_SERVICE_FAILED);
    }
#endif
}

void Engine::loginImpl(bool bSkipLoadingFromSettings)
{
    QMutexLocker lockerLoginSettings(&loginSettingsMutex_);
    /*QMutexLocker locker(&mutex_);
    {
        QMutexLocker lockerApiInfo(&mutexApiInfo_);
        apiInfo_.reset();
    }*/

    QSettings settings;
    if (!bSkipLoadingFromSettings && settings.contains("authHash"))
    {
        QString authHash = settings.value("authHash").toString();

        // try load ApiInfo from settings
        QSharedPointer<ApiInfo> apiInfo(new ApiInfo());

        if (apiInfo->loadFromSettings())
        {
            apiInfo->setAuthHash(authHash);

            qCDebug(LOG_BASIC) << "ApiInfo readed from settings";

            {
                QMutexLocker lockerApiInfo(&mutexApiInfo_);
                apiInfo_ = apiInfo;

                loginSettings_.setServerCredentials(apiInfo_->getServerCredentials());
            }

            prevSessionStatus_.set(apiInfo_->getSessionStatus()->isPremium, apiInfo_->getSessionStatus()->billingPlanId,
                                   apiInfo_->getSessionStatus()->getRevisionHash(), apiInfo_->getSessionStatus()->alc,
                                   apiInfo_->getSessionStatus()->staticIps);
            updateSessionStatus();
            updateServerLocations();
            updateCurrentNetworkInterface();
            emit loginFinished(true);
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
    QMutexLocker locker(&mutexApiInfo_);
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

    if (isBlockConnect_ && locationId_.getId() != LocationID::CUSTOM_OVPN_CONFIGS_LOCATION_ID)
    {
        connectStateController_->setDisconnectedState(DISCONNECTED_WITH_ERROR, CONNECTION_BLOCKED);
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
            emit firewallStateChanged(true);
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
    QMutexLocker locker(&mutexApiInfo_);
    QString userName;
    if (!apiInfo_.isNull())
    {
        userName = apiInfo_->getSessionStatus()->username;
    }
    QString log = MergeLog::mergePrevLogs();
    log += "================================================================================================================================================================================================\n";
    log += "================================================================================================================================================================================================\n";
    log += MergeLog::mergeLogs();
    serverAPI_->debugLog(userName, log, serverApiUserRole_, true);
}

// function consists of two parts (first - disconnect if need, second - do other signout stuff)
void Engine::signOutImpl()
{
    if (!connectionManager_->isDisconnected())
    {
        connectionManager_->setProperty("senderSource", "signOutImpl");
        connectionManager_->clickDisconnect();
    }
    else
    {
        signOutImplAfterDisconnect();
    }
}

void Engine::signOutImplAfterDisconnect()
{
    if (loginController_)
    {
        SAFE_DELETE(loginController_);
        loginState_ = LOGIN_NONE;
    }
    checkUpdateTimer_->stop();
    updateSessionStatusTimer_->stop();
    notificationsUpdateTimer_->stop();

    lastCopyOfServerlocations_.clear();
    serversModel_->clear();
    prevSessionStatus_.clear();

    helper_->enableFirewallOnBoot(false);

    {
        QMutexLocker locker(&mutexApiInfo_);
        if (!apiInfo_.isNull())
        {
            serverAPI_->deleteSession(apiInfo_->getAuthHash(), serverApiUserRole_, true);
            apiInfo_->removeFromSettings();
            apiInfo_.reset();
            QSettings settings;
            settings.remove("authHash");
        }
    }

    firewallController_->firewallOff();
    emit firewallStateChanged(false);

    emit signOutFinished();
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
            customOvpnAuthCredentialsStorage_->setAuthCredentials(connectionManager_->getCustomOvpnConfigFilePath(), username, password);
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
            customOvpnAuthCredentialsStorage_->setAuthCredentials(connectionManager_->getCustomOvpnConfigFilePath(), "", password);
        }
        connectionManager_->continueWithPassword(password, isNeedReconnectAfterRequestUsernameAndPassword_);
    }
}

void Engine::gotoCustomOvpnConfigModeImpl()
{
    updateServerLocations();
    emit gotoCustomOvpnConfigModeFinished();
}

void Engine::updateCurrentInternetConnectivityImpl()
{
    online_ = networkStateManager_->isOnline();
    emit internetConnectivityChanged(online_);
}

void Engine::updateCurrentNetworkInterfaceImpl(bool requested)
{
    networkDetectionManager_->updateCurrentNetworkInterface(requested);
}

void Engine::firewallOnImpl()
{
    QString ips = firewallExceptions_.getIPAddressesForFirewall();
    firewallController_->firewallOn(ips, engineSettings_.isAllowLanTraffic());
    emit firewallStateChanged(true);
}

void Engine::firewallOffImpl()
{
    firewallController_->firewallOff();
    emit firewallStateChanged(false);
}

void Engine::speedRatingImpl(int rating, const QString &localExternalIp)
{
    if (rating == 0)
    {
        nodesSpeedRatings_->thumbDownRatingForNode(lastConnectingHostname_);
    }
    else if (rating == 1)
    {
        nodesSpeedRatings_->thumbUpRatingForNode(lastConnectingHostname_);
    }
    else
    {
        qCDebug(LOG_BASIC) << "Engine::speedRatingImpl, incorrect rating value =" << rating;
        return;

    }

    QMutexLocker locker(&mutexApiInfo_);
    serverAPI_->speedRating(apiInfo_->getAuthHash(), lastConnectingHostname_, localExternalIp, rating, serverApiUserRole_, true);
}

void Engine::setSettingsImpl(const EngineSettings &engineSettings)
{
    qCDebug(LOG_BASIC) << "Engine::setSettingsImpl";

    bool isAllowLanTrafficChanged = engineSettings_.isAllowLanTraffic() != engineSettings.isAllowLanTraffic();
    bool isBetaChannelChanged = engineSettings_.isBetaChannel() != engineSettings.isBetaChannel();
    bool isLanguageChanged = engineSettings_.language() != engineSettings.language();
    bool isProtocolChanged = !engineSettings_.connectionSettings().protocol().isEqual(engineSettings.connectionSettings().protocol());
    bool isCloseTcpSocketsChanged = engineSettings_.isCloseTcpSockets() != engineSettings.isCloseTcpSockets();
    bool isDnsPolicyChanged = engineSettings_.getDnsPolicy() != engineSettings.getDnsPolicy();
    bool isCustomOvpnConfigsPathChanged = engineSettings_.getCustomOvpnConfigsPath() != engineSettings.getCustomOvpnConfigsPath();
    bool isMACSpoofingChanged = !google::protobuf::util::MessageDifferencer::Equals(engineSettings_.getMacAddrSpoofing(), engineSettings.getMacAddrSpoofing());
    bool isPacketSizeChanged =  !google::protobuf::util::MessageDifferencer::Equals(engineSettings_.getPacketSize(),      engineSettings.getPacketSize());
    engineSettings_ = engineSettings;

    if (isDnsPolicyChanged)
    {
        firewallExceptions_.setDnsPolicy(engineSettings_.getDnsPolicy());
        DnsResolver::instance().setDnsPolicy(engineSettings_.getDnsPolicy());
    }

    if (isAllowLanTrafficChanged || isDnsPolicyChanged)
    {
        updateFirewallSettings();
    }

    if (isBetaChannelChanged)
    {
        serverAPI_->checkUpdate(engineSettings_.isBetaChannel(), serverApiUserRole_, true);
    }
    if (isLanguageChanged || isProtocolChanged)
    {
        QMutexLocker locker(&mutexApiInfo_);
        if (!apiInfo_.isNull())
        {
            SessionStatus ss = *apiInfo_->getSessionStatus();
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
                else
                {
                    Q_ASSERT(false);
                }
            }
            serverLocationsApiWrapper_->serverLocations(apiInfo_->getAuthHash(), engineSettings_.language(), serverApiUserRole_, true, ss.getRevisionHash(),
                                                        ss.isPro(), engineSettings_.connectionSettings().protocol(), ss.alc);
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
        customOvpnConfigs_->changeDir(engineSettings_.getCustomOvpnConfigsPath());
    }

    keepAliveManager_->setEnabled(engineSettings_.isKeepAliveEnabled());

    updateProxySettings();

    OpenVpnVersionController::instance().setUseWinTun(engineSettings_.isUseWintun());
}

void Engine::onLoginControllerFinished(LOGIN_RET retCode, QSharedPointer<ApiInfo> apiInfo, bool bFromConnectedToVPNState)
{
    qCDebug(LOG_BASIC) << "onLoginControllerFinished, retCode =" << loginRetToString(retCode) << ";" << "bFromConnectedToVPNState =" << bFromConnectedToVPNState;

    Q_ASSERT(loginState_ != LOGIN_FINISHED);
    if (retCode == LOGIN_SUCCESS)
    {
        if (!emergencyController_->isDisconnected())
        {
            emergencyController_->blockingDisconnect();
            emergencyConnectStateController_->setDisconnectedState(DISCONNECTED_ITSELF, NO_CONNECT_ERROR);
            emit emergencyDisconnected();

            qCDebug(LOG_BASIC) << "Update ip after emergency connect";
            getMyIPController_->getIPFromDisconnectedState(1);

            //todo check
            //qCDebug(LOG_BASIC) << "Update best location  after emergency connect";
            //serverAPI_->bestLocation(apiInfo->getAuthHash(), serverApiUserRole_, true);
        }

        QString curRevisionHash;
        {
            QMutexLocker locker(&mutexApiInfo_);
            apiInfo_ = apiInfo;
            curRevisionHash = apiInfo_->getSessionStatus()->getRevisionHash();

            QSettings settings;
            settings.setValue("authHash", apiInfo->getAuthHash());
        }

        // if updateServerLocation not called in loginImpl
        if (!prevSessionStatus_.isInitialized())
        {
            {
                QMutexLocker locker(&mutexApiInfo_);
                prevSessionStatus_.set(apiInfo_->getSessionStatus()->isPremium, apiInfo_->getSessionStatus()->billingPlanId,
                                       apiInfo_->getSessionStatus()->getRevisionHash(), apiInfo_->getSessionStatus()->alc,
                                       apiInfo_->getSessionStatus()->staticIps);
            }
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
        updateSessionStatusTimer_->start(UPDATE_SESSION_STATUS_PERIOD);

        if (!bFromConnectedToVPNState)
        {
            loginState_ = LOGIN_FINISHED;
        }
        updateCurrentNetworkInterface();
        emit loginFinished(false);
    }
    else if (retCode == LOGIN_NO_CONNECTIVITY)
    {
        loginState_ = LOGIN_NONE;
        emit loginError(LOGIN_NO_CONNECTIVITY);
    }
    else if (retCode == LOGIN_NO_API_CONNECTIVITY)
    {
        loginState_ = LOGIN_NONE;
        emit loginError(LOGIN_NO_API_CONNECTIVITY);

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
        emit loginError(LOGIN_PROXY_AUTH_NEED);
    }
    else if (retCode == LOGIN_INCORRECT_JSON)
    {
        loginState_ = LOGIN_NONE;
        emit loginError(LOGIN_INCORRECT_JSON);
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
            emit loginError(LOGIN_SSL_ERROR);
        }
    }
    else if (retCode == LOGIN_BAD_USERNAME)
    {
        loginState_ = LOGIN_NONE;
        emit loginError(LOGIN_BAD_USERNAME);
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
    checkUpdateTimer_->start(CHECK_UPDATE_PERIOD);
    onStartCheckUpdate();
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
    emit loginStepMessage(msg);
}

void Engine::onServerLocationsAnswer(SERVER_API_RET_CODE retCode, QVector<QSharedPointer<ServerLocation> > serverLocations, QStringList forceDisconnectNodes, uint userRole)
{
    if (userRole == serverApiUserRole_)
    {
        if (retCode == SERVER_RETURN_SUCCESS)
        {
            if (!serverLocations.isEmpty())
            {
                {
                    QMutexLocker locker(&mutexApiInfo_);

                    apiInfo_->setServerLocations(serverLocations);
                    apiInfo_->setForceDisconnectNodes(forceDisconnectNodes);
                }
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

void Engine::onBestLocationChanged(QVector<QSharedPointer<ServerLocation> > &serverLocations)
{
    if (!serverLocations.isEmpty())
    {
        qCDebug(LOG_BASIC) << "BestLocation updated";
        {
            QMutexLocker locker(&mutexApiInfo_);
            apiInfo_->setServerLocations(serverLocations);
        }
        updateServerLocations();
    }
}

void Engine::onSessionAnswer(SERVER_API_RET_CODE retCode, QSharedPointer<SessionStatus> sessionStatus, uint userRole)
{
    if (userRole == serverApiUserRole_)
    {
        if (retCode == SERVER_RETURN_SUCCESS)
        {
            {
                QMutexLocker locker(&mutexApiInfo_);
                apiInfo_->setSessionStatus(sessionStatus);
            }
            updateSessionStatus();
            // updateCurrentNetworkInterface();
        }
        else if (retCode == SERVER_RETURN_BAD_USERNAME)
        {
            emit sessionDeleted();
        }
     }
}

void Engine::onNotificationsAnswer(SERVER_API_RET_CODE retCode, QSharedPointer<ApiNotifications> notifications, uint userRole)
{
    if (userRole == serverApiUserRole_)
    {
        if (retCode == SERVER_RETURN_SUCCESS)
        {
            emit notificationsUpdated(notifications);
        }
    }
}

void Engine::onServerConfigsAnswer(SERVER_API_RET_CODE retCode, QByteArray config, uint userRole)
{
    if (userRole == serverApiUserRole_)
    {
        if (retCode == SERVER_RETURN_SUCCESS)
        {
            QMutexLocker locker(&mutexApiInfo_);
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

void Engine::onCheckUpdateAnswer(bool available, const QString &version, bool isBeta, int latestBuild, const QString &url, bool supported, bool bNetworkErrorOccured, uint userRole)
{
    if (userRole == serverApiUserRole_)
    {
        if (!bNetworkErrorOccured)
        {
            emit checkUpdateUpdated(available, version, isBeta, latestBuild, url, supported);
        }
        else
        {
            QTimer::singleShot(60000, this, SLOT(onStartCheckUpdate()));
        }
    }
}

void Engine::onHostIPsChanged(const QStringList &hostIps)
{
    qCDebug(LOG_BASIC) << "on host ips changed event:" << hostIps;
    firewallExceptions_.setHostIPs(hostIps);
    updateFirewallSettings();
}

void Engine::onMyIpAnswer(const QString &ip, bool success, bool isDisconnected)
{
    emit myIpUpdated(ip, success, isDisconnected);
}

void Engine::onDebugLogAnswer(SERVER_API_RET_CODE retCode, uint userRole)
{
    if (userRole == serverApiUserRole_)
    {
        emit sendDebugLogFinished(retCode == SERVER_RETURN_SUCCESS);
    }
}

void Engine::onConfirmEmailAnswer(SERVER_API_RET_CODE retCode, uint userRole)
{
    if (userRole == serverApiUserRole_)
    {
        emit confirmEmailFinished(retCode == SERVER_RETURN_SUCCESS);
    }
}

void Engine::onStaticIpsAnswer(SERVER_API_RET_CODE retCode, QSharedPointer<StaticIpsLocation> staticIpsLocation, uint userRole)
{
    if (userRole == serverApiUserRole_)
    {
        if (retCode == SERVER_RETURN_SUCCESS)
        {
            {
                QMutexLocker locker(&mutexApiInfo_);
                apiInfo_->setStaticIpsLocation(staticIpsLocation);
            }
            updateServerLocations();
        }
        else
        {
            qCDebug(LOG_BASIC) << "Failed get static ips";
            // try again with 3 sec
            QTimer::singleShot(3000, this, SLOT(onStartCheckUpdate()));
        }
    }
}

void Engine::onStartCheckUpdate()
{
    serverAPI_->checkUpdate(engineSettings_.isBetaChannel(), serverApiUserRole_, true);
}

void Engine::onStartStaticIpsUpdate()
{
    QMutexLocker locker(&mutexApiInfo_);
    serverAPI_->staticIps(apiInfo_->getAuthHash(), GetDeviceId::instance().getDeviceId(), serverApiUserRole_, true);
}

void Engine::onUpdateSessionStatusTimer()
{
    QMutexLocker locker(&mutexApiInfo_);
    serverAPI_->session(apiInfo_->getAuthHash(), serverApiUserRole_, true);
}

void Engine::onTimerGetBestLocation()
{
    //todo check
    /*QDateTime curDateTime = QDateTime::currentDateTime();
    if ((curDateTime.toTime_t() - lastUpdateBestLocationTime_.toTime_t()) > BEST_LOCATION_UPDATE_PERIOD/1000 && connectionManager_->isDisconnected())
    {
        qCDebug(LOG_BASIC) << "Update best location";
        QMutexLocker locker(&mutexApiInfo_);
        serverAPI_->bestLocation(apiInfo_->getAuthHash(), serverApiUserRole_, true);
        bestLocationTimer_->stop();
    }*/
}

void Engine::onConnectionManagerConnected()
{
    if (engineSettings_.firewallSettings().mode() == ProtoTypes::FIREWALL_MODE_AUTOMATIC && engineSettings_.firewallSettings().when() == ProtoTypes::FIREWALL_WHEN_AFTER_CONNECTION)
    {
        if (!firewallController_->firewallActualState())
        {
            qCDebug(LOG_BASIC) << "Automatic enable firewall after connection";
            QString ips = firewallExceptions_.getIPAddressesForFirewall();
            firewallController_->firewallOn(ips, engineSettings_.isAllowLanTraffic());
            emit firewallStateChanged(true);
        }
    }

#ifdef Q_OS_WIN
    AdapterMetricsController_win::updateMetrics(connectionManager_->getConnectedTapTunAdapter(), helper_);
#endif

#ifdef Q_OS_MAC

    // detect tap or ikev2 interface
    QString tapInterface = MacUtils::lastConnectedNetworkInterfaceName();
    firewallController_->setInterfaceToSkip_mac(tapInterface);

    splitTunnelingNetworkInfo_.setConnectedIp(connectionManager_->getLastConnectedIp());
    splitTunnelingNetworkInfo_.setProtocol(lastConnectingProtocol_);
    splitTunnelingNetworkInfo_.setVpnAdapterName(tapInterface);

    if (lastConnectingProtocol_ == ProtoTypes::PROTOCOL_IKEV2)
    {
        QStringList dnsServers = MacUtils::getDnsServersForInterface(tapInterface);
        splitTunnelingNetworkInfo_.setIkev2DnsServers(dnsServers);
    }
    else
    {
        // detect network params from openvpn dns.sh script (need for routing in helper)
        splitTunnelingNetworkInfo_.detectInfoFromOpenVpnScript();
    }
    splitTunnelingNetworkInfo_.outToLog();
    helper_->sendConnectStatus(true, splitTunnelingNetworkInfo_);

#else
    QString tapInterface = connectionManager_->getConnectedTapTunAdapter();
#endif

    if (firewallController_->firewallActualState())
    {
        firewallController_->firewallChange(firewallExceptions_.getIPAddressesForFirewallForConnectedState(connectionManager_->getLastConnectedIp()), engineSettings_.isAllowLanTraffic());
    }

    helper_->setIPv6EnabledInFirewall(false);

    if (engineSettings_.connectionSettings().protocol().isIkev2Protocol())
    {
        int mtu = mss_ + 40;
        if (mtu > 100)
        {
            qCDebug(LOG_BASIC) << "Applying MTU on " << tapInterface << ": " << mtu;
#ifdef Q_OS_MAC
            const QString setIkev2MtuCmd = QString("ifconfig %1 mtu %2").arg(tapInterface).arg(mtu);
            helper_->executeRootCommand(setIkev2MtuCmd);
#else
            helper_->executeChangeMtu(tapInterface, mtu);
#endif
        }
        else
        {
            qCDebug(LOG_BASIC) << "MTU too low, will use adapter default";
        }
    }

    if (connectionManager_->isStaticIpsLocation())
    {
        firewallController_->whitelistPorts(connectionManager_->getStatisIps());
        qCDebug(LOG_BASIC) << "the firewall rules are added for static IPs location, ports:" << connectionManager_->getStatisIps().getAsStringWithDelimiters();
    }

    serverAPI_->disableProxy();
    serversModel_->disableProxy();
    serverAPI_->setUseCustomDns(false);

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
        helper_->closeAllTcpConnections(engineSettings_.isAllowLanTraffic());
    }

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

    // get sender source for additional actions in this hanlder
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
        signOutImplAfterDisconnect();
    }
    else if (senderSource == "reconnect")
    {
        connectClickImpl(locationId_);
        return;
    }
    else
    {
        getMyIPController_->getIPFromDisconnectedState(1);
    }

    connectStateController_->setDisconnectedState(reason, NO_CONNECT_ERROR);
}

void Engine::onConnectionManagerReconnecting()
{
    qCDebug(LOG_BASIC) << "on reconnecting event";

    if (firewallController_->firewallActualState())
    {
        firewallController_->firewallChange(firewallExceptions_.getIPAddressesForFirewall(), engineSettings_.isAllowLanTraffic());
    }

    connectStateController_->setConnectingState(LocationID());
}

void Engine::onConnectionManagerError(CONNECTION_ERROR err)
{
    if (err == AUTH_ERROR)
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
            customOvpnAuthCredentialsStorage_->removeCredentials(connectionManager_->getCustomOvpnConfigFilePath());

            isNeedReconnectAfterRequestUsernameAndPassword_ = true;
            emit requestUsername();
        }
        else
        {
            // goto update server credentials and try connect again
            if (refetchServerCredentialsHelper_ == NULL)
            {
                // force update session status (for check blocked, banned account state)
                {
                    QMutexLocker locker(&mutexApiInfo_);
                    serverAPI_->session(apiInfo_->getAuthHash(), serverApiUserRole_, true);
                }

                refetchServerCredentialsHelper_ = new RefetchServerCredentialsHelper(this, apiInfo_->getAuthHash(), serverAPI_);
                connect(refetchServerCredentialsHelper_, SIGNAL(finished(bool,ServerCredentials)), SLOT(onRefetchServerCredentialsFinished(bool,ServerCredentials)));
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
        emit connectError(IKEV_FAILED_REINSTALL_WAN_WIN);
    }*/
#ifdef Q_OS_WIN
    else if (err == NO_INSTALLED_TUN_TAP)
    {
        //emit connectError(NO_INSTALLED_TUN_TAP);
    }
    else if (err == ALL_TAP_IN_USE)
    {
        /*if (CheckAdapterEnable::isAdapterDisabled(helper_, "Windscribe VPN"))
        {
            qCDebug(LOG_BASIC) << "Detected TAP-adapter disabled or broken, try reinstall it";
            //CheckAdapterEnable::enable(helper_, "Windscribe VPN");

#ifdef Q_OS_WIN
            if (!TapInstall_win::reinstallTap(TapInstall_win::TAP6))
            {
                qCDebug(LOG_BASIC) << "Failed reinstall TAP-adapter";
                qCDebug(LOG_BASIC) << "Try install legacy TAP5-adapter";
                if (!TapInstall_win::reinstallTap(TapInstall_win::TAP5))
                {
                    qCDebug(LOG_BASIC) << "Failed reinstall legacy TAP-adapter";
                }
                else
                {
                    qCDebug(LOG_BASIC) << "Legacy TAP5-adapter reinstalled successfully";
                }
            }
            else
            {
                qCDebug(LOG_BASIC) << "TAP-adapter reinstalled successfully";
            }
#endif

            doConnect(true);
            return;
        }
        else
        {
            //emit connectError(ALL_TAP_IN_USE);
        }*/
    }
#endif
    else
    {
        //emit connectError(err);
    }

#ifdef Q_OS_MAC
    Ipv6Controller_mac::instance().restoreIpv6();
#endif
    connectStateController_->setDisconnectedState(DISCONNECTED_WITH_ERROR, err);
}

void Engine::onConnectionManagerInternetConnectivityChanged(bool connectivity)
{
    online_ = connectivity;
    emit internetConnectivityChanged(connectivity);
}

void Engine::onConnectionManagerStatisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes)
{
    emit statisticsUpdated(bytesIn, bytesOut, isTotalBytes);
}

void Engine::onConnectionManagerConnectingToHostname(const QString &hostname)
{
    lastConnectingHostname_ = hostname;
    connectStateController_->setConnectingState(locationId_);
}

void Engine::onConnectionManagerProtocolPortChanged(const ProtoTypes::Protocol &protocol, const uint port)
{
    lastConnectingProtocol_ = protocol;
    emit protocolPortChanged(protocol, port);
}

void Engine::onConnectionManagerTestTunnelResult(bool success, const QString &ipAddress)
{
    emit testTunnelResult(success); // stops protocol/port flashing
    emit myIpUpdated(ipAddress, success, false); // sends IP address to UI // test should only occur in connected state
}

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
        emit requestUsername();
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
        emit requestPassword();
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

void Engine::detectPacketSizeMssImpl()
{
    if (networkDetectionManager_->isOnline())
    {
        qCDebug(LOG_BASIC) << "Detecting appropriate packet size";
        runningPacketDetection_ = true;
        emit packetSizeDetectionStateChanged(true);
        packetSizeController_->detectPacketSizeMss();
    }
    else
    {
         qCDebug(LOG_BASIC) << "No internet, cannot detect appropriate packet size. Using: " << QString::number(mss_);
    }
}

void Engine::onEmergencyControllerConnected()
{
    qCDebug(LOG_BASIC) << "Engine::onEmergencyControllerConnected()";

#ifdef Q_OS_WIN
    AdapterMetricsController_win::updateMetrics(emergencyController_->getConnectedTapAdapter_win(), helper_);
#endif

    serverAPI_->disableProxy();
    serverAPI_->setUseCustomDns(false);

    emergencyConnectStateController_->setConnectedState(LocationID());
    emit emergencyConnected();
}

void Engine::onEmergencyControllerDisconnected(DISCONNECT_REASON reason)
{
    qCDebug(LOG_BASIC) << "Engine::onEmergencyControllerDisconnected(), reason =" << reason;

    serverAPI_->enableProxy();
    serverAPI_->setUseCustomDns(true);

    emergencyConnectStateController_->setDisconnectedState(reason, NO_CONNECT_ERROR);
    emit emergencyDisconnected();
}

void Engine::onEmergencyControllerError(CONNECTION_ERROR err)
{
    qCDebug(LOG_BASIC) << "Engine::onEmergencyControllerError(), err =" << err;
    emergencyConnectStateController_->setDisconnectedState(DISCONNECTED_WITH_ERROR, err);
    emit emergencyConnectError(err);
}

void Engine::onRefetchServerCredentialsFinished(bool success, const ServerCredentials &serverCredentials)
{
    bool bFromAuthError = refetchServerCredentialsHelper_->property("fromAuthError").isValid();
    refetchServerCredentialsHelper_->deleteLater();
    refetchServerCredentialsHelper_ = NULL;

    if (success)
    {
        qCDebug(LOG_BASIC) << "Engine::onRefetchServerCredentialsFinished, successfully";
        apiInfo_->setServerCredentials(serverCredentials);
        doConnect(!bFromAuthError);
    }
    else
    {
        qCDebug(LOG_BASIC) << "Engine::onRefetchServerCredentialsFinished, failed";
        getMyIPController_->getIPFromDisconnectedState(1);
        connectStateController_->setDisconnectedState(DISCONNECTED_WITH_ERROR, COULD_NOT_FETCH_CREDENTAILS);
    }
}

void Engine::getNewNotifications()
{
    QMutexLocker locker(&mutexApiInfo_);
    serverAPI_->notifications(apiInfo_->getAuthHash(), serverApiUserRole_, true);
}

void Engine::onUpdateFirewallIpsForLocations(QVector<QSharedPointer<ServerLocation> > &serverLocations)
{
    firewallExceptions_.setLocations(serverLocations);
    updateFirewallSettings();
}

void Engine::onCustomOvpnConfigsChanged()
{
    qCDebug(LOG_BASIC) << "Custom ovpn-configs changed";
    updateServerLocations();
}

void Engine::onCustomOvpnConfgsIpsChanged(const QStringList &ips)
{
    firewallExceptions_.setCustomOvpnIps(ips);
    updateFirewallSettings();
}

void Engine::onNetworkChange(ProtoTypes::NetworkInterface networkInterface)
{
    emit networkChanged(networkInterface);
}

void Engine::stopPacketDetection()
{
    QMetaObject::invokeMethod(this, "stopPacketDetectionImpl");
}

void Engine::onNetworkStateManagerStateChanged(bool isActive, const QString & /*networkInterface*/)
{
    if (!isActive && runningPacketDetection_)
    {
        qCDebug(LOG_BASIC) << "Internet lost during packet size detection -- stopping";
        stopPacketDetection();
    }
}

void Engine::onMacAddressSpoofingChanged(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing)
{
    qCDebug(LOG_BASIC) << "Engine::onMacAddressSpoofingChanged";
    setSettingsMacAddressSpoofing(macAddrSpoofing);
}

void Engine::onPacketSizeControllerPacketSizeChanged(bool isAuto, int mss)
{
    mss_ = mss;
    connectionManager_->setMss(mss);
    emergencyController_->setMss(mss);

    if (mss    != engineSettings_.getPacketSize().mss() ||
        isAuto != engineSettings_.getPacketSize().is_automatic())
    {
        ProtoTypes::PacketSize packetSize;
        packetSize.set_mss(mss);
        packetSize.set_is_automatic(isAuto);
        engineSettings_.setPacketSize(packetSize);

        // Connection to EngineServer is chewing the parameters to garbage when passed as ProtoTypes::PacketSize
        // Probably has something to do with EngineThread
        emit packetSizeChanged(packetSize.is_automatic(), packetSize.mss());
    }
}

void Engine::onPacketSizeControllerFinishedSizeDetection()
{
    runningPacketDetection_ = false;
    emit packetSizeDetectionStateChanged(false);
}

void Engine::onMacAddressControllerSendUserWarning(ProtoTypes::UserWarningType userWarningType)
{
    emit sendUserWarning(userWarningType);
}

void Engine::updateServerConfigsImpl()
{
    QMutexLocker locker(&mutexApiInfo_);
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
        Q_FOREACH(const QString &sn, forceDisconnectNodes)
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

void Engine::forceUpdateServerLocationsImpl()
{
    serversModel_->updateServers(lastCopyOfServerlocations_);
}

void Engine::startProxySharingImpl(PROXY_SHARING_TYPE proxySharingType)
{
    vpnShareController_->startProxySharing(proxySharingType);
    emit proxySharingStateChanged(true, proxySharingType);
}

void Engine::stopProxySharingImpl()
{
    vpnShareController_->stopProxySharing();
    emit proxySharingStateChanged(false, PROXY_SHARING_HTTP);
}

void Engine::startWifiSharingImpl(const QString &ssid, const QString &password)
{
    vpnShareController_->stopWifiSharing(); //  need to stop it first
    vpnShareController_->startWifiSharing(ssid, password);
    emit wifiSharingStateChanged(true, ssid);
}

void Engine::stopWifiSharingImpl()
{
    bool bInitialState = vpnShareController_->isWifiSharingEnabled();
    vpnShareController_->stopWifiSharing();
    if (bInitialState == true)  // emit signal if state changed
    {
        emit wifiSharingStateChanged(false, "");
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
    emit macAddrSpoofingChanged(macAddrSpoofing);
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
    loginController_ = new LoginController(this, helper_, networkStateManager_, serverAPI_, serverLocationsApiWrapper_, engineSettings_.language(), engineSettings_.connectionSettings().protocol());
    connect(loginController_, SIGNAL(finished(LOGIN_RET, QSharedPointer<ApiInfo>, bool)), SLOT(onLoginControllerFinished(LOGIN_RET, QSharedPointer<ApiInfo>,bool)));
    connect(loginController_, SIGNAL(readyForNetworkRequests()), SLOT(onReadyForNetworkRequests()));
    connect(loginController_, SIGNAL(stepMessage(LOGIN_MESSAGE)), SLOT(onLoginControllerStepMessage(LOGIN_MESSAGE)));
    loginController_->startLoginProcess(loginSettings, engineSettings_.dnsResolutionSettings(), bFromConnectedState);
}

void Engine::updateSessionStatus()
{
    mutexApiInfo_.lock();
    if (!apiInfo_.isNull())
    {
        qCDebug(LOG_BASIC) << "update session status";

        SessionStatus ss = *apiInfo_->getSessionStatus();

        serversModel_->setSessionStatus(ss.isPremium == 0);

        QSharedPointer<SessionStatus> ssForSignal(new SessionStatus());
        *ssForSignal = ss;
        emit sessionStatusUpdated(ssForSignal);

        if (prevSessionStatus_.getRevisionHash() != ss.getRevisionHash() || prevSessionStatus_.getStaticIps() != ss.staticIps || ss.staticIpsUpdateDevices.contains(GetDeviceId::instance().getDeviceId()))
        {
            if (ss.staticIps > 0)
            {
                serverAPI_->staticIps(apiInfo_->getAuthHash(), GetDeviceId::instance().getDeviceId(), serverApiUserRole_, true);
            }
            else
            {
                QSharedPointer<StaticIpsLocation> emptyLocation;
                apiInfo_->setStaticIpsLocation(emptyLocation);
                updateServerLocations();
            }
        }

        if (prevSessionStatus_.getRevisionHash() != ss.getRevisionHash() || prevSessionStatus_.getIsPremium() != ss.isPremium ||
            prevSessionStatus_.getAlcList() != ss.alc)

        {
            serverLocationsApiWrapper_->serverLocations(apiInfo_->getAuthHash(), engineSettings_.language(), serverApiUserRole_, true, ss.getRevisionHash(), ss.isPro(),
                                                        engineSettings_.connectionSettings().protocol(), ss.alc);
        }

        if (prevSessionStatus_.getBillingPlanId() != INT_MIN && prevSessionStatus_.getBillingPlanId() != ss.billingPlanId)
        {
            serverAPI_->notifications(apiInfo_->getAuthHash(), serverApiUserRole_, true);
            notificationsUpdateTimer_->start(NOTIFICATIONS_UPDATE_PERIOD);
        }

        prevSessionStatus_.set(ss.isPremium, ss.billingPlanId, ss.getRevisionHash(), ss.alc, ss.staticIps);
    }
    mutexApiInfo_.unlock();
}

void Engine::updateServerLocations()
{
    QMutexLocker locker(&mutexApiInfo_);
    QVector<QSharedPointer<ServerLocation> > serverLocations;

    if (!apiInfo_.isNull())
    {
        serverLocations = apiInfo_->getServerLocations();
    }

    // if statics ip location exists, then add this location
    if (!apiInfo_.isNull() && apiInfo_->getSessionStatus()->staticIps > 0 && !apiInfo_->getStaticIpsLocation().isNull())
    {
        QSharedPointer<ServerLocation> staticLocation = apiInfo_->getStaticIpsLocation()->makeServerLocation();
        firewallExceptions_.setStaticLocationIps(staticLocation);
        serverLocations.insert(0, staticLocation);
    }
    else
    {
        firewallExceptions_.clearStaticLocationIps();
    }

    // if custom ovpn configs exists, then add this location to top of the list
    if (customOvpnConfigs_->isExist())
    {
        serverLocations.insert(0, customOvpnConfigs_->getLocation());
    }

    // compare new servers list with prev servers list
    bool isEqual = true;
    if (lastCopyOfServerlocations_.count() == serverLocations.count())
    {
        for (int i = 0; i < lastCopyOfServerlocations_.count(); ++i)
        {
            if (!lastCopyOfServerlocations_[i]->isEqual(serverLocations[i].data()))
            {
                isEqual = false;
                break;
            }
        }
    }
    else
    {
        isEqual = false;
    }

    if (!isEqual)
    {
        qCDebug(LOG_BASIC) << "Servers locations changed";
        firewallExceptions_.setLocations(serverLocations);
        updateFirewallSettings();

        serversModel_->updateServers(serverLocations);

        // emit signal for update in GUI
        emit serverLocationsUpdated();

        lastCopyOfServerlocations_.clear();
        lastCopyOfServerlocations_ = makeCopyOfServerLocationsVector(serverLocations);

        if (!apiInfo_.isNull() && !apiInfo_->getForceDisconnectNodes().isEmpty())
        {
            checkForceDisconnectNode(apiInfo_->getForceDisconnectNodes());
        }
    }
}

void Engine::updateFirewallSettings()
{
    if (firewallController_->firewallActualState())
    {
        if (connectStateController_->currentState() != CONNECT_STATE_CONNECTED)
        {
            firewallController_->firewallChange(firewallExceptions_.getIPAddressesForFirewall(), engineSettings_.isAllowLanTraffic());
        }
        else
        {
            firewallController_->firewallChange(firewallExceptions_.getIPAddressesForFirewallForConnectedState(connectionManager_->getLastConnectedIp()), engineSettings_.isAllowLanTraffic());
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
            QHostInfo hostInfo = DnsResolver::instance().lookupBlocked(strHost, true);
            if (hostInfo.error() == QHostInfo::NoError && hostInfo.addresses().count() > 0)
            {
                qCDebug(LOG_BASIC) << "Resolved IP address for" << strHost << ":" << hostInfo.addresses()[0];
                ip = hostInfo.addresses()[0].toString();
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
    vpnShareController_->onConnectingOrConnectedToVPNEvent();
    while (vpnShareController_->isUpdateIcsInProgress())
    {
        QThread::msleep(1);
    }

    locationId_ = checkLocationIdExistingAndReturnNewIfNeed(locationId_);

    QSharedPointer<MutableLocationInfo> mli = serversModel_->getMutableLocationInfoById(locationId_);
    if (mli.isNull())
    {
        connectStateController_->setDisconnectedState(DISCONNECTED_WITH_ERROR, LOCATION_NOT_EXIST);
        getMyIPController_->getIPFromDisconnectedState(1);
        qCDebug(LOG_BASIC) << "Engine::connectError(LOCATION_NOT_EXIST)";
        return;
    }
    if (!mli->isExistSelectedNode())
    {
        connectStateController_->setDisconnectedState(DISCONNECTED_WITH_ERROR, LOCATION_NO_ACTIVE_NODES);
        getMyIPController_->getIPFromDisconnectedState(1);
        qCDebug(LOG_BASIC) << "Engine::connectError(LOCATION_NO_ACTIVE_NODES)";
        return;
    }

    if (mli->isCustomOvpnConfig())
    {
        QString outIP = mli->resolveHostName();
        if (outIP.isEmpty())
        {
            connectStateController_->setDisconnectedState(DISCONNECTED_WITH_ERROR, CANT_RESOLVE_HOSTNAME);
            getMyIPController_->getIPFromDisconnectedState(1);
            qCDebug(LOG_BASIC) << "Engine::connectError(CANT_RESOLVE_HOSTNAME)";
            return;
        }
        else
        {
            bool bChanged = false;
            firewallExceptions_.setCustomOvpnIp(outIP, bChanged);
            if (bChanged)
            {
                updateFirewallSettings();
            }
        }
    }

    locationName_ = mli->getName();

#ifdef Q_OS_WIN
    helper_->clearDnsOnTap();
    CheckAdapterEnable::enableIfNeed(helper_, "Windscribe VPN");
#endif

#ifdef Q_OS_MAC
    splitTunnelingNetworkInfo_.detectDefaultRoute();
#endif

    QString authHash;
    QByteArray ovpnConfig;
    ServerCredentials serverCredentials;
    PortMap portmap;
    {
        //copy info from apiInfo and release mutex
        QMutexLocker locker(&mutexApiInfo_);
        if (!apiInfo_.isNull())
        {
            authHash = apiInfo_->getAuthHash();
            serverCredentials = apiInfo_->getServerCredentials();
            ovpnConfig = apiInfo_->getOvpnConfig();
            portmap = *apiInfo_->getPortMap();
        }
    }

    if (!serverCredentials.isInitialized() && locationId_.getId() != LocationID::CUSTOM_OVPN_CONFIGS_LOCATION_ID)
    {
        qCDebug(LOG_BASIC) << "radius username/password empty, refetch server credentials";

        if (refetchServerCredentialsHelper_ == NULL)
        {
            refetchServerCredentialsHelper_ = new RefetchServerCredentialsHelper(this, authHash, serverAPI_);
            connect(refetchServerCredentialsHelper_, SIGNAL(finished(bool,ServerCredentials)), SLOT(onRefetchServerCredentialsFinished(bool,ServerCredentials)));
            refetchServerCredentialsHelper_->startRefetch();
        }
    }
    else
    {
        if (mli->isStaticIp())
        {
            qCDebug(LOG_BASIC) << "radiusUsername openvpn: " << mli->getStaticIpUsername();
        }
        else
        {
            if (serverCredentials.isInitialized())
            {
                qCDebug(LOG_BASIC) << "radiusUsername openvpn: " << serverCredentials.usernameForOpenVpn();
                qCDebug(LOG_BASIC) << "radiusUsername ikev2: " << serverCredentials.usernameForIkev2();
            }
        }
        qCDebug(LOG_BASIC) << "Connecting to" << locationName_;

        connectionManager_->clickConnect(ovpnConfig, serverCredentials, mli,
            engineSettings_.connectionSettings(), portmap,
            ProxyServerController::instance().getCurrentProxySettings(), bEmitAuthError);
    }
}

LocationID Engine::checkLocationIdExistingAndReturnNewIfNeed(const LocationID &locationId)
{
    QSharedPointer<MutableLocationInfo> mli = serversModel_->getMutableLocationInfoById(locationId);
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
    }
}

void Engine::doDisconnectRestoreStuff()
{
    vpnShareController_->onDisconnectedFromVPNEvent();

    serverAPI_->enableProxy();
    serversModel_->enableProxy();
    serverAPI_->setUseCustomDns(true);

#ifdef Q_OS_MAC
    firewallController_->setInterfaceToSkip_mac("");
#endif
    if (firewallController_->firewallActualState())
    {
        QString ips = firewallExceptions_.getIPAddressesForFirewall();
        firewallController_->firewallChange(ips, engineSettings_.isAllowLanTraffic());
    }

    helper_->setIPv6EnabledInFirewall(true);
#ifdef Q_OS_MAC
    Ipv6Controller_mac::instance().restoreIpv6();
#endif
}

void Engine::stopPacketDetectionImpl()
{
    packetSizeController_->earlyStop();
}

void Engine::onConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON /*reason*/, CONNECTION_ERROR /*err*/, const LocationID & /*location*/)
{
#ifdef Q_OS_MAC
    if (helper_)
    {
        if (state != CONNECT_STATE_CONNECTED)
        {
            helper_->sendConnectStatus(false, SplitTunnelingNetworkInfo());
        }
    }
#else
    Q_UNUSED(state);
#endif
}

void Engine::updateProxySettings()
{
    if (ProxyServerController::instance().updateProxySettings(engineSettings_.proxySettings())) {
        const auto &proxySettings = ProxyServerController::instance().getCurrentProxySettings();
        serverAPI_->setProxySettings(proxySettings);
        serversModel_->setProxySettings(proxySettings);
        firewallExceptions_.setProxyIP(proxySettings);
        updateFirewallSettings();
        if (connectStateController_->currentState() == CONNECT_STATE_DISCONNECTED)
            getMyIPController_->getIPFromDisconnectedState(500);
    }
}
