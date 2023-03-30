#ifndef BACKEND_H
#define BACKEND_H

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QProcess>
#include "ipc/iconnection.h"
#include "ipc/command.h"
#include "ipc/servercommands.h"
#include "types/locationid.h"
#include "types/upgrademodetype.h"
#include "locations/locationsmodel_manager.h"
#include "preferences/preferences.h"
#include "preferences/preferenceshelper.h"
#include "preferences/accountinfo.h"
#include "connectstatehelper.h"
#include "firewallstatehelper.h"
#include "types/sessionstatus.h"
#include "types/proxysharinginfo.h"
#include "types/wifisharinginfo.h"
#include "types/splittunneling.h"
//#include "engine/engineserver.h"

class EngineServer;

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
    void sendEngineSettingsIfChanged();

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

private slots:
    void onConnectionNewCommand(IPC::Command *command);

signals:
    // emited when connected to engine and received the engine settings, or error in initState variable
    void initFinished(INIT_STATE initState);
    void initTooLong();
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
    types::EngineSettings latestEngineSettings_;
    ConnectStateHelper connectStateHelper_;
    ConnectStateHelper emergencyConnectStateHelper_;
    FirewallStateHelper firewallStateHelper_;

    EngineServer *engineServer_;
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
    void getOpenVpnVersionsFromInitCommand(const IPC::ServerCommands::InitFinished &state);
};

#endif // BACKEND_H
