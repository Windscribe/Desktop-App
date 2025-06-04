#include "backendcommander.h"

#include <chrono>
#include <iostream>
#include <QTimer>
#include <thread>
#ifdef CLI_ONLY
#include <unistd.h>
#endif

#include "ipc/clicommands.h"
#include "ipc/connection.h"
#include "languagecontroller.h"
#include "strings.h"
#include "utils.h"
#include "utils/log/categories.h"
#include "utils/utils.h"


BackendCommander::BackendCommander(const CliArguments &cliArgs) : QObject(), cliArgs_(cliArgs)
{
    unsigned long cliPid = Utils::getCurrentPid();
    qCDebug(LOG_CLI) << "CLI pid: " << cliPid;
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
    if (command->getStringId() == IPC::CliCommands::Acknowledge::getCommandStringId()) {
        // There are currently no commands that return a real value we need to parse here
        onAcknowledge();
    } else if (command->getStringId() == IPC::CliCommands::State::getCommandStringId()) {
        if (cliArgs_.cliCommand() == CLI_COMMAND_STATUS) {
            // If we explicitly requested status, print it.
            onStateResponse(command);
        } else if (!bCommandSent_){
            // If it's a response for the initial state request, send the command now.
            IPC::CliCommands::State *state = static_cast<IPC::CliCommands::State *>(command);
            sendCommand(state);
        } else {
            // Otherwise, this is an ongoing command in blocking mode.
            onStateUpdated(command);
        }
    } else if (command->getStringId() == IPC::CliCommands::LocationsList::getCommandStringId()) {
        IPC::CliCommands::LocationsList *cmd = static_cast<IPC::CliCommands::LocationsList *>(command);

        if (cliArgs_.cliCommand() == CLI_COMMAND_LOCATIONS_STATIC) {
            std::cout << deviceNameString(cmd->deviceName_).toStdString() << std::endl << std::endl;
        }

        if (cmd->locations_.isEmpty()) {
            emit finished(1, tr("No locations."));
        } else {
            emit finished(0, cmd->locations_.join("\n"));
        }
    }
}

