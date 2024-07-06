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
    if (strId == IPC::CliCommands::Acknowledge::getCommandStringId()) {
        return new IPC::CliCommands::Acknowledge(buf, size);
    } else if (strId == IPC::CliCommands::Connect::getCommandStringId()) {
        return new IPC::CliCommands::Connect(buf, size);
    } else if (strId == IPC::CliCommands::Disconnect::getCommandStringId()) {
        return new IPC::CliCommands::Disconnect(buf, size);
    } else if (strId == IPC::CliCommands::ShowLocations::getCommandStringId()) {
        return new IPC::CliCommands::ShowLocations(buf, size);
    } else if (strId == IPC::CliCommands::LocationsList::getCommandStringId()) {
        return new IPC::CliCommands::LocationsList(buf, size);
    } else if (strId == IPC::CliCommands::GetState::getCommandStringId()) {
        return new IPC::CliCommands::GetState(buf, size);
    } else if (strId == IPC::CliCommands::State::getCommandStringId()) {
        return new IPC::CliCommands::State(buf, size);
    } else if (strId == IPC::CliCommands::Firewall::getCommandStringId()) {
        return new IPC::CliCommands::Firewall(buf, size);
    } else if (strId == IPC::CliCommands::Login::getCommandStringId()) {
        return new IPC::CliCommands::Login(buf, size);
    } else if (strId == IPC::CliCommands::Logout::getCommandStringId()) {
        return new IPC::CliCommands::Logout(buf, size);
    } else if (strId == IPC::CliCommands::Update::getCommandStringId()) {
        return new IPC::CliCommands::Update(buf, size);
    } else if (strId == IPC::CliCommands::SendLogs::getCommandStringId()) {
        return new IPC::CliCommands::SendLogs(buf, size);
    } else if (strId == IPC::CliCommands::ReloadConfig::getCommandStringId()) {
        return new IPC::CliCommands::ReloadConfig(buf, size);
    }

    WS_ASSERT(false);
    return NULL;
}

} // namespace IPC
