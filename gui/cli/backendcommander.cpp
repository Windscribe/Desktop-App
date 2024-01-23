#include "backendcommander.h"

#include <QTimer>

#include "ipc/clicommands.h"
#include "ipc/connection.h"
#include "types/locationid.h"
#include "utils/logger.h"
#include "utils/utils.h"

BackendCommander::BackendCommander(const CliArguments &cliArgs) : QObject()
    , cliArgs_(cliArgs)
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

void BackendCommander::initAndSend(bool isGuiAlreadyRunning)
{
    isGuiAlreadyRunning_ = isGuiAlreadyRunning;
    connection_ = new IPC::Connection();
    connect(connection_, &IPC::Connection::newCommand, this, &BackendCommander::onConnectionNewCommand, Qt::QueuedConnection);
    connect(connection_, &IPC::Connection::stateChanged, this, &BackendCommander::onConnectionStateChanged, Qt::QueuedConnection);
    connectingTimer_.start();
    ipcState_ = IPC_CONNECTING;
    connection_->connect();
}

void BackendCommander::onConnectionNewCommand(IPC::Command *command, IPC::Connection * /*connection*/)
{
    if (bCommandSent_ && command->getStringId() == IPC::CliCommands::LocationsShown::getCommandStringId()) {
        emit finished(0, tr("Viewing Locations..."));
    }
    else if (bCommandSent_ && command->getStringId() == IPC::CliCommands::ConnectToLocationAnswer::getCommandStringId()) {
        IPC::CliCommands::ConnectToLocationAnswer *cmd = static_cast<IPC::CliCommands::ConnectToLocationAnswer *>(command);

        if (cmd->isSuccess_) {
            qCDebug(LOG_BASIC) << "Connecting to" << cmd->location_;
            emit report("Connecting to " + cmd->location_);
        }
        else {
            emit finished(1, tr("Error: Could not find server matching: \"") + cliArgs_.location() + "\" or the location is disabled");
        }
    }
    else if (bCommandSent_ && command->getStringId() == IPC::CliCommands::ConnectStateChanged::getCommandStringId()) {
        IPC::CliCommands::ConnectStateChanged *cmd = static_cast<IPC::CliCommands::ConnectStateChanged *>(command);

        if (cliArgs_.cliCommand() >= CLI_COMMAND_CONNECT && cliArgs_.cliCommand() <= CLI_COMMAND_DISCONNECT) {
            if (cmd->connectState.connectState == CONNECT_STATE_CONNECTED) {
                if (cmd->connectState.location.isValid()) {
                    LocationID currentLocation = cmd->connectState.location;

                    QString locationConnectedTo;
                    if (currentLocation.isBestLocation()) {
                        locationConnectedTo = tr("Best Location");
                    }
                    else {
                        locationConnectedTo = cmd->connectState.location.city();
                    }
                    emit finished(0, tr("Connected to ") + locationConnectedTo);
                }
            }
            else if (cmd->connectState.connectState == CONNECT_STATE_DISCONNECTED) {
                emit finished(0, tr("Disconnected"));
            }
        }
    }
    else if (bCommandSent_ && command->getStringId() == IPC::CliCommands::AlreadyDisconnected::getCommandStringId()) {
        emit finished(0, tr("Already Disconnected"));
    }
    else if (command->getStringId() == IPC::CliCommands::State::getCommandStringId()) {
        if (cliArgs_.cliCommand() == CLI_COMMAND_STATUS) {
            onStatusResponse(command);
        }
        else {
            onLoginStateResponse(command);
        }
    }
    else if (command->getStringId() == IPC::CliCommands::FirewallStateChanged::getCommandStringId()) {
        IPC::CliCommands::FirewallStateChanged *cmd = static_cast<IPC::CliCommands::FirewallStateChanged *>(command);
        if (cmd->isFirewallEnabled_) {
            if (cmd->isFirewallAlwaysOn_) {
                emit finished(0, tr("Firewall is in Always On mode and cannot be changed"));
            }
            else {
                emit finished(0, tr("Firewall is ON"));
            }
        }
        else {
            emit finished(0, tr("Firewall is OFF"));
        }
    }
    else if (bCommandSent_ && command->getStringId() == IPC::CliCommands::SignedOut::getCommandStringId()) {
        emit finished(0, tr("Signed out"));
    }
    else if (bCommandSent_ && command->getStringId() == IPC::CliCommands::LoginResult::getCommandStringId()) {
        IPC::CliCommands::LoginResult *cmd = static_cast<IPC::CliCommands::LoginResult *>(command);
        if (cmd->isLoggedIn_) {
            emit finished(0, tr("login successful"));
        }
        else {
            QString errorMessage(tr("login failed"));
            if (!cmd->loginError_.isEmpty()) {
                errorMessage += tr(". %1").arg(cmd->loginError_);
            }
            emit finished(1, errorMessage);
        }
    }
}

