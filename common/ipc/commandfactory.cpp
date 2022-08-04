#include <QObject>
#include <QDebug>

#include "commandfactory.h"
#include "protobufcommand.h"
#include "utils/protobuf_includes.h"

namespace IPC
{

Command *CommandFactory::makeCommand(const std::string strId, char *buf, int size)
{
    qDebug() << QString::fromStdString(strId);

    // CLI commands
    if (strId == CliIpc::Connect::descriptor()->full_name())
    {
        return new ProtobufCommand<CliIpc::Connect>(buf, size);
    }
    else if (strId == CliIpc::ConnectToLocationAnswer::descriptor()->full_name())
    {
        return new ProtobufCommand<CliIpc::ConnectToLocationAnswer>(buf, size);
    }
    else if (strId == CliIpc::ConnectStateChanged::descriptor()->full_name())
    {
        return new ProtobufCommand<CliIpc::ConnectStateChanged>(buf, size);
    }
    else if (strId == CliIpc::Disconnect::descriptor()->full_name())
    {
        return new ProtobufCommand<CliIpc::Disconnect>(buf, size);
    }
    else if (strId == CliIpc::AlreadyDisconnected::descriptor()->full_name())
    {
        return new ProtobufCommand<CliIpc::AlreadyDisconnected>(buf, size);
    }
    else if (strId == CliIpc::ShowLocations::descriptor()->full_name())
    {
        return new ProtobufCommand<CliIpc::ShowLocations>(buf, size);
    }
    else if (strId == CliIpc::LocationsShown::descriptor()->full_name())
    {
        return new ProtobufCommand<CliIpc::LocationsShown>(buf, size);
    }
    else if (strId == CliIpc::GetState::descriptor()->full_name())
    {
        return new ProtobufCommand<CliIpc::GetState>(buf, size);
    }
    else if (strId == CliIpc::State::descriptor()->full_name())
    {
        return new ProtobufCommand<CliIpc::State>(buf, size);
    }
    else if (strId == CliIpc::Firewall::descriptor()->full_name())
    {
        return new ProtobufCommand<CliIpc::Firewall>(buf, size);
    }
    else if (strId == CliIpc::FirewallStateChanged::descriptor()->full_name())
    {
        return new ProtobufCommand<CliIpc::FirewallStateChanged>(buf, size);
    }
    else if (strId == CliIpc::Login::descriptor()->full_name())
    {
        return new ProtobufCommand<CliIpc::Login>(buf, size);
    }
    else if (strId == CliIpc::LoginResult::descriptor()->full_name())
    {
        return new ProtobufCommand<CliIpc::LoginResult>(buf, size);
    }
    else if (strId == CliIpc::SignOut::descriptor()->full_name())
    {
        return new ProtobufCommand<CliIpc::SignOut>(buf, size);
    }
    else if (strId == CliIpc::SignedOut::descriptor()->full_name())
    {
        return new ProtobufCommand<CliIpc::SignedOut>(buf, size);
    }


    Q_ASSERT(false);
    return NULL;
}

} // namespace IPC
