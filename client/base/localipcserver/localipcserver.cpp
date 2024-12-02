#include "localipcserver.h"

#include "backend/persistentstate.h"
#include "ipc/server.h"
#include "utils/log/categories.h"
#include "utils/utils.h"
#include "utils/ws_assert.h"

LocalIPCServer::LocalIPCServer(Backend *backend, QObject *parent) : QObject(parent)
  , backend_(backend), updateError_(UPDATE_VERSION_ERROR_NO_ERROR), updateProgress_(0), disconnectedByKeyLimit_(false)
{
    connect(backend_, &Backend::checkUpdateChanged, this, &LocalIPCServer::onBackendCheckUpdateChanged);
    connect(backend_, &Backend::connectStateChanged, this, &LocalIPCServer::onBackendConnectStateChanged);
    connect(backend_, &Backend::internetConnectivityChanged, this, &LocalIPCServer::onBackendInternetConnectivityChanged);
    connect(backend_, &Backend::loginError, this, &LocalIPCServer::onBackendLoginError);
    connect(backend_, &Backend::loginFinished, this, &LocalIPCServer::onBackendLoginFinished);
    connect(backend_, &Backend::logoutFinished, this, &LocalIPCServer::onBackendLogoutFinished);
    connect(backend_, &Backend::protocolPortChanged, this, &LocalIPCServer::onBackendProtocolPortChanged);
    connect(backend_, &Backend::testTunnelResult, this, &LocalIPCServer::onBackendTestTunnelResult);
    connect(backend_, &Backend::updateDownloaded, this, &LocalIPCServer::onBackendUpdateDownloaded);
    connect(backend_, &Backend::updateVersionChanged, this, &LocalIPCServer::onBackendUpdateVersionChanged);
    connect(backend_, &Backend::connectionIdChanged, this, &LocalIPCServer::onBackendConnectionIdChanged);
}

