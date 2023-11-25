#pragma once

#include "command.h"
#include "types/enginesettings.h"
#include "types/connectstate.h"

namespace IPC
{

namespace CliCommands
{

class Connect : public Command
{
public:
    Connect() {}
    explicit Connect(char *buf, int size)
    {
        QByteArray arr(buf, size);
        QDataStream ds(&arr, QIODevice::ReadOnly);
        ds >> location_;
    }

    std::vector<char> getData() const override
    {
        QByteArray arr;
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << location_;
        return std::vector<char>(arr.begin(), arr.end());
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
    explicit Disconnect(char *buf, int size)
    {
        Q_UNUSED(buf)
        Q_UNUSED(size)
    }

    std::vector<char> getData() const override
    {
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
    explicit ShowLocations(char *buf, int size)
    {
        Q_UNUSED(buf)
        Q_UNUSED(size)
    }

    std::vector<char> getData() const override
    {
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
    explicit Firewall(char *buf, int size)
    {
        QByteArray arr(buf, size);
        QDataStream ds(&arr, QIODevice::ReadOnly);
        ds >> isEnable_;
    }

    std::vector<char> getData() const override
    {
        QByteArray arr;
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << isEnable_;
        return std::vector<char>(arr.begin(), arr.end());
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
    explicit GetState(char *buf, int size)
    {
        Q_UNUSED(buf)
        Q_UNUSED(size)
    }

    std::vector<char> getData() const override
    {
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
    explicit Login(char *buf, int size)
    {
        QByteArray arr(buf, size);
        QDataStream ds(&arr, QIODevice::ReadOnly);
        ds >> username_ >> password_ >> code2fa_;
    }

    std::vector<char> getData() const override
    {
        QByteArray arr;
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << username_ << password_ << code2fa_;
        return std::vector<char>(arr.begin(), arr.end());
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
    explicit SignOut(char *buf, int size)
    {
        QByteArray arr(buf, size);
        QDataStream ds(&arr, QIODevice::ReadOnly);
        ds >> isKeepFirewallOn_;
    }

    std::vector<char> getData() const override
    {
        QByteArray arr;
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << isKeepFirewallOn_;
        return std::vector<char>(arr.begin(), arr.end());
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
    explicit ConnectToLocationAnswer(char *buf, int size)
    {
        QByteArray arr(buf, size);
        QDataStream ds(&arr, QIODevice::ReadOnly);
        ds >> isSuccess_ >> location_;
    }

    std::vector<char> getData() const override
    {
        QByteArray arr;
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << isSuccess_ << location_;
        return std::vector<char>(arr.begin(), arr.end());
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
    explicit ConnectStateChanged(char *buf, int size)
    {
        QByteArray arr(buf, size);
        QDataStream ds(&arr, QIODevice::ReadOnly);
        ds >> connectState;
    }

    std::vector<char> getData() const override
    {
        QByteArray arr;
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << connectState;
        return std::vector<char>(arr.begin(), arr.end());
    }

    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "CliCommands::ConnectStateChanged debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::ConnectStateChanged";  }

    types::ConnectState connectState;
};

class FirewallStateChanged : public Command
{
public:
    FirewallStateChanged() {}
    explicit FirewallStateChanged(char *buf, int size)
    {
        QByteArray arr(buf, size);
        QDataStream ds(&arr, QIODevice::ReadOnly);
        ds >> isFirewallEnabled_ >> isFirewallAlwaysOn_;
    }

    std::vector<char> getData() const override
    {
        QByteArray arr;
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << isFirewallEnabled_ << isFirewallAlwaysOn_;
        return std::vector<char>(arr.begin(), arr.end());
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
    explicit LocationsShown(char *buf, int size)
    {
        Q_UNUSED(buf)
        Q_UNUSED(size)
    }

    std::vector<char> getData() const override
    {
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
    explicit AlreadyDisconnected(char *buf, int size)
    {
        Q_UNUSED(buf)
        Q_UNUSED(size)
    }

    std::vector<char> getData() const override
    {
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
    explicit State(char *buf, int size)
    {
        QByteArray arr(buf, size);
        QDataStream ds(&arr, QIODevice::ReadOnly);
        ds >> isLoggedIn_ >> waitingForLoginInfo_;
    }

    std::vector<char> getData() const override
    {
        QByteArray arr;
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << isLoggedIn_ << waitingForLoginInfo_;
        return std::vector<char>(arr.begin(), arr.end());
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
    explicit LoginResult(char *buf, int size)
    {
        QByteArray arr(buf, size);
        QDataStream ds(&arr, QIODevice::ReadOnly);
        ds >> isLoggedIn_ >> loginError_;
    }

    std::vector<char> getData() const override
    {
        QByteArray arr;
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << isLoggedIn_ << loginError_;
        return std::vector<char>(arr.begin(), arr.end());
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
    explicit SignedOut(char *buf, int size)
    {
        Q_UNUSED(buf)
        Q_UNUSED(size)
    }

    std::vector<char> getData() const override
    {
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
