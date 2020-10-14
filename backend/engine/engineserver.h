#ifndef ENGINESERVER_H
#define ENGINESERVER_H

#include <QObject>
#include <QHash>
#include "ipc/iserver.h"
#include "ipc/generated_proto/types.pb.h"
#include "clientconnectiondescr.h"
#include "engine/engine.h"

class EngineServer : public QObject
{
    Q_OBJECT
public:
    explicit EngineServer(QObject *parent = nullptr);
    virtual ~EngineServer();

    void sendCmdToAllAuthorizedAndGetStateClients(const IPC::Command &cmd, bool bWithLog);
    void sendCmdToAllAuthorizedAndGetStateClientsOfType(const IPC::Command &cmd, bool bWithLog, unsigned int clientId);

public slots:
    void run();

signals:
    void finished();

private slots:
    void onServerCallbackAcceptFunction(IPC::IConnection *connection);
    void onConnectionCommandCallback(IPC::Command *command, IPC::IConnection *connection);
    void onConnectionStateCallback(int state, IPC::IConnection *connection);

    void onEngineCleanupFinished();
    void onEngineInitFinished(ENGINE_INIT_RET_CODE retCode);
    void onEngineBfeEnableFinished(ENGINE_INIT_RET_CODE retCode);
    void onEngineFirewallStateChanged(bool isEnabled);
    void onEngineLoginFinished(bool isLoginFromSavedSettings);
    void onEngineLoginError(LOGIN_RET retCode);
    void onEngineLoginMessage(LOGIN_MESSAGE msg);
    void onEngineSessionDeleted();
    void onEngineUpdateSessionStatus(QSharedPointer<SessionStatus> sessionStatus);
    void onEngineNotificationsUpdated(QSharedPointer<ApiNotifications> notifications);
    void onEngineCheckUpdateUpdated(bool available, const QString &version, const ProtoTypes::UpdateChannel updateChannel, int latestBuild, const QString &url, bool supported);
    void onEngineUpdateVersionChanged(uint progressPercent, const ProtoTypes::UpdateVersionState &state, const ProtoTypes::UpdateVersionError &error);
    void onEngineMyIpUpdated(const QString &ip, bool success, bool isDisconnected);
    void onEngineConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, CONNECTION_ERROR err, const LocationID &locationId);
    void onEngineStatisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes);
    void onEngineProtocolPortChanged(const ProtoTypes::Protocol &protocol, const uint port);

    void onEngineEmergencyConnected();
    void onEngineEmergencyDisconnected();
    void onEngineEmergencyConnectError(CONNECTION_ERROR err);

    void onEngineTestTunnelResult(bool bSuccess);
    void onEngineLostConnectionToHelper();
    void onEngineProxySharingStateChanged(bool bEnabled, PROXY_SHARING_TYPE proxySharingType);
    void onEngineWifiSharingStateChanged(bool bEnabled, const QString &ssid);
    void onEngineConnectedWifiUsersCountChanged(int usersCount);
    void onEngineConnectedProxyUsersCountChanged(int usersCount);
    void onEngineSignOutFinished();

    void onEngineGotoCustomOvpnConfigModeFinished();

    void onEngineNetworkChanged(ProtoTypes::NetworkInterface networkInterface);

    void onEngineDetectionCpuUsageAfterConnected(QStringList list);

    void onEngineRequestUsername();
    void onEngineRequestPassword();

    void onEngineInternetConnectivityChanged(bool connectivity);

    void onEngineSendDebugLogFinished(bool bSuccess);
    void onEngineConfirmEmailFinished(bool bSuccess);

    void onEngineServersModelItemsUpdated(QSharedPointer<QVector<ModelExchangeLocationItem> > items);
    void onEngineServersModelConnectionSpeedChanged(LocationID id, PingTime timeMs);

    void onMacAddrSpoofingChanged(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing);
    void onEngineSendUserWarning(ProtoTypes::UserWarningType userWarningType);
    void onEnginePacketSizeChanged(bool isAuto, int mss);
    void onEnginePacketSizeDetectionStateChanged(bool on);

private:
    IPC::IServer *server_;

    Engine *engine_;
    QThread *threadEngine_;
    EngineSettings curEngineSettings_;

    QHash<IPC::IConnection *, ClientConnectionDescr> connections_;

    //void serverCallbackAcceptFunction(IPC::IConnection *connection);
    bool handleCommand(IPC::Command *command);
    void sendEngineInitReturnCode(ENGINE_INIT_RET_CODE retCode);
    void sendConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, CONNECTION_ERROR err, const LocationID &locationId);

    void sendFirewallStateChanged(bool isEnabled);
};

#endif // ENGINESERVER_H
