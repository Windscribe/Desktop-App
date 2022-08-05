#ifndef IPC_CLI_COMMANDS_H
#define IPC_CLI_COMMANDS_H

#include "command.h"
#include "types/enginesettings.h"
#include "types/locationid.h"
#include "types/splittunneling.h"

namespace IPC
{

namespace CliCommands
{

class Connect : public Command
{
public:
    Connect() {}
    explicit Connect(char *buf, int size) {}

    std::vector<char> getData() const override
    {
        Q_ASSERT(false);
        return std::vector<char>();
    }

    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "CliCommands::Connect debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::Connect";  }

    QString location_;
};

class Disconnect : public Command
{
public:
    Disconnect() {}
    explicit Disconnect(char *buf, int size) {}

    std::vector<char> getData() const override
    {
        Q_ASSERT(false);
        return std::vector<char>();
    }

    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "CliCommands::Disconnect debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::Disconnect";  }
};

class ShowLocations : public Command
{
public:
    ShowLocations() {}
    explicit ShowLocations(char *buf, int size) {}

    std::vector<char> getData() const override
    {
        Q_ASSERT(false);
        return std::vector<char>();
    }

    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "CliCommands::ShowLocations debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::ShowLocations";  }
};

class Firewall : public Command
{
public:
    Firewall() {}
    explicit Firewall(char *buf, int size) {}

    std::vector<char> getData() const override
    {
        Q_ASSERT(false);
        return std::vector<char>();
    }

    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "CliCommands::Firewall debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::Firewall";  }

    bool isEnable_ = false;
};

class GetState : public Command
{
public:
    GetState() {}
    explicit GetState(char *buf, int size) {}

    std::vector<char> getData() const override
    {
        Q_ASSERT(false);
        return std::vector<char>();
    }

    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "CliCommands::GetState debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::GetState";  }
};

class Login : public Command
{
public:
    Login() {}
    explicit Login(char *buf, int size) {}

    std::vector<char> getData() const override
    {
        Q_ASSERT(false);
        return std::vector<char>();
    }

    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "CliCommands::Login debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::Login";  }

    QString username_;
    QString password_;
    QString code2fa_;
};

class SignOut : public Command
{
public:
    SignOut() {}
    explicit SignOut(char *buf, int size) {}

    std::vector<char> getData() const override
    {
        Q_ASSERT(false);
        return std::vector<char>();
    }

    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "CliCommands::SignOut debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::SignOut";  }

    bool isKeepFirewallOn_;
};

class ConnectToLocationAnswer : public Command
{
public:
    ConnectToLocationAnswer() {}
    explicit ConnectToLocationAnswer(char *buf, int size) {}

    std::vector<char> getData() const override
    {
        Q_ASSERT(false);
        return std::vector<char>();
    }

    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "CliCommands::ConnectToLocationAnswer debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::ConnectToLocationAnswer";  }

    bool isSuccess_;
    QString location_;
};

class ConnectStateChanged : public Command
{
public:
    ConnectStateChanged() {}
    explicit ConnectStateChanged(char *buf, int size) {}

    std::vector<char> getData() const override
    {
        Q_ASSERT(false);
        return std::vector<char>();
    }

    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "CliCommands::ConnectStateChanged debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::ConnectStateChanged";  }
};

class FirewallStateChanged : public Command
{
public:
    FirewallStateChanged() {}
    explicit FirewallStateChanged(char *buf, int size) {}

    std::vector<char> getData() const override
    {
        Q_ASSERT(false);
        return std::vector<char>();
    }

    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "CliCommands::FirewallStateChanged debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::FirewallStateChanged";  }

    bool isFirewallEnabled_;
    bool isFirewallAlwaysOn_;
};

class LocationsShown : public Command
{
public:
    LocationsShown() {}
    explicit LocationsShown(char *buf, int size) {}

    std::vector<char> getData() const override
    {
        Q_ASSERT(false);
        return std::vector<char>();
    }

    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "CliCommands::LocationsShown debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::LocationsShown";  }
};

class AlreadyDisconnected : public Command
{
public:
    AlreadyDisconnected() {}
    explicit AlreadyDisconnected(char *buf, int size) {}

    std::vector<char> getData() const override
    {
        Q_ASSERT(false);
        return std::vector<char>();
    }

    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "CliCommands::AlreadyDisconnected debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::AlreadyDisconnected";  }
};

class State : public Command
{
public:
    State() {}
    explicit State(char *buf, int size) {}

    std::vector<char> getData() const override
    {
        Q_ASSERT(false);
        return std::vector<char>();
    }

    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "CliCommands::State debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::State";  }

    bool isLoggedIn_ = false;
    bool waitingForLoginInfo_ = false;
};

class LoginResult : public Command
{
public:
    LoginResult() {}
    explicit LoginResult(char *buf, int size) {}

    std::vector<char> getData() const override
    {
        Q_ASSERT(false);
        return std::vector<char>();
    }

    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "CliCommands::LoginResult debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::LoginResult";  }

    bool isLoggedIn_;
    QString loginError_;
};

class SignedOut : public Command
{
public:
    SignedOut() {}
    explicit SignedOut(char *buf, int size) {}

    std::vector<char> getData() const override
    {
        Q_ASSERT(false);
        return std::vector<char>();
    }

    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "CliCommands::SignedOut debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::SignedOut";  }
};

} // namespace CliCommands
} // namespace IPC

#endif // IPC_CLI_COMMANDS_H
