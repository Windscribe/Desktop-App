#include "backendcommander.h"

#include <iostream>
#include <QTimer>
#ifdef CLI_ONLY
#include <unistd.h>
#endif

#include "ipc/clicommands.h"
#include "ipc/connection.h"
#include "languagecontroller.h"
#include "strings.h"
#include "utils/logger.h"
#include "utils/utils.h"


BackendCommander::BackendCommander(const CliArguments &cliArgs) : QObject(), cliArgs_(cliArgs)
{
    unsigned long cliPid = Utils::getCurrentPid();
    qCDebug(LOG_BASIC) << "CLI pid: " << cliPid;
}

BackendCommander::~BackendCommander()
{
    if (connection_) {
        connection_->close();
        delete connection_;
    }
}

void BackendCommander::initAndSend()
{
    connection_ = new IPC::Connection();
    connect(connection_, &IPC::Connection::newCommand, this, &BackendCommander::onConnectionNewCommand, Qt::QueuedConnection);
    connect(connection_, &IPC::Connection::stateChanged, this, &BackendCommander::onConnectionStateChanged, Qt::QueuedConnection);
    connectingTimer_.start();
    ipcState_ = IPC_CONNECTING;
    connection_->connect();
}

void BackendCommander::onConnectionNewCommand(IPC::Command *command, IPC::Connection * /*connection*/)
{
#ifdef CLI_ONLY
    // Update is special in that it's a blocking command
    if (cliArgs_.cliCommand() == CLI_COMMAND_UPDATE && bCommandSent_) {
        if (command->getStringId() == IPC::CliCommands::Acknowledge::getCommandStringId()) {
            sendStateCommand();
            return;
        }
        onUpdateStateResponse(command);
        return;
    }
#endif

    if (command->getStringId() == IPC::CliCommands::Acknowledge::getCommandStringId()) {
        // There are currently no commands that return a real value we need to parse here
        onAcknowledge();
    } else if (command->getStringId() == IPC::CliCommands::State::getCommandStringId()) {
        if (cliArgs_.cliCommand() == CLI_COMMAND_STATUS) {
            // If we explicitly requested status, print it.
            onStateResponse(command);
        } else {
            // If it's a response for the initial state request, send the command now.
            IPC::CliCommands::State *state = static_cast<IPC::CliCommands::State *>(command);
            sendCommand(state);
        }
    } else if (command->getStringId() == IPC::CliCommands::LocationsList::getCommandStringId()) {
        IPC::CliCommands::LocationsList *cmd = static_cast<IPC::CliCommands::LocationsList *>(command);
        emit finished(0, cmd->locations_.join("\n"));
    }
}

void BackendCommander::onConnectionStateChanged(int state, IPC::Connection * /*connection*/)
{
    if (state == IPC::CONNECTION_CONNECTED) {
        qCDebug(LOG_BASIC) << "Connected to app";
        ipcState_ = IPC_CONNECTED;
        loggedInTimer_.start();
        sendStateCommand();
    }
    else if (state == IPC::CONNECTION_DISCONNECTED) {
        qCDebug(LOG_BASIC) << "Disconnected from app";
        emit finished(0, "");
    }
    else if (state == IPC::CONNECTION_ERROR) {
        if (ipcState_ == IPC_CONNECTING) {
            if (connectingTimer_.isValid() && connectingTimer_.elapsed() > MAX_WAIT_TIME_MS) {
                connectingTimer_.invalidate();
                emit finished(1, QObject::tr("Aborting: app did not start in time"));
            }
            else {
                // Try connect again. Delay is necessary so that Engine process will actually start
                // running on low resource systems.
                QTimer::singleShot(100, this, [this]() { connection_->connect(); } );
            }
        }
        else {
            emit finished(1, QObject::tr("Aborting: IPC communication error"));
        }
    }
}

