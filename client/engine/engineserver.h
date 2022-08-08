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

public slots:
    void run();

signals:
    void finished();
    void emitCommand(IPC::Command *command);
    void wireGuardAtKeyLimit();
    void wireGuardKeyLimitUserResponse(bool deleteOldestKey);

private slots:
    void onServerCallbackAcceptFunction(IPC::IConnection *connection);
    void onConnectionStateCallback(int state, IPC::IConnection *connection);

    void onEngineCleanupFinished();
    void onEngineInitFinished(ENGINE_INIT_RET_CODE retCode);
    void onEngineBfeEnableFinished(ENGINE_INIT_RET_CODE retCode);
    void onEngineFirewallStateChanged(bool isEnabled);
    void onEngineLoginFinished(bool isLoginFromSavedSettings, const QString &authHash, const types::PortMap &portMap);
    void onEngineLoginError(LOGIN_RET retCode, const QString &errorMessage);
    void onEngineLoginMessage(LOGIN_MESSAGE msg);
    void onEngineSessionDeleted();
    void onEngineUpdateSessionStatus(const types::SessionStatus &sessionStatus);
    void onEngineNotificationsUpdated(const QVector<types::Notification> &notifications);
    void onEngineCheckUpdateUpdated(const types::CheckUpdate &checkUpdate);
    void onEngineUpdateVersionChanged(uint progressPercent, const UPDATE_VERSION_STATE &state, const UPDATE_VERSION_ERROR &error);
    void onEngineMyIpUpdated(const QString &ip, bool success, bool isDisconnected);
    void onEngineConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, CONNECT_ERROR err, const LocationID &locationId);
    void onEngineStatisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes);
    void onEngineProtocolPortChanged(const PROTOCOL &protocol, const uint port);

    void onEngineEmergencyConnected();
    void onEngineEmergencyDisconnected();
    void onEngineEmergencyConnectError(CONNECT_ERROR err);

    void onEngineTestTunnelResult(bool bSuccess);
    void onEngineLostConnectionToHelper();
    void onEngineProxySharingStateChanged(bool bEnabled, PROXY_SHARING_TYPE proxySharingType);
    void onEngineWifiSharingStateChanged(bool bEnabled, const QString &ssid);
    void onEngineConnectedWifiUsersCountChanged(int usersCount);
    void onEngineConnectedProxyUsersCountChanged(int usersCount);
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

    void onEngineLocationsModelItemsUpdated(const LocationID &bestLocation, const QString &staticIpDeviceName, QSharedPointer< QVector<types::LocationItem> > items);
    void onEngineLocationsModelBestLocationUpdated(const LocationID &bestLocation);
    void onEngineLocationsModelCustomConfigItemsUpdated(QSharedPointer< QVector<types::LocationItem> > items);
    void onEngineLocationsModelPingChangedChanged(const LocationID &id, PingTime timeMs);

    void onMacAddrSpoofingChanged(const types::MacAddrSpoofing &macAddrSpoofing);
    void onEngineSendUserWarning(USER_WARNING_TYPE userWarningType);
    void onEnginePacketSizeChanged(bool isAuto, int mtu);
    void onEnginePacketSizeDetectionStateChanged(bool on, bool isError);

    void onHostsFileBecameWritable();

private:
    IPC::IServer *server_;

    Engine *engine_;
    QThread *threadEngine_;
    types::EngineSettings curEngineSettings_;

    bool bClientAuthReceived_;
    QHash<IPC::IConnection *, ClientConnectionDescr> connections_;

    //void serverCallbackAcceptFunction(IPC::IConnection *connection);
    bool handleCommand(IPC::Command *command);
    void sendEngineInitReturnCode(ENGINE_INIT_RET_CODE retCode);
    void sendConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, CONNECT_ERROR err, const LocationID &locationId);

    void sendFirewallStateChanged(bool isEnabled);
};

#endif // ENGINESERVER_H
