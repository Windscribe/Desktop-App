#include "localipcserver.h"
#include "utils/utils.h"
#include "utils/logger.h"
#include "ipc/server.h"
#include "ipc/protobufcommand.h"

LocalIPCServer::LocalIPCServer(QObject *parent) : QObject(parent)
  , server_(NULL)

{

}

LocalIPCServer::~LocalIPCServer()
{
    for (IPC::IConnection * connection : connections_)
    {
        connection->close();
        delete connection;
    }
    connections_.clear();
    SAFE_DELETE(server_);
    qCDebug(LOG_CLI_IPC) << "IPC server for CLI stopped";
}

void LocalIPCServer::start()
{
    Q_ASSERT(server_ == NULL);
    server_ = new IPC::Server();
    connect(dynamic_cast<QObject*>(server_), SIGNAL(newConnection(IPC::IConnection *)), SLOT(onServerCallbackAcceptFunction(IPC::IConnection *)));

    if (!server_->start())
    {
        qCDebug(LOG_CLI_IPC) << "Can't start IPC server for CLI";
    }
    else
    {
        qCDebug(LOG_CLI_IPC) << "IPC server for CLI started";
    }
}

void LocalIPCServer::sendLocationsShown()
{
    for (IPC::IConnection * connection : connections_)
    {
        IPC::ProtobufCommand<CliIpc::LocationsShown> cmd;
        connection->sendCommand(cmd);
    }
}

void LocalIPCServer::onServerCallbackAcceptFunction(IPC::IConnection *connection)
{
    qCDebug(LOG_IPC) << "Client connected:" << connection;

    connections_.append(connection);

    connect(dynamic_cast<QObject*>(connection), SIGNAL(newCommand(IPC::Command *, IPC::IConnection *)), SLOT(onConnectionCommandCallback(IPC::Command *, IPC::IConnection *)));
    connect(dynamic_cast<QObject*>(connection), SIGNAL(stateChanged(int, IPC::IConnection *)), SLOT(onConnectionStateCallback(int, IPC::IConnection *)));
}

void LocalIPCServer::onConnectionCommandCallback(IPC::Command *command, IPC::IConnection *connection)
{
    if (command->getStringId() == CliIpc::ShowLocations::descriptor()->full_name())
    {
        emit showLocations();
    }
}

void LocalIPCServer::onConnectionStateCallback(int state, IPC::IConnection *connection)
{
    if (state == IPC::CONNECTION_DISCONNECTED)
    {
        qCDebug(LOG_BASIC) << "CLI disconnected from GUI server";
        connections_.removeOne(connection);
        connection->close();
        delete connection;
    }
    else if (state == IPC::CONNECTION_ERROR)
    {
        qCDebug(LOG_BASIC) << "CLI disconnected from GUI server with error";
        connections_.removeOne(connection);
        connection->close();
        delete connection;
    }

}
