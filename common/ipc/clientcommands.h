#ifndef IPC_CLIENT_COMMANDS_H
#define IPC_CLIENT_COMMANDS_H

#include "command.h"
#include "types/enginesettings.h"

namespace IPC
{

namespace ClientCommands
{

class SetSettings : public Command
{
public:
    explicit SetSettings(const types::EngineSettings &es) : es_(es)  {}

    std::vector<char> getData() const override
    {
        Q_ASSERT(false);
        return std::vector<char>();
    }

    std::string getStringId() const override
    {
        return getCommandStringId();
    }

    std::string getDebugString() const override
    {
        return "SetSettings debug string";
    }

    static std::string getCommandStringId()
    {
        return "SetSettings";
    }

    const types::EngineSettings &getEngineSettings() const
    {
        return es_;
    }

private:
    types::EngineSettings es_;
};


} // namespace ClientCommands
} // namespace IPC

#endif // IPC_CLIENT_COMMANDS_H
