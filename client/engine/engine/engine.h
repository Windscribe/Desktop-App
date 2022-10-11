#pragma once

#include <QObject>
#include "firewall/firewallexceptions.h"
#include "helper/ihelper.h"
#include "helper/initializehelper.h"
#include "networkdetectionmanager/inetworkdetectionmanager.h"
#include "firewall/firewallcontroller.h"
#include "serverapi/serverapi.h"
#include "types/notification.h"
#include "locationsmodel/enginelocationsmodel.h"
#include "connectionmanager/connectionmanager.h"
#include "connectstatecontroller/connectstatecontroller.h"
#include "engine/vpnshare/vpnsharecontroller.h"
#include "engine/emergencycontroller/emergencycontroller.h"
#include "apiresources/myipmanager.h"
#include "types/enginesettings.h"
#include "types/checkupdate.h"
#include "engine/customconfigs/customconfigs.h"
#include "engine/customconfigs/customovpnauthcredentialsstorage.h"
#include <atomic>
#include "engine/macaddresscontroller/imacaddresscontroller.h"
#include "engine/ping/keepalivemanager.h"
#include "packetsizecontroller.h"
#include "autoupdater/downloadhelper.h"
#include "autoupdater/autoupdaterhelper_mac.h"
#include "networkaccessmanager/networkaccessmanager.h"
#include "apiresources/apiresourcesmanager.h"
#include "apiresources/checkupdatemanager.h"

#ifdef Q_OS_WIN
    #include "measurementcpuusage.h"
#endif

// all the functionality of the connections, firewall, helper, etc
// need create in separate QThread

class Engine : public QObject
{
    Q_OBJECT
public:
    explicit Engine(const types::EngineSettings &engineSettings);
    virtual ~Engine();

    void setSettings(const types::EngineSettings &engineSettings);

    void cleanup(bool isExitWithRestart, bool isFirewallChecked, bool isFirewallAlwaysOn, bool isLaunchOnStart);
    bool isCleanupFinished();

    bool isInitialized();
    void enableBFE_win();

    void loginWithAuthHash();
    void loginWithUsernameAndPassword(const QString &username, const QString &password, const QString &code2fa);
    bool isApiSavedSettingsExists();

    void signOut(bool keepFirewallOn);

    void gotoCustomOvpnConfigMode();

    void continueWithUsernameAndPassword(const QString &username, const QString &password, bool bSave);
    void continueWithPassword(const QString &password, bool bSave);

    void sendDebugLog();
    void setIPv6EnabledInOS(bool b);
    bool IPv6StateInOS();
    void getWebSessionToken(WEB_SESSION_PURPOSE purpose);
    void getRobertFilters();
    void setRobertFilter(const types::RobertFilter &filter);
    void syncRobert();

    locationsmodel::LocationsModel *getLocationsModel();
    IConnectStateController *getConnectStateController();

    bool isFirewallEnabled();
    bool firewallOn();
    bool firewallOff();

