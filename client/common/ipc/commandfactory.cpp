#include <QObject>
#include <QDebug>

#include "commandfactory.h"
#include "clicommands.h"
#include "utils/ws_assert.h"

namespace IPC
{

Command *CommandFactory::makeCommand(const std::string strId, char *buf, int size)
{
    qDebug() << QString::fromStdString(strId);

    // CLI commands
    if (strId == IPC::CliCommands::Connect::getCommandStringId())
    {
        return new IPC::CliCommands::Connect(buf, size);
    }
    else if (strId == IPC::CliCommands::ConnectToLocationAnswer::getCommandStringId())
    {
        return new IPC::CliCommands::ConnectToLocationAnswer(buf, size);
    }
    else if (strId == IPC::CliCommands::ConnectStateChanged::getCommandStringId())
    {
        return new IPC::CliCommands::ConnectStateChanged(buf, size);
    }
    else if (strId == IPC::CliCommands::Disconnect::getCommandStringId())
    {
        return new IPC::CliCommands::Disconnect(buf, size);
    }
    else if (strId == IPC::CliCommands::AlreadyDisconnected::getCommandStringId())
    {
        return new IPC::CliCommands::AlreadyDisconnected(buf, size);
    }
    else if (strId == IPC::CliCommands::ShowLocations::getCommandStringId())
    {
        return new IPC::CliCommands::ShowLocations(buf, size);
    }
    else if (strId == IPC::CliCommands::LocationsShown::getCommandStringId())
    {
        return new IPC::CliCommands::LocationsShown(buf, size);
    }
    else if (strId == IPC::CliCommands::GetState::getCommandStringId())
    {
        return new IPC::CliCommands::GetState(buf, size);
    }
    else if (strId == IPC::CliCommands::State::getCommandStringId())
    {
        return new IPC::CliCommands::State(buf, size);
    }
    else if (strId == IPC::CliCommands::Firewall::getCommandStringId())
    {
        return new IPC::CliCommands::Firewall(buf, size);
    }
    else if (strId == IPC::CliCommands::FirewallStateChanged::getCommandStringId())
    {
        return new IPC::CliCommands::FirewallStateChanged(buf, size);
    }
    else if (strId == IPC::CliCommands::Login::getCommandStringId())
    {
        return new IPC::CliCommands::Login(buf, size);
    }
    else if (strId == IPC::CliCommands::LoginResult::getCommandStringId())
    {
        return new IPC::CliCommands::LoginResult(buf, size);
    }
    else if (strId == IPC::CliCommands::SignOut::getCommandStringId())
    {
        return new IPC::CliCommands::SignOut(buf, size);
    }
    else if (strId == IPC::CliCommands::SignedOut::getCommandStringId())
    {
        return new IPC::CliCommands::SignedOut(buf, size);
    }

    WS_ASSERT(false);
    return NULL;
}

} // namespace IPC
