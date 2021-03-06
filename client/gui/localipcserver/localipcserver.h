#ifndef LOCALIPCSERVER_H
#define LOCALIPCSERVER_H

#include <QVector>
#include "ipc/iserver.h"
#include "ipc/iconnection.h"
#include "backend/backend.h"

// Local server for receive and execute commands from local processes (currently only from the CLI).
class LocalIPCServer : public QObject
{
    Q_OBJECT
public:
    explicit LocalIPCServer(Backend *backend, QObject *parent = nullptr);
    ~LocalIPCServer();

    void start();
    void sendLocationsShown();

signals:
    void showLocations();
    void connectToLocation(const LocationID &id);

private slots:
    void onServerCallbackAcceptFunction(IPC::IConnection *connection);
    void onConnectionCommandCallback(IPC::Command *command, IPC::IConnection *connection);
    void onConnectionStateCallback(int state, IPC::IConnection *connection);

    void onBackendConnectStateChanged(const ProtoTypes::ConnectState &connectState);
    void onBackendFirewallStateChanged(bool isEnabled);
    void onBackendLoginFinished(bool isLoginFromSavedSettings);
    void onBackendSignOutFinished();

private:
    Backend *backend_;
    IPC::IServer *server_;
    QVector<IPC::IConnection *> connections_;
    bool isLoggedIn_;


    void sendCommand(const IPC::Command &command);
};

#endif // LOCALIPCSERVER_H