    void connectClick(const LocationID &locationId);
    void disconnectClick();

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
    void startProxySharing(PROXY_SHARING_TYPE proxySharingType);
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

public slots:
    void init();
    void stopPacketDetection();
    void onWireGuardKeyLimitUserResponse(bool deleteOldestKey);

signals:
    void initFinished(ENGINE_INIT_RET_CODE retCode, bool isCanLoginWithAuthHash);
    void bfeEnableFinished(ENGINE_INIT_RET_CODE retCode, bool isCanLoginWithAuthHash);
    void cleanupFinished();
    void loginFinished(bool isLoginFromSavedSettings, const QString &authHash, const types::PortMap &portMap);
    void loginStepMessage(LOGIN_MESSAGE msg);
    void loginError(LOGIN_RET retCode, const QString &errorMessage);
    void sessionDeleted();
    void sessionStatusUpdated(const types::SessionStatus &sessionStatus);
    void notificationsUpdated(const QVector<types::Notification> &notifications);
    void checkUpdateUpdated(const types::CheckUpdate &checkUpdate);
    void updateVersionChanged(uint progressPercent, const UPDATE_VERSION_STATE &state, const UPDATE_VERSION_ERROR &error);
    void myIpUpdated(const QString &ip, bool isDisconnected);
    void statisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes);
    void protocolPortChanged(const PROTOCOL &protocol, const uint port);
    void robertFiltersUpdated(bool success, const QVector<types::RobertFilter> &filters);
    void setRobertFilterFinished(bool success);
    void syncRobertFinished(bool success);

    void requestUsername();
    void requestPassword();

    void emergencyConnected();
    void emergencyDisconnected();
    void emergencyConnectError(CONNECT_ERROR err);

    void sendDebugLogFinished(bool bSuccess);
    void confirmEmailFinished(bool bSuccess);
    void webSessionToken(WEB_SESSION_PURPOSE purpose, const QString &tempSessionToken);
    void firewallStateChanged(bool isEnabled);
    void testTunnelResult(bool bSuccess);
    void lostConnectionToHelper();
    void proxySharingStateChanged(bool bEnabled, PROXY_SHARING_TYPE proxySharingType);
    void wifiSharingStateChanged(bool bEnabled, const QString &ssid);
    void vpnSharingConnectedWifiUsersCountChanged(int usersCount);
    void vpnSharingConnectedProxyUsersCountChanged(int usersCount);

    void signOutFinished();

    void gotoCustomOvpnConfigModeFinished();

    void detectionCpuUsageAfterConnected(const QStringList processesList);

    void networkChanged(types::NetworkInterface networkInterface);

    void macAddrSpoofingChanged(const types::MacAddrSpoofing &macAddrSpoofing);
    void sendUserWarning(USER_WARNING_TYPE userWarningType);
    void internetConnectivityChanged(bool connectivity);
    void packetSizeChanged(bool isAuto, int mtu);
    void packetSizeDetectionStateChanged(bool on, bool isError);

    void hostsFileBecameWritable();

    void wireGuardAtKeyLimit();
    void initCleanup(bool isExitWithRestart, bool isFirewallChecked, bool isFirewallAlwaysOn, bool isLaunchOnStart);

private slots:
    void onLostConnectionToHelper();
    void onInitializeHelper(INIT_HELPER_RET ret);

    void cleanupImpl(bool isExitWithRestart, bool isFirewallChecked, bool isFirewallAlwaysOn, bool isLaunchOnStart);
    void enableBFE_winImpl();
    void setIgnoreSslErrorsImlp(bool bIgnoreSslErrors);
    void recordInstallImpl();
    void sendConfirmEmailImpl();
    void connectClickImpl(const LocationID &locationId);
    void disconnectClickImpl();
    void sendDebugLogImpl();
    void getWebSessionTokenImpl(WEB_SESSION_PURPOSE purpose);
    void signOutImpl(bool keepFirewallOn);
    void signOutImplAfterDisconnect(bool keepFirewallOn);
    void continueWithUsernameAndPasswordImpl(const QString &username, const QString &password, bool bSave);
    void continueWithPasswordImpl(const QString &password, bool bSave);

    void gotoCustomOvpnConfigModeImpl();

    void updateCurrentInternetConnectivityImpl();
    void updateCurrentNetworkInterfaceImpl();

    void firewallOnImpl();
    void firewallOffImpl();
    void speedRatingImpl(int rating, const QString &localExternalIp);
    void setSettingsImpl(const types::EngineSettings &engineSettings);
    void checkForceDisconnectNode(const QStringList &forceDisconnectNodes);

    void startProxySharingImpl(PROXY_SHARING_TYPE proxySharingType);
    void stopProxySharingImpl();

    void startWifiSharingImpl(const QString &ssid, const QString &password);
    void stopWifiSharingImpl();

    void setSettingsMacAddressSpoofingImpl(const types::MacAddrSpoofing &macAddrSpoofing);
    void setSplitTunnelingSettingsImpl(bool isActive, bool isExclude, const QStringList &files,
                                       const QStringList &ips, const QStringList &hosts);

    void onApiResourcesManagerReadyForLogin();
    void onApiResourcesManagerLoginFailed(LOGIN_RET retCode, const QString &errorMessage);
    void onApiResourcesManagerSessionDeleted();
    void onApiResourcesManagerSessionUpdated(const types::SessionStatus &sessionStatus);
    void onApiResourcesManagerLocationsUpdated();
    void onApiResourcesManagerStaticIpsUpdated();
    void onApiResourcesManagerNotificationsUpdated(const QVector<types::Notification> &notifications);
    void onApiResourcesManagerServerCredentialsFetched();


    void onLoginControllerStepMessage(LOGIN_MESSAGE msg);

    void onCheckUpdateUpdated(const types::CheckUpdate &checkUpdate);
    void onHostIPsChanged(const QSet<QString> &hostIps);
    void onMyIpManagerIpChanged(const QString &ip, bool isFromDisconnectedState);
    void onDebugLogAnswer();
    void onConfirmEmailAnswer();
    void onWebSessionAnswer();
    void onGetRobertFiltersAnswer();
    void onSetRobertFilterAnswer();
    void onSyncRobertAnswer();

    void onConnectionManagerConnected();
    void onConnectionManagerDisconnected(DISCONNECT_REASON reason);
    void onConnectionManagerReconnecting();
    void onConnectionManagerError(CONNECT_ERROR err);
    void onConnectionManagerInternetConnectivityChanged(bool connectivity);
    void onConnectionManagerStatisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes);
    void onConnectionManagerInterfaceUpdated(const QString &interfaceName);
    void onConnectionManagerConnectingToHostname(const QString &hostname, const QString &ip, const QString &dnsServer);
    void onConnectionManagerProtocolPortChanged(const PROTOCOL &protocol, const uint port);
    void onConnectionManagerTestTunnelResult(bool success, const QString & ipAddress);
    void onConnectionManagerWireGuardAtKeyLimit();

    void onConnectionManagerRequestUsername(const QString &pathCustomOvpnConfig);
    void onConnectionManagerRequestPassword(const QString &pathCustomOvpnConfig);

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
    void setRobertFilterImpl(const types::RobertFilter &filter);
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
#ifdef Q_OS_MAC
    void onMacAddressControllerRobustMacSpoofApplied();
