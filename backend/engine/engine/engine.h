#ifndef ENGINE_H
#define ENGINE_H

#include <QObject>
#include "firewall/firewallexceptions.h"
#include "logincontroller/logincontroller.h"
#include "helper/ihelper.h"
#include "helper/initializehelper.h"
#include "networkstatemanager/inetworkstatemanager.h"
#include "networkdetectionmanager/inetworkdetectionmanager.h"
#include "firewall/firewallcontroller.h"
#include "serverapi/serverapi.h"
#include "locationsmodel/locationsmodel.h"
#include "connectionmanager/connectionmanager.h"
#include "connectstatecontroller/connectstatecontroller.h"
#include "engine/refetchservercredentialshelper.h"
#include "engine/vpnshare/vpnsharecontroller.h"
#include "engine/emergencycontroller/emergencycontroller.h"
#include "getmyipcontroller.h"
#include "enginesettings.h"
#include "sessionstatustimer.h"
#include "engine/customconfigs/customconfigs.h"
#include "engine/customconfigs/customovpnauthcredentialsstorage.h"
#include <atomic>
#include "engine/macaddresscontroller/imacaddresscontroller.h"
#include "engine/ping/keepalivemanager.h"
#include "packetsizecontroller.h"
#include "autoupdater/downloadhelper.h"
#include "autoupdater/autoupdaterhelper_mac.h"
#include "engine/splittunnelingnetworkinfo/splittunnelingnetworkinfo.h"

#ifdef Q_OS_WIN
    #include "measurementcpuusage.h"
#endif

// all the functionality of the connections, firewall, helper, etc
// need create in separate QThread

class Engine : public QObject
{
    Q_OBJECT
public:
    explicit Engine(const EngineSettings &engineSettings);
    virtual ~Engine();

    void setSettings(const EngineSettings &engineSettings);

    void cleanup(bool isExitWithRestart, bool isFirewallChecked, bool isFirewallAlwaysOn, bool isLaunchOnStart);
    bool isCleanupFinished();

    bool isInitialized();
    void enableBFE_win();

    void loginWithAuthHash(const QString &authHash);
    void loginWithUsernameAndPassword(const QString &username, const QString &password, const QString &code2fa);
    void loginWithLastLoginSettings();
    bool isApiSavedSettingsExists();
    void signOut();

    void gotoCustomOvpnConfigMode();

    void continueWithUsernameAndPassword(const QString &username, const QString &password, bool bSave);
    void continueWithPassword(const QString &password, bool bSave);

    void sendDebugLog();
    void setIPv6EnabledInOS(bool b);
    bool IPv6StateInOS();

    LoginSettings getLastLoginSettings();
    QString getAuthHash();
    void clearCredentials();

    locationsmodel::LocationsModel *getLocationsModel();
    IConnectStateController *getConnectStateController();

    bool isFirewallEnabled();
    bool firewallOn();
    bool firewallOff();

    void connectClick(const LocationID &locationId);
    void disconnectClick();

    void setBlockConnect(bool isBlockConnect);

    void recordInstall();
    void sendConfirmEmail();

    void speedRating(int rating, const QString &localExternalIp);  //rate current connection(0 - down, 1 - up)

    void updateServerConfigs();
    void updateCurrentNetworkInterface(bool requested = false);
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
    void applicationDeactivated();

    void forceUpdateServerLocations();

    void detectAppropriatePacketSize();
    void setSettingsMacAddressSpoofing(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing);
    void setSplitTunnelingSettings(bool isActive, bool isExclude, const QStringList &files,
                                   const QStringList &ips, const QStringList &hosts);

    void updateVersion(qint32 windowHandle);
    void stopUpdateVersion();

public slots:
    void init();

    void stopPacketDetection();

signals:
    void initFinished(ENGINE_INIT_RET_CODE retCode);
    void bfeEnableFinished(ENGINE_INIT_RET_CODE retCode);
    void cleanupFinished();
    void loginFinished(bool isLoginFromSavedSettings, const QString &authHash, const apiinfo::PortMap &portMap);
    void loginStepMessage(LOGIN_MESSAGE msg);
    void loginError(LOGIN_RET retCode);
    void sessionDeleted();
    void sessionStatusUpdated(const apiinfo::SessionStatus &sessionStatus);
    void notificationsUpdated(const QVector<apiinfo::Notification> &notifications);
    void checkUpdateUpdated(bool available, const QString &version, const ProtoTypes::UpdateChannel updateChannel, int latestBuild, const QString &url, bool supported);
    void updateVersionChanged(uint progressPercent, const ProtoTypes::UpdateVersionState &state, const ProtoTypes::UpdateVersionError &error);
    void myIpUpdated(const QString &ip, bool success, bool isDisconnected);
    void statisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes);
    void protocolPortChanged(const ProtoTypes::Protocol &protocol, const uint port);

    void requestUsername();
    void requestPassword();

    void emergencyConnected();
    void emergencyDisconnected();
    void emergencyConnectError(CONNECTION_ERROR err);

    void sendDebugLogFinished(bool bSuccess);
    void confirmEmailFinished(bool bSuccess);
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

    void networkChanged(ProtoTypes::NetworkInterface networkInterface);
    // void engineSettingsChanged(const ProtoTypes::EngineSettings &engineSettings);

    void macAddrSpoofingChanged(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing);
    void sendUserWarning(ProtoTypes::UserWarningType userWarningType);
    void internetConnectivityChanged(bool connectivity);
    void packetSizeChanged(bool isAuto, int mtu);
    void packetSizeDetectionStateChanged(bool on, bool isError);