void BackendCommander::sendCommand(IPC::CliCommands::State *state)
{
    LanguageController::instance().setLanguage(state->language_);

    if (cliArgs_.cliCommand() == CLI_COMMAND_CONNECT || cliArgs_.cliCommand() == CLI_COMMAND_CONNECT_BEST || cliArgs_.cliCommand() == CLI_COMMAND_CONNECT_LOCATION) {
        if (state->loginState_ != LOGIN_STATE_LOGGED_IN) {
            emit finished(1, QObject::tr("Not logged in"));
            return;
        }
        IPC::CliCommands::Connect cmd;
        cmd.location_ = cliArgs_.location();
        cmd.protocol_ = cliArgs_.protocol();
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_DISCONNECT) {
        if (state->loginState_ != LOGIN_STATE_LOGGED_IN) {
            emit finished(1, QObject::tr("Not logged in"));
            return;
        }
        IPC::CliCommands::Disconnect cmd;
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_FIREWALL_ON) {
        if (state->isFirewallOn_) {
            emit finished(1, QObject::tr("Firewall already on"));
            return;
        }
        IPC::CliCommands::Firewall cmd;
        cmd.isEnable_ = true;
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_FIREWALL_OFF) {
        if (!state->isFirewallOn_) {
            emit finished(1, QObject::tr("Firewall already off"));
            return;
        }
        if (state->isFirewallAlwaysOn_) {
            emit finished(1, QObject::tr("Firewall set to always on and can't be turned off"));
            return;
        }

        IPC::CliCommands::Firewall cmd;
        cmd.isEnable_ = false;
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_SET_KEYLIMIT_BEHAVIOR) {
        IPC::CliCommands::SetKeyLimitBehavior cmd;
        cmd.keyLimitDelete_ = cliArgs_.keyLimitDelete();
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_LOCATIONS) {
        if (state->loginState_ != LOGIN_STATE_LOGGED_IN) {
            emit finished(1, QObject::tr("Not logged in"));
            return;
        }
        IPC::CliCommands::ShowLocations cmd;
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_LOGIN) {
        if (state->loginState_ == LOGIN_STATE_LOGGED_IN) {
            emit finished(1, QObject::tr("Already logged in"));
            return;
        }
        IPC::CliCommands::Login cmd;
        cmd.username_ = cliArgs_.username();
        if (cmd.username_.size() < 3) {
            emit finished(1, QObject::tr("Username too short"));
            return;
        }
        cmd.password_ = cliArgs_.password();
        if (cmd.password_.size() < 8) {
            emit finished(1, QObject::tr("Password too short"));
            return;
        }
        cmd.code2fa_ = cliArgs_.code2fa();
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_LOGOUT) {
        if (state->loginState_ == LOGIN_STATE_LOGGED_OUT) {
            emit finished(1, QObject::tr("Already logged out"));
            return;
        }
        IPC::CliCommands::Logout cmd;
        cmd.isKeepFirewallOn_ = cliArgs_.keepFirewallOn();
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_SEND_LOGS) {
        IPC::CliCommands::SendLogs cmd;
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_UPDATE) {
        if (state->loginState_ != LOGIN_STATE_LOGGED_IN) {
            emit finished(1, QObject::tr("Not logged in"));
            return;
        }
        if (state->updateAvailable_.isEmpty()) {
            emit finished(1, QObject::tr("No update available"));
            return;
        }
        IPC::CliCommands::Update cmd;
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_RELOAD_CONFIG) {
        IPC::CliCommands::ReloadConfig cmd;
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_STATUS) {
        sendStateCommand();
    }

    bCommandSent_ = true;
}

void BackendCommander::sendStateCommand()
{
    IPC::CliCommands::GetState cmd;
    connection_->sendCommand(cmd);
}

