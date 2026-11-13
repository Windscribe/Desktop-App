#include "backendcommander.h"

#include <QFile>
#include <QProcess>
#include <QTimer>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <thread>

#include "ipc/clicommands.h"
#include "ipc/connection.h"
#include "strings.h"
#include "utils.h"
#include "utils/hydraprofile.h"
#include "utils/log/categories.h"
#include "utils/networkingvalidation.h"
#include "utils/systemwideapplock.h"
#include "utils/utils.h"


BackendCommander::BackendCommander(const CliArguments &cliArgs) : QObject(), cliArgs_(cliArgs),
    username_(cliArgs.username()), password_(cliArgs.password())
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
    std::unique_ptr<IPC::Command> owner(command);

    if (command->getStringId() == IPC::CliCommands::Acknowledge::getCommandStringId()) {
        IPC::CliCommands::Acknowledge *ackCmd = static_cast<IPC::CliCommands::Acknowledge *>(command);
        onAcknowledge(ackCmd);
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

        if (cliArgs_.cliCommand() == CLI_COMMAND_LOCATIONS_STATIC && !cmd->locations_.isEmpty() && !cmd->deviceName_.isEmpty()) {
            std::cout << deviceNameString(cmd->deviceName_).toStdString() << std::endl << std::endl;
        }

        if (cmd->locations_.isEmpty()) {
            emit finished(1, QString("No locations."));
        } else {
            emit finished(0, locationsString(cmd->locations_));
        }
    } else if (command->getStringId() == IPC::CliCommands::PortsList::getCommandStringId()) {
        IPC::CliCommands::PortsList *cmd = static_cast<IPC::CliCommands::PortsList *>(command);
        if (cmd->ports_.isEmpty()) {
            emit finished(1, QString("No ports available for the specified protocol."));
        } else {
            QStringList portStrings;
            for (uint port : std::as_const(cmd->ports_)) {
                portStrings << QString::number(port);
            }
            emit finished(0, portStrings.join(", "));
        }
    } else if (command->getStringId() == IPC::CliCommands::AmneziawgPresetsList::getCommandStringId()) {
        IPC::CliCommands::AmneziawgPresetsList *cmd = static_cast<IPC::CliCommands::AmneziawgPresetsList *>(command);
        if (cmd->presets_.isEmpty()) {
            emit finished(1, QString("No AmneziaWG configurations available."));
        } else {
            QString msg;
            for (const QString &preset : std::as_const(cmd->presets_)) {
                msg += preset + "\n";
            }
#ifdef CLI_ONLY
            msg += "\nTo use a preset, set these in the [Connection] section of ~/.config/Windscribe/windscribe_cli.conf,\n"
                   "with the name exactly as listed:\n"
                   "  [Connection]\n"
                   "  ProtocolTweaksMethod=1\n"
                   "  AmneziawgPreset=" + cmd->presets_.first() + "\n"
                   "With ProtocolTweaksMethod=0 (Auto) a preset is used only when Windscribe suggests one,\n"
                   "and with ProtocolTweaksMethod=2 (Disabled) never.";
#else
            msg += "\nTo use a preset, open Preferences > Connection > Anti-Censorship Settings in the app,\n"
                   "set Protocol Tweaks to Enabled, and choose the preset under Amnezia Config.";
#endif
            emit finished(0, msg);
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
        // The installer closes the app, which can happen before the app reports the launch or before the next poll reads it.
        if (isUpdateInstallerRunning_) {
            std::cout << std::endl;
            emit finished(0, QString("The app closed while the update installer was starting."));
            return;
        }
        emit finished(0, "");
    }
    else if (state == IPC::CONNECTION_ERROR) {
        if (ipcState_ == IPC_CONNECTING) {
            if (connectingTimer_.isValid() && connectingTimer_.elapsed() > MAX_WAIT_TIME_MS) {
                connectingTimer_.invalidate();
                // Another user's app may have started after main() checked, and made ours quit.
                const bool otherUser = SystemWideAppLock::holder() == SystemWideAppLock::Holder::kOtherUser;
                emit finished(1, otherUser ? appRunningInAnotherSessionString() : QString("Aborting: app did not start in time"));
            }
            else {
                // Try connect again. Delay is necessary so that Engine process will actually start
                // running on low resource systems.
                QTimer::singleShot(100, this, [this]() { connection_->connect(); } );
            }
        }
        else {
            emit finished(1, QString("Aborting: IPC communication error"));
        }
    }
}