private slots:
    void onLostConnectionToHelper();
    void onInitializeHelper(INIT_HELPER_RET ret);

    void cleanupImpl(bool isExitWithRestart, bool isFirewallChecked, bool isFirewallAlwaysOn, bool isLaunchOnStart);
    void clearCredentialsImpl();
    void enableBFE_winImpl();
    void loginImpl(bool bSkipLoadingFromSettings);
    void setIgnoreSslErrorsImlp(bool bIgnoreSslErrors);
    void recordInstallImpl();
    void sendConfirmEmailImpl();
    void connectClickImpl(const LocationID &locationId);
    void disconnectClickImpl();
    void sendDebugLogImpl();
    void signOutImpl();
    void signOutImplAfterDisconnect();
    void continueWithUsernameAndPasswordImpl(const QString &username, const QString &password, bool bSave);
    void continueWithPasswordImpl(const QString &password, bool bSave);

    void gotoCustomOvpnConfigModeImpl();

    void updateCurrentInternetConnectivityImpl();
    void updateCurrentNetworkInterfaceImpl(bool requested);

    void firewallOnImpl();
    void firewallOffImpl();
    void speedRatingImpl(int rating, const QString &localExternalIp);
    void setSettingsImpl(const EngineSettings &engineSettings);
    void updateServerConfigsImpl();
    void checkForceDisconnectNode(const QStringList &forceDisconnectNodes);

    void forceUpdateServerLocationsImpl();

    void startProxySharingImpl(PROXY_SHARING_TYPE proxySharingType);
    void stopProxySharingImpl();

    void startWifiSharingImpl(const QString &ssid, const QString &password);
    void stopWifiSharingImpl();

    void applicationActivatedImpl();
    void applicationDeactivatedImpl();

    void setSettingsMacAddressSpoofingImpl(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing);
    void setSplitTunnelingSettingsImpl(bool isActive, bool isExclude, const QStringList &files,
                                       const QStringList &ips, const QStringList &hosts);

    void onLoginControllerFinished(LOGIN_RET retCode, const apiinfo::ApiInfo &apiInfo, bool bFromConnectedToVPNState);
    void onReadyForNetworkRequests();
    void onLoginControllerStepMessage(LOGIN_MESSAGE msg);

    void onServerLocationsAnswer(SERVER_API_RET_CODE retCode, const QVector<apiinfo::Location> &serverLocations,
                                 QStringList forceDisconnectNodes, uint userRole);

    void onSessionAnswer(SERVER_API_RET_CODE retCode, const apiinfo::SessionStatus &sessionStatus, uint userRole);
    void onNotificationsAnswer(SERVER_API_RET_CODE retCode, const QVector<apiinfo::Notification> &notifications, uint userRole);
    void onServerConfigsAnswer(SERVER_API_RET_CODE retCode, const QString &config, uint userRole);
    void onCheckUpdateAnswer(bool available, const QString &version, const ProtoTypes::UpdateChannel updateChannel, int latestBuild, const QString &url, bool supported, bool bNetworkErrorOccured, uint userRole);
    void onHostIPsChanged(const QStringList &hostIps);
    void onMyIpAnswer(const QString &ip, bool success, bool isDisconnected);
    void onDebugLogAnswer(SERVER_API_RET_CODE retCode, uint userRole);
    void onConfirmEmailAnswer(SERVER_API_RET_CODE retCode, uint userRole);
    void onStaticIpsAnswer(SERVER_API_RET_CODE retCode, const apiinfo::StaticIps &staticIps, uint userRole);
    void onGetWireGuardConfigAnswer(SERVER_API_RET_CODE retCode, QSharedPointer<WireGuardConfig> config, uint userRole);

    void onStartCheckUpdate();
    void onStartStaticIpsUpdate();
    void onUpdateSessionStatusTimer();

    void onConnectionManagerConnected();
    void onConnectionManagerDisconnected(DISCONNECT_REASON reason);
    void onConnectionManagerReconnecting();
    void onConnectionManagerError(CONNECTION_ERROR err);
    void onConnectionManagerInternetConnectivityChanged(bool connectivity);
    void onConnectionManagerStatisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes);
    void onConnectionManagerInterfaceUpdated(const QString &interfaceName);
    void onConnectionManagerConnectingToHostname(const QString &hostname, const QString &ip);
    void onConnectionManagerProtocolPortChanged(const ProtoTypes::Protocol &protocol, const uint port);
    void onConnectionManagerTestTunnelResult(bool success, const QString & ipAddress);
    void onConnectionManagerGetWireGuardConfig();

    void onConnectionManagerRequestUsername(const QString &pathCustomOvpnConfig);
    void onConnectionManagerRequestPassword(const QString &pathCustomOvpnConfig);

    void emergencyConnectClickImpl();
    void emergencyDisconnectClickImpl();

    void detectAppropriatePacketSizeImpl();

    void updateVersionImpl(qint32 windowHandle);
    void stopUpdateVersionImpl();
    void onDownloadHelperProgressChanged(uint progressPercent);
    void onDownloadHelperFinished(const DownloadHelper::DownloadState &state);

    void onEmergencyControllerConnected();
    void onEmergencyControllerDisconnected(DISCONNECT_REASON reason);
    void onEmergencyControllerError(CONNECTION_ERROR err);

    void onRefetchServerCredentialsFinished(bool success, const apiinfo::ServerCredentials &serverCredentials);

    void getNewNotifications();

    void onCustomConfigsChanged();

    void onLocationsModelWhitelistIpsChanged(const QStringList &ips);
    void onLocationsModelWhitelistCustomConfigIpsChanged(const QStringList &ips);

    void onNetworkChange(ProtoTypes::NetworkInterface networkInterface);
    void onNetworkStateManagerStateChanged(bool isActive, const QString &networkInterface);
    void onMacAddressSpoofingChanged(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing);
    void onPacketSizeControllerPacketSizeChanged(bool isAuto, int mtu);
    void onPacketSizeControllerFinishedSizeDetection(bool isError);
    void onMacAddressControllerSendUserWarning(ProtoTypes::UserWarningType userWarningType);

    void stopPacketDetectionImpl();

    void onConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, CONNECTION_ERROR err, const LocationID &location);

    void fetchWireGuardConfig();

