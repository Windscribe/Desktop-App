#pragma once

#include <QObject>
#include <wsnet/WSNet.h>

#include "connectstatehelper.h"
#include "firewallstatehelper.h"
#include "locations/locationsmodel_manager.h"
#include "preferences/accountinfo.h"
#include "preferences/preferences.h"
#include "preferences/preferenceshelper.h"
#include "types/locationid.h"
#include "types/protocolstatus.h"
#include "types/proxysharinginfo.h"
#include "types/splittunneling.h"
#include "types/upgrademodetype.h"
#include "types/wifisharinginfo.h"
#include "api_responses/robertfilter.h"
#include "api_responses/checkupdate.h"
#include "api_responses/sessionstatus.h"
#include "api_responses/notification.h"

class Engine;

class Backend : public QObject
{
    Q_OBJECT
public:
    explicit Backend(QObject *parent);
    virtual ~Backend();

    void init();
    void cleanup(bool isExitWithRestart, bool isFirewallChecked, bool isFirewallAlwaysOn, bool isLaunchOnStart);
    void enableBFE_win();
    void login(const QString &username, const QString &password, const QString &code2fa);
    bool isCanLoginWithAuthHash() const;
    bool isSavedApiSettingsExists() const;
    void loginWithAuthHash();
    void loginWithLastLoginSettings();
    void setLoginError(wsnet::LoginResult err, const QString &msg);
    bool isLastLoginWithAuthHash() const;
    void logout(bool keepFirewallOn);
    void sendConnect(const LocationID &lid, const types::ConnectionSettings &connectionSettings = types::ConnectionSettings(types::Protocol(), 0, true));
    void sendDisconnect();
    bool isDisconnected() const;
    LOGIN_STATE currentLoginState() const;
    wsnet::LoginResult lastLoginError() const;
    CONNECT_STATE currentConnectState() const;
    LocationID currentLocation() const;

    void firewallOn(bool updateHelperFirst = true);
    void firewallOff(bool updateHelperFirst = true);
    bool isFirewallEnabled();
    bool isFirewallAlwaysOn();

    void emergencyConnectClick();
    void emergencyDisconnectClick();
    bool isEmergencyDisconnected();

    void startWifiSharing(const QString &ssid, const QString &password);
    void stopWifiSharing();
    void startProxySharing(PROXY_SHARING_TYPE proxySharingMode, uint port);
    void stopProxySharing();

    void setIPv6StateInOS(bool bEnabled);
    void getAndUpdateIPv6StateInOS();

    void gotoCustomOvpnConfigMode();

    void recordInstall();
    void sendConfirmEmail();
    void sendDebugLog();
    void getWebSessionTokenForManageAccount();
    void getWebSessionTokenForAddEmail();
    void getWebSessionTokenForManageRobertRules();

    void speedRating(int rating, const QString &localExternalIp);

    void setBlockConnect(bool isBlockConnect);

    void getRobertFilters();
    void setRobertFilter(const api_responses::RobertFilter &filter);
    void syncRobert();

    bool isAppCanClose() const;

    void continueWithCredentialsForOvpnConfig(const QString &username, const QString &password, bool bSave);
    void continueWithPrivKeyPasswordForOvpnConfig(const QString &password, bool bSave);

    void sendAdvancedParametersChanged();

    gui_locations::LocationsModelManager *locationsModelManager();

    PreferencesHelper *getPreferencesHelper();
    Preferences *getPreferences();
    AccountInfo *getAccountInfo();

    void applicationActivated();
    void applicationDeactivated();

    const api_responses::SessionStatus &getSessionStatus() const;

    void handleNetworkChange(types::NetworkInterface networkInterface, bool manual=false);
    types::NetworkInterface getCurrentNetworkInterface();

    virtual void cycleMacAddress();
    void sendDetectPacketSize();
    void sendSplitTunneling(const types::SplitTunneling &st);

    void abortInitialization();

    void sendUpdateWindowInfo(qint32 mainWindowCenterX, qint32 mainWindowCenterY);
    void sendUpdateVersion(qint64 mainWindowHandle);
    void cancelUpdateVersion();

