#include "localipcserver.h"
#include "utils/utils.h"
#include "utils/logger.h"
#include "ipc/server.h"
#include "ipc/protobufcommand.h"
#include "backend/persistentstate.h"

LocalIPCServer::LocalIPCServer(Backend *backend, QObject *parent) : QObject(parent)
  , backend_(backend)
  , server_(NULL)

{
    connect(backend_, &Backend::connectStateChanged, this, &LocalIPCServer::onBackendConnectStateChanged);
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
    else if (command->getStringId() == CliIpc::Connect::descriptor()->full_name())
    {
        IPC::ProtobufCommand<CliIpc::Connect> *cmd = static_cast<IPC::ProtobufCommand<CliIpc::Connect> *>(command);
        QString locationStr = QString::fromStdString(cmd->getProtoObj().location());
        LocationID lid;
        if (locationStr.isEmpty())
        {
            lid = PersistentState::instance().lastLocation();
        }
        else if (locationStr == "best")
        {
            lid = backend_->getLocationsModel()->getBestLocationId();
        }
        else
        {
            lid = backend_->getLocationsModel()->findLocationByFilter(locationStr);
            /*if (lid.isValid())
            {
                qCDebug(LOG_BASIC) << "Connecting to" << lid.getHashString();
                backend_->sendConnect(lid);
            }
            else
            {
                emit finished(tr("Error: Could not find server matching: \"") + locationStr_ + "\"");
            }*/
        }

        for (IPC::IConnection * connection : connections_)
        {
            IPC::ProtobufCommand<CliIpc::ConnectToLocationAnswer> cmd;
            if (lid.isValid())
            {
                cmd.getProtoObj().set_is_success(true);
                cmd.getProtoObj().set_location(lid.city().toStdString());
            }
            else
            {
                cmd.getProtoObj().set_is_success(false);
            }
            connection->sendCommand(cmd);
        }
        if (lid.isValid())
        {
            emit connectToLocation(lid);
        }
    }
    else if (command->getStringId() == CliIpc::Disconnect::descriptor()->full_name())
    {
        if (backend_->isDisconnected())
        {
            for (IPC::IConnection * connection : connections_)
            {
                IPC::ProtobufCommand<CliIpc::AlreadyDisconnected> cmd;
                connection->sendCommand(cmd);
            }
        }
        else
        {
            backend_->sendDisconnect();
        }
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

void LocalIPCServer::onBackendConnectStateChanged(const ProtoTypes::ConnectState &connectState)
{
    for (IPC::IConnection * connection : connections_)
    {
        IPC::ProtobufCommand<CliIpc::ConnectStateChanged> cmd;
        *cmd.getProtoObj().mutable_connect_state() = connectState;
        connection->sendCommand(cmd);
    }
}