LocalIPCServer::~LocalIPCServer()
{
    for (IPC::Connection * connection : connections_) {
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
    connect(server_, &IPC::Server::newConnection, this, &LocalIPCServer::onServerCallbackAcceptFunction);

    if (!server_->start()) {
        qCCritical(LOG_CLI_IPC) << "Can't start IPC server for CLI";
    } else {
        qCDebug(LOG_CLI_IPC) << "IPC server for CLI started";
    }
}

void LocalIPCServer::sendLocations(const QStringList &list)
{
    IPC::CliCommands::LocationsList cmd;
    cmd.locations_ = list;
    sendCommand(cmd);
}

void LocalIPCServer::onServerCallbackAcceptFunction(IPC::Connection *connection)
{
    qCDebug(LOG_IPC) << "Client connected:" << connection;

    connections_.append(connection);

    connect(connection, &IPC::Connection::newCommand, this, &LocalIPCServer::onConnectionCommandCallback);
    connect(connection, &IPC::Connection::stateChanged, this, &LocalIPCServer::onConnectionStateCallback);
}

void LocalIPCServer::onConnectionCommandCallback(IPC::Command *command, IPC::Connection * /*connection*/)
{
    if (command->getStringId() ==IPC::CliCommands::ShowLocations::getCommandStringId()) {
        IPC::CliCommands::ShowLocations *cmd = static_cast<IPC::CliCommands::ShowLocations *>(command);
        emit showLocations(cmd->locationType_);
#ifdef CLI_ONLY
        // For a headless client, do not return Acknowledge here; we need the MainService to provide us
        // the list of locations to return via sendLocations().
        return;
#endif
    } else if (command->getStringId() == IPC::CliCommands::GetState::getCommandStringId()) {
        sendState();
        return;
    } else if (command->getStringId() == IPC::CliCommands::Connect::getCommandStringId()) {
        IPC::CliCommands::Connect *cmd = static_cast<IPC::CliCommands::Connect *>(command);
        IPC::CliCommands::LocationType type = cmd->locationType_;
        QString locationStr = cmd->location_;
        types::Protocol protocol = types::Protocol::fromString(cmd->protocol_);
        LocationID lid;

        if (type == IPC::CliCommands::LocationType::kStaticIp) {
            emit connectToStaticIpLocation(locationStr, protocol);
        } else {
            if (locationStr.isEmpty()) {
                lid = PersistentState::instance().lastLocation();
                if (!lid.isValid()) {
                    lid = backend_->locationsModelManager()->getBestLocationId();
                }
            } else if (locationStr == "best") {
                lid = backend_->locationsModelManager()->getBestLocationId();
            } else {
                lid = backend_->locationsModelManager()->findLocationByFilter(locationStr);
            }

            if (lid.isValid()) {
                emit connectToLocation(lid, protocol);
            }
        }
    } else if (command->getStringId() == IPC::CliCommands::Disconnect::getCommandStringId()) {
        if (!backend_->isDisconnected()) {
            backend_->sendDisconnect();
        }
    } else if (command->getStringId() == IPC::CliCommands::Firewall::getCommandStringId()) {
        IPC::CliCommands::Firewall *cmd = static_cast<IPC::CliCommands::Firewall *>(command);
        if (cmd->isEnable_ && !backend_->isFirewallEnabled()) {
            backend_->firewallOn();
        } else if (!cmd->isEnable_ && backend_->isFirewallEnabled() && !backend_->isFirewallAlwaysOn()) {
            backend_->firewallOff();
        }
    } else if (command->getStringId() == IPC::CliCommands::Login::getCommandStringId()) {
        if (backend_->currentLoginState() == LOGIN_STATE_LOGGED_OUT || backend_->currentLoginState() == LOGIN_STATE_LOGIN_ERROR) {
            IPC::CliCommands::Login *cmd = static_cast<IPC::CliCommands::Login *>(command);
            emit attemptLogin(cmd->username_, cmd->password_, cmd->code2fa_);
        }
    } else if (command->getStringId() == IPC::CliCommands::Logout::getCommandStringId()) {
        if (backend_->currentLoginState() != LOGIN_STATE_LOGGED_OUT) {
            IPC::CliCommands::Logout *cmd = static_cast<IPC::CliCommands::Logout *>(command);
            backend_->logout(cmd->isKeepFirewallOn_);
        }
    } else if (command->getStringId() == IPC::CliCommands::SendLogs::getCommandStringId()) {
        backend_->sendDebugLog();
    } else if (command->getStringId() == IPC::CliCommands::Update::getCommandStringId()) {
        if (!updateAvailable_.isEmpty()) {
#ifdef CLI_ONLY
            backend_->sendUpdateVersion(0);
#else
            emit update();
#endif
        }
    } else if (command->getStringId() == IPC::CliCommands::ReloadConfig::getCommandStringId()) {
        backend_->getPreferences()->loadIni();
    } else if (command->getStringId() == IPC::CliCommands::SetKeyLimitBehavior::getCommandStringId()) {
        IPC::CliCommands::SetKeyLimitBehavior *cmd = static_cast<IPC::CliCommands::SetKeyLimitBehavior *>(command);
        emit setKeyLimitBehavior(cmd->keyLimitDelete_);
    }

    IPC::CliCommands::Acknowledge cmd;
    sendCommand(cmd);
}

void LocalIPCServer::onConnectionStateCallback(int state, IPC::Connection *connection)
{
    if (state == IPC::CONNECTION_DISCONNECTED) {
        connections_.removeOne(connection);
        connection->close();
        delete connection;
    } else if (state == IPC::CONNECTION_ERROR) {
        qCWarning(LOG_BASIC) << "CLI disconnected from server with error";
        connections_.removeOne(connection);
        connection->close();
        delete connection;
    }
}

void LocalIPCServer::onBackendLoginFinished(bool /*isLoginFromSavedSettings*/)
{
    loginState_ = LOGIN_STATE_LOGGED_IN;
}

void LocalIPCServer::onBackendLoginError(wsnet::LoginResult code, const QString &msg)
{
    lastLoginError_ = code;
    lastLoginErrorMessage_ = msg;
}

void LocalIPCServer::onBackendLogoutFinished()
{
    loginState_ = LOGIN_STATE_LOGGED_OUT;
}

void LocalIPCServer::sendCommand(const IPC::Command &command)
{
    for (IPC::Connection * connection : connections_) {
        connection->sendCommand(command);
    }
}

void LocalIPCServer::sendState()
{
    IPC::CliCommands::State cmd;
    cmd.language_ = backend_->getPreferences()->language();
    cmd.connectivity_ = connectivity_;
    cmd.loginState_ = backend_->currentLoginState();
    cmd.loginError_ = lastLoginError_;
    cmd.loginErrorMessage_ = lastLoginErrorMessage_;
    cmd.connectState_ = connectState_;
    cmd.connectId_ = connectId_;
    cmd.tunnelTestState_ = tunnelTestState_;
    cmd.protocol_ = protocol_;
    cmd.port_ = port_;
    cmd.location_ = backend_->currentLocation();
    cmd.isFirewallOn_ = backend_->isFirewallEnabled();
    cmd.isFirewallAlwaysOn_ = backend_->isFirewallAlwaysOn();
    cmd.updateState_ = updateState_;
    cmd.updateError_ = updateError_;
    cmd.updateProgress_ = updateProgress_;
    cmd.updatePath_ = updatePath_;
    cmd.updateAvailable_ = updateAvailable_;
    cmd.trafficUsed_ = backend_->getAccountInfo()->trafficUsed();
    cmd.trafficMax_ = backend_->getAccountInfo()->plan();
    sendCommand(cmd);
}

void LocalIPCServer::onBackendCheckUpdateChanged(const api_responses::CheckUpdate &info)
{
    if (info.isAvailable()) {
        updateAvailable_ = info.version() + "." + QString::number(info.latestBuild());
    } else {
        updateAvailable_ = "";
    }
}

void LocalIPCServer::onBackendConnectStateChanged(const types::ConnectState &state)
{
    connectState_ = state;
    tunnelTestState_ = TUNNEL_TEST_STATE_UNKNOWN;

    if (connectState_.connectState == CONNECT_STATE_DISCONNECTED && disconnectedByKeyLimit_) {
        // Normally this looks like a regular disconnect; override this with a special reason
        connectState_.disconnectReason = DISCONNECTED_BY_KEY_LIMIT;
        disconnectedByKeyLimit_ = false;
    }
}

void LocalIPCServer::onBackendProtocolPortChanged(const types::Protocol &protocol, uint port)
{
    protocol_ = protocol;
    port_ = port;
}

void LocalIPCServer::onBackendInternetConnectivityChanged(bool connectivity)
{
    connectivity_ = connectivity;
}

void LocalIPCServer::onBackendTestTunnelResult(bool success)
{
    tunnelTestState_ = success ? TUNNEL_TEST_STATE_SUCCESS : TUNNEL_TEST_STATE_FAILURE;
}

void LocalIPCServer::onBackendUpdateVersionChanged(uint progressPercent, UPDATE_VERSION_STATE state, UPDATE_VERSION_ERROR error)
{
    updateState_ = state;

    if (state == UPDATE_VERSION_STATE_DONE) {
        updateError_ = error;
    } else if (state == UPDATE_VERSION_STATE_DOWNLOADING) {
        updateProgress_ = progressPercent;
    } else if (state == UPDATE_VERSION_STATE_RUNNING) {
        updateProgress_ = 100;
    }
}

void LocalIPCServer::onBackendUpdateDownloaded(const QString &path)
{
    updatePath_ = path;
}

void LocalIPCServer::setDisconnectedByKeyLimit()
{
    disconnectedByKeyLimit_ = true;
}

void LocalIPCServer::onBackendConnectionIdChanged(const QString &connId)
{
    connectId_ = connId;
}