void BackendCommander::onStateResponse(IPC::Command *command)
{
    IPC::CliCommands::State *cmd = static_cast<IPC::CliCommands::State *>(command);

    LanguageController::instance().setLanguage(cmd->language_);

    QString msg = QString("%1\n%2\n%3")
        .arg(connectivityString(cmd->connectivity_))
        .arg(loginStateString(cmd->loginState_, cmd->loginError_, cmd->loginErrorMessage_))
        .arg(firewallStateString(cmd->isFirewallOn_, cmd->isFirewallAlwaysOn_));

    if (cmd->loginState_ != LOGIN_STATE_LOGGED_OUT) {
        msg += "\n" + connectStateString(cmd->connectState_, cmd->location_, cmd->tunnelTestState_);
    }

    if (cmd->connectState_.connectState != CONNECT_STATE_DISCONNECTED && cmd->protocol_.isValid()) {
        msg += "\n" + protocolString(cmd->protocol_, cmd->port_);
    }

    if (cmd->loginState_ == LOGIN_STATE_LOGGED_IN) {
        msg += "\n" + tr("Data usage: %1 / %2")
            .arg(dataString(cmd->language_, cmd->trafficUsed_))
            .arg((cmd->trafficMax_ == -1) ? tr("Unlimited") : dataString(cmd->language_, cmd->trafficMax_));
        if (!cmd->updateAvailable_.isEmpty()) {
            msg += "\n" + updateString(cmd->updateAvailable_);
        }
    }

    emit finished(0, msg);
}

void BackendCommander::onUpdateStateResponse(IPC::Command *command)
{
#ifdef CLI_ONLY
    static int printedLength = 0;

    IPC::CliCommands::State *cmd = static_cast<IPC::CliCommands::State *>(command);

    // If an error has occurred, print it and quit
    if (cmd->updateError_ != UPDATE_VERSION_ERROR_NO_ERROR) {
        emit finished(1, updateErrorString(cmd->updateError_));
        return;
    }

    // Render 'Downloading n%' line
    for (int i = 0; i < printedLength; i++) {
        std::cout << '\b';
    }

    QString str = QObject::tr("Downloading: %1%").arg(cmd->updateProgress_);
    std::cout << str.toStdString();

    for (int i = str.toUtf8().size(); i < printedLength; i++) {
        std::cout << " ";
    }
    std::cout << std::flush;
    // the length is in bytes, not characters, because we need to 'backspace' this many times.
    printedLength = str.toUtf8().size();

    if (cmd->updateState_ == UPDATE_VERSION_STATE_RUNNING) {
        std::cout << std::endl;

        // Update has been downloaded, run it now
        QString updateCmd = (QString("/etc/windscribe/install-update ") + cmd->updatePath_);
        int ret = system(updateCmd.toStdString().c_str());
        std::error_code ec;
        std::filesystem::remove(cmd->updatePath_.toStdString(), ec);

        emit finished(ret, "");
        return;
    }

    // If still in progress, wait 1 second and then request state again.
    sleep(1);
    sendStateCommand();
#endif
}

void BackendCommander::onAcknowledge()
{
    if (cliArgs_.cliCommand() == CLI_COMMAND_CONNECT || cliArgs_.cliCommand() == CLI_COMMAND_CONNECT_BEST || cliArgs_.cliCommand() == CLI_COMMAND_CONNECT_LOCATION) {
        emit(finished(0, QObject::tr("Connection is in progress.  Use 'windscribe-cli status' to check for connection status.")));
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_DISCONNECT) {
        emit(finished(0, QObject::tr("Disconnection is in progress.  Use 'windscribe-cli status' to check for connection status.")));
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_FIREWALL_ON) {
        emit(finished(0, QObject::tr("Firewall is on.")));
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_FIREWALL_OFF) {
        emit(finished(0, QObject::tr("Firewall is off.")));
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_SET_KEYLIMIT_BEHAVIOR) {
        emit(finished(0, QObject::tr("Key limit behavior is set.")));
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_LOGIN) {
        emit(finished(0, QObject::tr("Login is in progress.  Use 'windscribe-cli status' to check for status.")));
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_LOGOUT) {
        emit(finished(0, QObject::tr("Logout is in progress.  Use 'windscribe-cli status' to check for status.")));
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_SEND_LOGS) {
        emit(finished(0, QObject::tr("Logs sent.")));
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_RELOAD_CONFIG) {
        emit(finished(0, QObject::tr("Preferences reloaded.")));
    }
}
