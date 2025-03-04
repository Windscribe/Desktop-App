#pragma once

#include <atomic>

#include <QObject>
#include <QWaitCondition>
#include "firewall/firewallexceptions.h"
#include "helper/ihelper.h"
#include "helper/initializehelper.h"
#include "networkdetectionmanager/inetworkdetectionmanager.h"
#include "networkdetectionmanager/waitfornetworkconnectivity.h"
#include "firewall/firewallcontroller.h"
#include "api_responses/notification.h"
#include "locationsmodel/enginelocationsmodel.h"
#include "connectionmanager/connectionmanager.h"
#include "connectstatecontroller/connectstatecontroller.h"
#include "engine/vpnshare/vpnsharecontroller.h"
#include "engine/emergencycontroller/emergencycontroller.h"
#include "apiresources/myipmanager.h"
#include "types/enginesettings.h"
#include "engine/customconfigs/customconfigs.h"
#include "engine/customconfigs/customovpnauthcredentialsstorage.h"
#include <atomic>
#include "engine/macaddresscontroller/imacaddresscontroller.h"
#include "engine/ping/keepalivemanager.h"
#include "packetsizecontroller.h"
#include "autoupdater/downloadhelper.h"
#include "autoupdater/autoupdaterhelper_mac.h"
#include "api_responses/robertfilter.h"
#include "api_responses/checkupdate.h"
#include "api_responses/sessionstatus.h"
#include "api_responses/serverlist.h"

#if defined(Q_OS_WIN)
    #include "measurementcpuusage.h"
    #include "utils/crashhandler.h"
#elif defined(Q_OS_MACOS)
    #include "autoupdater/autoupdaterhelper_mac.h"
    #include "splittunnelextension/systemextensions_mac.h"
#endif

// all the functionality of the connections, firewall, helper, etc
// need create in separate QThread

class Engine : public QObject
{
    Q_OBJECT
public:
    explicit Engine();
    virtual ~Engine();

    void setSettings(const types::EngineSettings &engineSettings);

    void cleanup(bool isExitWithRestart, bool isFirewallChecked, bool isFirewallAlwaysOn, bool isLaunchOnStart);
    bool isCleanupFinished();

    bool isInitialized();
    void enableBFE_win();

    void loginWithAuthHash();
    void loginWithUsernameAndPassword(const QString &username, const QString &password, const QString &code2fa);
    bool isApiSavedSettingsExists();

    void logout(bool keepFirewallOn);

    void gotoCustomOvpnConfigMode();

    void continueWithUsernameAndPassword(const QString &username, const QString &password, bool bSave);
    void continueWithPassword(const QString &password, bool bSave);
    void continueWithPrivKeyPassword(const QString &password, bool bSave);

    void sendDebugLog();
    void getWebSessionToken(WEB_SESSION_PURPOSE purpose);
    void getRobertFilters();
    void setRobertFilter(const api_responses::RobertFilter &filter);
    void syncRobert();

    locationsmodel::LocationsModel *getLocationsModel();
    IConnectStateController *getConnectStateController();

    bool isFirewallEnabled();
    bool firewallOn();
    bool firewallOff();

    void connectClick(const LocationID &locationId, const types::ConnectionSettings &connectionSettings);
    void disconnectClick(DISCONNECT_REASON reason);

    bool isBlockConnect() const;
    void setBlockConnect(bool isBlockConnect);

    void recordInstall();
    void sendConfirmEmail();

    void speedRating(int rating, const QString &localExternalIp);  //rate current connection(0 - down, 1 - up)

    void updateCurrentInternetConnectivity();

    // emergency connect functions
    void emergencyConnectClick();
    void emergencyDisconnectClick();
    bool isEmergencyDisconnected();

    // vpn sharing functions
    bool isWifiSharingSupported();
    void startWifiSharing(const QString &ssid, const QString &password);
    void stopWifiSharing();
    void startProxySharing(PROXY_SHARING_TYPE proxySharingType, uint port, bool whileConnected);
    void stopProxySharing();
    QString getProxySharingAddress();
    QString getSharingCaption();

    void applicationActivated();

    void detectAppropriatePacketSize();
    void setSettingsMacAddressSpoofing(const types::MacAddrSpoofing &macAddrSpoofing);
    void setSplitTunnelingSettings(bool isActive, bool isExclude, const QStringList &files,
                                   const QStringList &ips, const QStringList &hosts);

