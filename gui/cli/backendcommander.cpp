#include "backendcommander.h"

#include "utils/utils.h"
#include "utils/logger.h"
#include "types/locationid.h"
#include "ipc/connection.h"
#include "ipc/protobufcommand.h"

#include <QTimer>

BackendCommander::BackendCommander(const CliArguments &cliArgs) : QObject()
    , cliArgs_(cliArgs)
{
    unsigned long cliPid = Utils::getCurrentPid();
    qCDebug(LOG_BASIC) << "CLI pid: " << cliPid;
}

BackendCommander::~BackendCommander()
{
    if (connection_)
    {
        connection_->close();
        delete connection_;
    }
}

void BackendCommander::initAndSend(bool isGuiAlreadyRunning)
{
    isGuiAlreadyRunning_ = isGuiAlreadyRunning;
    connection_ = new IPC::Connection();
    connect(dynamic_cast<QObject*>(connection_), SIGNAL(newCommand(IPC::Command *, IPC::IConnection *)), SLOT(onConnectionNewCommand(IPC::Command *, IPC::IConnection *)), Qt::QueuedConnection);
    connect(dynamic_cast<QObject*>(connection_), SIGNAL(stateChanged(int, IPC::IConnection *)), SLOT(onConnectionStateChanged(int, IPC::IConnection *)), Qt::QueuedConnection);
    connectingTimer_.start();
    ipcState_ = IPC_CONNECTING;
    connection_->connect();
}

void BackendCommander::onConnectionNewCommand(IPC::Command *command, IPC::IConnection * /*connection*/)
{
    if (bCommandSent_ && command->getStringId() == CliIpc::LocationsShown::descriptor()->full_name())
    {
        emit finished(tr("Viewing Locations..."));
    }
    else if (bCommandSent_ && command->getStringId() == CliIpc::ConnectToLocationAnswer::descriptor()->full_name())
    {
        IPC::ProtobufCommand<CliIpc::ConnectToLocationAnswer> *cmd = static_cast<IPC::ProtobufCommand<CliIpc::ConnectToLocationAnswer> *>(command);

        if (cmd->getProtoObj().is_success())
        {
            qCDebug(LOG_BASIC) << "Connecting to" << QString::fromStdString(cmd->getProtoObj().location());
            emit report("Connecting to " + QString::fromStdString(cmd->getProtoObj().location()));
        }
        else
        {
            emit finished(tr("Error: Could not find server matching: \"") + cliArgs_.location() + "\"");
        }
    }
    else if (bCommandSent_ && command->getStringId() == CliIpc::ConnectStateChanged::descriptor()->full_name())
    {
        IPC::ProtobufCommand<CliIpc::ConnectStateChanged> *cmd = static_cast<IPC::ProtobufCommand<CliIpc::ConnectStateChanged> *>(command);

        if (cliArgs_.cliCommand() >= CLI_COMMAND_CONNECT && cliArgs_.cliCommand() <= CLI_COMMAND_DISCONNECT)
        {
            if (cmd->getProtoObj().connect_state().connect_state_type() == ProtoTypes::CONNECTED)
            {
                if (cmd->getProtoObj().connect_state().has_location())
                {
                    LocationID currentLocation = LocationID::createFromProtoBuf(cmd->getProtoObj().connect_state().location());

                    QString locationConnectedTo;
                    if (currentLocation.isBestLocation())
                    {
                        locationConnectedTo = tr("Best Location");
                    }
                    else
                    {
                        locationConnectedTo = QString::fromStdString(cmd->getProtoObj().connect_state().location().city());
                    }
                    emit finished(tr("Connected to ") + locationConnectedTo);
                }
            }
            else if (cmd->getProtoObj().connect_state().connect_state_type() == ProtoTypes::DISCONNECTED)
            {
                emit finished(tr("Disconnected"));
            }
        }
    }
    else if (bCommandSent_ && command->getStringId() == CliIpc::AlreadyDisconnected::descriptor()->full_name())
    {
        emit finished(tr("Already Disconnected"));
    }
    else if (command->getStringId() == CliIpc::State::descriptor()->full_name())
    {
        onStateResponse(command);
    }
    else if (command->getStringId() == CliIpc::FirewallStateChanged::descriptor()->full_name())
    {
        IPC::ProtobufCommand<CliIpc::FirewallStateChanged> *cmd = static_cast<IPC::ProtobufCommand<CliIpc::FirewallStateChanged> *>(command);
        if (cmd->getProtoObj().is_firewall_enabled())
        {
            if (cmd->getProtoObj().is_firewall_always_on())
            {
                emit finished(tr("Firewall is in Always On mode and cannot be changed"));
            }
            else
            {
                emit finished(tr("Firewall is ON"));
            }
        }
        else
        {
            emit finished(tr("Firewall is OFF"));
        }
    }
    else if (bCommandSent_ && command->getStringId() == CliIpc::SignedOut::descriptor()->full_name())
    {
        emit finished(tr("Signed out"));
    }
    else if (bCommandSent_ && command->getStringId() == CliIpc::LoginResult::descriptor()->full_name())
    {
        IPC::ProtobufCommand<CliIpc::LoginResult> *cmd = static_cast<IPC::ProtobufCommand<CliIpc::LoginResult> *>(command);
        if (cmd->getProtoObj().is_logged_in()) {
            emit finished(tr("login successful"));
        }
        else
        {
            QString errorMessage(tr("login failed"));
            if (!cmd->getProtoObj().login_error().empty()) {
                errorMessage += tr(". %1").arg(QString::fromStdString(cmd->getProtoObj().login_error()));
            }
            emit finished(errorMessage);
        }
    }
}

