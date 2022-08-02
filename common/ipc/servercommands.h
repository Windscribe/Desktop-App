#ifndef IPC_SERVER_COMMANDS_H
#define IPC_SERVER_COMMANDS_H

#include "command.h"
#include "types/sessionstatus.h"
#include "types/enums.h"
#include "types/enginesettings.h"

namespace IPC
{

namespace ServerCommands
{

class SessionStatusUpdated : public Command
{
public:
    explicit SessionStatusUpdated(const types::SessionStatus &ss) : ss_(ss)  {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
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

// reply on successfully auth command
class AuthReply : public Command
{
public:
    explicit AuthReply()  {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "AuthReply debug string";
    }

    static std::string getCommandStringId()
    {
        return "AuthReply";
    }
};


class InitFinished : public Command
{
public:
    explicit InitFinished(INIT_STATE initState, const types::EngineSettings &engineSettings, const QStringList &availableOpenvpnVersions,
                          bool isWifiSharingSupported, bool isSavedApiSettingsExists, const QString &authHash) :
        initState_(initState), engineSettings_(engineSettings), availableOpenvpnVersions_(availableOpenvpnVersions), isWifiSharingSupported_(isWifiSharingSupported),
        isSavedApiSettingsExists_(isSavedApiSettingsExists), authHash_(authHash)
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "InitFinished debug string";
    }
    static std::string getCommandStringId()
    {
        return "InitFinished";
    }
    INIT_STATE initState_;
    types::EngineSettings engineSettings_;
    QStringList availableOpenvpnVersions_;
    bool isWifiSharingSupported_;
    bool isSavedApiSettingsExists_;
    QString authHash_;
};

class EngineSettingsChanged : public Command
{
public:
    explicit EngineSettingsChanged(const types::EngineSettings &engineSettings) : engineSettings_(engineSettings)  {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "EngineSettingsChanged debug string";
    }

    static std::string getCommandStringId()
    {
        return "EngineSettingsChanged";
    }
    types::EngineSettings engineSettings_;
};


} // namespace ServerCommands
} // namespace IPC

#endif // IPC_SERVER_COMMANDS_H