void BackendCommander::onConnectionStateChanged(int state, IPC::Connection * /*connection*/)
{
    if (state == IPC::CONNECTION_CONNECTED) {
        qCDebug(LOG_BASIC) << "Connected to GUI server";
        ipcState_ = IPC_CONNECTED;
        loggedInTimer_.start();
        sendStateCommand();
    }
    else if (state == IPC::CONNECTION_DISCONNECTED) {
        qCDebug(LOG_BASIC) << "Disconnected from GUI server";
        emit finished(0, "");
    }
    else if (state == IPC::CONNECTION_ERROR) {
        if (ipcState_ == IPC_CONNECTING) {
            if (connectingTimer_.isValid() && connectingTimer_.elapsed() > MAX_WAIT_TIME_MS) {
                connectingTimer_.invalidate();
                emit finished(1, "Aborting: Gui did not start in time");
            }
            else {
               // Try connect again. Delay is necessary so that Engine process will actually start
               // running on low resource systems.
                QTimer::singleShot(100, this, [this]() { connection_->connect(); } );
            }
        }
        else {
            emit finished(1, "Aborting: IPC communication error");
        }
    }
}

void BackendCommander::sendCommand()
{
    if (cliArgs_.cliCommand() == CLI_COMMAND_CONNECT || cliArgs_.cliCommand() == CLI_COMMAND_CONNECT_BEST || cliArgs_.cliCommand() == CLI_COMMAND_CONNECT_LOCATION) {
        qCDebug(LOG_BASIC) << "Connecting to last";

        IPC::CliCommands::Connect cmd;
        cmd.location_ = cliArgs_.location();
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_DISCONNECT) {
        IPC::CliCommands::Disconnect cmd;
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_FIREWALL_ON) {
        IPC::CliCommands::Firewall cmd;
        cmd.isEnable_ = true;
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_FIREWALL_OFF) {
        IPC::CliCommands::Firewall cmd;
        cmd.isEnable_ = false;
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_LOCATIONS) {
        IPC::CliCommands::ShowLocations cmd;
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_LOGIN) {
        IPC::CliCommands::Login cmd;
        cmd.username_ = cliArgs_.username();
        cmd.password_ = cliArgs_.password();
        cmd.code2fa_ = cliArgs_.code2fa();
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_SIGN_OUT) {
        IPC::CliCommands::SignOut cmd;
        cmd.isKeepFirewallOn_ = cliArgs_.keepFirewallOn();
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

void BackendCommander::onLoginStateResponse(IPC::Command *command)
{
    IPC::CliCommands::State *cmd = static_cast<IPC::CliCommands::State *>(command);

    if (cmd->isLoggedIn_) {
        if (cliArgs_.cliCommand() == CLI_COMMAND_LOGIN) {
            emit finished(0, tr("The application is already logged in"));
        }
        else {
            sendCommand();
        }
    }
    else {
        if (cliArgs_.cliCommand() == CLI_COMMAND_LOGIN && cmd->waitingForLoginInfo_) {
            // The app has let us know that it is not logged in and does not have cached login info.
            loggedInTimer_.invalidate();

            if (isGuiAlreadyRunning_) {
                sendCommand();
            }
            else {
                // Encountered an issue where the app UI gets stuck on the 'logging in' screen if we
                // send the login command as soon as the app reports that its backend init has finished.
                // The app does log in, but the UI doesn't update to reflect this state.  The app does
                // not currently have a mechanism to let us know when it has finished transitioning to
                // the login screen and is ready for us to submit the login request.
                QTimer::singleShot(2500, this, &BackendCommander::sendCommand);
            }
        }
        else if (cliArgs_.cliCommand() == CLI_COMMAND_SIGN_OUT && cmd->waitingForLoginInfo_) {
            loggedInTimer_.invalidate();
            emit finished(0, tr("The application is already signed out"));
        }
        else {
            if (loggedInTimer_.isValid() && loggedInTimer_.elapsed() > MAX_LOGIN_TIME_MS) {
                loggedInTimer_.invalidate();
                emit finished(1, "Aborting: GUI did not login in time");
            }
            else {
                if (!bLogginInMessageShown_) {
                    bLogginInMessageShown_ = true;
                    emit report("GUI is not logged in. Waiting for the login...");
                }
                QTimer::singleShot(100, this, &BackendCommander::sendStateCommand);
            }
        }
    }
}

void BackendCommander::onStatusResponse(IPC::Command *command)
{
    IPC::CliCommands::State *cmd = static_cast<IPC::CliCommands::State *>(command);

    QString msg;
    if (!cmd->isLoggedIn_) {
        msg = tr("Signed out");
    }
    else {
        switch (cmd->connectState_) {
        case CONNECT_STATE_DISCONNECTED:
            msg = tr("Disconnected");
            break;
        case CONNECT_STATE_CONNECTED:
            msg = tr("Connected");
            break;
        case CONNECT_STATE_CONNECTING:
            msg = tr("Connecting");
            break;
        case CONNECT_STATE_DISCONNECTING:
            msg = tr("Disconnecting");
            break;
        }

        if (cmd->location_.isValid()) {
            msg += QString(": %1").arg(cmd->location_.city());
        }
    }

    emit finished(0, msg);
}
