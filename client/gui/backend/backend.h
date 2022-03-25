#ifndef BACKEND_H
#define BACKEND_H

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QProcess>
#include "ibackend.h"
#include "ipc/iconnection.h"
#include "ipc/command.h"
#include "types/locationid.h"
#include "types/upgrademodetype.h"
#include "locationsmodel/locationsmodel.h"
#include "preferences/preferences.h"
#include "preferences/preferenceshelper.h"
#include "preferences/accountinfo.h"
#include "connectstatehelper.h"
#include "firewallstatehelper.h"
//#include "engine/engineserver.h"

class EngineServer;

class Backend : public QObject
{
    Q_OBJECT
public:
    explicit Backend(QObject *parent);
    virtual ~Backend();

    void init();
    void basicInit();
    void basicClose();
    void cleanup(bool isExitWithRestart, bool isFirewallChecked, bool isFirewallAlwaysOn, bool isLaunchOnStart);
    void enableBFE_win();
    void login(const QString &username, const QString &password, const QString &code2fa);
    bool isCanLoginWithAuthHash() const;
    bool isSavedApiSettingsExists() const;
    QString getCurrentAuthHash() const;
    void loginWithAuthHash(const QString &authHash);
    void loginWithLastLoginSettings();
    bool isLastLoginWithAuthHash() const;
    void signOut();
    void sendConnect(const LocationID &lid);
    void sendDisconnect();
    bool isDisconnected() const;
    ProtoTypes::ConnectStateType currentConnectState() const;

    void forceCliStateUpdate();

    void firewallOn(bool updateHelperFirst = true);
    void firewallOff(bool updateHelperFirst = true);
    bool isFirewallEnabled();
    bool isFirewallAlwaysOn();

    void emergencyConnectClick();
    void emergencyDisconnectClick();
    bool isEmergencyDisconnected();

    void startWifiSharing(const QString &ssid, const QString &password);
    void stopWifiSharing();
    void startProxySharing(ProtoTypes::ProxySharingMode proxySharingMode);
    void stopProxySharing();

    void setIPv6StateInOS(bool bEnabled);
    void getAndUpdateIPv6StateInOS();

    void gotoCustomOvpnConfigMode();

    void recordInstall();
    void sendConfirmEmail();
    void sendDebugLog();
    void getWebSessionTokenForEditAccountDetails();
    void getWebSessionTokenForAddEmail();

    void speedRating(int rating, const QString &localExternalIp);

    void setBlockConnect(bool isBlockConnect);
    void clearCredentials();

    bool isAppCanClose() const;

    void continueWithCredentialsForOvpnConfig(const QString &username, const QString &password, bool bSave);

    void sendAdvancedParametersChanged();
    void sendEngineSettingsIfChanged();

    LocationsModel *getLocationsModel();

    PreferencesHelper *getPreferencesHelper();
    Preferences *getPreferences();
    AccountInfo *getAccountInfo();

    void applicationActivated();
    void applicationDeactivated();

    const ProtoTypes::SessionStatus &getSessionStatus() const;

    void handleNetworkChange(ProtoTypes::NetworkInterface networkInterface);

    virtual void cycleMacAddress();
    void sendDetectPacketSize();
    void sendSplitTunneling(ProtoTypes::SplitTunneling st);

    void abortInitialization();

    void sendUpdateWindowInfo(qint32 mainWindowCenterX, qint32 mainWindowCenterY);
    void sendUpdateVersion(qint32 mainWindowHandle);
    void cancelUpdateVersion();

    void sendMakeHostsFilesWritableWin();

private slots:
    void onConnectionNewCommand(IPC::Command *command);

signals:
    // emited when connected to engine and received the engine settings, or error in initState variable
    void initFinished(ProtoTypes::InitState initState);
    void initTooLong();
    void cleanupFinished();

    void gotoCustomOvpnConfigModeFinished();

    void loginFinished(bool isLoginFromSavedSettings);
    void loginStepMessage(ProtoTypes::LoginMessage msg);
    void loginError(ProtoTypes::LoginError loginError);

    void signOutFinished();

    void myIpChanged(QString ip, bool isFromDisconnectedState);
    void connectStateChanged(const ProtoTypes::ConnectState &connectState);
    void emergencyConnectStateChanged(const ProtoTypes::ConnectState &connectState);
    void firewallStateChanged(bool isEnabled);
    void notificationsChanged(const ProtoTypes::ArrayApiNotification &arr);
    void networkChanged(ProtoTypes::NetworkInterface interface);
    void sessionStatusChanged(const ProtoTypes::SessionStatus &sessionStatus);
    void checkUpdateChanged(const ProtoTypes::CheckUpdateInfo &checkUpdateInfo);
    void locationsUpdated();
    void splitTunnelingStateChanged(bool isActive);

    void confirmEmailResult(bool bSuccess);
    void debugLogResult(bool bSuccess);
    void statisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes);

    void proxySharingInfoChanged(const ProtoTypes::ProxySharingInfo &psi);
    void wifiSharingInfoChanged(const ProtoTypes::WifiSharingInfo &wsi);
    void webSessionTokenForEditAccountDetails(const QString &temp_session_token);
    void webSessionTokenForAddEmail(const QString &temp_session_token);

    void requestCustomOvpnConfigCredentials();

    void sessionDeleted();
    void testTunnelResult(bool success);
    void lostConnectionToHelper();
    void highCpuUsage(const QStringList &processesList);
    void userWarning(ProtoTypes::UserWarningType userWarningType);
    void internetConnectivityChanged(bool connectivity);
    void protocolPortChanged(const ProtoTypes::Protocol &protocol, const uint port);
    void packetSizeDetectionStateChanged(bool on, bool isError);
    void updateVersionChanged(uint progressPercent, ProtoTypes::UpdateVersionState state, ProtoTypes::UpdateVersionError error);

    void engineCrash();
    void engineRecoveryFailed();

    void wireGuardAtKeyLimit();
    void wireGuardKeyLimitUserResponse(bool deleteOldestKey);

private:
    bool isSavedApiSettingsExists_;
    bool bLastLoginWithAuthHash_;

    ProtoTypes::SessionStatus latestSessionStatus_;
    ProtoTypes::EngineSettings latestEngineSettings_;
    ConnectStateHelper connectStateHelper_;
    ConnectStateHelper emergencyConnectStateHelper_;
    FirewallStateHelper firewallStateHelper_;

    EngineServer *engineServer_;
    bool isCleanupFinished_;

    quint32 cmdId_;

    LocationsModel *locationsModel_;

    bool isFirewallEnabled_;

    PreferencesHelper preferencesHelper_;
    Preferences preferences_;
    AccountInfo accountInfo_;

    bool isExternalConfigMode_;
    UpgradeModeType upgradeMode_;

    ProtoTypes::NetworkInterface currentNetworkInterface_;

    QString generateNewFriendlyName();
    void updateAccountInfo();
    void getOpenVpnVersionsFromInitCommand(const IPCServerCommands::InitFinished &state);
};

#endif // BACKEND_H