    void updateWindowInfo(qint32 windowCenterX, qint32 windowCenterY);
    void updateVersion(qint64 windowHandle);
    void stopUpdateVersion();
    void updateAdvancedParams();

    void makeHostsFileWritableWin();

    void updateCurrentNetworkInterface();
    void reconnect();

public slots:
    void init();
    void stopPacketDetection();
    void onWireGuardKeyLimitUserResponse(bool deleteOldestKey);

signals:
    void initFinished(ENGINE_INIT_RET_CODE retCode, bool isCanLoginWithAuthHash, const types::EngineSettings &engineSettings);
    void bfeEnableFinished(ENGINE_INIT_RET_CODE retCode, bool isCanLoginWithAuthHash, const types::EngineSettings &engineSettings);
    void cleanupFinished();
    void loginFinished(bool isLoginFromSavedSettings, const api_responses::PortMap &portMap);
    void tryingBackupEndpoint(int num, int cnt);
    void loginError(wsnet::LoginResult retCode, const QString &errorMessage);
    void sessionDeleted();
    void sessionStatusUpdated(const api_responses::SessionStatus &sessionStatus);
    void notificationsUpdated(const QVector<api_responses::Notification> &notifications);
    void checkUpdateUpdated(const api_responses::CheckUpdate &checkUpdate);
    void updateVersionChanged(uint progressPercent, const UPDATE_VERSION_STATE &state, const UPDATE_VERSION_ERROR &error);
    void updateDownloaded(const QString &path);
    void myIpUpdated(const QString &ip, bool isDisconnected);
    void statisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes);
    void protocolPortChanged(const types::Protocol &protocol, const uint port);
    void robertFiltersUpdated(bool success, const QVector<api_responses::RobertFilter> &filters);
    void setRobertFilterFinished(bool success);
    void syncRobertFinished(bool success);

    void requestUsernameAndPassword(const QString &username);
    void requestPrivKeyPassword();

    void emergencyConnected();
    void emergencyDisconnected();
    void emergencyConnectError(CONNECT_ERROR err);

    void sendDebugLogFinished(bool bSuccess);
    void confirmEmailFinished(bool bSuccess);
    void webSessionToken(WEB_SESSION_PURPOSE purpose, const QString &tempSessionToken);
    void firewallStateChanged(bool isEnabled);
    void testTunnelResult(bool bSuccess);
    void lostConnectionToHelper();
    void proxySharingStateChanged(bool bEnabled, PROXY_SHARING_TYPE proxySharingType, const QString &address, int usersCount);
    void wifiSharingStateChanged(bool bEnabled, const QString &ssid, int usersCount);
    void wifiSharingFailed(WIFI_SHARING_ERROR error);

    void logoutFinished();

    void gotoCustomOvpnConfigModeFinished();

    void detectionCpuUsageAfterConnected(const QStringList processesList);

    void networkChanged(types::NetworkInterface networkInterface);

    void macAddrSpoofingChanged(const types::EngineSettings &engineSettings);
    void sendUserWarning(USER_WARNING_TYPE userWarningType);
    void internetConnectivityChanged(bool connectivity);
    void packetSizeChanged(const types::EngineSettings &engineSettings);
    void packetSizeDetectionStateChanged(bool on, bool isError);

    void hostsFileBecameWritable();

    void wireGuardAtKeyLimit();
    void protocolStatusChanged(const QVector<types::ProtocolStatus> status);
    void initCleanup(bool isExitWithRestart, bool isFirewallChecked, bool isFirewallAlwaysOn, bool isLaunchOnStart);

    void helperSplitTunnelingStartFailed();

    void autoEnableAntiCensorship();

    void connectionIdChanged(const QString &connId);