private:
    void updateProxySettings();

    EngineSettings engineSettings_;
    IHelper *helper_;
    INetworkStateManager *networkStateManager_;
    FirewallController *firewallController_;
    ServerAPI *serverAPI_;
    ConnectionManager *connectionManager_;
    ConnectStateController *connectStateController_;
    uint serverApiUserRole_;
    GetMyIPController *getMyIPController_;
    VpnShareController *vpnShareController_;
    EmergencyController *emergencyController_;
    ConnectStateController *emergencyConnectStateController_;
    customconfigs::CustomConfigs *customConfigs_;
    CustomOvpnAuthCredentialsStorage *customOvpnAuthCredentialsStorage_;
    INetworkDetectionManager *networkDetectionManager_;
    IMacAddressController *macAddressController_;
    KeepAliveManager *keepAliveManager_;
    PacketSizeController *packetSizeController_;

#ifdef Q_OS_WIN
    MeasurementCpuUsage *measurementCpuUsage_;
#endif

    SplitTunnelingNetworkInfo *splitTunnelingNetworkInfo_;

    InitializeHelper *inititalizeHelper_;
    bool bInitialized_;

    QScopedPointer<apiinfo::ApiInfo> apiInfo_;
    LoginController *loginController_;
    enum LOGIN_STATE { LOGIN_NONE, LOGIN_IN_PROGRESS, LOGIN_FINISHED};
    LOGIN_STATE loginState_;
    FirewallExceptions firewallExceptions_;

    LoginSettings loginSettings_;
    QMutex loginSettingsMutex_;

    QTimer *checkUpdateTimer_;
    SessionStatusTimer *updateSessionStatusTimer_;
    QTimer *notificationsUpdateTimer_;
    QTimer *fetchWireguardConfigTimer_;

    locationsmodel::LocationsModel *locationsModel_;

    RefetchServerCredentialsHelper *refetchServerCredentialsHelper_;

    DownloadHelper *downloadHelper_;
#ifdef Q_OS_MAC
    AutoUpdaterHelper_mac *autoUpdaterHelper_;
#endif

    QMutex mutex_;

    apiinfo::SessionStatus prevSessionStatus_;

    std::atomic<bool> isBlockConnect_;
    std::atomic<bool> isCleanupFinished_;

    LocationID locationId_;
    QString locationName_;

    QString lastConnectingHostname_;
    ProtoTypes::Protocol lastConnectingProtocol_;

    bool isNeedReconnectAfterRequestUsernameAndPassword_;

    bool online_;

    ProtoTypes::PacketSize packetSize_;
    QThread *packetSizeControllerThread_;
    bool runningPacketDetection_;

    enum {UPDATE_SESSION_STATUS_PERIOD = 60 * 1000}; // 1 min
    enum {CHECK_UPDATE_PERIOD = 24 * 60 * 60 * 1000}; // 24 hours
    enum {NOTIFICATIONS_UPDATE_PERIOD = 60 * 60 * 1000}; // 1 hour

    void startLoginController(const LoginSettings &loginSettings, bool bFromConnectedState);
    void updateSessionStatus();
    void updateServerLocations();
    void updateFirewallSettings();

    void addCustomRemoteIpToFirewallIfNeed();
    void doConnect(bool bEmitAuthError);
    LocationID checkLocationIdExistingAndReturnNewIfNeed(const LocationID &locationId);
    void doDisconnectRestoreStuff();

    uint lastDownloadProgress_;
    QString installerUrl_;
    qint32 guiWindowHandle_;
};

#endif // ENGINE_H
