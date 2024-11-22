#include "engine.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <spdlog/spdlog.h>
#include <wsnet/WSNet.h>
#include "utils/ws_assert.h"
#include "utils/utils.h"
#include "version/appversion.h"
#include "utils/log/categories.h"
#include "utils/log/mergelog.h"
#include "utils/log/logger.h"
#include "utils/extraconfig.h"
#include "utils/ipvalidation.h"
#include "utils/hardcodedsettings.h"
#include "utils/executable_signature/executable_signature.h"
#include "utils/languagesutil.h"
#include "connectionmanager/connectionmanager.h"
#include "connectionmanager/finishactiveconnections.h"
#include "wireguardconfig/getwireguardconfig.h"
#include "proxy/proxyservercontroller.h"
#include "connectstatecontroller/connectstatecontroller.h"
#include "dns_utils/dnsserversconfiguration.h"
#include "crossplatformobjectfactory.h"
#include "types/global_consts.h"
#include "api_responses/websession.h"
#include "api_responses/debuglog.h"
#include "firewall/firewallexceptions.h"
#include "getdeviceid.h"
#include "openvpnversioncontroller.h"
#include "apiresources/apiutils.h"

#ifdef Q_OS_WIN
    #include <Objbase.h>
    #include <shellapi.h>
    #include "engine/adaptermetricscontroller_win.h"
    #include "engine/dnsinfo_win.h"
    #include "helper/helper_win.h"
    #include "utils/bfe_service_win.h"
    #include "utils/executable_signature/executable_signature.h"
    #include "utils/network_utils/network_utils_win.h"
    #include "utils/network_utils/wlan_utils_win.h"
    #include "utils/winutils.h"
#elif defined Q_OS_MACOS
    #include "networkdetectionmanager/networkdetectionmanager_mac.h"
    #include "networkdetectionmanager/reachabilityevents.h"
    #include "utils/network_utils/network_utils_mac.h"
    #include "utils/interfaceutils_mac.h"
#elif defined Q_OS_LINUX
    #include "helper/helper_linux.h"
    #include "utils/executable_signature/executablesignature_linux.h"
    #include "utils/dnsscripts_linux.h"
    #include "utils/linuxutils.h"
    #include "utils/network_utils/network_utils_linux.h"
#endif

using namespace wsnet;

Engine::Engine() : QObject(nullptr),
    helper_(nullptr),
    firewallController_(nullptr),
    connectionManager_(nullptr),
    connectStateController_(nullptr),
    vpnShareController_(nullptr),
    emergencyController_(nullptr),
    customConfigs_(nullptr),
    customOvpnAuthCredentialsStorage_(nullptr),
    networkDetectionManager_(nullptr),
    macAddressController_(nullptr),
    keepAliveManager_(nullptr),
    packetSizeController_(nullptr),
    myIpManager_(nullptr),
#ifdef Q_OS_WIN
    measurementCpuUsage_(nullptr),
#endif
    inititalizeHelper_(nullptr),
    bInitialized_(false),
    locationsModel_(nullptr),
    downloadHelper_(nullptr),
#ifdef Q_OS_MACOS
    autoUpdaterHelper_(nullptr),
    macSpoofTimer_(nullptr),
#endif
    isBlockConnect_(false),
    isCleanupFinished_(false),
    isNeedReconnectAfterRequestAuth_(false),
    online_(false),
    packetSizeControllerThread_(nullptr),
    runningPacketDetection_(false),
    lastDownloadProgress_(0),
    installerUrl_(""),
    guiWindowHandle_(0),
    overrideUpdateChannelWithInternal_(false),
    bPrevNetworkInterfaceInitialized_(false),
    connectionSettingsOverride_(types::Protocol(types::Protocol::TYPE::UNINITIALIZED), 0, true)
{
    WSNet::setLogger([](const std::string &logStr) {
        // log wsnet outputs without formatting
        spdlog::get("raw")->info(logStr);
    }, false);

    if (ExtraConfig::instance().getUsePQAlgorithms()) {
        // Enable PQ algorithms.  We need to set some environment variables so OpenSSL can find the right provider.
#ifdef Q_OS_WIN
        wchar_t p[MAX_PATH];
        const auto pathLen = ::GetModuleFileNameW(NULL, p, MAX_PATH);
        if (pathLen == 0) {
            qCDebug(LOG_BASIC) << L"Engine GetModuleFileName failed (PQ algorithms not enabled):" << ::GetLastError();
        }
        else {
            std::string pathStr = std::filesystem::path(p).parent_path().string();
            _putenv_s("OPENSSL_MODULES", pathStr.c_str());
            pathStr = std::filesystem::path(p).parent_path().append("openssl.cnf").string();
            _putenv_s("OPENSSL_CONF", pathStr.c_str());
        }
#elif defined(Q_OS_MACOS)
        setenv("OPENSSL_CONF", "/Applications/Windscribe.app/Contents/Resources/openssl.cnf", 1);
        setenv("OPENSSL_MODULES", "/Applications/Windscribe.app/Contents/Frameworks", 1);
#elif defined(Q_OS_LINUX)
        setenv("OPENSSL_CONF", "/opt/windscribe/openssl.cnf", 1);
        setenv("OPENSSL_MODULES", "/opt/windscribe/lib", 1);
#endif
    }

    QSettings settings;
    std::string wsnetSettings = settings.value("wsnetSettings").toString().toStdString();
    bool bWsnetSuccess = WSNet::initialize(Utils::getBasePlatformName().toStdString(), Utils::getPlatformNameSafe().toStdString(),
                                           AppVersion::instance().semanticVersionString().toStdString(),
                                           GetDeviceId::instance().getDeviceId().toStdString(),
                                           OpenVpnVersionController::instance().getOpenVpnVersion().toStdString(),
                                           "3", // must supply session_type_id where 3 = DESKTOP
                                           AppVersion::instance().isStaging(), LanguagesUtil::systemLanguage().toStdString(), wsnetSettings);
    WS_ASSERT(bWsnetSuccess);

    WSNet::instance()->apiResourcersManager()->setCallback([this](ApiResourcesManagerNotification notification, LoginResult loginResult, const std::string &errorMessage) {
        QMetaObject::invokeMethod(this, [this, notification, loginResult, errorMessage] {
            onApiResourceManagerCallback(notification, loginResult, errorMessage);
        });
    });

    // To correctly upgrade from older versions of the client where auth_hash was stored in QSettings
    if (settings.contains("authHash")) {
        WSNet::instance()->apiResourcersManager()->setAuthHash(settings.value("authHash").toString().toStdString());
    }


    // Skip printing the engine settings if we loaded the defaults.
    if (engineSettings_.loadFromSettings()) {
        qCDebug(LOG_BASIC) << "Engine settings" << engineSettings_;
    } else {
        checkAutoEnableAntiCensorship_ = true;
    }

    connectStateController_ = new ConnectStateController(nullptr);
    connect(connectStateController_, &ConnectStateController::stateChanged, this, &Engine::onConnectStateChanged);
    emergencyConnectStateController_ = new ConnectStateController(nullptr);
#ifdef Q_OS_LINUX
    DnsScripts_linux::instance().setDnsManager(engineSettings_.dnsManager());
#endif
}

Engine::~Engine()
{
    SAFE_DELETE(connectStateController_);
    SAFE_DELETE(emergencyConnectStateController_);
    if (packetSizeControllerThread_) {
        packetSizeControllerThread_->exit();
        packetSizeControllerThread_->wait();
        packetSizeControllerThread_->deleteLater();
    }
    if (packetSizeController_) {
        packetSizeController_->deleteLater();
    }
    qCDebug(LOG_BASIC) << "Engine destructor finished";
}

void Engine::setSettings(const types::EngineSettings &engineSettings)
{
    QMutexLocker locker(&mutex_);
    QMetaObject::invokeMethod(this, "setSettingsImpl", Q_ARG(types::EngineSettings, engineSettings));
}

void Engine::cleanup(bool isExitWithRestart, bool isFirewallChecked, bool isFirewallAlwaysOn, bool isLaunchOnStart)
{
    // Cannot use invokeMethod("cleanupImpl") here.  Any code called by cleanupImpl causing the message queue
    // to be processed (e.g. qApp->processEvents() in ConnectionManager::blockingDisconnect) would then cause
    // cleanupImpl to be invoked repeatedly before the initial call has completed.  One of the cleanupImpl calls
    // would SAFE_DELETE all the pointers, thereby causing the other pending calls to segfault.
    emit initCleanup(isExitWithRestart, isFirewallChecked, isFirewallAlwaysOn, isLaunchOnStart);
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
    WS_ASSERT(bInitialized_);
    QMetaObject::invokeMethod(this, "enableBFE_winImpl");
}

void Engine::loginWithAuthHash()
{
    QMetaObject::invokeMethod(this, [this]() {
        loginImpl(true, QString(), QString(), QString());
    }, Qt::QueuedConnection);
}

void Engine::loginWithUsernameAndPassword(const QString &username, const QString &password, const QString &code2fa)
{
    QMetaObject::invokeMethod(this, [this, username, password, code2fa]() {
        loginImpl(false, username, password, code2fa);
    }, Qt::QueuedConnection);
}

bool Engine::isApiSavedSettingsExists()
{
    return WSNet::instance()->apiResourcersManager()->isExist();
}

