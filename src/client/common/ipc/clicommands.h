#pragma once

#include "command.h"
#include "types/enginesettings.h"
#include "types/connectstate.h"
#include <wsnet/WSNet.h>

namespace IPC
{

namespace CliCommands
{

enum class LocationType {
    kRegular = 0,
    kStaticIp = 1,
    kFavourite = 2,
};

enum class LocationFlags {
    kNone = 0,
    kDisabled = 1,
    kPremium = 2,
    k10Gbps = 4
};

struct IpcLocation {
    QString country;
    QString city;
    QString nickname;
    QString pinnedIp;
    int flags;

    IpcLocation() : flags(0) {}
    IpcLocation(const QString &country_, const QString &city_, const QString &nickname_, const QString &pinnedIp_ = QString(), int flags_ = 0)
        : country(country_), city(city_), nickname(nickname_), pinnedIp(pinnedIp_), flags(flags_) {}
};

// QDataStream operators for serialization
inline QDataStream &operator<<(QDataStream &stream, const IpcLocation &location)
{
    stream << location.country << location.city << location.nickname << location.pinnedIp << location.flags;
    return stream;
}

inline QDataStream &operator>>(QDataStream &stream, IpcLocation &location)
{
    stream >> location.country >> location.city >> location.nickname >> location.pinnedIp >> location.flags;
    return stream;
}

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

    int code_ = 0;
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
        ds >> locationType_ >> location_ >> protocol_ >> port_;
    }

    std::vector<char> getData() const override
    {
        QByteArray arr;
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << locationType_ << location_ << protocol_ << port_;
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
    uint port_;
    LocationType locationType_;
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
        QByteArray arr(buf, size);
        QDataStream ds(&arr, QIODevice::ReadOnly);
        ds >> locationType_;
    }

    std::vector<char> getData() const override
    {
        QByteArray arr;
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << locationType_;
        return std::vector<char>(arr.begin(), arr.end());
    }

    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "CliCommands::ShowLocations debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::ShowLocations";  }

    LocationType locationType_;
};

class LocationsList : public Command
{
public:
    LocationsList() {}
    explicit LocationsList(char *buf, int size)
    {
        QByteArray arr(buf, size);
        QDataStream ds(&arr, QIODevice::ReadOnly);
        ds >> locations_ >> deviceName_;
    }

    std::vector<char> getData() const override
    {
        QByteArray arr;
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << locations_ << deviceName_;
        return std::vector<char>(arr.begin(), arr.end());
    }

    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "CliCommands::LocationsList debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::LocationsList";  }

    QList<IpcLocation> locations_;
    QString deviceName_;
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
        ds >> username_ >> password_ >> code2fa_ >> captchaSolution_;
    }

    std::vector<char> getData() const override
    {
        QByteArray arr;
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << username_ << password_ << code2fa_ << captchaSolution_;
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
    QString captchaSolution_;
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
        ds >> language_ >> connectivity_ >> loginState_ >> loginError_ >> loginErrorMessage_
            >> connectState_ >> isCaptchaRequired_ >> asciiArt_ >> connectId_ >> protocol_ >> port_ >> tunnelTestState_ >> location_
           >> isFirewallOn_ >> isFirewallAlwaysOn_
           >> updateState_ >> updateError_ >> updateProgress_ >> updatePath_ >> updateAvailable_
           >> trafficUsed_ >> trafficMax_;
    }

    std::vector<char> getData() const override
    {
        QByteArray arr;
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << language_ << connectivity_ << loginState_ << loginError_ << loginErrorMessage_
           << connectState_ << isCaptchaRequired_ << asciiArt_ << connectId_ << protocol_ << port_ << tunnelTestState_ << location_
           << isFirewallOn_ << isFirewallAlwaysOn_
           << updateState_ << updateError_ << updateProgress_ << updatePath_ << updateAvailable_
           << trafficUsed_ << trafficMax_;
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
    wsnet::LoginResult loginError_;
    QString loginErrorMessage_;
    types::ConnectState connectState_;
    bool isCaptchaRequired_ = false;
    QString asciiArt_;
    QString connectId_;
    types::Protocol protocol_;
    uint port_;
    TUNNEL_TEST_STATE tunnelTestState_;
    LocationID location_;
    bool isFirewallOn_;
    bool isFirewallAlwaysOn_;
    UPDATE_VERSION_STATE updateState_;
    UPDATE_VERSION_ERROR updateError_;
    uint updateProgress_;
    QString updatePath_;
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

class SetKeyLimitBehavior: public Command
{
public:
    SetKeyLimitBehavior() {}
    explicit SetKeyLimitBehavior(char *buf, int size)
    {
        QByteArray arr(buf, size);
        QDataStream ds(&arr, QIODevice::ReadOnly);
        ds >> keyLimitDelete_;
    }

    std::vector<char> getData() const override
    {
        QByteArray arr;
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << keyLimitDelete_;
        return std::vector<char>(arr.begin(), arr.end());
    }

    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "CliCommands::SetKeyLimitBehavior debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::SetKeyLimitBehavior";  }

    bool keyLimitDelete_;
};

class RotateIp : public Command
{
public:
    RotateIp() {}
    explicit RotateIp(char *buf, int size)
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
        return "CliCommands::RotateIp debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::RotateIp";  }
};

class FavIp : public Command
{
public:
    FavIp() {}
    explicit FavIp(char *buf, int size)
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
        return "CliCommands::FavIp debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::FavIp";  }
};

class UnfavIp : public Command
{
public:
    UnfavIp() {}
    explicit UnfavIp(char *buf, int size)
    {
        QByteArray arr(buf, size);
        QDataStream ds(&arr, QIODevice::ReadOnly);
        ds >> ip_;
    }

    std::vector<char> getData() const override
    {
        QByteArray arr;
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << ip_;
        return std::vector<char>(arr.begin(), arr.end());
    }

    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "CliCommands::UnfavIp debug string";
    }
    static std::string getCommandStringId() { return "CliCommands::UnfavIp";  }

    QString ip_;
};


} // namespace CliCommands
} // namespace IPC