    void sendMakeHostsFilesWritableWin();

    bool haveAutoLoginCredentials(QString &username, QString &password);

    void updateCurrentNetworkInterface();

private slots:
    void onEngineSettingsChangedInPreferences();

    void onEngineCleanupFinished();
    void onEngineInitFinished(ENGINE_INIT_RET_CODE retCode, bool isCanLoginWithAuthHash, const types::EngineSettings &engineSettings);
    void onEngineBfeEnableFinished(ENGINE_INIT_RET_CODE retCode, bool isCanLoginWithAuthHash, const types::EngineSettings &engineSettings);
    void onEngineFirewallStateChanged(bool isEnabled);
    void onEngineLoginFinished(bool isLoginFromSavedSettings, const api_responses::PortMap &portMap);
    void onEngineLoginError(wsnet::LoginResult retCode, const QString &errorMessage);
    void onEngineTryingBackupEndpoint(int num, int cnt);
    void onEngineSessionDeleted();
    void onEngineUpdateSessionStatus(const api_responses::SessionStatus &sessionStatus);
    void onEngineNotificationsUpdated(const QVector<api_responses::Notification> &notifications);
    void onEngineCheckUpdateUpdated(const api_responses::CheckUpdate &checkUpdate);
    void onEngineUpdateDownloaded(const QString &path);
    void onEngineUpdateVersionChanged(uint progressPercent, const UPDATE_VERSION_STATE &state, const UPDATE_VERSION_ERROR &error);
    void onEngineMyIpUpdated(const QString &ip, bool isDisconnected);
    void onEngineConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, CONNECT_ERROR err, const LocationID &locationId);
    void onEngineStatisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes);
    void onEngineProtocolPortChanged(const types::Protocol &protocol, const uint port);
    void onEngineRobertFiltersUpdated(bool success, const QVector<api_responses::RobertFilter> &filters);
    void onEngineSetRobertFilterFinished(bool success);
    void onEngineSyncRobertFinished(bool success);
    void onEngineProtocolStatusChanged(const QVector<types::ProtocolStatus> &status);

    void onEngineEmergencyConnected();
    void onEngineEmergencyDisconnected();
    void onEngineEmergencyConnectError(CONNECT_ERROR err);

    void onEngineTestTunnelResult(bool bSuccess);
    void onEngineLostConnectionToHelper();
    void onEngineProxySharingStateChanged(bool bEnabled, PROXY_SHARING_TYPE proxySharingType, const QString &address, int usersCount);
    void onEngineWifiSharingStateChanged(bool bEnabled, const QString &ssid, int usersCount);
    void onEngineLogoutFinished();

    void onEngineGotoCustomOvpnConfigModeFinished();

    void onEngineNetworkChanged(types::NetworkInterface networkInterface);

    void onEngineDetectionCpuUsageAfterConnected(QStringList list);

    void onEngineRequestUsernameAndPassword(const QString &username);
    void onEngineRequestPrivKeyPassword();

    void onEngineInternetConnectivityChanged(bool connectivity);

    void onEngineSendDebugLogFinished(bool bSuccess);
    void onEngineConfirmEmailFinished(bool bSuccess);
    void onEngineWebSessionToken(WEB_SESSION_PURPOSE purpose, const QString &token);

    void onEngineLocationsModelItemsUpdated(const LocationID &bestLocation, const QString &staticIpDeviceName, QSharedPointer< QVector<types::Location> > items);
    void onEngineLocationsModelBestLocationUpdated(const LocationID &bestLocation);
    void onEngineLocationsModelCustomConfigItemsUpdated(QSharedPointer<types::Location> item);
    void onEngineLocationsModelPingChangedChanged(const LocationID &id, PingTime timeMs);

    void onEngineMacAddrSpoofingChanged(const types::EngineSettings &engineSettings);
    void onEngineSendUserWarning(USER_WARNING_TYPE userWarningType);
    void onEnginePacketSizeChanged(const types::EngineSettings &engineSettings);
    void onEnginePacketSizeDetectionStateChanged(bool on, bool isError);
    void onEngineHostsFileBecameWritable();
    void onEngineAutoEnableAntiCensorship();

