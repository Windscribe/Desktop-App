#pragma once

#include <QVector>
#include "api_responses/checkupdate.h"
#include "backend/backend.h"
#include "ipc/clicommands.h"
#include "ipc/server.h"
#include "ipc/connection.h"
#include "types/connectstate.h"
#include "types/enums.h"
#include "types/protocol.h"

// Local server for receive and execute commands from local processes (currently only from the CLI).
class LocalIPCServer : public QObject
{
    Q_OBJECT
public:
    explicit LocalIPCServer(Backend *backend, QObject *parent = nullptr);
    ~LocalIPCServer();

    void start();
    void sendLocations(const QStringList &locations);
    void setDisconnectedByKeyLimit();

signals:
    void showLocations(IPC::CliCommands::LocationType type);
    void connectToLocation(const LocationID &id, const types::Protocol &protocol);
    void connectToStaticIpLocation(const QString &location, const types::Protocol &protocol);
    void attemptLogin(const QString &username, const QString &password, const QString &code2fa);
    void setKeyLimitBehavior(bool deleteKey);
    void update();

private slots:
    void onServerCallbackAcceptFunction(IPC::Connection *connection);
    void onConnectionCommandCallback(IPC::Command *command, IPC::Connection *connection);
    void onConnectionStateCallback(int state, IPC::Connection *connection);

    void onBackendCheckUpdateChanged(const api_responses::CheckUpdate &checkUpdate);
    void onBackendConnectStateChanged(const types::ConnectState &state);
    void onBackendInternetConnectivityChanged(bool connectivity);
    void onBackendLoginFinished(bool isLoginFromSavedSettings);
    void onBackendLoginError(wsnet::LoginResult code, const QString &msg);
    void onBackendLogoutFinished();
    void onBackendProtocolPortChanged(const types::Protocol &protocol, uint port);
    void onBackendTestTunnelResult(bool success);
    void onBackendUpdateDownloaded(const QString &path);
    void onBackendUpdateVersionChanged(uint progressPercent, UPDATE_VERSION_STATE state, UPDATE_VERSION_ERROR error);
    void onBackendConnectionIdChanged(const QString &connId);

private:
    Backend *backend_;
    IPC::Server *server_ = nullptr;
    QVector<IPC::Connection *> connections_;

    bool connectivity_;
    LOGIN_STATE loginState_;
    UPDATE_VERSION_STATE updateState_;
    UPDATE_VERSION_ERROR updateError_;
    uint updateProgress_;
    QString updatePath_;
    QString updateAvailable_;
    wsnet::LoginResult lastLoginError_;
    QString lastLoginErrorMessage_;
    types::ConnectState connectState_;
    types::Protocol protocol_;
    uint port_;
    TUNNEL_TEST_STATE tunnelTestState_;
    bool disconnectedByKeyLimit_;
    QString connectId_;
    bool tunnelTestSuccess_;

    void sendState();
    void sendCommand(const IPC::Command &command);
};
