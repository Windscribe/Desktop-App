#include "localipcserver.h"
#include "utils/ws_assert.h"
#include "utils/utils.h"
#include "utils/logger.h"
#include "ipc/server.h"
#include "ipc/clicommands.h"
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
    WS_ASSERT(server_ == nullptr);
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
        IPC::CliCommands::LocationsShown cmd;
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
    if (command->getStringId() ==IPC::CliCommands::ShowLocations::getCommandStringId())
    {
        emit showLocations();
    }
    else if (command->getStringId() == IPC::CliCommands::Connect::getCommandStringId())
    {
        IPC::CliCommands::Connect *cmd = static_cast<IPC::CliCommands::Connect *>(command);
        QString locationStr = cmd->location_;
        LocationID lid;
        if (locationStr.isEmpty())
        {
            lid = PersistentState::instance().lastLocation();
        }
        else if (locationStr == "best")
        {
            lid = backend_->locationsModelManager()->getBestLocationId();
        }
        else
        {
            lid = backend_->locationsModelManager()->findLocationByFilter(locationStr);
        }

        IPC::CliCommands::ConnectToLocationAnswer cmd_send;
        if (lid.isValid())
        {
            cmd_send.isSuccess_ = true;
            cmd_send.location_ = lid.city();
        }
        else
        {
            cmd_send.isSuccess_ = false;
        }
        sendCommand(cmd_send);

        if (lid.isValid())
        {
            emit connectToLocation(lid);
        }
    }
    else if (command->getStringId() == IPC::CliCommands::Disconnect::getCommandStringId())
    {
        if (backend_->isDisconnected())
        {
            IPC::CliCommands::AlreadyDisconnected cmd;
            sendCommand(cmd);
        }
        else
        {
            backend_->sendDisconnect();
        }
    }
    else if (command->getStringId() == IPC::CliCommands::GetState::getCommandStringId())
    {
        IPC::CliCommands::State cmd;
        cmd.isLoggedIn_ = isLoggedIn_;
        cmd.waitingForLoginInfo_ = !backend_->isCanLoginWithAuthHash();
        sendCommand(cmd);
    }
    else if (command->getStringId() == IPC::CliCommands::Firewall::getCommandStringId())
    {
        IPC::CliCommands::Firewall *cmd = static_cast<IPC::CliCommands::Firewall *>(command);
        if (cmd->isEnable_)
        {
            if (!backend_->isFirewallEnabled())
            {
                backend_->firewallOn();
            }
            else
            {
                IPC::CliCommands::FirewallStateChanged cmd;
                cmd.isFirewallEnabled_ = true;
                cmd.isFirewallAlwaysOn_ = backend_->isFirewallAlwaysOn();
                sendCommand(cmd);
            }
        }
        else
        {
            if (!backend_->isFirewallEnabled())
            {
                IPC::CliCommands::FirewallStateChanged cmd;
                cmd.isFirewallEnabled_ = false;
                cmd.isFirewallAlwaysOn_ = backend_->isFirewallAlwaysOn();
                sendCommand(cmd);
            }
            else if (!backend_->isFirewallAlwaysOn())
            {
                backend_->firewallOff();
            }
            else
            {
                IPC::CliCommands::FirewallStateChanged cmd;
                cmd.isFirewallEnabled_ = true;
                cmd.isFirewallAlwaysOn_ = backend_->isFirewallAlwaysOn();
                sendCommand(cmd);
            }
        }
    }
    else if (command->getStringId() == IPC::CliCommands::Login::getCommandStringId())
    {
        if (isLoggedIn_) {
            notifyCliLoginFinished();
        }
        else
        {
            connect(backend_, &Backend::loginFinished, this, &LocalIPCServer::notifyCliLoginFinished);
            connect(backend_, &Backend::loginError, this, &LocalIPCServer::notifyCliLoginFailed);
            IPC::CliCommands::Login *cmd = static_cast<IPC::CliCommands::Login *>(command);
            Q_EMIT attemptLogin(cmd->username_, cmd->password_, cmd->code2fa_);
        }
    }
    else if (command->getStringId() == IPC::CliCommands::SignOut::getCommandStringId())
    {
        if (isLoggedIn_)
        {
            connect(backend_, &Backend::signOutFinished, this, &LocalIPCServer::notifyCliSignOutFinished);
            IPC::CliCommands::SignOut *cmd = static_cast<IPC::CliCommands::SignOut *>(command);
            backend_->signOut(cmd->isKeepFirewallOn_);
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

void LocalIPCServer::onBackendConnectStateChanged(const types::ConnectState &connectState)
{
    IPC::CliCommands::ConnectStateChanged cmd;
    cmd.connectState = connectState;
    sendCommand(cmd);
}

void LocalIPCServer::onBackendFirewallStateChanged(bool isEnabled)
{
    IPC::CliCommands::FirewallStateChanged cmd;
    cmd.isFirewallEnabled_ = isEnabled;
    cmd.isFirewallAlwaysOn_ = backend_->isFirewallAlwaysOn();
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

void LocalIPCServer::notifyCliLoginFailed(LOGIN_RET loginError, const QString &errorMessage)
{
    Q_UNUSED(loginError)
    sendLoginResult(false, errorMessage);
}

void LocalIPCServer::notifyCliSignOutFinished()
{
    disconnect(backend_, &Backend::signOutFinished, this, &LocalIPCServer::notifyCliSignOutFinished);
    IPC::CliCommands::SignedOut cmd;
    sendCommand(cmd);
}

void LocalIPCServer::sendLoginResult(bool isLoggedIn, const QString &errorMessage)
{
    disconnect(backend_, &Backend::loginFinished, this, &LocalIPCServer::notifyCliLoginFinished);
    disconnect(backend_, &Backend::loginError, this, &LocalIPCServer::notifyCliLoginFailed);
    IPC::CliCommands::LoginResult cmd;
    cmd.isLoggedIn_ = isLoggedIn;
    if (!errorMessage.isEmpty()) {
        cmd.loginError_ = errorMessage;
    }
    sendCommand(cmd);
}