void BackendCommander::sendCommand(IPC::CliCommands::State *state)
{
#ifndef CLI_ONLY
    // The app may still be restoring its saved session, e.g. when this CLI has just launched it. Wait for that to
    // finish rather than report "Not logged in". Logged out and login errors still fail at once below.
    if (isLoginRequired() && state->loginState_ == LOGIN_STATE_LOGGING_IN) {
        if (loggedInTimer_.elapsed() > MAX_LOGIN_TIME_MS) {
            emit finished(1, QString("Aborting: the app did not finish logging in within %1 seconds").arg(MAX_LOGIN_TIME_MS / 1000));
            return;
        }
        if (!bLoggingInMessageShown_) {
            bLoggingInMessageShown_ = true;
            emit report(QString("Waiting for the app to log in..."));
        }
        // Return before bCommandSent_ is set, so the next state reply comes back here.
        QTimer::singleShot(100, this, &BackendCommander::sendStateCommand);
        return;
    }
#endif

    if (cliArgs_.cliCommand() == CLI_COMMAND_CONNECT || cliArgs_.cliCommand() == CLI_COMMAND_CONNECT_BEST
            || cliArgs_.cliCommand() == CLI_COMMAND_CONNECT_LOCATION || cliArgs_.cliCommand() == CLI_COMMAND_CONNECT_STATIC) {
#ifdef CLI_ONLY
        if (!state->connectivity_) {
            emit finished(1, noConnectivityString());
            return;
        }
#endif

        if (state->loginState_ != LOGIN_STATE_LOGGED_IN) {
            emit finished(1, notLoggedInString());
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
        cmd.port_ = cliArgs_.port();
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_DISCONNECT) {
        if (state->loginState_ != LOGIN_STATE_LOGGED_IN) {
            emit finished(1, notLoggedInString());
            return;
        }
        if (state->connectState_.connectState == CONNECT_STATE_DISCONNECTED) {
            emit finished(0, QString("Already disconnected"));
            return;
        }
        IPC::CliCommands::Disconnect cmd;
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_FIREWALL_ON) {
        if (state->isFirewallOn_) {
            emit finished(0, QString("Firewall already on"));
            return;
        }
        IPC::CliCommands::Firewall cmd;
        cmd.isEnable_ = true;
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_FIREWALL_OFF) {
        if (!state->isFirewallOn_) {
            emit finished(0, QString("Firewall already off"));
            return;
        }
        if (state->isFirewallAlwaysOn_) {
            emit finished(1, QString("Firewall set to always on and can't be turned off"));
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
    else if (cliArgs_.cliCommand() == CLI_COMMAND_LOCATIONS || cliArgs_.cliCommand() == CLI_COMMAND_LOCATIONS_STATIC || cliArgs_.cliCommand() == CLI_COMMAND_LOCATIONS_FAV) {
        if (state->loginState_ != LOGIN_STATE_LOGGED_IN) {
            emit finished(1, notLoggedInString());
            return;
        }
        IPC::CliCommands::ShowLocations cmd;
        if (cliArgs_.cliCommand() == CLI_COMMAND_LOCATIONS) {
            cmd.locationType_ = IPC::CliCommands::LocationType::kRegular;
        } else if (cliArgs_.cliCommand() == CLI_COMMAND_LOCATIONS_STATIC) {
            cmd.locationType_ = IPC::CliCommands::LocationType::kStaticIp;
        } else {
            cmd.locationType_ = IPC::CliCommands::LocationType::kFavourite;
        }

        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_PORTS) {
        if (state->loginState_ != LOGIN_STATE_LOGGED_IN) {
            emit finished(1, notLoggedInString());
            return;
        }
        IPC::CliCommands::ShowPorts cmd;
        cmd.protocol_ = cliArgs_.protocol();
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_AMNEZIAWG) {
        IPC::CliCommands::ShowAmneziawg cmd;
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_HYDRA_SHOW || cliArgs_.cliCommand() == CLI_COMMAND_HYDRA_IMPORT
             || cliArgs_.cliCommand() == CLI_COMMAND_HYDRA_REMOVE) {
        IPC::CliCommands::SetHydraProfile cmd;
        cmd.remove_ = cliArgs_.cliCommand() == CLI_COMMAND_HYDRA_REMOVE;
        if (cliArgs_.cliCommand() == CLI_COMMAND_HYDRA_IMPORT) {
            // Read here, as the user running the CLI: the service may not be able to open the path.
            QFile file(cliArgs_.hydraFile());
            if (!file.open(QIODevice::ReadOnly)) {
                emit finished(1, QObject::tr("Could not read %1: %2").arg(cliArgs_.hydraFile(), file.errorString()));
                return;
            }
            cmd.json_ = file.read(HydraProfile::kMaxFileSize + 1);
            if (cmd.json_.isEmpty()) {
                emit finished(1, QObject::tr("%1 is empty.").arg(cliArgs_.hydraFile()));
                return;
            }
        }
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_LOGIN) {
#ifdef CLI_ONLY
        if (!state->connectivity_) {
            emit finished(1, noConnectivityString());
            return;
        }
#endif
        if (state->loginState_ == LOGIN_STATE_LOGGED_IN) {
            emit finished(0, QString("Already logged in"));
            return;
        }
        // Get missing credentials only now that we know a login is needed.  Without a terminal they can only come
        // from input piped to stdin, one per line; never prompt there, or read /dev/tty as readpassphrase() would.
        if (!Utils::isInteractive()) {
            if (username_.isEmpty()) {
                username_ = Utils::readLine();
            }
            if (password_.isEmpty()) {
                password_ = Utils::readLine();
            }
            if (username_.isEmpty() || password_.isEmpty()) {
                emit finished(1, QString("Username and password are required. Run \"windscribe-cli login <username> <password>\", pipe them to stdin one per line, or run \"windscribe-cli login\" in a terminal."));
                return;
            }
        } else {
            // Checked before any prompt, so the user isn't asked for a username only to be refused.
            if (password_.isEmpty() && !Utils::canDisableEcho()) {
                emit finished(1, QString("This terminal would show the password as it is typed. Pipe the credentials to stdin one per line, or run \"winpty windscribe-cli login\"."));
                return;
            }
            if (username_.isEmpty()) {
                username_ = Utils::getInput("Username: ", false);
            }
            if (password_.isEmpty()) {
                password_ = Utils::getInput("Password: ", true);
            }
        }
        IPC::CliCommands::Login cmd;
        cmd.username_ = username_;
        if (cmd.username_.size() < 3) {
            emit finished(1, QString("Username too short"));
            return;
        }
        // Checked here as well as in the GUI: the GUI refuses these without ever calling login, so the
        // only thing that reaches us is a state of LOGGED_OUT, which we would report as "Logged out".
        if (cmd.username_.contains("@")) {
            emit finished(1, QString("Username should not be an email address"));
            return;
        }
        cmd.password_ = password_;
        if (cmd.password_.size() < 8) {
            emit finished(1, QString("Password too short"));
            return;
        }
        cmd.code2fa_ = code2fa_;
        cmd.captchaSolution_ = captchaSolution_;
        captchaSolution_.clear();
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_LOGOUT) {
#ifdef CLI_ONLY
        if (!state->connectivity_) {
            emit finished(1, noConnectivityString());
            return;
        }
#endif
        if (state->loginState_ == LOGIN_STATE_LOGGED_OUT) {
            emit finished(0, QString("Already logged out"));
            return;
        }
        IPC::CliCommands::Logout cmd;
        cmd.isKeepFirewallOn_ = cliArgs_.keepFirewallOn();
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_SEND_LOGS) {
#ifdef CLI_ONLY
        if (!state->connectivity_) {
            emit finished(1, noConnectivityString());
            return;
        }
#endif
        IPC::CliCommands::SendLogs cmd;
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_UPDATE) {
#ifdef CLI_ONLY
        if (!state->connectivity_) {
            emit finished(1, noConnectivityString());
            return;
        }
#endif
        if (state->loginState_ != LOGIN_STATE_LOGGED_IN) {
            emit finished(1, notLoggedInString());
            return;
        }
        if (state->updateAvailable_.isEmpty()) {
            emit finished(1, QString("No update available"));
            return;
        }
        IPC::CliCommands::Update cmd;
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_SET_IGNORE_SSL_ERRORS) {
        if (cliArgs_.ignoreSslErrors() && state->loginError_ != wsnet::LoginResult::kSslError) {
            emit finished(1, QString("Ignoring SSL errors is only available when unable to log in."));
            return;
        }
        IPC::CliCommands::SetIgnoreSslErrors cmd;
        cmd.isEnable_ = cliArgs_.ignoreSslErrors();
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_IP_ROTATE) {
        if (state->loginState_ != LOGIN_STATE_LOGGED_IN) {
            emit finished(1, notLoggedInString());
            return;
        }
        if (state->connectState_.connectState != CONNECT_STATE_CONNECTED) {
            emit finished(1, QString("Not connected"));
            return;
        }
        IPC::CliCommands::RotateIp cmd;
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_IP_FAV) {
        if (state->loginState_ != LOGIN_STATE_LOGGED_IN) {
            emit finished(1, notLoggedInString());
            return;
        }
        if (state->connectState_.connectState != CONNECT_STATE_CONNECTED) {
            emit finished(1, QString("Not connected"));
            return;
        }
        IPC::CliCommands::FavIp cmd;
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_IP_UNFAV) {
        if (state->loginState_ != LOGIN_STATE_LOGGED_IN) {
            emit finished(1, notLoggedInString());
            return;
        }
        QString ipAddress = cliArgs_.ipAddress();
        if (!NetworkingValidation::isIp(ipAddress)) {
            emit finished(1, QString("Invalid IP address"));
            return;
        }
        IPC::CliCommands::UnfavIp cmd;
        cmd.ip_ = ipAddress;
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_STATUS) {
        sendStateCommand();
    }

    bCommandSent_ = true;
}

#ifndef CLI_ONLY
// The commands that sendCommand() refuses unless the app is logged in.
bool BackendCommander::isLoginRequired() const
{
    switch (cliArgs_.cliCommand()) {
    case CLI_COMMAND_CONNECT:
    case CLI_COMMAND_CONNECT_BEST:
    case CLI_COMMAND_CONNECT_LOCATION:
    case CLI_COMMAND_CONNECT_STATIC:
    case CLI_COMMAND_DISCONNECT:
    case CLI_COMMAND_LOCATIONS:
    case CLI_COMMAND_LOCATIONS_STATIC:
    case CLI_COMMAND_LOCATIONS_FAV:
    case CLI_COMMAND_PORTS:
    case CLI_COMMAND_UPDATE:
    case CLI_COMMAND_IP_ROTATE:
    case CLI_COMMAND_IP_FAV:
    case CLI_COMMAND_IP_UNFAV:
        return true;
    default:
        return false;
    }
}
#endif

void BackendCommander::sendStateCommand()
{
    IPC::CliCommands::GetState cmd;
    connection_->sendCommand(cmd);
}

void BackendCommander::onStateResponse(IPC::Command *command)
{
    IPC::CliCommands::State *cmd = static_cast<IPC::CliCommands::State *>(command);

    QList<QPair<QString, QString>> items;
    items << qMakePair(QString("Login"), loginStateString(cmd->loginState_, cmd->loginError_, cmd->loginErrorMessage_));

    if (cmd->loginState_ != LOGIN_STATE_LOGGED_OUT) {
        items << qMakePair(QString("Connection"), connectStateString(cmd->connectState_, cmd->location_, cmd->tunnelTestState_));
    }

    // While connecting or disconnecting, the reported address still belongs to the previous state.
    // Once connected, show it only after a successful connection check. A later check that fails
    // leaves the previous VPN address in place; labeling that address public is wrong, so omit it.
    const bool showIp = !cmd->myIp_.isEmpty() &&
                        (cmd->connectState_.connectState == CONNECT_STATE_DISCONNECTED ||
                         (cmd->connectState_.connectState == CONNECT_STATE_CONNECTED &&
                          cmd->tunnelTestState_ == TUNNEL_TEST_STATE_SUCCESS));

    if (cmd->connectState_.connectState == CONNECT_STATE_CONNECTED && cmd->protocol_.isValid()) {
        items << qMakePair(QString("Protocol"), protocolString(cmd->protocol_, cmd->port_));
    }

    if (showIp) {
        const bool isVpnIp = cmd->connectState_.connectState == CONNECT_STATE_CONNECTED;
        items << qMakePair(QString("IP"), ipString(cmd->myIp_, isVpnIp));
    }

    items << qMakePair(QString("Firewall"), firewallStateString(cmd->isFirewallOn_, cmd->isFirewallAlwaysOn_));

    if (cmd->loginState_ == LOGIN_STATE_LOGGED_IN) {
        items << qMakePair(QString("Data usage"), QString("%1 / %2")
            .arg(dataString(cmd->trafficUsed_))
            .arg((cmd->trafficMax_ == -1) ? QString("Unlimited") : dataString(cmd->trafficMax_)));
    }

    items << qMakePair(QString("Internet"), connectivityString(cmd->connectivity_));

    if (cmd->loginState_ == LOGIN_STATE_LOGGED_IN && !cmd->updateAvailable_.isEmpty()) {
        items << qMakePair(QString("Update"), QString("%1 available").arg(cmd->updateAvailable_));
    }

    const QString msg = statusString(items);
    emit finished(0, msg);
}

void BackendCommander::onUpdateStateResponse(IPC::Command *command)
{
    static int printedLength = 0;

    IPC::CliCommands::State *cmd = static_cast<IPC::CliCommands::State *>(command);

    // If an error has occurred, print it and quit
    if (cmd->updateError_ != UPDATE_VERSION_ERROR_NO_ERROR) {
        if (printedLength > 0) {
            std::cout << std::endl;
        }
        emit finished(1, updateErrorString(cmd->updateError_));
        return;
    }

    // Render 'Downloading n%' line
    for (int i = 0; i < printedLength; i++) {
        std::cout << '\b';
    }

    QString str = QString("Downloading: %1%").arg(cmd->updateProgress_);
    std::cout << str.toStdString();

    for (int i = str.toUtf8().size(); i < printedLength; i++) {
        std::cout << " ";
    }
    std::cout << std::flush;
    // the length is in bytes, not characters, because we need to 'backspace' this many times.
    printedLength = str.toUtf8().size();

#ifdef CLI_ONLY
    if (cmd->updateState_ == UPDATE_VERSION_STATE_RUNNING) {
        std::cout << std::endl;

        // Update has been downloaded, run it now
        int ret = QProcess::execute(WS_LINUX_INSTALL_DIR "/scripts/install-update", {cmd->updatePath_});
        std::error_code ec;
        std::filesystem::remove(cmd->updatePath_.toStdString(), ec);

        emit finished(ret, "");
        return;
    }
#else
    // The app runs the installer itself once the download is done, and reports when it has started it.
    isUpdateInstallerRunning_ = cmd->updateState_ == UPDATE_VERSION_STATE_RUNNING;
    if (cmd->updateState_ == UPDATE_VERSION_STATE_DONE) {
        std::cout << std::endl;
        emit finished(0, QString("The update installer has started."));
        return;
    }
#endif

    // If still in progress, wait 1 second and then request state again.
    std::this_thread::sleep_for(std::chrono::seconds(1));
    sendStateCommand();
}

void BackendCommander::onAcknowledge(IPC::CliCommands::Acknowledge *ackCmd)
{
    if (cliArgs_.cliCommand() == CLI_COMMAND_HYDRA_SHOW || cliArgs_.cliCommand() == CLI_COMMAND_HYDRA_IMPORT
        || cliArgs_.cliCommand() == CLI_COMMAND_HYDRA_REMOVE) {
        emit finished(ackCmd->code_, ackCmd->message_);
        return;
    }

    if (!cliArgs_.nonBlocking()) {
        sendStateCommand();
        return;
    }

    if (cliArgs_.cliCommand() == CLI_COMMAND_CONNECT || cliArgs_.cliCommand() == CLI_COMMAND_CONNECT_BEST
            || cliArgs_.cliCommand() == CLI_COMMAND_CONNECT_LOCATION || cliArgs_.cliCommand() == CLI_COMMAND_CONNECT_STATIC) {
        emit(finished(0, QString("Connecting. Run \"windscribe-cli status\" to check the connection status.")));
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_DISCONNECT) {
        emit(finished(0, QString("Disconnecting. Run \"windscribe-cli status\" to check the connection status.")));
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_FIREWALL_ON) {
        emit(finished(0, QString("Firewall is on.")));
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_FIREWALL_OFF) {
        emit(finished(0, QString("Firewall is off.")));
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_SET_KEYLIMIT_BEHAVIOR) {
        emit(finished(0, QString("Key limit behaviour is set.")));
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_SEND_LOGS) {
        if (ackCmd->code_ != 0) {
            emit(finished(ackCmd->code_, QString("Could not send logs.")));
        } else {
            emit(finished(0, QString("Logs sent.")));
        }
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_SET_IGNORE_SSL_ERRORS) {
        if (ackCmd->code_ != 0) {
            emit(finished(ackCmd->code_, ackCmd->message_));
        } else if (cliArgs_.ignoreSslErrors()) {
            emit(finished(0, QString("Ignoring SSL errors for this session.")));
        } else {
            emit(finished(0, QString("SSL errors will no longer be ignored.")));
        }
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_IP_ROTATE) {
        if (ackCmd->code_ == 2) {
            emit(finished(ackCmd->code_, QString("IP rotate already in progress.")));
        } else if (ackCmd->code_ != 0) {
            emit(finished(ackCmd->code_, QString("Could not rotate IP. Please check that you have Windscribe Pro or have this location in your plan, or try again later.")));
        } else {
            emit(finished(0, QString("IP rotated.")));
        }
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_IP_FAV) {
        if (ackCmd->code_ == 2) {
            emit(finished(ackCmd->code_, QString("The IP is still being rotated. Try again in a moment.")));
        } else if (ackCmd->code_ == 3) {
            emit(finished(1, QString("The current IP is not known yet. Try again once the connection check has succeeded.")));
        } else if (ackCmd->code_ != 0) {
            emit(finished(ackCmd->code_, QString("Could not favourite IP. Please check that you have Windscribe Pro or have this location in your plan, or try again later.")));
        } else {
            emit(finished(0, QString("IP favourited.")));
        }
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_IP_UNFAV) {
        if (ackCmd->code_ != 0) {
            emit(finished(ackCmd->code_, QString("Could not unfavourite IP. Please check that the provided IP is valid.")));
        } else {
            emit(finished(0, QString("IP unfavourited.")));
        }
    }
    else {
        emit(finished(0, ""));
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
        // Our own connect request was counted before the acknowledgement, so this first state is the baseline.
        if (!isConnectRequestCountKnown_) {
            isConnectRequestCountKnown_ = true;
            connectRequestCount_ = cmd->connectRequestCount_;
        }

        if (cmd->connectState_.connectState == CONNECT_STATE_DISCONNECTED && cmd->connectState_.connectError != ConnectError::kNoError ||
            cmd->connectState_.connectState == CONNECT_STATE_DISCONNECTED && cmd->connectState_.disconnectReason == DISCONNECTED_BY_KEY_LIMIT ||
            cmd->connectState_.connectState == CONNECT_STATE_DISCONNECTED && cmd->connectId_.isEmpty())
        {
            // Disconnected with error
            emit(finished(1, connectStateString(cmd->connectState_, cmd->location_, cmd->tunnelTestState_)));
        } else if (cmd->connectRequestCount_ != connectRequestCount_) {
            emit(finished(1, QString("Connection has been overridden by another command.")));
        } else if (cmd->connectState_.connectState == CONNECT_STATE_CONNECTED && cmd->tunnelTestState_ != TUNNEL_TEST_STATE_UNKNOWN) {
            // connected with a known tunnel test state
            emit(finished(0, connectStateString(cmd->connectState_, cmd->location_, cmd->tunnelTestState_)));
        } else if (cmd->connectState_.connectState == CONNECT_STATE_DISCONNECTED) {
            // Disconnected without an error, e.g. by the user, before this connection completed.
            emit(finished(1, QString("Connection has been overridden by another command.")));
        } else {
            str = connectStateString(cmd->connectState_, cmd->location_, cmd->tunnelTestState_).toStdString();
            if (str != prevStr) {
                std::cout << str << std::endl;
                prevStr = str;
            }
            // In progress: wait a short period before requesting status again
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            sendStateCommand();
        }
    } else if (cliArgs_.cliCommand() == CLI_COMMAND_DISCONNECT) {
        // The first state after the acknowledgement is the baseline.
        if (!isConnectRequestCountKnown_) {
            isConnectRequestCountKnown_ = true;
            connectRequestCount_ = cmd->connectRequestCount_;
        }
        // A connection still being set up may not have its ID yet when the baseline arrives; adopt it once it does.
        if (connectId_.isEmpty()) {
            connectId_ = cmd->connectId_;
        }

        // Only DISCONNECTED means done: the connection ID is cleared as soon as the tunnel is down, before the app
        // restores DNS and turns the firewall off.
        if (cmd->connectState_.connectState == CONNECT_STATE_DISCONNECTED) {
            emit(finished(0, connectStateString(cmd->connectState_, cmd->location_, cmd->tunnelTestState_)));
        } else if (cmd->connectRequestCount_ != connectRequestCount_ ||
                   (!cmd->connectId_.isEmpty() && cmd->connectId_ != connectId_)) {
            emit(finished(1, QString("Disconnection has been overridden by another command.")));
        } else {
            str = connectStateString(cmd->connectState_, cmd->location_, cmd->tunnelTestState_).toStdString();
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
            // Without a terminal, the code can only come from input piped to stdin, on the line after any credentials.
            const bool isInteractive = Utils::isInteractive();
            code2fa_ = isInteractive ? Utils::getInput("2FA code: ", false) : Utils::readLine();
            // An empty code would only be asked for again, forever.
            if (code2fa_.isEmpty()) {
                emit(finished(1, isInteractive ? QString("No 2FA code entered.")
                                               : QString("A 2FA code is required. Pipe it to stdin, or run \"windscribe-cli login\" in a terminal.")));
                return;
            }
            sendCommand(cmd);
#ifdef CLI_ONLY
        } else if (cmd->isCaptchaRequired_) {
            if (!Utils::isInteractive()) {
                emit(finished(1, QString("A CAPTCHA must be solved. Run \"windscribe-cli login\" in a terminal.")));
                return;
            }
            QString asciiArt = QByteArray::fromBase64(cmd->asciiArt_.toUtf8());
            std::cout << asciiArt.toStdString() << std::endl;
            captchaSolution_ = Utils::getInput(QString("Please type the numbers above to continue:"), false);
            if (captchaSolution_.isEmpty()) {
                emit(finished(1, QString("No CAPTCHA solution entered.")));
                return;
            }
            sendCommand(cmd);
#endif
        } else if (cmd->loginState_ == LOGIN_STATE_LOGGED_IN) {
            emit(finished(0, loginStateString(cmd->loginState_, cmd->loginError_, cmd->loginErrorMessage_)));
        } else if (cmd->loginState_ == LOGIN_STATE_LOGIN_ERROR || cmd->loginState_ == LOGIN_STATE_LOGGED_OUT) {
            // Both mean the login did not happen: the attempt failed, or it was abandoned, e.g. by closing the CAPTCHA.
            emit(finished(1, loginStateString(cmd->loginState_, cmd->loginError_, cmd->loginErrorMessage_)));
        } else {
            str = loginStateString(cmd->loginState_, cmd->loginError_, cmd->loginErrorMessage_).toStdString();
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
            emit(finished(0, loginStateString(cmd->loginState_, cmd->loginError_, cmd->loginErrorMessage_)));
        } else {
            str = QString("Logging out").toStdString();
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