#endif

    void stopPacketDetectionImpl();

    void onConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, CONNECT_ERROR err, const LocationID &location);

#ifdef Q_OS_MAC
    void onRobustMacSpoofTimerTick();
#endif

private:
    void initPart2();
    void updateProxySettings();
    bool verifyContentsSha256(const QString &filename, const QString &compareHash);

    types::EngineSettings engineSettings_;
    IHelper *helper_;
    FirewallController *firewallController_;
    NetworkAccessManager *networkAccessManager_;
    server_api::ServerAPI *serverAPI_;
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

    QScopedPointer<api_resources::ApiResourcesManager> apiResourcesManager_;    // can be null for the custom config mode or when we in the logout state
    api_resources::CheckUpdateManager *checkUpdateManager_;
    api_resources::MyIpManager *myIpManager_;

#ifdef Q_OS_WIN
    MeasurementCpuUsage *measurementCpuUsage_;
#endif

    InitializeHelper *inititalizeHelper_;
    bool bInitialized_;

    FirewallExceptions firewallExceptions_;

    locationsmodel::LocationsModel *locationsModel_;

    DownloadHelper *downloadHelper_;
#ifdef Q_OS_MAC
    AutoUpdaterHelper_mac *autoUpdaterHelper_;
    QDateTime robustTimerStart_;
    QTimer *robustMacSpoofTimer_;
#endif

    QMutex mutex_;

    std::atomic<bool> isBlockConnect_;
    std::atomic<bool> isCleanupFinished_;

    LocationID locationId_;
    QString locationName_;

    QString lastConnectingHostname_;
    PROTOCOL lastConnectingProtocol_;

    bool isNeedReconnectAfterRequestUsernameAndPassword_;

    bool online_;

    types::PacketSize packetSize_;
    QThread *packetSizeControllerThread_;
    bool runningPacketDetection_;

    void doCheckUpdate();
    void loginImpl(bool isUseAuthHash, const QString &username, const QString &password, const QString &code2fa);
    void updateServerLocations();
    void updateFirewallSettings();

    void addCustomRemoteIpToFirewallIfNeed();
    void doConnect(bool bEmitAuthError);
    void doDisconnectRestoreStuff();

    void stopFetchingServerCredentials();

    uint lastDownloadProgress_;
    QString installerUrl_;
    QString installerPath_;
    QString installerHash_;
    qint64 guiWindowHandle_;

    bool overrideUpdateChannelWithInternal_;
    bool bPrevNetworkInterfaceInitialized_;
    types::NetworkInterface prevNetworkInterface_;
};