void BackendCommander::onConnectionStateChanged(int state, IPC::Connection * /*connection*/)
{
    if (state == IPC::CONNECTION_CONNECTED) {
        qCDebug(LOG_CLI) << "Connected to app";
        ipcState_ = IPC_CONNECTED;
        loggedInTimer_.start();
        sendStateCommand();
    }
    else if (state == IPC::CONNECTION_DISCONNECTED) {
        qCDebug(LOG_CLI) << "Disconnected from app";
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

    if (cliArgs_.cliCommand() == CLI_COMMAND_CONNECT || cliArgs_.cliCommand() == CLI_COMMAND_CONNECT_BEST
            || cliArgs_.cliCommand() == CLI_COMMAND_CONNECT_LOCATION || cliArgs_.cliCommand() == CLI_COMMAND_CONNECT_STATIC) {
#ifdef CLI_ONLY
        if (!state->connectivity_) {
            emit(finished(1, QObject::tr("Internet connectivity is not available.  Try again later.")));
            return;
        }
#endif

        if (state->loginState_ != LOGIN_STATE_LOGGED_IN) {
            emit finished(1, QObject::tr("Not logged in"));
            return;
        }
        IPC::CliCommands::Connect cmd;
        if (cliArgs_.cliCommand() == CLI_COMMAND_CONNECT_STATIC) {
            cmd.locationType_ = IPC::CliCommands::LocationType::kStaticIp;
        } else {
            cmd.locationType_ = IPC::CliCommands::LocationType::kRegular;
        }
        cmd.location_ = cliArgs_.location();
        cmd.protocol_ = cliArgs_.protocol();
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_DISCONNECT) {
        if (state->loginState_ != LOGIN_STATE_LOGGED_IN) {
            emit finished(1, QObject::tr("Not logged in"));
            return;
        }
        if (state->connectState_.connectState == CONNECT_STATE_DISCONNECTED) {
            emit finished(1, QObject::tr("Already disconnected"));
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
    else if (cliArgs_.cliCommand() == CLI_COMMAND_LOCATIONS || cliArgs_.cliCommand() == CLI_COMMAND_LOCATIONS_STATIC) {
        if (state->loginState_ != LOGIN_STATE_LOGGED_IN) {
            emit finished(1, QObject::tr("Not logged in"));
            return;
        }
        IPC::CliCommands::ShowLocations cmd;
        if (cliArgs_.cliCommand() == CLI_COMMAND_LOCATIONS) {
            cmd.locationType_ = IPC::CliCommands::LocationType::kRegular;
        } else {
            cmd.locationType_ = IPC::CliCommands::LocationType::kStaticIp;
        }

        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_LOGIN) {
#ifdef CLI_ONLY
        if (!state->connectivity_) {
            emit(finished(1, QObject::tr("Internet connectivity is not available.  Try again later.")));
            return;
        }
#endif
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
        cmd.code2fa_ = code2fa_;
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_LOGOUT) {
#ifdef CLI_ONLY
        if (!state->connectivity_) {
            emit(finished(1, QObject::tr("Internet connectivity is not available.  Try again later.")));
            return;
        }
#endif
        if (state->loginState_ == LOGIN_STATE_LOGGED_OUT) {
            emit finished(1, QObject::tr("Already logged out"));
            return;
        }
        IPC::CliCommands::Logout cmd;
        cmd.isKeepFirewallOn_ = cliArgs_.keepFirewallOn();
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_SEND_LOGS) {
#ifdef CLI_ONLY
        if (!state->connectivity_) {
            emit(finished(1, QObject::tr("Internet connectivity is not available.  Try again later.")));
            return;
        }
#endif
        IPC::CliCommands::SendLogs cmd;
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_UPDATE) {
#ifdef CLI_ONLY
        if (!state->connectivity_) {
            emit(finished(1, QObject::tr("Internet connectivity is not available.  Try again later.")));
            return;
        }
#endif
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
    //  This only occurs on CLI with GUI backend; it just changes the appropriate window/tab.  Do not wait for this operation.
    if (cliArgs_.cliCommand() == CLI_COMMAND_LOCATIONS || cliArgs_.cliCommand() == CLI_COMMAND_LOCATIONS_STATIC) {
        emit(finished(0, ""));
        return;
    }

    if (!cliArgs_.nonBlocking()) {
        sendStateCommand();
        return;
    }

    if (cliArgs_.cliCommand() == CLI_COMMAND_CONNECT || cliArgs_.cliCommand() == CLI_COMMAND_CONNECT_BEST
            || cliArgs_.cliCommand() == CLI_COMMAND_CONNECT_LOCATION || cliArgs_.cliCommand() == CLI_COMMAND_CONNECT_STATIC) {
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
        emit(finished(0, QObject::tr("Key limit behaviour is set.")));
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_SEND_LOGS) {
        emit(finished(0, QObject::tr("Logs sent.")));
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_RELOAD_CONFIG) {
        emit(finished(0, QObject::tr("Preferences reloaded.")));
    }
}

void BackendCommander::onStateUpdated(IPC::Command *command)
{
    static std::string prevStr;
    std::string str;

    IPC::CliCommands::State *cmd = static_cast<IPC::CliCommands::State *>(command);

    if (cliArgs_.cliCommand() == CLI_COMMAND_CONNECT || cliArgs_.cliCommand() == CLI_COMMAND_CONNECT_BEST
            || cliArgs_.cliCommand() == CLI_COMMAND_CONNECT_LOCATION || cliArgs_.cliCommand() == CLI_COMMAND_CONNECT_STATIC) {
        if (connectId_.isEmpty() && cmd->connectState_.connectState == CONNECT_STATE_CONNECTING) {
            connectId_ = cmd->connectId_;
        }

        if (cmd->connectState_.connectState == CONNECT_STATE_DISCONNECTED && cmd->connectState_.connectError != NO_CONNECT_ERROR ||
            cmd->connectState_.connectState == CONNECT_STATE_DISCONNECTED && cmd->connectState_.disconnectReason == DISCONNECTED_BY_KEY_LIMIT ||
            cmd->connectState_.connectState == CONNECT_STATE_DISCONNECTED && cmd->connectId_.isEmpty())
        {
            // Disconnected with error
            emit(finished(1, connectStateString(cmd->connectState_, cmd->location_, cmd->tunnelTestState_, false)));
        } else if (cmd->connectState_.connectState == CONNECT_STATE_CONNECTED && cmd->tunnelTestState_ != TUNNEL_TEST_STATE_UNKNOWN) {
            // connected with a known tunnel test state
            emit(finished(0, connectStateString(cmd->connectState_, cmd->location_, cmd->tunnelTestState_, false)));
        } else if (cmd->connectId_ != connectId_ && connectId_.isEmpty() && !cmd->connectId_.isEmpty() &&
            cmd->connectState_.connectState == CONNECT_STATE_CONNECTING || cmd->connectState_.connectState == CONNECT_STATE_DISCONNECTED)
        {
            emit(finished(1, QObject::tr("Connection has been overridden by another command.")));
        } else {
            str = connectStateString(cmd->connectState_, cmd->location_, cmd->tunnelTestState_, false).toStdString();
            if (str != prevStr) {
                std::cout << str << std::endl;
                prevStr = str;
            }
            // In progress: wait a short period before requesting status again
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            sendStateCommand();
        }
    } else if (cliArgs_.cliCommand() == CLI_COMMAND_DISCONNECT) {
        if (connectId_.isEmpty()) {
            connectId_ = cmd->connectId_;
        }

        if (cmd->connectState_.connectState == CONNECT_STATE_DISCONNECTED || cmd->connectId_.isEmpty()) {
            cmd->connectState_.connectState = CONNECT_STATE_DISCONNECTED;
            emit(finished(0, connectStateString(cmd->connectState_, cmd->location_, cmd->tunnelTestState_, false)));
        } else if (cmd->connectId_ != connectId_) {
            emit(finished(1, QObject::tr("Disconnection has been overridden by another command.")));
        } else {
            str = connectStateString(cmd->connectState_, cmd->location_, cmd->tunnelTestState_, false).toStdString();
            if (str != prevStr) {
                std::cout << str << std::endl;
                prevStr = str;
            }
            // In progress: wait a short period before requesting status again
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            sendStateCommand();
        }
    } else if (cliArgs_.cliCommand() == CLI_COMMAND_LOGIN) {
        if (cmd->loginState_ == LOGIN_STATE_LOGIN_ERROR && cmd->loginError_ == wsnet::LoginResult::kMissingCode2fa) {
            code2fa_ = Utils::getInput("2FA code: ", false);
            sendCommand(cmd);
        } else if (cmd->loginState_ == LOGIN_STATE_LOGGED_IN || cmd->loginState_ == LOGIN_STATE_LOGIN_ERROR) {
            emit(finished(0, loginStateString(cmd->loginState_, cmd->loginError_, cmd->loginErrorMessage_, false)));
        } else {
            str = loginStateString(cmd->loginState_, cmd->loginError_, cmd->loginErrorMessage_, false).toStdString();
            if (str != prevStr) {
                std::cout << str << std::endl;
                prevStr = str;
            }
            // In progress: wait a short period before requesting status again
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            sendStateCommand();
        }
    } else if (cliArgs_.cliCommand() == CLI_COMMAND_LOGOUT) {
        if (cmd->loginState_ == LOGIN_STATE_LOGGED_OUT) {
            emit(finished(0, loginStateString(cmd->loginState_, cmd->loginError_, cmd->loginErrorMessage_, false)));
        } else {
            str = QObject::tr("Logging out").toStdString();
            if (str != prevStr) {
                std::cout << str << std::endl;
                prevStr = str;
            }
            // In progress: wait a short period before requesting status again
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            sendStateCommand();
        }
    } else if (cliArgs_.cliCommand() == CLI_COMMAND_UPDATE) {
        onUpdateStateResponse(command);
    }
}
