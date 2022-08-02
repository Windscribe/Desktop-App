#ifndef COMMANDS_H
#define COMMANDS_H

#include "command.h"
#include "types/sessionstatus.h"

namespace IPC
{

class SessionStatusUpdated : public Command
{
public:
    explicit SessionStatusUpdated(const types::SessionStatus &ss) : ss_(ss)  {}

    std::vector<char> getData() const override
    {
        QByteArray arr;
        QDataStream ds(arr);
        ds << ss_;
        return std::vector<char>(arr.data(),  arr.data() + arr.size());
    }

    std::string getStringId() const override
    {
        return getCommandStringId();
    }

    std::string getDebugString() const override
    {
        return "SessionStatusUpdated debug string";
    }

    static std::string getCommandStringId()
    {
        return "SessionStatusUpdated";
    }

    const types::SessionStatus &getSessionStatus() const
    {
        return ss_;
    }

private:
    types::SessionStatus ss_;
};


} // namespace IPC

#endif // COMMANDS_H
