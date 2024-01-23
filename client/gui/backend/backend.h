#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QProcess>
#include <QTimer>

#include "connectstatehelper.h"
#include "firewallstatehelper.h"
#include "locations/locationsmodel_manager.h"
#include "preferences/accountinfo.h"
#include "preferences/preferences.h"
#include "preferences/preferenceshelper.h"
#include "types/checkupdate.h"
#include "types/locationid.h"
#include "types/notification.h"
#include "types/protocolstatus.h"
#include "types/proxysharinginfo.h"
#include "types/robertfilter.h"
#include "types/sessionstatus.h"
#include "types/splittunneling.h"
#include "types/upgrademodetype.h"
#include "types/wifisharinginfo.h"

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
    bool isLastLoginWithAuthHash() const;
    void signOut(bool keepFirewallOn);
    void sendConnect(const LocationID &lid, const types::ConnectionSettings &connectionSettings = types::ConnectionSettings(types::Protocol(), 0, true));
    void sendDisconnect();
    bool isDisconnected() const;
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
    void startProxySharing(PROXY_SHARING_TYPE proxySharingMode);
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
    void setRobertFilter(const types::RobertFilter &filter);
    void syncRobert();

    bool isAppCanClose() const;

    void continueWithCredentialsForOvpnConfig(const QString &username, const QString &password, bool bSave);

    void sendAdvancedParametersChanged();

    gui_locations::LocationsModelManager *locationsModelManager();

    PreferencesHelper *getPreferencesHelper();
    Preferences *getPreferences();
    AccountInfo *getAccountInfo();

    void applicationActivated();
    void applicationDeactivated();

    const types::SessionStatus &getSessionStatus() const;

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

private slots:
    void onEngineSettingsChangedInPreferences();

    void onEngineCleanupFinished();
    void onEngineInitFinished(ENGINE_INIT_RET_CODE retCode, bool isCanLoginWithAuthHash, const types::EngineSettings &engineSettings);
    void onEngineBfeEnableFinished(ENGINE_INIT_RET_CODE retCode, bool isCanLoginWithAuthHash, const types::EngineSettings &engineSettings);
    void onEngineFirewallStateChanged(bool isEnabled);
    void onEngineLoginFinished(bool isLoginFromSavedSettings, const QString &authHash, const types::PortMap &portMap);
    void onEngineLoginError(LOGIN_RET retCode, const QString &errorMessage);
    void onEngineTryingBackupEndpoint(int num, int cnt);
    void onEngineSessionDeleted();
    void onEngineUpdateSessionStatus(const types::SessionStatus &sessionStatus);
    void onEngineNotificationsUpdated(const QVector<types::Notification> &notifications);
    void onEngineCheckUpdateUpdated(const types::CheckUpdate &checkUpdate);
    void onEngineUpdateVersionChanged(uint progressPercent, const UPDATE_VERSION_STATE &state, const UPDATE_VERSION_ERROR &error);
    void onEngineMyIpUpdated(const QString &ip, bool isDisconnected);
    void onEngineConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, CONNECT_ERROR err, const LocationID &locationId);
    void onEngineStatisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes);
    void onEngineProtocolPortChanged(const types::Protocol &protocol, const uint port);
    void onEngineRobertFiltersUpdated(bool success, const QVector<types::RobertFilter> &filters);
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
    void onEngineSignOutFinished();

    void onEngineGotoCustomOvpnConfigModeFinished();

    void onEngineNetworkChanged(types::NetworkInterface networkInterface);

    void onEngineDetectionCpuUsageAfterConnected(QStringList list);

    void onEngineRequestUsername();
    void onEngineRequestPassword();

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
    void loginError(LOGIN_RET loginError, const QString &errorMessage);

    void signOutFinished();

    void myIpChanged(QString ip, bool isFromDisconnectedState);
    void connectStateChanged(const types::ConnectState &connectState);
    void emergencyConnectStateChanged(const types::ConnectState &connectState);
    void firewallStateChanged(bool isEnabled);
    void notificationsChanged(const QVector<types::Notification> &arr);
    void robertFiltersChanged(bool success, const QVector<types::RobertFilter> &arr);
    void setRobertFilterResult(bool success);
    void syncRobertResult(bool success);
    void networkChanged(types::NetworkInterface interface);
    void sessionStatusChanged(const types::SessionStatus &sessionStatus);
    void checkUpdateChanged(const types::CheckUpdate &checkUpdateInfo);
    void splitTunnelingStateChanged(bool isActive);

    void confirmEmailResult(bool bSuccess);
    void debugLogResult(bool bSuccess);
    void statisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes);

    void proxySharingInfoChanged(const types::ProxySharingInfo &psi);
    void wifiSharingInfoChanged(const types::WifiSharingInfo &wsi);
    void webSessionTokenForManageAccount(const QString &temp_session_token);
    void webSessionTokenForAddEmail(const QString &temp_session_token);
    void webSessionTokenForManageRobertRules(const QString &temp_session_token);

    void requestCustomOvpnConfigCredentials();

    void sessionDeleted();
    void testTunnelResult(bool success);
    void lostConnectionToHelper();
    void highCpuUsage(const QStringList &processesList);
    void userWarning(USER_WARNING_TYPE userWarningType);
    void internetConnectivityChanged(bool connectivity);
    void protocolPortChanged(const types::Protocol &protocol, const uint port);
    void packetSizeDetectionStateChanged(bool on, bool isError);
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

    types::SessionStatus latestSessionStatus_;
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
};
