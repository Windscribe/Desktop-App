#ifndef ENGINESERVER_H
#define ENGINESERVER_H

#include <QObject>
#include <QHash>
#include "ipc/iserver.h"
#include "clientconnectiondescr.h"
#include "engine/engine.h"

class EngineServer : public QObject
{
    Q_OBJECT
public:
    explicit EngineServer(QObject *parent = nullptr);
    virtual ~EngineServer();

    void sendCommand(IPC::Command *command);
    void sendCmdToAllAuthorizedAndGetStateClients(IPC::Command *cmd, bool bWithLog);
    //void sendCmdToAllAuthorizedAndGetStateClientsOfType(const IPC::Command &cmd, bool bWithLog, unsigned int clientId, bool* bLogged = nullptr);

signals:
    void finished();
    void emitCommand(IPC::Command *command);
    void wireGuardAtKeyLimit();
    void wireGuardKeyLimitUserResponse(bool deleteOldestKey);
    void helperSplitTunnelingStartFailed();

private slots:
    void onServerCallbackAcceptFunction(IPC::IConnection *connection);
    void onConnectionStateCallback(int state, IPC::IConnection *connection);

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

    void onMacAddrSpoofingChanged(const types::EngineSettings &engineSettings);
    void onEngineSendUserWarning(USER_WARNING_TYPE userWarningType);
    void onEnginePacketSizeChanged(const types::EngineSettings &engineSettings);
    void onEnginePacketSizeDetectionStateChanged(bool on, bool isError);

    void onHostsFileBecameWritable();

private:
    IPC::IServer *server_;

    Engine *engine_;
    QThread *threadEngine_;

    bool bClientAuthReceived_;
    QHash<IPC::IConnection *, ClientConnectionDescr> connections_;

    //void serverCallbackAcceptFunction(IPC::IConnection *connection);
    bool handleCommand(IPC::Command *command);
    void sendEngineInitReturnCode(ENGINE_INIT_RET_CODE retCode, bool isCanLoginWithAuthHash, const types::EngineSettings &engineSettings);
    void sendConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, CONNECT_ERROR err, const LocationID &locationId);

    void sendFirewallStateChanged(bool isEnabled);
};

#endif // ENGINESERVER_H
