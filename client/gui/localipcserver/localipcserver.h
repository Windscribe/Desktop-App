#ifndef LOCALIPCSERVER_H
#define LOCALIPCSERVER_H

#include "ipc/iserver.h"

// Local server for receive and execute commands from local processes (currently only from the CLI).
class LocalIPCServer : public QObject
{
public:
    explicit LocalIPCServer(QObject *parent = nullptr);
    ~LocalIPCServer();

    void start();

private slots:
    void onServerCallbackAcceptFunction(IPC::IConnection *connection);
    void onConnectionCommandCallback(IPC::Command *command, IPC::IConnection *connection);
    void onConnectionStateCallback(int state, IPC::IConnection *connection);

private:
    IPC::IServer *server_;
};

#endif // LOCALIPCSERVER_H
