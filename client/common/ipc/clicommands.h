#pragma once

#include "command.h"
#include "types/enginesettings.h"
#include "types/connectstate.h"

namespace IPC
{

namespace CliCommands
{

class Acknowledge : public Command
{
public:
    Acknowledge() {}
    explicit Acknowledge(char *buf, int size)
    {
        QByteArray arr(buf, size);
        QDataStream ds(&arr, QIODevice::ReadOnly);
        ds >> code_ >> message_;
    }

    std::vector<char> getData() const override
    {
        QByteArray arr;
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << code_ << message_;
        return std::vector<char>(arr.begin(), arr.end());
    }

    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "CliCommands::Acknowledge debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::Acknowledge";  }

    int code_;
    QString message_;
};

class Connect : public Command
{
public:
    Connect() {}
    explicit Connect(char *buf, int size)
    {
        QByteArray arr(buf, size);
        QDataStream ds(&arr, QIODevice::ReadOnly);
        ds >> location_ >> protocol_;
    }

    std::vector<char> getData() const override
    {
        QByteArray arr;
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << location_ << protocol_;
        return std::vector<char>(arr.begin(), arr.end());
    }

    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "CliCommands::Connect debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::Connect";  }

    QString location_;
    QString protocol_;
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

class LocationsList : public Command
{
public:
    LocationsList() {}
    explicit LocationsList(char *buf, int size)
    {
        QByteArray arr(buf, size);
        QDataStream ds(&arr, QIODevice::ReadOnly);
        ds >> locations_;
    }

    std::vector<char> getData() const override
    {
        QByteArray arr;
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << locations_;
        return std::vector<char>(arr.begin(), arr.end());
    }

    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "CliCommands::LocationsList debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::LocationsList";  }

    QStringList locations_;
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

class Logout : public Command
{
public:
    Logout() {}
    explicit Logout(char *buf, int size)
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
        return "CliCommands::Logout debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::Logout";  }

    bool isKeepFirewallOn_;
};

class SendLogs: public Command
{
public:
    SendLogs() {}
    explicit SendLogs(char *buf, int size)
    {
        Q_UNUSED(buf);
        Q_UNUSED(size);
    }

    std::vector<char> getData() const override
    {
        return std::vector<char>();
    }

    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "CliCommands::SendLogs debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::SendLogs";  }
};

class State : public Command
{
public:
    State() {}
    explicit State(char *buf, int size)
    {
        QByteArray arr(buf, size);
        QDataStream ds(&arr, QIODevice::ReadOnly);
        ds >> language_ >> connectivity_ >> loginState_ >> loginError_ >> loginErrorMessage_ >> connectState_
           >> protocol_ >> port_ >> tunnelTestState_ >> location_ >> isFirewallOn_ >> isFirewallAlwaysOn_
           >> updateError_ >> updateProgress_ >> updateAvailable_ >> trafficUsed_ >> trafficMax_;
    }

    std::vector<char> getData() const override
    {
        QByteArray arr;
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << language_ << connectivity_ << loginState_ << loginError_ << loginErrorMessage_ << connectState_
           << protocol_ << port_ << tunnelTestState_ << location_ << isFirewallOn_ << isFirewallAlwaysOn_
           << updateError_ << updateProgress_ << updateAvailable_ << trafficUsed_ << trafficMax_;
        return std::vector<char>(arr.begin(), arr.end());
    }

    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "CliCommands::State debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::State";  }

    QString language_;
    bool connectivity_;
    LOGIN_STATE loginState_ = LOGIN_STATE_LOGGED_OUT;
    LOGIN_RET loginError_;
    QString loginErrorMessage_;
    types::ConnectState connectState_;
    types::Protocol protocol_;
    uint port_;
    TUNNEL_TEST_STATE tunnelTestState_;
    LocationID location_;
    bool isFirewallOn_;
    bool isFirewallAlwaysOn_;
    UPDATE_VERSION_ERROR updateError_;
    uint updateProgress_;
    QString updateAvailable_;
    qint64 trafficUsed_;
    qint64 trafficMax_;
};

class Update : public Command
{
public:
    Update() {}
    explicit Update(char *buf, int size)
    {
        Q_UNUSED(buf);
        Q_UNUSED(size);
    }

    std::vector<char> getData() const override
    {
        return std::vector<char>();
    }

    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "CliCommands::Update debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::Update";  }
};

class ReloadConfig : public Command
{
public:
    ReloadConfig() {}
    explicit ReloadConfig(char *buf, int size)
    {
        Q_UNUSED(buf);
        Q_UNUSED(size);
    }

    std::vector<char> getData() const override
    {
        return std::vector<char>();
    }

    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "CliCommands::ReloadConfig debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::ReloadConfig";  }
};

} // namespace CliCommands
} // namespace IPC
