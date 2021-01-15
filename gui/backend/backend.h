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

// need to define one of the 3 defines
#ifdef QT_DEBUG
    #define BACKEND_BUILTIN_LOCATIONS               // for build GUI without need engine (for testing GUI)
    //#define BACKEND_WITHOUT_START_ENGINE_PROCESS    // engine must be already running at startup GUI
    //#define BACKEND_START_ENGINE_PROCESS            // the default behavior of the GUI for release, GUI starts engine process
#else
    #define BACKEND_START_ENGINE_PROCESS        // always for release build
#endif


class Backend : public QObject
{
    Q_OBJECT
public:
    explicit Backend(unsigned int clientId, unsigned long clientPid, const QString &clientName, QObject *parent);
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

    void speedRating(int rating, const QString &localExternalIp);

    void setBlockConnect(bool isBlockConnect);
    void clearCredentials();

    bool isInitFinished() const;
    bool isAppCanClose() const;

    void continueWithCredentialsForOvpnConfig(const QString &username, const QString &password, bool bSave);

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

    void sendUpdateVersion();
    void cancelUpdateVersion();

private slots:
    void onProcessStarted();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

    void onConnectionNewCommand(IPC::Command *command, IPC::IConnection *connection);
    void onConnectionStateChanged(int state, IPC::IConnection *connection);
    void onConnectionConnectAttempt();

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

private:

    enum IPC_STATE { IPC_INIT_STATE, IPC_DOING_CLEANUP, IPC_STARTING_PROCESS, IPC_CONNECTING, IPC_CONNECTED,
                     IPC_INIT_SENDING, IPC_READY, IPC_FINISHED_STATE };
    IPC_STATE ipcState_;
    bool bRecoveringState_;

    unsigned int protocolVersion_;
    unsigned int clientId_;
    unsigned long clientPid_;
    QString clientName_;

    bool isSavedApiSettingsExists_;
    bool bLastLoginWithAuthHash_;

    QTimer connectionAttemptTimer_;
    QElapsedTimer connectingTimer_;
    static constexpr int TOO_LONG_CONNECTING_TIME = 10000;

    ProtoTypes::SessionStatus latestSessionStatus_;
    ProtoTypes::EngineSettings latestEngineSettings_;
    ConnectStateHelper connectStateHelper_;
    ConnectStateHelper emergencyConnectStateHelper_;
    FirewallStateHelper firewallStateHelper_;

    QProcess *process_;
    IPC::IConnection *connection_;
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
    void engineCrashedDoRecovery();
};

#endif // BACKEND_H