void Engine::logout(bool keepFirewallOn)
{
    QMetaObject::invokeMethod(this, "logoutImpl", Q_ARG(bool, keepFirewallOn));
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

void Engine::continueWithPrivKeyPassword(const QString &password, bool bSave)
{
    QMetaObject::invokeMethod(this, "continueWithPrivKeyPasswordImpl", Q_ARG(QString, password), Q_ARG(bool, bSave));
}

void Engine::sendDebugLog()
{
    QMetaObject::invokeMethod(this, "sendDebugLogImpl");
}

void Engine::getWebSessionToken(WEB_SESSION_PURPOSE purpose)
{
    QMetaObject::invokeMethod(this, "getWebSessionTokenImpl", Q_ARG(WEB_SESSION_PURPOSE, purpose));
}

locationsmodel::LocationsModel *Engine::getLocationsModel()
{
    WS_ASSERT(locationsModel_ != NULL);
    return locationsModel_;
}

IConnectStateController *Engine::getConnectStateController()
{
    WS_ASSERT(connectStateController_ != NULL);
    return connectStateController_;
}

bool Engine::isFirewallEnabled()
{
    QMutexLocker locker(&mutex_);
    WS_ASSERT(bInitialized_);
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
    WS_ASSERT(bInitialized_);
    QMetaObject::invokeMethod(this, "firewallOnImpl");
    return true;
}

bool Engine::firewallOff()
{
    QMutexLocker locker(&mutex_);
    WS_ASSERT(bInitialized_);
    QMetaObject::invokeMethod(this, "firewallOffImpl");
    return true;
}

void Engine::connectClick(const LocationID &locationId, const types::ConnectionSettings &connectionSettings)
{
    QMutexLocker locker(&mutex_);
    if (bInitialized_)
    {
        locationId_ = locationId;
        connectStateController_->setConnectingState(locationId_);
        QMetaObject::invokeMethod(this, "connectClickImpl", Q_ARG(LocationID, locationId), Q_ARG(types::ConnectionSettings, connectionSettings));
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
        emergencyConnectStateController_->setDisconnectedState(DISCONNECTED_ITSELF, CONNECT_ERROR::NO_CONNECT_ERROR);
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
        emergencyConnectStateController_->setDisconnectedState(DISCONNECTED_ITSELF, CONNECT_ERROR::NO_CONNECT_ERROR);
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
    WS_ASSERT(bInitialized_);
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
    WS_ASSERT(bInitialized_);
    if (bInitialized_)
    {
        QMetaObject::invokeMethod(this, "startWifiSharingImpl", Q_ARG(QString, ssid), Q_ARG(QString, password));
    }
}

void Engine::stopWifiSharing()
{
    QMutexLocker locker(&mutex_);
    WS_ASSERT(bInitialized_);
    if (bInitialized_)
    {
        QMetaObject::invokeMethod(this, "stopWifiSharingImpl");
    }
}

void Engine::startProxySharing(PROXY_SHARING_TYPE proxySharingType, uint port, bool whileConnected)
{
    QMutexLocker locker(&mutex_);
    WS_ASSERT(bInitialized_);
    if (bInitialized_) {
        QMetaObject::invokeMethod(this, "startProxySharingImpl", Q_ARG(PROXY_SHARING_TYPE, proxySharingType), Q_ARG(uint, port), Q_ARG(bool, whileConnected));
    }
}

void Engine::stopProxySharing()
{
    QMutexLocker locker(&mutex_);
    WS_ASSERT(bInitialized_);
    if (bInitialized_)
    {
        QMetaObject::invokeMethod(this, "stopProxySharingImpl");
    }
}

QString Engine::getProxySharingAddress()
{
    QMutexLocker locker(&mutex_);
    WS_ASSERT(bInitialized_);
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
    WS_ASSERT(bInitialized_);
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
    QMetaObject::invokeMethod(this, [this]() { // NOLINT
        // WSNet will have already been released if we are cleaning up (e.g. exiting due to an OS restart).
        if (isLoggedIn_ && WSNet::isValid()) {
            WSNet::instance()->apiResourcersManager()->fetchSession();
        }
    }, Qt::QueuedConnection);
}

void Engine::updateCurrentInternetConnectivity()
{
    QMetaObject::invokeMethod(this, "updateCurrentInternetConnectivityImpl");
}

void Engine::detectAppropriatePacketSize()
{
    QMetaObject::invokeMethod(this, "detectAppropriatePacketSizeImpl");
}

void Engine::setSettingsMacAddressSpoofing(const types::MacAddrSpoofing &macAddrSpoofing)
{
    QMetaObject::invokeMethod(this, "setSettingsMacAddressSpoofingImpl", Q_ARG(types::MacAddrSpoofing, macAddrSpoofing));
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

void Engine::updateVersion(qint64 windowHandle)
{
    QMetaObject::invokeMethod(this, "updateVersionImpl", Q_ARG(qint64, windowHandle));
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
    if (winHelper) {
        if (winHelper->makeHostsFileWritable()) {
            emit hostsFileBecameWritable();
        }
        else {
            qCDebug(LOG_BASIC) << "Error: was not able to make 'hosts' file writable.";
        }
    }
#endif
}

void Engine::updateCurrentNetworkInterface()
{
    QMetaObject::invokeMethod(this, "updateCurrentNetworkInterfaceImpl");
}

void Engine::init()
{
#ifdef Q_OS_WIN
    crashHandler_.reset(new Debug::CrashHandlerForThread());

    HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        qCDebug(LOG_BASIC) << "Error: CoInitializeEx failed:" << hr;
    }
#endif

    isCleanupFinished_ = false;
    connect(this, &Engine::initCleanup, this, &Engine::cleanupImpl);

    helper_ = CrossPlatformObjectFactory::createHelper(this);
    connect(helper_, &IHelper::lostConnectionToHelper, this, &Engine::onLostConnectionToHelper);
    helper_->startInstallHelper();

    inititalizeHelper_ = new InitializeHelper(this, helper_);
    connect(inititalizeHelper_, &InitializeHelper::finished, this, &Engine::onInitializeHelper);
    inititalizeHelper_->start();
}

// init part2 (after helper initialized)
void Engine::initPart2()
{
#if defined(Q_OS_WIN)
    WlanUtils_win::setHelper(helper_);
#elif defined(Q_OS_MACOS)
    InterfaceUtils_mac::instance().setHelper(static_cast<Helper_mac *>(helper_));
    ReachAbilityEvents::instance().init();
#endif

    networkDetectionManager_ = CrossPlatformObjectFactory::createNetworkDetectionManager(this, helper_);

    DnsServersConfiguration::instance().setDnsServersPolicy(engineSettings_.dnsPolicy());
    WSNet::instance()->dnsResolver()->setDnsServers(DnsServersConfiguration::instance().getCurrentDnsServers());

    firewallExceptions_.setDnsPolicy(engineSettings_.dnsPolicy());

    types::MacAddrSpoofing macAddrSpoofing = engineSettings_.macAddrSpoofing();
    //todo refactor
#ifdef Q_OS_MACOS
    macAddrSpoofing.networkInterfaces = InterfaceUtils_mac::instance().currentNetworkInterfaces(true);
#elif defined Q_OS_WIN
    macAddrSpoofing.networkInterfaces = NetworkUtils_win::currentNetworkInterfaces(true);
#elif defined Q_OS_LINUX
    macAddrSpoofing.networkInterfaces = NetworkUtils_linux::currentNetworkInterfaces(true);
#endif
    setSettingsMacAddressSpoofing(macAddrSpoofing);

    connect(networkDetectionManager_, &INetworkDetectionManager::onlineStateChanged, this, &Engine::onNetworkOnlineStateChange);
    connect(networkDetectionManager_, &INetworkDetectionManager::networkChanged, this, &Engine::onNetworkChange);

    WSNet::instance()->setConnectivityState(networkDetectionManager_->isOnline());

    macAddressController_ = CrossPlatformObjectFactory::createMacAddressController(this, networkDetectionManager_, helper_);
    macAddressController_->initMacAddrSpoofing(macAddrSpoofing);
    connect(macAddressController_, &IMacAddressController::macAddrSpoofingChanged, this, &Engine::onMacAddressSpoofingChanged);
    connect(macAddressController_, &IMacAddressController::sendUserWarning, this, &Engine::onMacAddressControllerSendUserWarning);
#ifdef Q_OS_MACOS
    connect(macAddressController_, &IMacAddressController::macSpoofApplied, this, &Engine::onMacAddressControllerMacSpoofApplied);
#endif

    packetSizeControllerThread_ = new QThread(this);

    types::PacketSize packetSize = engineSettings_.packetSize();
    packetSizeController_ = new PacketSizeController(nullptr);
    packetSizeController_->setPacketSize(packetSize);
    packetSize_ = packetSize;
    connect(packetSizeController_, &PacketSizeController::packetSizeChanged, this, &Engine::onPacketSizeControllerPacketSizeChanged);
    connect(packetSizeController_, &PacketSizeController::finishedPacketSizeDetection, this, &Engine::onPacketSizeControllerFinishedSizeDetection);
    packetSizeController_->moveToThread(packetSizeControllerThread_);
    connect(packetSizeControllerThread_, &QThread::started, packetSizeController_, &PacketSizeController::init);
    connect(packetSizeControllerThread_, &QThread::finished, packetSizeController_, &PacketSizeController::finish);
    packetSizeControllerThread_->start(QThread::LowPriority);

    firewallController_ = CrossPlatformObjectFactory::createFirewallController(this, helper_);
#ifdef Q_OS_LINUX
    // On Linux, the system may reboot without ever sending us SIGTERM, which means we never get to clean up and
    // set the boot firewall rules.  Set the boot rules here if applicable.
    if (engineSettings_.firewallSettings().mode == FIREWALL_MODE_ALWAYS_ON) {
        firewallController_->setFirewallOnBoot(true, firewallExceptions_.getIPAddressesForFirewall(), engineSettings_.isAllowLanTraffic());
    } else {
        firewallController_->setFirewallOnBoot(false);
    }
#endif

    // do not return from this function until Engine::onHostIPsChanged() is finished
    // callback comes from another thread, so synchronization is needed
    WSNet::instance()->httpNetworkManager()->setWhitelistIpsCallback([this](const std::set<std::string> &ips) {
        mutexForOnHostIPsChanged_.lock();
        QMetaObject::invokeMethod(this, [this, ips] {
            QSet<QString> hostIps;
            for (const auto &ip : ips)
                hostIps.insert(QString::fromStdString(ip));
            onHostIPsChanged(hostIps);
        });
        waitConditionForOnHostIPsChanged_.wait(&mutexForOnHostIPsChanged_);
        mutexForOnHostIPsChanged_.unlock();
    });

    WSNet::instance()->serverAPI()->setTryingBackupEndpointCallback([this](std::uint32_t num, std::uint32_t count) {
        QMetaObject::invokeMethod(this, [this, num, count] {
            onFailOverTryingBackupEndpoint(num, count);
        });
    });
    WSNet::instance()->serverAPI()->setIgnoreSslErrors(engineSettings_.isIgnoreSslErrors());
    WSNet::instance()->serverAPI()->setApiResolutionsSettings(engineSettings_.apiResolutionSettings().getIsAutomatic(), engineSettings_.apiResolutionSettings().getManualAddress().toStdString());

    myIpManager_ = new api_resources::MyIpManager(this, networkDetectionManager_, connectStateController_);
    connect(myIpManager_, &api_resources::MyIpManager::myIpChanged, this, &Engine::onMyIpManagerIpChanged);

    customOvpnAuthCredentialsStorage_ = new CustomOvpnAuthCredentialsStorage();

    connectionManager_ = new ConnectionManager(this, helper_, networkDetectionManager_, customOvpnAuthCredentialsStorage_);
    connectionManager_->setPacketSize(packetSize_);
    connectionManager_->setConnectedDnsInfo(engineSettings_.connectedDnsInfo());
    connect(connectionManager_, &ConnectionManager::connected, this, &Engine::onConnectionManagerConnected);
    connect(connectionManager_, &ConnectionManager::disconnected, this, &Engine::onConnectionManagerDisconnected);
    connect(connectionManager_, &ConnectionManager::reconnecting, this, &Engine::onConnectionManagerReconnecting);
    connect(connectionManager_, &ConnectionManager::errorDuringConnection, this, &Engine::onConnectionManagerError);
    connect(connectionManager_, &ConnectionManager::statisticsUpdated, this, &Engine::onConnectionManagerStatisticsUpdated);
    connect(connectionManager_, &ConnectionManager::interfaceUpdated, this, &Engine::onConnectionManagerInterfaceUpdated);
    connect(connectionManager_, &ConnectionManager::testTunnelResult, this, &Engine::onConnectionManagerTestTunnelResult);
    connect(connectionManager_, &ConnectionManager::connectingToHostname, this, &Engine::onConnectionManagerConnectingToHostname);
    connect(connectionManager_, &ConnectionManager::protocolPortChanged, this, &Engine::onConnectionManagerProtocolPortChanged);
    connect(connectionManager_, &ConnectionManager::internetConnectivityChanged, this, &Engine::onConnectionManagerInternetConnectivityChanged);
    connect(connectionManager_, &ConnectionManager::wireGuardAtKeyLimit, this, &Engine::onConnectionManagerWireGuardAtKeyLimit);
    connect(connectionManager_, &ConnectionManager::requestUsername, this, &Engine::onConnectionManagerRequestUsername);
    connect(connectionManager_, &ConnectionManager::requestPassword, this, &Engine::onConnectionManagerRequestPassword);
    connect(connectionManager_, &ConnectionManager::requestPrivKeyPassword, this, &Engine::onConnectionManagerRequestPrivKeyPassword);
    connect(connectionManager_, &ConnectionManager::protocolStatusChanged, this, &Engine::protocolStatusChanged);
    connect(connectionManager_, &ConnectionManager::connectionEnded, this, &Engine::onConnectionManagerConnectionEnded);

    locationsModel_ = new locationsmodel::LocationsModel(this, connectStateController_, networkDetectionManager_);
    connect(locationsModel_, &locationsmodel::LocationsModel::whitelistLocationsIpsChanged, this, &Engine::onLocationsModelWhitelistIpsChanged);
    connect(locationsModel_, &locationsmodel::LocationsModel::whitelistCustomConfigsIpsChanged, this, &Engine::onLocationsModelWhitelistCustomConfigIpsChanged);

    vpnShareController_ = new VpnShareController(this, helper_);
    connect(vpnShareController_, &VpnShareController::connectedWifiUsersChanged, this, &Engine::wifiSharingStateChanged);
    connect(vpnShareController_, &VpnShareController::connectedProxyUsersChanged, this, &Engine::proxySharingStateChanged);
    connect(vpnShareController_, &VpnShareController::wifiSharingFailed, this, &Engine::wifiSharingFailed);

    keepAliveManager_ = new KeepAliveManager(this, connectStateController_);
    keepAliveManager_->setEnabled(engineSettings_.isKeepAliveEnabled());

    emergencyController_ = new EmergencyController(this, helper_);
    emergencyController_->setPacketSize(packetSize_);
    connect(emergencyController_, &EmergencyController::connected, this, &Engine::onEmergencyControllerConnected);
    connect(emergencyController_, &EmergencyController::disconnected, this, &Engine::onEmergencyControllerDisconnected);
    connect(emergencyController_, &EmergencyController::errorDuringConnection, this, &Engine::onEmergencyControllerError);

    customConfigs_ = new customconfigs::CustomConfigs(this);
    customConfigs_->changeDir(engineSettings_.customOvpnConfigsPath());
    connect(customConfigs_, &customconfigs::CustomConfigs::changed, this, &Engine::onCustomConfigsChanged);

    downloadHelper_ = new DownloadHelper(this, Utils::getPlatformName());
    connect(downloadHelper_, &DownloadHelper::finished, this, &Engine::onDownloadHelperFinished);
    connect(downloadHelper_, &DownloadHelper::progressChanged, this, &Engine::onDownloadHelperProgressChanged);

#ifdef Q_OS_MACOS
    autoUpdaterHelper_ = new AutoUpdaterHelper_mac();

    macSpoofTimer_ = new QTimer(this);
    connect(macSpoofTimer_, &QTimer::timeout, this, &Engine::onMacSpoofTimerTick);
    macSpoofTimer_->setInterval(1000);
#endif

#ifdef Q_OS_WIN
    measurementCpuUsage_ = new MeasurementCpuUsage(this, helper_, connectStateController_);
    connect(measurementCpuUsage_, &MeasurementCpuUsage::detectionCpuUsageAfterConnected, this, &Engine::detectionCpuUsageAfterConnected);
    measurementCpuUsage_->setEnabled(engineSettings_.isTerminateSockets());
#endif

    updateProxySettings();
    updateAdvancedParams();
}

void Engine::onLostConnectionToHelper()
{
    emit lostConnectionToHelper();
}

void Engine::onInitializeHelper(INIT_HELPER_RET ret)
{
    bool isAuthHashExists =  !WSNet::instance()->apiResourcersManager()->authHash().empty();
    if (ret == INIT_HELPER_SUCCESS)
    {
        QMutexLocker locker(&mutex_);
        bInitialized_ = true;

        initPart2();

        FinishActiveConnections::finishAllActiveConnections(helper_);

        // turn off split tunneling (for case the state remains from the last launch)
        helper_->sendConnectStatus(false, engineSettings_.isTerminateSockets(), engineSettings_.isAllowLanTraffic(), AdapterGatewayInfo::detectAndCreateDefaultAdapterInfo(), AdapterGatewayInfo(), QString(), types::Protocol());

        helper_->setSplitTunnelingSettings(false, false, false, QStringList(), QStringList(), QStringList());

    #ifdef Q_OS_WIN
        // check BFE service status
        if (!BFE_Service_win::instance().isBFEEnabled())
        {
            emit initFinished(ENGINE_INIT_BFE_SERVICE_FAILED, isAuthHashExists, engineSettings_);
        }
        else
        {
            emit initFinished(ENGINE_INIT_SUCCESS, isAuthHashExists, engineSettings_);
        }
    #else
        emit initFinished(ENGINE_INIT_SUCCESS, isAuthHashExists, engineSettings_);
    #endif
    }
    else if (ret == INIT_HELPER_FAILED)
    {
        emit initFinished(ENGINE_INIT_HELPER_FAILED, isAuthHashExists, engineSettings_);
    }
    else if (ret == INIT_HELPER_USER_CANCELED)
    {
        emit initFinished(ENGINE_INIT_HELPER_USER_CANCELED, isAuthHashExists, engineSettings_);
    }
    else
    {
        WS_ASSERT(false);
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

    saveWsnetSettings();
    // stop all network requests here, because we won't have callback's called for deleted objects
    WSNet::cleanup();

#ifdef Q_OS_MACOS
    if (macSpoofTimer_) {
        macSpoofTimer_->stop();
    }
#endif

    // to skip blocking calls
    if (helper_) {
        helper_->setNeedFinish();
    }

    if (emergencyController_) {
        emergencyController_->blockingDisconnect();
    }

    if (connectionManager_) {
        bool bWasIsConnected = !connectionManager_->isDisconnected();
        connectionManager_->blockingDisconnect();
        if (bWasIsConnected) {
#ifdef Q_OS_WIN
                enableDohSettings();
                // Avoid delaying app cleanup if the PC is restarting.
                if (!isExitWithRestart) {
                    DnsInfo_win::outputDebugDnsInfo();
                }
#endif
            qCDebug(LOG_BASIC) << "Cleanup, connection manager disconnected";
        } else {
            qCDebug(LOG_BASIC) << "Cleanup, connection manager no need disconnect";
        }

        connectionManager_->removeIkev2ConnectionFromOS();
    }

    // turn off split tunneling
    if (helper_) {
        helper_->sendConnectStatus(false, engineSettings_.isTerminateSockets(), engineSettings_.isAllowLanTraffic(), AdapterGatewayInfo::detectAndCreateDefaultAdapterInfo(), AdapterGatewayInfo(), QString(), types::Protocol());
        helper_->setSplitTunnelingSettings(false, false, false, QStringList(), QStringList(), QStringList());
    }

#ifdef Q_OS_WIN
    Helper_win *helper_win = dynamic_cast<Helper_win *>(helper_);
    if (helper_win) {
        helper_win->removeWindscribeNetworkProfiles();
    }
#endif

    if (!isExitWithRestart) {
        if (vpnShareController_) {
            vpnShareController_->stopWifiSharing();
            vpnShareController_->stopProxySharing();
        }
    }

    if (helper_ && firewallController_) {
        if (isFirewallChecked) {
            if (isExitWithRestart) {
                if (isLaunchOnStart) {
#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
                    firewallController_->setFirewallOnBoot(true, firewallExceptions_.getIPAddressesForFirewall(), engineSettings_.isAllowLanTraffic());
#endif
                } else {
                    if (isFirewallAlwaysOn) {
#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
                        firewallController_->setFirewallOnBoot(true, firewallExceptions_.getIPAddressesForFirewall(), engineSettings_.isAllowLanTraffic());
#endif
                    } else {
#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
                        firewallController_->setFirewallOnBoot(false);
#endif
                        firewallController_->firewallOff();
                    }
                }
            } else { // if exit without restart
                if (isFirewallAlwaysOn) {
#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
                    firewallController_->setFirewallOnBoot(true, firewallExceptions_.getIPAddressesForFirewall(), engineSettings_.isAllowLanTraffic());
#endif
                } else {
#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
                    firewallController_->setFirewallOnBoot(false);
#endif
                    firewallController_->firewallOff();
                }
            }
        } else { // if (!isFirewallChecked)
            firewallController_->firewallOff();
#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
            firewallController_->setFirewallOnBoot(false);
#endif
        }
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
    SAFE_DELETE(myIpManager_);
    SAFE_DELETE(locationsModel_);
    SAFE_DELETE(networkDetectionManager_);
    SAFE_DELETE(downloadHelper_);
    isCleanupFinished_ = true;
    qCDebug(LOG_BASIC) << "Cleanup finished";

#ifdef Q_OS_WIN
    crashHandler_.reset();
#endif

    // Do not accept any new events.
    disconnect(this);
    // Clear any existing events.
    QCoreApplication::removePostedEvents(this);
    // Quit this thread.
    thread()->quit();
}

void Engine::enableBFE_winImpl()
{
#ifdef Q_OS_WIN

    bool bSuccess = BFE_Service_win::instance().checkAndEnableBFE(helper_);
    if (bSuccess)
        emit bfeEnableFinished(ENGINE_INIT_SUCCESS, !WSNet::instance()->apiResourcersManager()->authHash().empty(), engineSettings_);
    else
        emit bfeEnableFinished(ENGINE_INIT_BFE_SERVICE_FAILED, !WSNet::instance()->apiResourcersManager()->authHash().empty(), engineSettings_);
#endif
}

void Engine::recordInstallImpl()
{
    WSNet::instance()->serverAPI()->recordInstall(true, [](ServerApiRetCode serverApiRetCode, const std::string &jsonData) {
        // nothing to do in callback, just log message
        qCDebug(LOG_BASIC) << "The recordInstall request finished with an answer:" << jsonData;
    });
}

void Engine::sendConfirmEmailImpl()
{
    if (isLoggedIn_) {
        WSNet::instance()->serverAPI()->confirmEmail(WSNet::instance()->apiResourcersManager()->authHash(),
                                                            [this](ServerApiRetCode serverApiRetCode, const std::string &jsonData) {
            emit confirmEmailFinished(serverApiRetCode == ServerApiRetCode::kSuccess);
        });
    }
}

void Engine::connectClickImpl(const LocationID &locationId, const types::ConnectionSettings &connectionSettings)
{
    locationId_ = locationId;
    connectionSettingsOverride_ = connectionSettings;

    // if connected, then first disconnect
    if (!connectionManager_->isDisconnected())
    {
        connectionManager_->setProperty("senderSource", "reconnect");
        connectionManager_->clickDisconnect();
        return;
    }

    if (isBlockConnect_ && !locationId_.isCustomConfigsLocation())
    {
        connectStateController_->setDisconnectedState(DISCONNECTED_WITH_ERROR, CONNECT_ERROR::CONNECTION_BLOCKED);
        myIpManager_->getIP(1);
        return;
    }

    addCustomRemoteIpToFirewallIfNeed();

#ifdef Q_OS_WIN
    DnsInfo_win::outputDebugDnsInfo();
#endif

    stopFetchingServerCredentials();

    if (engineSettings_.firewallSettings().mode == FIREWALL_MODE_AUTOMATIC && engineSettings_.firewallSettings().when == FIREWALL_WHEN_BEFORE_CONNECTION)
    {
        bool bFirewallStateOn = firewallController_->firewallActualState();
        if (!bFirewallStateOn) {
            qCDebug(LOG_BASIC) << "Automatic enable firewall before connection";
            firewallController_->firewallOn(
                firewallExceptions_.connectingIp(),
                firewallExceptions_.getIPAddressesForFirewall(),
                engineSettings_.isAllowLanTraffic(),
                locationId_.isCustomConfigsLocation());
            emit firewallStateChanged(true);
        }
    }
    doConnect(true);
}

void Engine::disconnectClickImpl()
{
    stopFetchingServerCredentials();
    connectionManager_->setProperty("senderSource", QVariant());
    connectionManager_->clickDisconnect();
}

void Engine::sendDebugLogImpl()
{
    QString userName;
    api_responses::SessionStatus ss(WSNet::instance()->apiResourcersManager()->sessionStatus());
    userName = ss.getUsername();

    QString log = log_utils::MergeLog::mergePrevLogs();
    log += "================================================================================================================================================================================================\n";
    log += "================================================================================================================================================================================================\n";
    log += log_utils::MergeLog::mergeLogs();

    WSNet::instance()->serverAPI()->debugLog(userName.toStdString(), log.toStdString(),
        [this](ServerApiRetCode serverApiRetCode, const std::string &jsonData) {

            if (serverApiRetCode != ServerApiRetCode::kSuccess) {
                qCDebug(LOG_BASIC) << "DebugLog returned failed error code:" << (int)serverApiRetCode;
                emit sendDebugLogFinished(false);
                return;
            }

            api_responses::DebugLog debugLog(jsonData);
            if (debugLog.isSuccess()) {
                qCDebug(LOG_BASIC) << "DebugLog sent";
                emit sendDebugLogFinished(true);
            } else {
                qCDebug(LOG_BASIC) << "DebugLog returned error in json:" << QString::fromStdString(jsonData);
                emit sendDebugLogFinished(false);
            }
    });
}

void Engine::getWebSessionTokenImpl(WEB_SESSION_PURPOSE purpose)
{
    WSNet::instance()->serverAPI()->webSession(WSNet::instance()->apiResourcersManager()->authHash(),
                                             [this, purpose](ServerApiRetCode serverApiRetCode, const std::string &jsonData) {
        if (serverApiRetCode == ServerApiRetCode::kSuccess) {
            api_responses::WebSession webSession(jsonData);
            emit webSessionToken(purpose, webSession.token());
        } else {
            // Failure indicated by empty token
            emit webSessionToken(purpose, "");
        }
    });
}

// function consists of two parts (first - disconnect if need, second - do other logout stuff)
void Engine::logoutImpl(bool keepFirewallOn)
{
    if (!connectionManager_->isDisconnected())
    {
        connectionManager_->setProperty("senderSource", (keepFirewallOn ? "logoutImplKeepFirewallOn" : "logoutImpl"));
        connectionManager_->clickDisconnect();
    }
    else
    {
        logoutImplAfterDisconnect(keepFirewallOn);
    }
}

void Engine::logoutImplAfterDisconnect(bool keepFirewallOn)
{
    locationsModel_->clear();

#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
    firewallController_->setFirewallOnBoot(false);
#endif

    if (isLoggedIn_) {
        WSNet::instance()->apiResourcersManager()->logout();
        isLoggedIn_ = false;
        tryLoginNextConnectOrDisconnect_ = false;
    }

    GetWireGuardConfig::removeWireGuardSettings();
    if (!keepFirewallOn)
    {
        firewallController_->firewallOff();
        emit firewallStateChanged(false);
    }

    emit logoutFinished();
}

void Engine::continueWithUsernameAndPasswordImpl(const QString &username, const QString &password, bool bSave)
{
    // if username and password is empty, then disconnect
    if (username.isEmpty() && password.isEmpty()) {
        connectionManager_->clickDisconnect();
    } else {
        if (bSave) {
            customOvpnAuthCredentialsStorage_->setAuthCredentials(connectionManager_->getCustomOvpnConfigFileName(), username, password);
        }
        lastUsernameForCustomConfig_ = username;
        connectionManager_->continueWithUsernameAndPassword(username, password, isNeedReconnectAfterRequestAuth_);
    }
}

void Engine::continueWithPasswordImpl(const QString &password, bool bSave)
{
    // if password is empty, then disconnect
    if (password.isEmpty()) {
        connectionManager_->clickDisconnect();
    } else {
        if (bSave) {
            customOvpnAuthCredentialsStorage_->setAuthCredentials(connectionManager_->getCustomOvpnConfigFileName(), "", password);
        }
        connectionManager_->continueWithPassword(password, isNeedReconnectAfterRequestAuth_);
    }
}

void Engine::continueWithPrivKeyPasswordImpl(const QString &password, bool bSave)
{
    // if password is empty, then disconnect
    if (password.isEmpty()) {
        connectionManager_->clickDisconnect();
    } else {
        if (bSave) {
            customOvpnAuthCredentialsStorage_->setPrivKeyPassword(connectionManager_->getCustomOvpnConfigFileName(), password);
        }
        connectionManager_->continueWithPrivKeyPassword(password, isNeedReconnectAfterRequestAuth_);
    }
}

void Engine::gotoCustomOvpnConfigModeImpl()
{
    api_responses::ServerList serverLocations(WSNet::instance()->apiResourcersManager()->locations());
    api_responses::StaticIps staticIps(WSNet::instance()->apiResourcersManager()->staticIps());
    updateServerLocations(serverLocations, staticIps);
    myIpManager_->getIP(1);
    doCheckUpdate();
    emit gotoCustomOvpnConfigModeFinished();
}

void Engine::updateCurrentInternetConnectivityImpl()
{
    online_ = networkDetectionManager_->isOnline();
    emit internetConnectivityChanged(online_);
}

void Engine::updateCurrentNetworkInterfaceImpl()
{
    if (!networkDetectionManager_ || !helper_) {
        return;
    }

    types::NetworkInterface networkInterface;
    networkDetectionManager_->getCurrentNetworkInterface(networkInterface, true);

    if (!bPrevNetworkInterfaceInitialized_ || networkInterface != prevNetworkInterface_)
    {
        prevNetworkInterface_ = networkInterface;
        bPrevNetworkInterfaceInitialized_ = true;

        if (helper_ && connectStateController_->currentState() == CONNECT_STATE_DISCONNECTED) {
            helper_->sendConnectStatus(false,
                                       engineSettings_.isTerminateSockets(),
                                       engineSettings_.isAllowLanTraffic(),
                                       AdapterGatewayInfo::detectAndCreateDefaultAdapterInfo(),
                                       AdapterGatewayInfo(),
                                       QString(),
                                       types::Protocol());
        }

        emit networkChanged(networkInterface);
    }
}

void Engine::firewallOnImpl()
{
    if (connectStateController_->currentState() != CONNECT_STATE_CONNECTED) {
        firewallController_->firewallOn(
            firewallExceptions_.connectingIp(),
            firewallExceptions_.getIPAddressesForFirewall(),
            engineSettings_.isAllowLanTraffic(),
            locationId_.isCustomConfigsLocation());
    } else {
        firewallController_->firewallOn(
            connectionManager_->getLastConnectedIp(),
            firewallExceptions_.getIPAddressesForFirewallForConnectedState(),
            engineSettings_.isAllowLanTraffic(),
            locationId_.isCustomConfigsLocation());
    }
    emit firewallStateChanged(true);
}

void Engine::firewallOffImpl()
{
    firewallController_->firewallOff();
    emit firewallStateChanged(false);
}

void Engine::speedRatingImpl(int rating, const QString &localExternalIp)
{
    WSNet::instance()->serverAPI()->speedRating(WSNet::instance()->apiResourcersManager()->authHash(), lastConnectingHostname_.toStdString(), localExternalIp.toStdString(), rating,
        [](ServerApiRetCode serverApiRetCode, const std::string &jsonData) {
            // We don't need a result.
        });
}

void Engine::setSettingsImpl(const types::EngineSettings &engineSettings)
{
    if (engineSettings_ == engineSettings)
        return;

    qCDebug(LOG_BASIC) << "Engine::setSettingsImpl";

    bool isAllowLanTrafficChanged = engineSettings_.isAllowLanTraffic() != engineSettings.isAllowLanTraffic();
    bool isUpdateChannelChanged = engineSettings_.updateChannel() != engineSettings.updateChannel();
    bool isTerminateSocketsChanged = engineSettings_.isTerminateSockets() != engineSettings.isTerminateSockets();
    bool isDnsPolicyChanged = engineSettings_.dnsPolicy() != engineSettings.dnsPolicy();
    bool isCustomOvpnConfigsPathChanged = engineSettings_.customOvpnConfigsPath() != engineSettings.customOvpnConfigsPath();
    bool isMACSpoofingChanged = engineSettings_.macAddrSpoofing() != engineSettings.macAddrSpoofing();
    bool isPacketSizeChanged =  engineSettings_.packetSize() != engineSettings.packetSize();
    bool isDnsWhileConnectedChanged = engineSettings_.connectedDnsInfo() != engineSettings.connectedDnsInfo();

#ifdef Q_OS_LINUX
    // On Linux, the system may reboot without ever sending us SIGTERM, which means we never get to clean up and
    // set the boot firewall rules.  Set the boot rules here if applicable.
    if (engineSettings.firewallSettings().mode == FIREWALL_MODE_ALWAYS_ON) {
        firewallController_->setFirewallOnBoot(true, firewallExceptions_.getIPAddressesForFirewall(), engineSettings_.isAllowLanTraffic());
    } else {
        firewallController_->setFirewallOnBoot(false);
    }
#endif
    engineSettings_ = engineSettings;
    engineSettings_.saveToSettings();

#ifdef Q_OS_LINUX
    DnsScripts_linux::instance().setDnsManager(engineSettings.dnsManager());
#endif

    if (isDnsPolicyChanged) {
        firewallExceptions_.setDnsPolicy(engineSettings_.dnsPolicy());
        if (connectStateController_->currentState() != CONNECT_STATE_CONNECTED && emergencyConnectStateController_->currentState() != CONNECT_STATE_CONNECTED) {
            DnsServersConfiguration::instance().setDnsServersPolicy(engineSettings_.dnsPolicy());
            WSNet::instance()->dnsResolver()->setDnsServers(DnsServersConfiguration::instance().getCurrentDnsServers());
        }
    }

    if (isDnsWhileConnectedChanged) {
        // tell connection manager about new settings (it will use them onConnect)
        connectionManager_->setConnectedDnsInfo(engineSettings.connectedDnsInfo());
    }

    if (isAllowLanTrafficChanged || isDnsPolicyChanged)
        updateFirewallSettings();

    if (isUpdateChannelChanged)
        doCheckUpdate();

    if (isTerminateSocketsChanged) {
    #ifdef Q_OS_WIN
        measurementCpuUsage_->setEnabled(engineSettings_.isTerminateSockets());
    #endif
    }

    if (isMACSpoofingChanged) {
        qCDebug(LOG_BASIC) << "Set MAC Spoofing (Engine)";
        macAddressController_->setMacAddrSpoofing(engineSettings_.macAddrSpoofing());
    }

    if (isPacketSizeChanged) {
        qCDebug(LOG_BASIC) << "Engine updating packet size controller";
        packetSizeController_->setPacketSize(engineSettings_.packetSize());
    }

    WSNet::instance()->serverAPI()->setIgnoreSslErrors(engineSettings_.isIgnoreSslErrors());
    WSNet::instance()->advancedParameters()->setAPIExtraTLSPadding(ExtraConfig::instance().getAPIExtraTLSPadding() || engineSettings_.isAntiCensorship());

    if (isCustomOvpnConfigsPathChanged) {
        customConfigs_->changeDir(engineSettings_.customOvpnConfigsPath());
        if (engineSettings_.customOvpnConfigsPath().isEmpty()) {
            customOvpnAuthCredentialsStorage_->clearCredentials();
        }
    }

    keepAliveManager_->setEnabled(engineSettings_.isKeepAliveEnabled());

    WSNet::instance()->serverAPI()->setApiResolutionsSettings(engineSettings_.apiResolutionSettings().getIsAutomatic(), engineSettings_.apiResolutionSettings().getManualAddress().toStdString());
    updateProxySettings();
}

void Engine::onFailOverTryingBackupEndpoint(int num, int cnt)
{
    emit tryingBackupEndpoint(num, cnt);
}

void Engine::onCheckUpdateUpdated(const api_responses::CheckUpdate &checkUpdate)
{
    qCDebug(LOG_BASIC) << "Received Check Update Answer";

    installerUrl_ = checkUpdate.url();
    installerHash_ = checkUpdate.sha256();
    if (checkUpdate.isAvailable()) {
        qCDebug(LOG_BASIC) << "Installer URL: " << installerUrl_;
        qCDebug(LOG_BASIC) << "Installer Hash: " << installerHash_;
    }
    emit checkUpdateUpdated(checkUpdate);
}


void Engine::onHostIPsChanged(const QSet<QString> &hostIps)
{
    //qCDebug(LOG_BASIC) << "on host ips changed event:" << hostIps;    // too much spam from this
    firewallExceptions_.setHostIPs(hostIps);
    updateFirewallSettings();
    // resume callback from wsnet
    waitConditionForOnHostIPsChanged_.wakeAll();
}

void Engine::onMyIpManagerIpChanged(const QString &ip, bool isFromDisconnectedState)
{
    emit myIpUpdated(ip, isFromDisconnectedState);
}

void Engine::onConnectionManagerConnected()
{
    QString adapterName = connectionManager_->getVpnAdapterInfo().adapterName();

#ifdef Q_OS_WIN
    // wireguard-nt driver monitors metrics itself.
    if (!connectionManager_->currentProtocol().isWireGuardProtocol()) {
        AdapterMetricsController_win::updateMetrics(adapterName, helper_);
    }
#elif defined (Q_OS_MACOS) || defined (Q_OS_LINUX)
    firewallController_->setInterfaceToSkip_posix(adapterName);
#endif

    bool isFirewallAlreadyEnabled = false;
    if (engineSettings_.firewallSettings().mode == FIREWALL_MODE_AUTOMATIC) {
        const bool isAllowFirewallAfterConnection =
            connectionManager_->isAllowFirewallAfterConnection();

        if (isAllowFirewallAfterConnection &&
            engineSettings_.firewallSettings().when == FIREWALL_WHEN_AFTER_CONNECTION)
        {
            if (!firewallController_->firewallActualState())
            {
                qCDebug(LOG_BASIC) << "Automatic enable firewall after connection";
                firewallController_->firewallOn(
                    connectionManager_->getLastConnectedIp(),
                    firewallExceptions_.getIPAddressesForFirewallForConnectedState(),
                    engineSettings_.isAllowLanTraffic(),
                    locationId_.isCustomConfigsLocation());
                emit firewallStateChanged(true);
                isFirewallAlreadyEnabled = true;
            }
        }
        else if (!isAllowFirewallAfterConnection &&
            engineSettings_.firewallSettings().when == FIREWALL_WHEN_BEFORE_CONNECTION)
        {
            if (firewallController_->firewallActualState())
            {
                qCDebug(LOG_BASIC) << "Automatic disable firewall after connection";
                firewallController_->firewallOff();
                emit firewallStateChanged(false);
            }
        }
    }

    // For Windows we should to set the custom dns for the adapter explicitly except WireGuard protocol
#ifdef Q_OS_WIN
    Helper_win *helper_win = dynamic_cast<Helper_win *>(helper_);
    if (connectionManager_->connectedDnsInfo().type == CONNECTED_DNS_TYPE_CUSTOM && connectionManager_->currentProtocol() != types::Protocol::WIREGUARD)
    {
        WS_ASSERT(connectionManager_->getVpnAdapterInfo().dnsServers().count() == 1);
        if (!helper_win->setCustomDnsWhileConnected( connectionManager_->getVpnAdapterInfo().ifIndex(),
                                                    connectionManager_->getVpnAdapterInfo().dnsServers().first()))
        {
            qCDebug(LOG_CONNECTED_DNS) << "Failed to set Custom 'while connected' DNS";
        }
    }
#endif

    bool result = helper_->sendConnectStatus(true, engineSettings_.isTerminateSockets(), engineSettings_.isAllowLanTraffic(),
                                             connectionManager_->getDefaultAdapterInfo(), connectionManager_->getVpnAdapterInfo(),
                                             connectionManager_->getLastConnectedIp(), lastConnectingProtocol_);
    if (!result) {
        emit helperSplitTunnelingStartFailed();
    }

    if (firewallController_->firewallActualState() && !isFirewallAlreadyEnabled)
    {
        firewallController_->firewallOn(
            connectionManager_->getLastConnectedIp(),
            firewallExceptions_.getIPAddressesForFirewallForConnectedState(),
            engineSettings_.isAllowLanTraffic(),
            locationId_.isCustomConfigsLocation());
    }


    if (packetSize_.isAutomatic) {
        qCDebug(LOG_PACKET_SIZE) << "Packet size mode auto - using default MTU (Engine)";
    } else {
        int mtu = -1;

        if (connectionManager_->currentProtocol().isIkev2Protocol()) {
            bool advParamIkevMtuOffset = false;
            int ikev2offset = ExtraConfig::instance().getMtuOffsetIkev2(advParamIkevMtuOffset);
            if (!advParamIkevMtuOffset) ikev2offset = MTU_OFFSET_IKEV2;

            mtu = packetSize_.mtu - ikev2offset;
            if (mtu > 0) {
                qCDebug(LOG_PACKET_SIZE) << "Applying MTU on " << adapterName << ": " << mtu;
                helper_->changeMtu(adapterName, mtu);
            } else {
                qCDebug(LOG_PACKET_SIZE) << "Using default MTU, mtu minus overhead is too low: " << mtu;
            }
        } else if (connectionManager_->currentProtocol().isWireGuardProtocol()) {
            bool advParamWireguardMtuOffset = false;
            int wgoffset = ExtraConfig::instance().getMtuOffsetWireguard(advParamWireguardMtuOffset);
            if (!advParamWireguardMtuOffset) wgoffset = MTU_OFFSET_WG;
            mtu = packetSize_.mtu - wgoffset;
            if (mtu > 0) {
                qCDebug(LOG_PACKET_SIZE) << "Applying MTU on WindscribeWireguard: " << mtu;
                // For WireGuard, this function needs the subinterface name, e.g. always "WindscribeWireguard"
#ifdef Q_OS_WIN
                helper_->changeMtu(QString::fromStdWString(kWireGuardAdapterIdentifier), mtu);
#else
                helper_->changeMtu(adapterName, mtu);
#endif
            } else {
                qCDebug(LOG_PACKET_SIZE) << "Using default MTU, mtu minus overhead is too low: " << mtu;
            }
        }
    }

    if (connectionManager_->isStaticIpsLocation())
    {
        firewallController_->whitelistPorts(connectionManager_->getStatisIps());
        qCDebug(LOG_CONNECTION) << "the firewall rules are added for static IPs location, ports:" << connectionManager_->getStatisIps().getAsStringWithDelimiters();
    }

    // disable proxy
    WSNet::instance()->httpNetworkManager()->setProxySettings();

    DnsServersConfiguration::instance().setConnectedState(connectionManager_->getVpnAdapterInfo().dnsServers());
    WSNet::instance()->dnsResolver()->setDnsServers(DnsServersConfiguration::instance().getCurrentDnsServers());

    if (engineSettings_.isTerminateSockets())
    {
#ifdef Q_OS_WIN
        Helper_win *helper_win = dynamic_cast<Helper_win *>(helper_);
        helper_win->closeAllTcpConnections(engineSettings_.isAllowLanTraffic());
#endif
    }

    // Update ICS sharing. The operation may take a few seconds.
    vpnShareController_->onConnectedToVPNEvent(adapterName);

    connectStateController_->setConnectedState(locationId_);
    connectionManager_->startTunnelTests(); // It is important that startTunnelTests() are after setConnectedState().

    if (tryLoginNextConnectOrDisconnect_) {
        loginImpl(true, QString(), QString(), QString());
    }
}

void Engine::onConnectionManagerDisconnected(DISCONNECT_REASON reason)
{
    qCDebug(LOG_CONNECTION) << "on disconnected event";

#if defined(Q_OS_WIN)
    enableDohSettings();
#endif

    if (connectionManager_->isStaticIpsLocation())
    {
        qCDebug(LOG_CONNECTION) << "the firewall rules are removed for static IPs location";
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

#ifdef Q_OS_WIN
    DnsInfo_win::outputDebugDnsInfo();
#endif

    if (senderSource == "logoutImpl")
    {
        logoutImplAfterDisconnect(false);
    }
    else if (senderSource == "logoutImplKeepFirewallOn")
    {
        logoutImplAfterDisconnect(true);
    }
    else if (senderSource == "reconnect")
    {
        connectClickImpl(locationId_, connectionSettingsOverride_);
        return;
    }
    else
    {
        myIpManager_->getIP(1);
        if (reason == DISCONNECTED_BY_USER && engineSettings_.firewallSettings().mode == FIREWALL_MODE_AUTOMATIC &&
            firewallController_->firewallActualState())
        {
            firewallController_->firewallOff();
            emit firewallStateChanged(false);
        }
    }

    // Connection Settings override is one-time only, reset it
    connectionSettingsOverride_ = types::ConnectionSettings(types::Protocol(types::Protocol::TYPE::UNINITIALIZED), 0, true);

    connectStateController_->setDisconnectedState(reason, CONNECT_ERROR::NO_CONNECT_ERROR);
}

void Engine::onConnectionManagerReconnecting()
{
    qCDebug(LOG_BASIC) << "on reconnecting event";

    DnsServersConfiguration::instance().setDisconnectedState();
    WSNet::instance()->dnsResolver()->setDnsServers(DnsServersConfiguration::instance().getCurrentDnsServers());

    if (firewallController_->firewallActualState()) {
        firewallController_->firewallOn(
            firewallExceptions_.connectingIp(),
            firewallExceptions_.getIPAddressesForFirewall(),
            engineSettings_.isAllowLanTraffic(),
            locationId_.isCustomConfigsLocation());
    }

    connectStateController_->setConnectingState(LocationID());
}

void Engine::onConnectionManagerError(CONNECT_ERROR err)
{
    if (err == CONNECT_ERROR::AUTH_ERROR)
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
            auto credentials = customOvpnAuthCredentialsStorage_->getAuthCredentials(connectionManager_->getCustomOvpnConfigFileName());
            customOvpnAuthCredentialsStorage_->removeCredentials(connectionManager_->getCustomOvpnConfigFileName());

            isNeedReconnectAfterRequestAuth_ = true;
            emit requestUsernameAndPassword(credentials.username.isEmpty() ? lastUsernameForCustomConfig_ : credentials.username);
        }
        else
        {
            if (isLoggedIn_) {
                // force update session status (for check blocked, banned account state)
                WSNet::instance()->apiResourcersManager()->fetchSession();
                // update server credentials and try connect again after update
                isFetchingServerCredentials_ = true;
                WSNet::instance()->apiResourcersManager()->fetchServerCredentials();
            }
        }
        return;
    }
    else if (err == CONNECT_ERROR::PRIV_KEY_PASSWORD_ERROR)
    {
        qCDebug(LOG_BASIC) << "Incorrect priv key password for custom ovpn config";
        doDisconnectRestoreStuff();

        customOvpnAuthCredentialsStorage_->removePrivKeyPassword(connectionManager_->getCustomOvpnConfigFileName());
        isNeedReconnectAfterRequestAuth_ = true;
        emit requestPrivKeyPassword();
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
    else if (err == CONNECT_ERROR::NO_INSTALLED_TUN_TAP)
    {
        qCDebug(LOG_BASIC) << "OpenVPN failed to detect the Windscribe wintun adapter";
        connectStateController_->setDisconnectedState(DISCONNECTED_WITH_ERROR, CONNECT_ERROR::WINTUN_FATAL_ERROR);
        return;
    }
    else if (err == CONNECT_ERROR::WINTUN_FATAL_ERROR)
    {
        qCDebug(LOG_BASIC) << "OpenVPN reported the Windscribe wintun adapter as already in use";
        connectStateController_->setDisconnectedState(DISCONNECTED_WITH_ERROR, CONNECT_ERROR::WINTUN_FATAL_ERROR);
        return;
    }
#endif
    else
    {
        //emit connectError(err);
    }

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

void Engine::onConnectionManagerInterfaceUpdated(const QString &interfaceName)
{
#if defined (Q_OS_MACOS) || defined(Q_OS_LINUX)
    firewallController_->setInterfaceToSkip_posix(interfaceName);
    updateFirewallSettings();
#else
    Q_UNUSED(interfaceName);
#endif
}

void Engine::onConnectionManagerConnectingToHostname(const QString &hostname, const QString &ip, const QStringList &dnsServers)
{
    lastConnectingHostname_ = hostname;
    connectStateController_->setConnectingState(locationId_);

    qCDebug(LOG_CONNECTION) << "Whitelist connecting ip:" << ip;
    if (!dnsServers.isEmpty())
    {
        qCDebug(LOG_CONNECTION) << "Whitelist DNS-server ip:" << dnsServers;
    }

    bool bChanged1 = false;
    firewallExceptions_.setConnectingIp(ip, bChanged1);
    bool bChanged2 = false;
    firewallExceptions_.setDNSServers(dnsServers, bChanged2);
    if (bChanged1 || bChanged2)
    {
        updateFirewallSettings();
    }
}

void Engine::onConnectionManagerProtocolPortChanged(const types::Protocol &protocol, const uint port)
{
    lastConnectingProtocol_ = protocol;
    emit protocolPortChanged(protocol, port);
}

void Engine::onConnectionManagerTestTunnelResult(bool success, const QString &ipAddress)
{
    emit testTunnelResult(success); // stops protocol/port flashing
    if (!ipAddress.isEmpty())
    {
        emit myIpUpdated(ipAddress, false); // sends IP address to UI // test should only occur in connected state
    }
}

void Engine::onConnectionManagerWireGuardAtKeyLimit()
{
    emit wireGuardAtKeyLimit();
}

#ifdef Q_OS_MACOS
void Engine::onMacSpoofTimerTick()
{
    QDateTime now = QDateTime::currentDateTime();

    // On MacOS the WindscribeNetworkListener may not trigger when the network comes back up,
    // so force a connectivity check for 15 seconds after the spoof
    // Not elegant, but lower risk as additional changes to the networkdetection module may affect network whitelisting
    if (macSpoofTimerStart_.secsTo(now) > 15) {
        macSpoofTimer_->stop();
        return;
    }

    updateCurrentInternetConnectivity();
}
#endif

void Engine::onConnectionManagerRequestUsername(const QString &pathCustomOvpnConfig)
{
    CustomOvpnAuthCredentialsStorage::Credentials c = customOvpnAuthCredentialsStorage_->getAuthCredentials(pathCustomOvpnConfig);

    if (!c.username.isEmpty() && !c.password.isEmpty()) {
        connectionManager_->continueWithUsernameAndPassword(c.username, c.password, false);
    } else {
        isNeedReconnectAfterRequestAuth_ = false;
        emit requestUsernameAndPassword(c.username);
    }
}

void Engine::onConnectionManagerRequestPassword(const QString &pathCustomOvpnConfig)
{
    CustomOvpnAuthCredentialsStorage::Credentials c = customOvpnAuthCredentialsStorage_->getAuthCredentials(pathCustomOvpnConfig);

    if (!c.password.isEmpty()) {
        connectionManager_->continueWithPassword(c.password, false);
    } else {
        isNeedReconnectAfterRequestAuth_ = false;
        emit requestUsernameAndPassword(c.username);
    }
}

void Engine::onConnectionManagerRequestPrivKeyPassword(const QString &pathCustomOvpnConfig)
{
    CustomOvpnAuthCredentialsStorage::Credentials c = customOvpnAuthCredentialsStorage_->getAuthCredentials(pathCustomOvpnConfig);

    if (!c.privKeyPassword.isEmpty()) {
        connectionManager_->continueWithPrivKeyPassword(c.privKeyPassword, false);
    } else {
        isNeedReconnectAfterRequestAuth_ = false;
        emit requestPrivKeyPassword();
    }
}

void Engine::emergencyConnectClickImpl()
{
    emergencyController_->clickConnect(ProxyServerController::instance().getCurrentProxySettings(), engineSettings_.isAntiCensorship());
}

void Engine::emergencyDisconnectClickImpl()
{
    emergencyController_->clickDisconnect();
}

void Engine::detectAppropriatePacketSizeImpl()
{
    if (networkDetectionManager_->isOnline())
    {
        qCDebug(LOG_PACKET_SIZE) << "Detecting appropriate packet size";
        runningPacketDetection_ = true;
        emit packetSizeDetectionStateChanged(true, false);
        packetSizeController_->detectAppropriatePacketSize(HardcodedSettings::instance().windscribeHost());
    }
    else
    {
        qCDebug(LOG_PACKET_SIZE) << "No internet, cannot detect appropriate packet size. Using: " << QString::number(packetSize_.mtu);
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

void Engine::updateVersionImpl(qint64 windowHandle)
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
        doCheckUpdate();
    }

    // send some parameters to wsnet
    WSNet::instance()->advancedParameters()->setAPIExtraTLSPadding(ExtraConfig::instance().getAPIExtraTLSPadding() || engineSettings_.isAntiCensorship());
    WSNet::instance()->advancedParameters()->setLogApiResponce(ExtraConfig::instance().getLogAPIResponse());
    std::optional<QString> countryOverride = ExtraConfig::instance().serverlistCountryOverride();
    WSNet::instance()->advancedParameters()->setCountryOverrideValue(countryOverride.has_value() ? countryOverride->toStdString() : "");
    WSNet::instance()->advancedParameters()->setIgnoreCountryOverride(ExtraConfig::instance().serverListIgnoreCountryOverride());
}

void Engine::onDownloadHelperProgressChanged(uint progressPercent)
{
    if (lastDownloadProgress_ != progressPercent)
    {
        lastDownloadProgress_ = progressPercent;
        emit updateVersionChanged(progressPercent, UPDATE_VERSION_STATE_DOWNLOADING, UPDATE_VERSION_ERROR_NO_ERROR);
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
        emit updateVersionChanged(0, UPDATE_VERSION_STATE_DONE, UPDATE_VERSION_ERROR_DL_FAIL);
        return;
    }
    qCDebug(LOG_DOWNLOADER) << "Successful download";

#ifdef Q_OS_WIN

    ExecutableSignature sigCheck;
    if (!sigCheck.verify(installerPath_.toStdWString()))
    {
        qCDebug(LOG_AUTO_UPDATER) << "Incorrect signature, removing unsigned installer: " << QString::fromStdString(sigCheck.lastError());
        QFile::remove(installerPath_);
        emit updateVersionChanged(0, UPDATE_VERSION_STATE_DONE, UPDATE_VERSION_ERROR_SIGN_FAIL);
        return;
    }
    qCDebug(LOG_AUTO_UPDATER) << "Installer signature valid";
#elif defined Q_OS_MACOS

    const QString tempInstallerFilename = autoUpdaterHelper_->copyInternalInstallerToTempFromDmg(installerPath_);
    QFile::remove(installerPath_);

    if (tempInstallerFilename == "")
    {
        emit updateVersionChanged(0, UPDATE_VERSION_STATE_DONE, autoUpdaterHelper_->error());
        return;
    }
    installerPath_ = tempInstallerFilename;
#elif defined Q_OS_LINUX

    // if api for some reason doesn't return sha256 field
    if (installerHash_ == "")
    {
        qCDebug(LOG_BASIC) << "Hash from API is empty -- cannot verify";
        if (QFile::exists(installerPath_)) QFile::remove(installerPath_);
        emit updateVersionChanged(0, UPDATE_VERSION_STATE_DONE, UPDATE_VERSION_ERROR_API_HASH_INVALID);
        return;
    }

    if (!verifyContentsSha256(installerPath_, installerHash_)) // installerPath_
    {
        qCDebug(LOG_AUTO_UPDATER) << "Incorrect hash, removing installer";
        if (QFile::exists(installerPath_)) QFile::remove(installerPath_);
        emit updateVersionChanged(0, UPDATE_VERSION_STATE_DONE, UPDATE_VERSION_ERROR_COMPARE_HASH_FAIL);
        return;
    }
#endif

    emit updateDownloaded(installerPath_);
    emit updateVersionChanged(0, UPDATE_VERSION_STATE_RUNNING, UPDATE_VERSION_ERROR_NO_ERROR);
}

void Engine::updateRunInstaller(qint32 windowCenterX, qint32 windowCenterY)
{
#ifdef Q_OS_WIN
    std::wstring installerPath = installerPath_.toStdWString();

    QString installerArgString{ "-update" };
    if (windowCenterX != INT_MAX && windowCenterY != INT_MAX) {
        installerArgString.append(QString(" -center %1 %2").arg(windowCenterX).arg(windowCenterY));
    }
    std::wstring installerArgs = installerArgString.toStdWString();

    SHELLEXECUTEINFO shExInfo;
    memset(&shExInfo, 0, sizeof(shExInfo));
    shExInfo.cbSize = sizeof(shExInfo);
    shExInfo.fMask = SEE_MASK_DEFAULT;
    shExInfo.lpVerb = L"runas";                // Operation to perform
    shExInfo.lpFile = installerPath.c_str();       // Application to start
    shExInfo.lpParameters = installerArgs.c_str();  // Additional parameters
    shExInfo.nShow = SW_SHOW;
    if (guiWindowHandle_ != 0) {
        shExInfo.hwnd = (HWND)guiWindowHandle_;
    }

    if (!ShellExecuteEx(&shExInfo)) {
        DWORD lastError = GetLastError();
        qCDebug(LOG_AUTO_UPDATER) << "Can't start installer: errorCode = " << lastError;
        QFile::remove(installerPath_);
        emit updateVersionChanged(0, UPDATE_VERSION_STATE_DONE, UPDATE_VERSION_ERROR_START_INSTALLER_FAIL);
        return;
    }

#elif defined Q_OS_MACOS
    QString additionalArgs;
    if (windowCenterX != INT_MAX && windowCenterY != INT_MAX) {
        additionalArgs.append(QString("-center %1 %2").arg(windowCenterX).arg(windowCenterY));
    }

    bool verifiedAndRan = autoUpdaterHelper_->verifyAndRun(installerPath_, additionalArgs);
    if (!verifiedAndRan) {
        emit updateVersionChanged(0, UPDATE_VERSION_STATE_DONE, autoUpdaterHelper_->error());
        return;
    }
#else // Linux
    Q_UNUSED(windowCenterX);
    Q_UNUSED(windowCenterY);

    Helper_linux* helperLinux = dynamic_cast<Helper_linux*>(helper_);
    WS_ASSERT(helperLinux != nullptr);

    auto result = helperLinux->installUpdate(installerPath_);
    if (!result.has_value()) {
        emit updateVersionChanged(0, UPDATE_VERSION_STATE_DONE, UPDATE_VERSION_ERROR_OTHER_FAIL);
        return;
    }
    if (!result.value()) {
        emit updateVersionChanged(0, UPDATE_VERSION_STATE_DONE, UPDATE_VERSION_ERROR_START_INSTALLER_FAIL);
        return;
    }
#endif

    qCDebug(LOG_AUTO_UPDATER) << "Installer valid and executed";
    installerPath_.clear();

    emit updateVersionChanged(0, UPDATE_VERSION_STATE_DONE, UPDATE_VERSION_ERROR_NO_ERROR);
}

void Engine::onEmergencyControllerConnected()
{
    qCDebug(LOG_BASIC) << "Engine::onEmergencyControllerConnected()";

#ifdef Q_OS_WIN
    AdapterMetricsController_win::updateMetrics(emergencyController_->getVpnAdapterInfo().adapterName(), helper_);
#endif

    // disable proxy
    WSNet::instance()->httpNetworkManager()->setProxySettings();
    DnsServersConfiguration::instance().setConnectedState(emergencyController_->getVpnAdapterInfo().dnsServers());
    WSNet::instance()->dnsResolver()->setDnsServers(DnsServersConfiguration::instance().getCurrentDnsServers());

    emergencyConnectStateController_->setConnectedState(LocationID());
    emit emergencyConnected();
}

void Engine::onEmergencyControllerDisconnected(DISCONNECT_REASON reason)
{
    qCDebug(LOG_BASIC) << "Engine::onEmergencyControllerDisconnected(), reason =" << reason;

    // enable proxy
    const auto &proxySettings = ProxyServerController::instance().getCurrentProxySettings();
    WSNet::instance()->httpNetworkManager()->setProxySettings(proxySettings.curlAddress().toStdString(), proxySettings.getUsername().toStdString(), proxySettings.getPassword().toStdString());
    DnsServersConfiguration::instance().setDisconnectedState();
    WSNet::instance()->dnsResolver()->setDnsServers(DnsServersConfiguration::instance().getCurrentDnsServers());

    emergencyConnectStateController_->setDisconnectedState(reason, CONNECT_ERROR::NO_CONNECT_ERROR);
    emit emergencyDisconnected();
}

void Engine::onEmergencyControllerError(CONNECT_ERROR err)
{
    qCDebug(LOG_BASIC) << "Engine::onEmergencyControllerError(), err =" << err;
    emergencyConnectStateController_->setDisconnectedState(DISCONNECTED_WITH_ERROR, err);
    emit emergencyConnectError(err);
}

void Engine::getRobertFiltersImpl()
{
    auto callback = [this](ServerApiRetCode serverApiRetCode, const std::string &jsonData)
    {
        if (serverApiRetCode == ServerApiRetCode::kSuccess) {
            api_responses::RobertFilters filters(jsonData);
            emit robertFiltersUpdated(true, filters.filters());
        } else {
            emit robertFiltersUpdated(false, QVector<api_responses::RobertFilter>());
        }
    };
    WSNet::instance()->serverAPI()->getRobertFilters(WSNet::instance()->apiResourcersManager()->authHash(), callback);
}

void Engine::setRobertFilterImpl(const api_responses::RobertFilter &filter)
{
    auto callback = [this](ServerApiRetCode serverApiRetCode, const std::string &jsonData)
    {
        emit setRobertFilterFinished(serverApiRetCode == ServerApiRetCode::kSuccess);
    };

    WSNet::instance()->serverAPI()->setRobertFilter(WSNet::instance()->apiResourcersManager()->authHash(), filter.id.toStdString(), filter.status,  callback);
}

void Engine::syncRobertImpl()
{
    auto callback = [this](ServerApiRetCode serverApiRetCode, const std::string &jsonData)
    {
        emit syncRobertFinished(serverApiRetCode == ServerApiRetCode::kSuccess);
    };
    WSNet::instance()->serverAPI()->syncRobert(WSNet::instance()->apiResourcersManager()->authHash(), callback);
}

void Engine::getRobertFilters()
{
    QMetaObject::invokeMethod(this, "getRobertFiltersImpl");
}

void Engine::setRobertFilter(const api_responses::RobertFilter &filter)
{
    QMetaObject::invokeMethod(this, "setRobertFilterImpl", Q_ARG(api_responses::RobertFilter, filter));
}

void Engine::syncRobert()
{
    QMetaObject::invokeMethod(this, "syncRobertImpl");
}

void Engine::onCustomConfigsChanged()
{
    qCDebug(LOG_BASIC) << "Custom configs changed";
    api_responses::ServerList serverLocations(WSNet::instance()->apiResourcersManager()->locations());
    api_responses::StaticIps staticIps(WSNet::instance()->apiResourcersManager()->staticIps());
    updateServerLocations(serverLocations, staticIps);
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
    WSNet::instance()->setConnectivityState(isOnline);
}

void Engine::onNetworkChange(const types::NetworkInterface &networkInterface)
{
    if (!networkInterface.networkOrSsid.isEmpty()) {

        if (isLoggedIn_) {
            api_responses::PortMap portMap(WSNet::instance()->apiResourcersManager()->portMap());
            connectionManager_->updateConnectionSettings(
                engineSettings_.connectionSettingsForNetworkInterface(networkInterface.networkOrSsid), portMap,
                ProxyServerController::instance().getCurrentProxySettings());
        } else {
            connectionManager_->updateConnectionSettings(
                engineSettings_.connectionSettingsForNetworkInterface(networkInterface.networkOrSsid),
                api_responses::PortMap(),
                ProxyServerController::instance().getCurrentProxySettings());
        }

        if (!WSNet::instance()->apiResourcersManager()->portMap().empty()) {
            connectionManager_->updateConnectionSettings(
                engineSettings_.connectionSettingsForNetworkInterface(networkInterface.networkOrSsid), api_responses::PortMap(WSNet::instance()->apiResourcersManager()->portMap()),
                ProxyServerController::instance().getCurrentProxySettings());

        } else {
            connectionManager_->updateConnectionSettings(
                engineSettings_.connectionSettingsForNetworkInterface(networkInterface.networkOrSsid),
                api_responses::PortMap(),
                ProxyServerController::instance().getCurrentProxySettings());
        }

        if (helper_ && connectStateController_->currentState() == CONNECT_STATE_DISCONNECTED) {
            helper_->sendConnectStatus(false,
                                       engineSettings_.isTerminateSockets(),
                                       engineSettings_.isAllowLanTraffic(),
                                       AdapterGatewayInfo::detectAndCreateDefaultAdapterInfo(),
                                       AdapterGatewayInfo(),
                                       QString(),
                                       types::Protocol());
        }
    }

    emit networkChanged(networkInterface);
}

void Engine::stopPacketDetection()
{
    QMetaObject::invokeMethod(this, "stopPacketDetectionImpl");
}

void Engine::onMacAddressSpoofingChanged(const types::MacAddrSpoofing &macAddrSpoofing)
{
    qCDebug(LOG_BASIC) << "Engine::onMacAddressSpoofingChanged";
    setSettingsMacAddressSpoofing(macAddrSpoofing);
}

void Engine::onPacketSizeControllerPacketSizeChanged(bool isAuto, int mtu)
{
    types::PacketSize packetSize;
    packetSize.isAutomatic = isAuto;
    packetSize.mtu = mtu;

    packetSize_ = packetSize;
    connectionManager_->setPacketSize(packetSize);
    emergencyController_->setPacketSize(packetSize);

    // update gui
    if (mtu    != engineSettings_.packetSize().mtu ||
        isAuto != engineSettings_.packetSize().isAutomatic)
    {

        // qDebug() << "Updating gui with mtu: " << mtu;
        engineSettings_.setPacketSize(packetSize);

        // Connection to EngineServer is chewing the parameters to garbage when passed as ProtoTypes::PacketSize
        // Probably has something to do with EngineThread
        emit packetSizeChanged(engineSettings_);
    }
}

void Engine::onPacketSizeControllerFinishedSizeDetection(bool isError)
{
    runningPacketDetection_ = false;
    emit packetSizeDetectionStateChanged(false, isError);
}

void Engine::onMacAddressControllerSendUserWarning(USER_WARNING_TYPE userWarningType)
{
    emit sendUserWarning(userWarningType);
}

#ifdef Q_OS_MACOS
void Engine::onMacAddressControllerMacSpoofApplied()
{
    // On MacOS, MAC-spoofing can confuse the app into thinking it is offline
    // Update the connectivity check to fix this
    macSpoofTimerStart_ = QDateTime::currentDateTime();
    macSpoofTimer_->start();
}
#endif


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

void Engine::startProxySharingImpl(PROXY_SHARING_TYPE proxySharingType, uint port, bool whileConnected)
{
    vpnShareController_->startProxySharing(proxySharingType, port, whileConnected);
    emit proxySharingStateChanged(true, proxySharingType, getProxySharingAddress(), 0);
}

void Engine::stopProxySharingImpl()
{
    vpnShareController_->stopProxySharing();
    emit proxySharingStateChanged(false, PROXY_SHARING_HTTP, "", 0);
}

void Engine::startWifiSharingImpl(const QString &ssid, const QString &password)
{
    vpnShareController_->stopWifiSharing(); //  need to stop it first
    vpnShareController_->startWifiSharing(ssid, password);
    emit wifiSharingStateChanged(true, ssid, 0);
}

void Engine::stopWifiSharingImpl()
{
    bool bInitialState = vpnShareController_->isWifiSharingEnabled();
    vpnShareController_->stopWifiSharing();
    if (bInitialState == true)  // emit signal if state changed
    {
        emit wifiSharingStateChanged(false, "", 0);
    }
}

void Engine::setSettingsMacAddressSpoofingImpl(const types::MacAddrSpoofing &macAddrSpoofing)
{
    engineSettings_.setMacAddrSpoofing(macAddrSpoofing);
    engineSettings_.saveToSettings();
    emit macAddrSpoofingChanged(engineSettings_);
}

void Engine::setSplitTunnelingSettingsImpl(bool isActive, bool isExclude, const QStringList &files, const QStringList &ips, const QStringList &hosts)
{
    WS_ASSERT(helper_ != NULL);
    helper_->setSplitTunnelingSettings(isActive, isExclude, engineSettings_.isAllowLanTraffic(), files, ips, hosts);
}

void Engine::onApiResourceManagerCallback(ApiResourcesManagerNotification notification, LoginResult loginResult, const std::string &errorMessage)
{
    // To keep the data in persistent settings when the OS reboots or kills the process
    saveWsnetSettings();

    if (notification == ApiResourcesManagerNotification::kLoginOk) {
        onApiResourcesManagerReadyForLogin(false);
    } else if (notification == ApiResourcesManagerNotification::kLoginFailed) {
        onApiResourcesManagerLoginFailed(loginResult, QString::fromStdString(errorMessage));
    } else if (notification == ApiResourcesManagerNotification::kSessionUpdated) {
        updateSessionStatus(WSNet::instance()->apiResourcersManager()->sessionStatus());
    } else if (notification == ApiResourcesManagerNotification::kLocationsUpdated) {
        onApiResourcesManagerLocationsUpdated();
    } else if (notification == ApiResourcesManagerNotification::kStaticIpsUpdated) {
        onApiResourcesManagerLocationsUpdated();
    } else if (notification == ApiResourcesManagerNotification::kNotificationsUpdated) {
        api_responses::Notifications notifications(WSNet::instance()->apiResourcersManager()->notifications());
        emit notificationsUpdated(notifications.notifications());
    } else if (notification == ApiResourcesManagerNotification::kCheckUpdate) {
        api_responses::CheckUpdate checkUpdate(WSNet::instance()->apiResourcersManager()->checkUpdate());
        onCheckUpdateUpdated(checkUpdate);
    } else if (notification == ApiResourcesManagerNotification::kLogoutFinished) {
        // nothing todo
    } else if (notification == ApiResourcesManagerNotification::kSessionDeleted) {
        emit sessionDeleted();
    } else if (notification == ApiResourcesManagerNotification::kServerCredentialsUpdated) {
        onApiResourcesManagerServerCredentialsFetched();
    } else {
        WS_ASSERT(false);
    }
}

void Engine::onApiResourcesManagerReadyForLogin(bool isLoginFromSavedSettings)
{
    isLoggedIn_ = true;
    tryLoginNextConnectOrDisconnect_ = false;

    if (!isLoginFromSavedSettings)
        qCDebug(LOG_BASIC) << "All API resources have been updated";
    else
        qCDebug(LOG_BASIC) << "Use all API resources from the previous run";

    updateSessionStatus(WSNet::instance()->apiResourcersManager()->sessionStatus());
    onApiResourcesManagerLocationsUpdated();

    api_responses::Notifications notifications(WSNet::instance()->apiResourcersManager()->notifications());
    emit notificationsUpdated(notifications.notifications());

    if (!emergencyController_->isDisconnected()) {
        emergencyController_->blockingDisconnect();
        emergencyConnectStateController_->setDisconnectedState(DISCONNECTED_ITSELF, CONNECT_ERROR::NO_CONNECT_ERROR);
        emit emergencyDisconnected();
    }

    myIpManager_->getIP(1);
    doCheckUpdate();
    updateCurrentNetworkInterfaceImpl();

    api_responses::PortMap portMap(WSNet::instance()->apiResourcersManager()->portMap());
    emit loginFinished(isLoginFromSavedSettings, portMap);
}

void Engine::onApiResourcesManagerLoginFailed(LoginResult loginResult, const QString &errorMessage)
{
    qCDebug(LOG_BASIC) << "onApiResourcesManagerLoginFailed, retCode =" << (int)loginResult << ";errorMessage =" << errorMessage;

    if (loginResult == LoginResult::kNoConnectivity) {
        emit loginError(LoginResult::kNoConnectivity, QString());
        tryLoginNextConnectOrDisconnect_ = true;
    } else if (loginResult == LoginResult::kNoApiConnectivity) {
        if (engineSettings_.isIgnoreSslErrors()) {
            emit loginError(LoginResult::kNoApiConnectivity, QString());
            tryLoginNextConnectOrDisconnect_ = true;
        }
        else
            emit loginError(LoginResult::kSslError, QString());
    } else if (loginResult == LoginResult::kIncorrectJson) {
        emit loginError(LoginResult::kIncorrectJson, QString());
        tryLoginNextConnectOrDisconnect_ = true;
    } else if (loginResult == LoginResult::kBadUsername || loginResult == LoginResult::kBadCode2fa ||
             loginResult == LoginResult::kMissingCode2fa || loginResult == LoginResult::kAccountDisabled ||
               loginResult == LoginResult::kSessionInvalid || loginResult == LoginResult::kRateLimited) {
        emit loginError(loginResult, errorMessage);
    } else {
        WS_ASSERT(false);
    }
}

void Engine::onApiResourcesManagerLocationsUpdated()
{
    api_responses::ServerList serverLocations(WSNet::instance()->apiResourcersManager()->locations());
    api_responses::StaticIps staticIps(WSNet::instance()->apiResourcersManager()->staticIps());
    updateServerLocations(serverLocations, staticIps);

    // Auto-enable anti-censorship for first-run users if the serverlist endpoint returned a country override.
    if (checkAutoEnableAntiCensorship_) {
        checkAutoEnableAntiCensorship_ = false;
        if (!serverLocations.countryOverride().isEmpty() && !ExtraConfig::instance().haveServerListCountryOverride()) {
            qCDebug(LOG_BASIC) << "Automatically enabled anti-censorship feature due to country override";
            emit autoEnableAntiCensorship();
        }
    }
}


void Engine::onApiResourcesManagerServerCredentialsFetched()
{
    if (isFetchingServerCredentials_) {
        stopFetchingServerCredentials();
        qCDebug(LOG_BASIC) << "Engine::onRefetchServerCredentialsFinished, successfully";
        doConnect(false);
        isFetchingServerCredentials_ = false;
    }
}

void Engine::updateServerLocations(const api_responses::ServerList &serverLocations, const api_responses::StaticIps &staticIps)
{
    qCDebug(LOG_BASIC) << "Servers locations changed";
    auto locations = serverLocations.locations();
    ApiUtils::mergeWindflixLocations(locations);
    locationsModel_->setApiLocations(locations, staticIps);
    locationsModel_->setCustomConfigLocations(customConfigs_->getConfigs());
    checkForceDisconnectNode(serverLocations.forceDisconnectNodes());
}

void Engine::updateFirewallSettings()
{
    if (firewallController_->firewallActualState()) {
        if (connectStateController_->currentState() != CONNECT_STATE_CONNECTED) {
            firewallController_->firewallOn(
                firewallExceptions_.connectingIp(),
                firewallExceptions_.getIPAddressesForFirewall(),
                engineSettings_.isAllowLanTraffic(),
                locationId_.isCustomConfigsLocation());
        } else {
            firewallController_->firewallOn(
                connectionManager_->getLastConnectedIp(),
                firewallExceptions_.getIPAddressesForFirewallForConnectedState(),
                engineSettings_.isAllowLanTraffic(),
                locationId_.isCustomConfigsLocation());
        }
    }
}

void Engine::addCustomRemoteIpToFirewallIfNeed()
{
    QString ip;
    QString strHost = ExtraConfig::instance().getRemoteIpFromExtraConfig();
    if (!strHost.isEmpty())
    {
        if (IpValidation::isIpv4Address(strHost))
        {
            ip = strHost;
        }
        else if (IpValidation::isDomain(strHost))
        {
            // make DNS-resolution for add IP to firewall exceptions
            qCDebug(LOG_BASIC) << "Make DNS-resolution for" << strHost;
            auto res = WSNet::instance()->dnsResolver()->lookupBlocked(strHost.toStdString());
            if (!res->isError() && res->ips().size() > 0) {
                qCDebug(LOG_BASIC) << "Resolved IP address for" << strHost << ":" << res->ips()[0];
                ip = QString::fromStdString(res->ips()[0]);
                ExtraConfig::instance().setDetectedIp(ip);
            } else {
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
    QSharedPointer<locationsmodel::BaseLocationInfo> bli = locationsModel_->getMutableLocationInfoById(locationId_);
    if (bli.isNull())
    {
        connectStateController_->setDisconnectedState(DISCONNECTED_WITH_ERROR, CONNECT_ERROR::LOCATION_NOT_EXIST);
        myIpManager_->getIP(1);
        qCDebug(LOG_BASIC) << "Engine::connectError(LOCATION_NOT_EXIST)";
        return;
    }
    if (!bli->isExistSelectedNode())
    {
        connectStateController_->setDisconnectedState(DISCONNECTED_WITH_ERROR, CONNECT_ERROR::LOCATION_NO_ACTIVE_NODES);
        myIpManager_->getIP(1);
        qCDebug(LOG_BASIC) << "Engine::connectError(LOCATION_NO_ACTIVE_NODES)";
        return;
    }

    locationName_ = bli->getName();

    types::NetworkInterface networkInterface;
    networkDetectionManager_->getCurrentNetworkInterface(networkInterface);

    log_utils::Logger::instance().startConnectionMode(createConnectionId().toStdString());

    if (isLoggedIn_)
    {
        api_responses::ServerCredentials ovpnCredentials(WSNet::instance()->apiResourcersManager()->serverCredentialsOvpn());
        api_responses::ServerCredentials ikev2Credentials(WSNet::instance()->apiResourcersManager()->serverCredentialsIkev2());
        api_responses::PortMap portMap(WSNet::instance()->apiResourcersManager()->portMap());
        QByteArray ovpnConfig = QByteArray::fromBase64(QByteArray(WSNet::instance()->apiResourcersManager()->serverConfigs().c_str()));

        if (!bli->locationId().isCustomConfigsLocation() && !bli->locationId().isStaticIpsLocation())
        {
            qCDebug(LOG_BASIC) << "radiusUsername openvpn: " << ovpnCredentials.username();
            qCDebug(LOG_BASIC) << "radiusUsername ikev2: " << ikev2Credentials.username();
        }
        qCDebug(LOG_CONNECTION) << "Connecting to" << locationName_;

        types::ConnectionSettings connectionSettings;
        // User requested one time override
        if (!connectionSettingsOverride_.isAutomatic()) {
            qCDebug(LOG_BASIC) << "One-time override (" << connectionSettingsOverride_.protocol().toLongString() << ")";
            connectionSettings = connectionSettingsOverride_;
        } else {
            connectionSettings = engineSettings_.connectionSettingsForNetworkInterface(networkInterface.networkOrSsid);
        }

        connectionManager_->setLastKnownGoodProtocol(engineSettings_.networkLastKnownGoodProtocol(networkInterface.networkOrSsid));
        connectionManager_->clickConnect(ovpnConfig, ovpnCredentials, ikev2Credentials, bli,
            connectionSettings, portMap, ProxyServerController::instance().getCurrentProxySettings(),
            bEmitAuthError, engineSettings_.customOvpnConfigsPath(), engineSettings_.isAntiCensorship());
    }
    // for custom configs without login
    else
    {
        qCDebug(LOG_CONNECTION) << "Connecting to" << locationName_;
        connectionManager_->clickConnect("", api_responses::ServerCredentials(), api_responses::ServerCredentials(), bli,
            engineSettings_.connectionSettingsForNetworkInterface(networkInterface.networkOrSsid), api_responses::PortMap(),
            ProxyServerController::instance().getCurrentProxySettings(), bEmitAuthError, engineSettings_.customOvpnConfigsPath(), engineSettings_.isAntiCensorship());
    }
}

void Engine::doDisconnectRestoreStuff()
{
    vpnShareController_->onDisconnectedFromVPNEvent();

    // enable proxy settings
    const auto &proxySettings = ProxyServerController::instance().getCurrentProxySettings();
    WSNet::instance()->httpNetworkManager()->setProxySettings(proxySettings.curlAddress().toStdString(), proxySettings.getUsername().toStdString(), proxySettings.getPassword().toStdString());
    DnsServersConfiguration::instance().setDisconnectedState();
    WSNet::instance()->dnsResolver()->setDnsServers(DnsServersConfiguration::instance().getCurrentDnsServers());


#if defined (Q_OS_MACOS) || defined(Q_OS_LINUX)
    firewallController_->setInterfaceToSkip_posix("");
#endif

    bool bChanged;
    firewallExceptions_.setConnectingIp("", bChanged);
    firewallExceptions_.setDNSServers(QStringList(), bChanged);

    if (firewallController_->firewallActualState()) {
        firewallController_->firewallOn(
            firewallExceptions_.connectingIp(),
            firewallExceptions_.getIPAddressesForFirewall(),
            engineSettings_.isAllowLanTraffic(),
            locationId_.isCustomConfigsLocation());
    }

    // If we have disconnected and are still not logged then try again.
    if (tryLoginNextConnectOrDisconnect_) {
        loginImpl(true, QString(), QString(), QString());
    }
}

void Engine::stopFetchingServerCredentials()
{
    isFetchingServerCredentials_ = false;
}

void Engine::updateSessionStatus(const std::string &json)
{
    api_responses::SessionStatus ss(WSNet::instance()->apiResourcersManager()->sessionStatus());
    QSettings settings;
    settings.setValue("userId", ss.getUserId());    // need for uninstaller program for open post uninstall webpage
    emit sessionStatusUpdated(ss);
}

void Engine::saveWsnetSettings()
{
    QString wsnetSettings = QString::fromStdString(WSNet::instance()->currentPersistentSettings());
    QSettings settings;
    settings.setValue("wsnetSettings", wsnetSettings);

    // To correctly downgrade keep authHash in QSettings
    QString authHash = QString::fromStdString(WSNet::instance()->apiResourcersManager()->authHash());
    if (!authHash.isEmpty())
        settings.setValue("authHash", authHash);
    else
        settings.remove("authHash");
}

void Engine::stopPacketDetectionImpl()
{
    packetSizeController_->earlyStop();
}

void Engine::onConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON /*reason*/, CONNECT_ERROR /*err*/, const LocationID & /*location*/)
{
    if (helper_) {
        if (state != CONNECT_STATE_CONNECTED) {
            helper_->sendConnectStatus(false, engineSettings_.isTerminateSockets(), engineSettings_.isAllowLanTraffic(), AdapterGatewayInfo::detectAndCreateDefaultAdapterInfo(), AdapterGatewayInfo(), QString(), types::Protocol());
        }
    }
    WSNet::instance()->setIsConnectedToVpnState(state == CONNECT_STATE_CONNECTED);
}

void Engine::updateProxySettings()
{
    if (ProxyServerController::instance().updateProxySettings(engineSettings_.proxySettings())) {
        const auto &proxySettings = ProxyServerController::instance().getCurrentProxySettings();
        WSNet::instance()->httpNetworkManager()->setProxySettings(proxySettings.curlAddress().toStdString(), proxySettings.getUsername().toStdString(), proxySettings.getPassword().toStdString());
        firewallExceptions_.setProxyIP(proxySettings);
        updateFirewallSettings();
    }
}

bool Engine::verifyContentsSha256(const QString &filename, const QString &compareHash)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
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

#ifdef Q_OS_WIN
void Engine::enableDohSettings()
{
    if (WinUtils::isDohSupported()) {
        auto* helperWin = dynamic_cast<Helper_win*>(helper_);
        WS_ASSERT(helperWin);
        helperWin->enableDohSettings();
    }
}
#endif

void Engine::doCheckUpdate()
{
    UPDATE_CHANNEL channel = engineSettings_.updateChannel();
    if (overrideUpdateChannelWithInternal_) {
        qCDebug(LOG_BASIC) << "Overriding update channel: internal";
        channel = UPDATE_CHANNEL_INTERNAL;
    }

    QString osVersion, osBuild;
    Utils::getOSVersionAndBuild(osVersion, osBuild);

    WSNet::instance()->apiResourcersManager()->checkUpdate((UpdateChannel)channel, AppVersion::instance().version().toStdString(), AppVersion::instance().build().toStdString(),
                                                           osVersion.toStdString(), osBuild.toStdString());
}

void Engine::loginImpl(bool isUseAuthHash, const QString &username, const QString &password, const QString &code2fa)
{
    if (isUseAuthHash) {
        if (WSNet::instance()->apiResourcersManager()->isExist()) {
            onApiResourcesManagerReadyForLogin(true);
        }
        WSNet::instance()->apiResourcersManager()->loginWithAuthHash();
    }
    else {
        WSNet::instance()->apiResourcersManager()->login(username.toStdString(), password.toStdString(), code2fa.toStdString());
    }
}

void Engine::onWireGuardKeyLimitUserResponse(bool deleteOldestKey)
{
    connectionManager_->onWireGuardKeyLimitUserResponse(deleteOldestKey);
}

QString Engine::createConnectionId()
{
    const qint64 currentTimeMillis = QDateTime::currentMSecsSinceEpoch();
    const QByteArray timeBytes(reinterpret_cast<const char*>(&currentTimeMillis), sizeof(currentTimeMillis));
    const QByteArray hashBytes = QCryptographicHash::hash(timeBytes, QCryptographicHash::Md5);

    connId_ = hashBytes.toHex().mid(8, 4);
    emit connectionIdChanged(connId_);

    return connId_;
}

void Engine::onConnectionManagerConnectionEnded()
{
    connId_.clear();
    emit connectionIdChanged(connId_);
}