private slots:
    void onLostConnectionToHelper();
    void onInitializeHelper(INIT_HELPER_RET ret);

    void cleanupImpl(bool isExitWithRestart, bool isFirewallChecked, bool isFirewallAlwaysOn, bool isLaunchOnStart);
    void enableBFE_winImpl();
    void recordInstallImpl();
    void sendConfirmEmailImpl();
    void connectClickImpl(const LocationID &locationId, const types::ConnectionSettings &connectionSettings);
    void disconnectClickImpl(DISCONNECT_REASON reason);
    void sendDebugLogImpl();
    void getWebSessionTokenImpl(WEB_SESSION_PURPOSE purpose);
    void logoutImpl(bool keepFirewallOn);
    void logoutImplAfterDisconnect(bool keepFirewallOn);
    void continueWithUsernameAndPasswordImpl(const QString &username, const QString &password, bool bSave);
    void continueWithPasswordImpl(const QString &password, bool bSave);
    void continueWithPrivKeyPasswordImpl(const QString &password, bool bSave);

    void gotoCustomOvpnConfigModeImpl();

    void updateCurrentInternetConnectivityImpl();
    void reconnectImpl();
    void updateCurrentNetworkInterfaceImpl();

    void firewallOnImpl();
    void firewallOffImpl();
    void speedRatingImpl(int rating, const QString &localExternalIp);
    void setSettingsImpl(const types::EngineSettings &engineSettings);
    void checkForceDisconnectNode(const QStringList &forceDisconnectNodes);

    void startProxySharingImpl(PROXY_SHARING_TYPE proxySharingType, uint port, bool whileConnected);
    void stopProxySharingImpl();

    void startWifiSharingImpl(const QString &ssid, const QString &password);
    void stopWifiSharingImpl();

    void setSettingsMacAddressSpoofingImpl(const types::MacAddrSpoofing &macAddrSpoofing);
    void setSplitTunnelingSettingsImpl(bool isActive, bool isExclude, const QStringList &files,
                                       const QStringList &ips, const QStringList &hosts);

    void onApiResourcesManagerReadyForLogin(bool isLoginFromSavedSettings);
    void onApiResourcesManagerLoginFailed(wsnet::LoginResult loginResult, const QString &errorMessage);
    void onApiResourcesManagerLocationsUpdated();
    void onApiResourcesManagerServerCredentialsFetched();

    void onApiResourceManagerCallback(wsnet::ApiResourcesManagerNotification notification, wsnet::LoginResult loginResult, const std::string &errorMessage);

    void onFailOverTryingBackupEndpoint(int num, int cnt);

    void onCheckUpdateUpdated(const api_responses::CheckUpdate &checkUpdate);
    void onHostIPsChanged(const QSet<QString> &hostIps);
    void onMyIpManagerIpChanged(const QString &ip, bool isFromDisconnectedState);

    void onConnectionManagerConnected();
    void onConnectionManagerDisconnected(DISCONNECT_REASON reason);
    void onConnectionManagerReconnecting();
    void onConnectionManagerError(CONNECT_ERROR err);
    void onConnectionManagerInternetConnectivityChanged(bool connectivity);
    void onConnectionManagerStatisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes);
    void onConnectionManagerInterfaceUpdated(const QString &interfaceName);
    void onConnectionManagerConnectingToHostname(const QString &hostname, const QString &ip, const QStringList &dnsServers);
    void onConnectionManagerProtocolPortChanged(const types::Protocol &protocol, const uint port);
    void onConnectionManagerTestTunnelResult(bool success, const QString & ipAddress);
    void onConnectionManagerWireGuardAtKeyLimit();
    void onConnectionManagerConnectionEnded();

    void onConnectionManagerRequestUsername(const QString &pathCustomOvpnConfig);
    void onConnectionManagerRequestPassword(const QString &pathCustomOvpnConfig);
    void onConnectionManagerRequestPrivKeyPassword(const QString &pathCustomOvpnConfig);

    void emergencyConnectClickImpl();
    void emergencyDisconnectClickImpl();

    void detectAppropriatePacketSizeImpl();

    void updateWindowInfoImpl(qint32 windowCenterX, qint32 windowCenterY);
    void updateVersionImpl(qint64 windowHandle);
    void stopUpdateVersionImpl();
    void updateAdvancedParamsImpl();
    void onDownloadHelperProgressChanged(uint progressPercent);
    void onDownloadHelperFinished(const DownloadHelper::DownloadState &state);
    void updateRunInstaller(qint32 windowCenterX, qint32 windowCenterY);

    void onEmergencyControllerConnected();
    void onEmergencyControllerDisconnected(DISCONNECT_REASON reason);
    void onEmergencyControllerError(CONNECT_ERROR err);

    void getRobertFiltersImpl();
    void setRobertFilterImpl(const api_responses::RobertFilter &filter);
    void syncRobertImpl();

    void onCustomConfigsChanged();

    void onLocationsModelWhitelistIpsChanged(const QStringList &ips);
    void onLocationsModelWhitelistCustomConfigIpsChanged(const QStringList &ips);

    void onNetworkOnlineStateChange(bool isOnline);
    void onNetworkChange(const types::NetworkInterface &networkInterface);
    void onPacketSizeControllerPacketSizeChanged(bool isAuto, int mtu);
    void onPacketSizeControllerFinishedSizeDetection(bool isError);

    void onMacAddressSpoofingChanged(const types::MacAddrSpoofing &macAddrSpoofing);
    void onMacAddressControllerSendUserWarning(USER_WARNING_TYPE userWarningType);