void BackendCommander::onConnectionStateChanged(int state, IPC::IConnection * /*connection*/)
{
    if (state == IPC::CONNECTION_CONNECTED)
    {
        qCDebug(LOG_BASIC) << "Connected to GUI server";
        ipcState_ = IPC_CONNECTED;
        loggedInTimer_.start();
        sendStateCommand();
    }
    else if (state == IPC::CONNECTION_DISCONNECTED)
    {
        qCDebug(LOG_BASIC) << "Disconnected from GUI server";
        emit finished("");
    }
    else if (state == IPC::CONNECTION_ERROR)
    {
        if (ipcState_ == IPC_CONNECTING)
        {
            if (connectingTimer_.isValid() && connectingTimer_.elapsed() > MAX_WAIT_TIME_MS)
            {
                connectingTimer_.invalidate();
                emit finished("Aborting: Gui did not start in time");
            }
            else
            {
               // Try connect again. Delay is necessary so that Engine process will actually start
               // running on low resource systems.
                QTimer::singleShot(100, [this]() { connection_->connect(); } );
            }
        }
        else
        {
            emit finished("Aborting: IPC communication error");
        }
    }
}

void BackendCommander::sendCommand()
{
    if (cliArgs_.cliCommand() == CLI_COMMAND_CONNECT || cliArgs_.cliCommand() == CLI_COMMAND_CONNECT_BEST || cliArgs_.cliCommand() == CLI_COMMAND_CONNECT_LOCATION)
    {
        qCDebug(LOG_BASIC) << "Connecting to last";

        IPC::ProtobufCommand<CliIpc::Connect> cmd;
        cmd.getProtoObj().set_location(cliArgs_.location().toStdString());
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_DISCONNECT)
    {
        IPC::ProtobufCommand<CliIpc::Disconnect> cmd;
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_FIREWALL_ON)
    {
        IPC::ProtobufCommand<CliIpc::Firewall> cmd;
        cmd.getProtoObj().set_is_enable(true);
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_FIREWALL_OFF)
    {
        IPC::ProtobufCommand<CliIpc::Firewall> cmd;
        cmd.getProtoObj().set_is_enable(false);
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_LOCATIONS)
    {
        IPC::ProtobufCommand<CliIpc::ShowLocations> cmd;
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_LOGIN)
    {
        IPC::ProtobufCommand<CliIpc::Login> cmd;
        cmd.getProtoObj().set_username(cliArgs_.username().toStdString());
        cmd.getProtoObj().set_password(cliArgs_.password().toStdString());
        cmd.getProtoObj().set_code2fa(cliArgs_.code2fa().toStdString());
        connection_->sendCommand(cmd);
    }
    else if (cliArgs_.cliCommand() == CLI_COMMAND_SIGN_OUT)
    {
        IPC::ProtobufCommand<CliIpc::SignOut> cmd;
        cmd.getProtoObj().set_is_keep_firewall_on(cliArgs_.keepFirewallOn());
        connection_->sendCommand(cmd);
    }

    bCommandSent_ = true;
}

void BackendCommander::sendStateCommand()
{
    IPC::ProtobufCommand<CliIpc::GetState> cmd;
    connection_->sendCommand(cmd);
}

void BackendCommander::onStateResponse(IPC::Command *command)
{
    IPC::ProtobufCommand<CliIpc::State> *cmd = static_cast<IPC::ProtobufCommand<CliIpc::State> *>(command);

    if (cmd->getProtoObj().is_logged_in())
    {
        if (cliArgs_.cliCommand() == CLI_COMMAND_LOGIN) {
            emit finished(tr("The application is already logged in"));
        }
        else {
            sendCommand();
        }
    }
    else
    {
        if (cliArgs_.cliCommand() == CLI_COMMAND_LOGIN && cmd->getProtoObj().waiting_for_login_info())
        {
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
        else if (cliArgs_.cliCommand() == CLI_COMMAND_SIGN_OUT && cmd->getProtoObj().waiting_for_login_info())
        {
            loggedInTimer_.invalidate();
            emit finished(tr("The application is already signed out"));
        }
        else
        {
            if (loggedInTimer_.isValid() && loggedInTimer_.elapsed() > MAX_LOGIN_TIME_MS)
            {
                loggedInTimer_.invalidate();
                emit finished("Aborting: GUI did not login in time");
            }
            else
            {
                if (!bLogginInMessageShown_)
                {
                    bLogginInMessageShown_ = true;
                    emit report("GUI is not logged in. Waiting for the login...");
                }
                QTimer::singleShot(100, this, &BackendCommander::sendStateCommand);
            }
        }
    }
}