signals:
    // emited when connected to engine and received the engine settings, or error in initState variable
    void initFinished(INIT_STATE initState);
    void cleanupFinished();

    void gotoCustomOvpnConfigModeFinished();

    void loginFinished(bool isLoginFromSavedSettings);
    void tryingBackupEndpoint(int num, int cnt);
    void loginError(wsnet::LoginResult loginError, const QString &errorMessage);

    void logoutFinished();

    void myIpChanged(QString ip, bool isFromDisconnectedState);
    void connectStateChanged(const types::ConnectState &connectState);
    void emergencyConnectStateChanged(const types::ConnectState &connectState);
    void firewallStateChanged(bool isEnabled);
    void notificationsChanged(const QVector<api_responses::Notification> &arr);
    void robertFiltersChanged(bool success, const QVector<api_responses::RobertFilter> &arr);
    void setRobertFilterResult(bool success);
    void syncRobertResult(bool success);
    void networkChanged(types::NetworkInterface interface);
    void sessionStatusChanged(const api_responses::SessionStatus &sessionStatus);
    void checkUpdateChanged(const api_responses::CheckUpdate &checkUpdateInfo);
    void splitTunnelingStateChanged(bool isActive);

    void confirmEmailResult(bool bSuccess);
    void debugLogResult(bool bSuccess);
    void statisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes);

    void proxySharingInfoChanged(const types::ProxySharingInfo &psi);
    void wifiSharingInfoChanged(const types::WifiSharingInfo &wsi);
    void wifiSharingFailed(WIFI_SHARING_ERROR error);

    void webSessionTokenForManageAccount(const QString &temp_session_token);
    void webSessionTokenForAddEmail(const QString &temp_session_token);
    void webSessionTokenForManageRobertRules(const QString &temp_session_token);

    void requestCustomOvpnConfigCredentials(const QString &username);
    void requestCustomOvpnConfigPrivKeyPassword();

    void sessionDeleted();
    void testTunnelResult(bool success);
    void lostConnectionToHelper();
    void highCpuUsage(const QStringList &processesList);
    void userWarning(USER_WARNING_TYPE userWarningType);
    void internetConnectivityChanged(bool connectivity);
    void protocolPortChanged(const types::Protocol &protocol, const uint port);
    void packetSizeDetectionStateChanged(bool on, bool isError);
    void updateDownloaded(const QString &path);
    void updateVersionChanged(uint progressPercent, UPDATE_VERSION_STATE state, UPDATE_VERSION_ERROR error);

    void engineCrash();
    void engineRecoveryFailed();

    void wireGuardAtKeyLimit();
    void wireGuardKeyLimitUserResponse(bool deleteOldestKey);
    void protocolStatusChanged(const QVector<types::ProtocolStatus> &status);

    void helperSplitTunnelingStartFailed();

private:
    bool isSavedApiSettingsExists_;

    bool bLastLoginWithAuthHash_;
    QString lastUsername_;
    QString lastPassword_;
    QString lastCode2fa_;

    api_responses::SessionStatus latestSessionStatus_;
    ConnectStateHelper connectStateHelper_;
    ConnectStateHelper emergencyConnectStateHelper_;
    FirewallStateHelper firewallStateHelper_;

    QThread *threadEngine_;
    Engine *engine_;
    bool isCleanupFinished_;

    quint32 cmdId_;

    gui_locations::LocationsModelManager *locationsModelManager_;

    bool isCanLoginWithAuthHash_;
    bool isFirewallEnabled_;

    PreferencesHelper preferencesHelper_;
    Preferences preferences_;
    AccountInfo accountInfo_;

    bool isExternalConfigMode_;
    UpgradeModeType upgradeMode_;

    types::NetworkInterface currentNetworkInterface_;

    QString generateNewFriendlyName();
    void updateAccountInfo();

    QString getAutoLoginCredential(const QString &key);
    void clearAutoLoginCredentials();

    LOGIN_STATE loginState_;
    wsnet::LoginResult lastLoginError_;
};