#ifdef Q_OS_MACOS
    void onMacAddressControllerMacSpoofApplied();
#endif

    void stopPacketDetectionImpl();

    void onConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, CONNECT_ERROR err, const LocationID &location);

#ifdef Q_OS_MACOS
    void onMacSpoofTimerTick();
    void onSystemExtensionStateChanged(SystemExtensions_mac::SystemExtensionState newState);
#endif

private:
    void initPart2();
    void updateProxySettings();
    bool verifyContentsSha256(const QString &filename, const QString &compareHash);

#ifdef Q_OS_WIN
    void enableDohSettings();
#endif

    types::EngineSettings engineSettings_;
    IHelper *helper_;
    FirewallController *firewallController_;
    ConnectionManager *connectionManager_;
    ConnectStateController *connectStateController_;
    VpnShareController *vpnShareController_;
    EmergencyController *emergencyController_;
    ConnectStateController *emergencyConnectStateController_;
    customconfigs::CustomConfigs *customConfigs_;
    CustomOvpnAuthCredentialsStorage *customOvpnAuthCredentialsStorage_;
    INetworkDetectionManager *networkDetectionManager_;
    IMacAddressController *macAddressController_;
    KeepAliveManager *keepAliveManager_;
    PacketSizeController *packetSizeController_;

    api_resources::MyIpManager *myIpManager_;

#ifdef Q_OS_WIN
    MeasurementCpuUsage *measurementCpuUsage_;
    QScopedPointer<Debug::CrashHandlerForThread> crashHandler_;
#endif

    InitializeHelper *inititalizeHelper_;
    bool bInitialized_;

    FirewallExceptions firewallExceptions_;

    locationsmodel::LocationsModel *locationsModel_;

    DownloadHelper *downloadHelper_;
#ifdef Q_OS_MACOS
    AutoUpdaterHelper_mac *autoUpdaterHelper_;
    QDateTime macSpoofTimerStart_;
    QTimer *macSpoofTimer_;
#endif

    QMutex mutex_;

    QMutex mutexForOnHostIPsChanged_;
    QWaitCondition waitConditionForOnHostIPsChanged_;

    std::atomic<bool> isBlockConnect_;
    std::atomic<bool> isCleanupFinished_;

    LocationID locationId_;
    QString locationName_;

    QString lastConnectingHostname_;
    types::Protocol lastConnectingProtocol_;

    bool isNeedReconnectAfterRequestAuth_;

    bool online_;

    types::PacketSize packetSize_;
    QThread *packetSizeControllerThread_;
    bool runningPacketDetection_;

    void doCheckUpdate();
    void loginImpl(bool isUseAuthHash, const QString &username, const QString &password, const QString &code2fa);
    void updateServerLocations(const api_responses::ServerList &serverLocations, const api_responses::StaticIps &staticIps);
    void updateFirewallSettings();

    void addCustomRemoteIpToFirewallIfNeed();
    void doConnect(bool bEmitAuthError);
    void doDisconnectRestoreStuff();

    void stopFetchingServerCredentials();

    void updateSessionStatus(const std::string &json);
    void saveWsnetSettings();

    uint lastDownloadProgress_;
    QString installerUrl_;
    QString installerPath_;
    QString installerHash_;
    qint64 guiWindowHandle_;

    bool overrideUpdateChannelWithInternal_;
    bool bPrevNetworkInterfaceInitialized_;
    types::NetworkInterface prevNetworkInterface_;

    types::ConnectionSettings connectionSettingsOverride_;

    bool checkAutoEnableAntiCensorship_ = false;

    bool isLoggedIn_ = false;
    bool isFetchingServerCredentials_ = false;
    bool tryLoginNextConnectOrDisconnect_ = false;

    QString lastUsernameForCustomConfig_;

    static constexpr int kLoginWaitTimeForNoNetwork = 10000;    // 10 sec
    WaitForNetworkConnectivity *loginWaitForNetworkConnectivity_;

    // Identifier for current connection.  This identifier changes when a new connect request is made to the engine.
    // This is also used for logging to separate logs for different connections.
    QString connId_;
    QString createConnectionId();
};
