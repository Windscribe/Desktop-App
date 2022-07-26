#include "localipcserver.h"
#include "utils/utils.h"
#include "utils/logger.h"
#include "ipc/server.h"
#include "ipc/protobufcommand.h"
#include "backend/persistentstate.h"

LocalIPCServer::LocalIPCServer(Backend *backend, QObject *parent) : QObject(parent)
  , backend_(backend)
{
    connect(backend_, &Backend::connectStateChanged, this, &LocalIPCServer::onBackendConnectStateChanged);
    connect(backend_, &Backend::firewallStateChanged, this, &LocalIPCServer::onBackendFirewallStateChanged);
    connect(backend_, &Backend::loginFinished, this, &LocalIPCServer::onBackendLoginFinished);
    connect(backend_, &Backend::signOutFinished, this, &LocalIPCServer::onBackendSignOutFinished);
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
    Q_ASSERT(server_ == nullptr);
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

void LocalIPCServer::onConnectionCommandCallback(IPC::Command *command, IPC::IConnection * /*connection*/)
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
        }

        IPC::ProtobufCommand<CliIpc::ConnectToLocationAnswer> cmd_send;
        if (lid.isValid())
        {
            cmd_send.getProtoObj().set_is_success(true);
            cmd_send.getProtoObj().set_location(lid.city().toStdString());
        }
        else
        {
            cmd_send.getProtoObj().set_is_success(false);
        }
        sendCommand(cmd_send);

        if (lid.isValid())
        {
            emit connectToLocation(lid);
        }
    }
    else if (command->getStringId() == CliIpc::Disconnect::descriptor()->full_name())
    {
        if (backend_->isDisconnected())
        {
            IPC::ProtobufCommand<CliIpc::AlreadyDisconnected> cmd;
            sendCommand(cmd);
        }
        else
        {
            backend_->sendDisconnect();
        }
    }
    else if (command->getStringId() == CliIpc::GetState::descriptor()->full_name())
    {
        IPC::ProtobufCommand<CliIpc::State> cmd;
        cmd.getProtoObj().set_is_logged_in(isLoggedIn_);
        cmd.getProtoObj().set_waiting_for_login_info(!backend_->isCanLoginWithAuthHash());
        sendCommand(cmd);
    }
    else if (command->getStringId() == CliIpc::Firewall::descriptor()->full_name())
    {
        IPC::ProtobufCommand<CliIpc::Firewall> *cmd = static_cast<IPC::ProtobufCommand<CliIpc::Firewall> *>(command);
        if (cmd->getProtoObj().is_enable())
        {
            if (!backend_->isFirewallEnabled())
            {
                backend_->firewallOn();
            }
            else
            {
                IPC::ProtobufCommand<CliIpc::FirewallStateChanged> cmd;
                cmd.getProtoObj().set_is_firewall_enabled(true);
                cmd.getProtoObj().set_is_firewall_always_on(backend_->isFirewallAlwaysOn());
                sendCommand(cmd);
            }
        }
        else
        {
            if (!backend_->isFirewallEnabled())
            {
                IPC::ProtobufCommand<CliIpc::FirewallStateChanged> cmd;
                cmd.getProtoObj().set_is_firewall_enabled(false);
                cmd.getProtoObj().set_is_firewall_always_on(backend_->isFirewallAlwaysOn());
                sendCommand(cmd);
            }
            else if (!backend_->isFirewallAlwaysOn())
            {
                backend_->firewallOff();
            }
            else
            {
                IPC::ProtobufCommand<CliIpc::FirewallStateChanged> cmd;
                cmd.getProtoObj().set_is_firewall_enabled(true);
                cmd.getProtoObj().set_is_firewall_always_on(backend_->isFirewallAlwaysOn());
                sendCommand(cmd);
            }
        }
    }
    else if (command->getStringId() == CliIpc::Login::descriptor()->full_name())
    {
        if (isLoggedIn_) {
            notifyCliLoginFinished();
        }
        else
        {
            connect(backend_, &Backend::loginFinished, this, &LocalIPCServer::notifyCliLoginFinished);
            connect(backend_, &Backend::loginError, this, &LocalIPCServer::notifyCliLoginFailed);
            IPC::ProtobufCommand<CliIpc::Login> *cmd = static_cast<IPC::ProtobufCommand<CliIpc::Login> *>(command);
            Q_EMIT attemptLogin(QString::fromStdString(cmd->getProtoObj().username()),
                                QString::fromStdString(cmd->getProtoObj().password()),
                                QString::fromStdString(cmd->getProtoObj().code2fa()));
        }
    }
    else if (command->getStringId() == CliIpc::SignOut::descriptor()->full_name())
    {
        if (isLoggedIn_)
        {
            connect(backend_, &Backend::signOutFinished, this, &LocalIPCServer::notifyCliSignOutFinished);
            IPC::ProtobufCommand<CliIpc::SignOut> *cmd = static_cast<IPC::ProtobufCommand<CliIpc::SignOut> *>(command);
            backend_->signOut(cmd->getProtoObj().is_keep_firewall_on());
        }
        else {
            notifyCliSignOutFinished();
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
    IPC::ProtobufCommand<CliIpc::ConnectStateChanged> cmd;
    *cmd.getProtoObj().mutable_connect_state() = connectState;
    sendCommand(cmd);
}

void LocalIPCServer::onBackendFirewallStateChanged(bool isEnabled)
{
    IPC::ProtobufCommand<CliIpc::FirewallStateChanged> cmd;
    cmd.getProtoObj().set_is_firewall_enabled(isEnabled);
    cmd.getProtoObj().set_is_firewall_always_on(backend_->isFirewallAlwaysOn());
    sendCommand(cmd);
}

void LocalIPCServer::onBackendLoginFinished(bool /*isLoginFromSavedSettings*/)
{
    isLoggedIn_ = true;
}

void LocalIPCServer::onBackendSignOutFinished()
{
    isLoggedIn_ = false;
}

void LocalIPCServer::sendCommand(const IPC::Command &command)
{
    for (IPC::IConnection * connection : connections_)
    {
        connection->sendCommand(command);
    }
}

void LocalIPCServer::notifyCliLoginFinished()
{
    sendLoginResult(true, QString());
}

void LocalIPCServer::notifyCliLoginFailed(ProtoTypes::LoginError loginError, const QString &errorMessage)
{
    Q_UNUSED(loginError)
    sendLoginResult(false, errorMessage);
}

void LocalIPCServer::notifyCliSignOutFinished()
{
    disconnect(backend_, &Backend::signOutFinished, this, &LocalIPCServer::notifyCliSignOutFinished);
    IPC::ProtobufCommand<CliIpc::SignedOut> cmd;
    sendCommand(cmd);
}

void LocalIPCServer::sendLoginResult(bool isLoggedIn, const QString &errorMessage)
{
    disconnect(backend_, &Backend::loginFinished, this, &LocalIPCServer::notifyCliLoginFinished);
    disconnect(backend_, &Backend::loginError, this, &LocalIPCServer::notifyCliLoginFailed);
    IPC::ProtobufCommand<CliIpc::LoginResult> cmd;
    cmd.getProtoObj().set_is_logged_in(isLoggedIn);
    if (!errorMessage.isEmpty()) {
        cmd.getProtoObj().set_login_error(errorMessage.toStdString());
    }
    sendCommand(cmd);
}
