#include "backendcommander.h"

#include "utils/utils.h"
#include "utils/logger.h"
#include "types/locationid.h"
#include "ipc/connection.h"
#include "ipc/protobufcommand.h"

#include <QTimer>

BackendCommander::BackendCommander(CliCommand cmd, const QString &location) : QObject()
    , ipcState_(IPC_INIT_STATE)
    , connection_(nullptr)
    , command_(cmd)
    , locationStr_(location)
    , bCommandSent_(false)
    , bLogginInMessageShown_(false)
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

void BackendCommander::initAndSend()
{
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
            emit finished(tr("Error: Could not find server matching: \"") + locationStr_ + "\"");
        }
    }
    else if (bCommandSent_ && command->getStringId() == CliIpc::ConnectStateChanged::descriptor()->full_name())
    {
        IPC::ProtobufCommand<CliIpc::ConnectStateChanged> *cmd = static_cast<IPC::ProtobufCommand<CliIpc::ConnectStateChanged> *>(command);


        if (command_ >= CLI_COMMAND_CONNECT && command_ <= CLI_COMMAND_DISCONNECT)
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
        IPC::ProtobufCommand<CliIpc::State> *cmd = static_cast<IPC::ProtobufCommand<CliIpc::State> *>(command);
        if (cmd->getProtoObj().is_logged_in())
        {
            sendCommand();
        }
        else
        {
            if (loggedInTimer_.isValid() && loggedInTimer_.elapsed() > MAX_LOGIN_TIME_MS)
            {
                loggedInTimer_.invalidate();
                emit finished("Aborting: Gui did not login in time");
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
    if (command_ == CLI_COMMAND_CONNECT || command_ == CLI_COMMAND_CONNECT_BEST || command_ == CLI_COMMAND_CONNECT_LOCATION)
    {
        qCDebug(LOG_BASIC) << "Connecting to last";

        IPC::ProtobufCommand<CliIpc::Connect> cmd;
        cmd.getProtoObj().set_location(locationStr_.toStdString());
        connection_->sendCommand(cmd);
    }
    else if (command_ == CLI_COMMAND_DISCONNECT)
    {
        IPC::ProtobufCommand<CliIpc::Disconnect> cmd;
        connection_->sendCommand(cmd);
    }
    else if (command_ == CLI_COMMAND_FIREWALL_ON)
    {
        IPC::ProtobufCommand<CliIpc::Firewall> cmd;
        cmd.getProtoObj().set_is_enable(true);
        connection_->sendCommand(cmd);
    }
    else if (command_ == CLI_COMMAND_FIREWALL_OFF)
    {
        IPC::ProtobufCommand<CliIpc::Firewall> cmd;
        cmd.getProtoObj().set_is_enable(false);
        connection_->sendCommand(cmd);
    }
    else if (command_ == CLI_COMMAND_LOCATIONS)
    {
        IPC::ProtobufCommand<CliIpc::ShowLocations> cmd;
        connection_->sendCommand(cmd);
    }
    bCommandSent_ = true;
}

void BackendCommander::sendStateCommand()
{
    IPC::ProtobufCommand<CliIpc::GetState> cmd;
    connection_->sendCommand(cmd);
}

