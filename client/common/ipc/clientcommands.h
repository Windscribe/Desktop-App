#ifndef IPC_CLIENT_COMMANDS_H
#define IPC_CLIENT_COMMANDS_H

#include "command.h"
#include "types/enginesettings.h"
#include "types/locationid.h"
#include "types/splittunneling.h"
#include "utils/ws_assert.h"

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
        WS_ASSERT(false);
        return std::vector<char>();
    }

    std::string getStringId() const override
    {
        return getCommandStringId();
    }

    std::string getDebugString() const override
    {
        return "GUI::SetSettings debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::SetSettings";
    }

    const types::EngineSettings &getEngineSettings() const
    {
        return es_;
    }

private:
    types::EngineSettings es_;
};

class ClientAuth : public Command
{
public:
    explicit ClientAuth()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::ClientAuth debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::ClientAuth";
    }
};

class ClientPing : public Command
{
public:
    explicit ClientPing()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::ClientPing debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::ClientPing";
    }
};

class GetState : public Command
{
public:
    explicit GetState()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::GetState debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::GetState";
    }
};

class Init : public Command
{
public:
    explicit Init()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::Init debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::Init";
    }
};

class Cleanup : public Command
{
public:
    explicit Cleanup()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::Cleanup debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::Cleanup";
    }

    bool isExitWithRestart_;
    bool isFirewallChecked_;
    bool isFirewallAlwaysOn_;
    bool isLaunchOnStartup_;
};

class EnableBfe_win : public Command
{
public:
    explicit EnableBfe_win()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::EnableBfe_win debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::EnableBfe_win";
    }
};

class Login : public Command
{
public:
    explicit Login()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::Login debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::Login";
    }

    QString username_;
    QString password_;
    QString code2fa_;
    bool isLoginWithAuthHash_ = false;
};


class SignOut : public Command
{
public:
    explicit SignOut()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::SignOut debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::SignOut";
    }
    bool isKeepFirewallOn_;
};

class Firewall : public Command
{
public:
    explicit Firewall()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::Firewall debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::Firewall";
    }
    bool isEnable_;
};

class Connect : public Command
{
public:
    explicit Connect()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::Connect debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::Connect";
    }

    LocationID locationId_;
    types::ConnectionSettings connectionSettings_;
};

class Disconnect : public Command
{
public:
    explicit Disconnect()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::Disconnect debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::Disconnect";
    }
};

class EmergencyConnect : public Command
{
public:
    explicit EmergencyConnect()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::EmergencyConnect debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::EmergencyConnect";
    }
};

class EmergencyDisconnect : public Command
{
public:
    explicit EmergencyDisconnect()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::EmergencyDisconnect debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::EmergencyDisconnect";
    }
};


class ApplicationActivated : public Command
{
public:
    explicit ApplicationActivated()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::ApplicationActivated debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::ApplicationActivated";
    }
    bool isActivated_;
};

class RecordInstall : public Command
{
public:
    explicit RecordInstall()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::RecordInstall debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::RecordInstall";
    }
};

class SendConfirmEmail : public Command
{
public:
    explicit SendConfirmEmail()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::SendConfirmEmail debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::SendConfirmEmail";
    }
};

class SendDebugLog : public Command
{
public:
    explicit SendDebugLog()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::SendDebugLog debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::SendDebugLog";
    }
};

class GetWebSessionToken : public Command
{
public:
    explicit GetWebSessionToken()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::GetWebSessionToken debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::GetWebSessionToken";
    }
    WEB_SESSION_PURPOSE purpose_;
};

class SetBlockConnect : public Command
{
public:
    explicit SetBlockConnect()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return QString("ClientCommand::SetBlockConnect: %1").arg(isBlockConnect_).toStdString();
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::SetBlockConnect";
    }

    bool isBlockConnect_;
};

class StartWifiSharing : public Command
{
public:
    explicit StartWifiSharing()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::StartWifiSharing debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::StartWifiSharing";
    }
    QString ssid_;
    QString password_;
};

class StopWifiSharing : public Command
{
public:
    explicit StopWifiSharing()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::StopWifiSharing debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::StopWifiSharing";
    }
};

class StartProxySharing : public Command
{
public:
    explicit StartProxySharing()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::StartProxySharing debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::StartProxySharing";
    }

    PROXY_SHARING_TYPE sharingMode_;
};

class StopProxySharing : public Command
{
public:
    explicit StopProxySharing()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::StopProxySharing debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::StopProxySharing";
    }
};

class SpeedRating : public Command
{
public:
    explicit SpeedRating()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::SpeedRating debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::SpeedRating";
    }

    int rating_;
    QString localExternalIp_;
};

class GotoCustomOvpnConfigMode : public Command
{
public:
    explicit GotoCustomOvpnConfigMode()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::GotoCustomOvpnConfigMode debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::GotoCustomOvpnConfigMode";
    }
};

class ContinueWithCredentialsForOvpnConfig : public Command
{
public:
    explicit ContinueWithCredentialsForOvpnConfig()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::ContinueWithCredentialsForOvpnConfig debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::ContinueWithCredentialsForOvpnConfig";
    }
    QString username_;
    QString password_;
    bool isSave_;
};

class GetIpv6StateInOS : public Command
{
public:
    explicit GetIpv6StateInOS()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::GetIpv6StateInOS debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::GetIpv6StateInOS";
    }
};

class SetIpv6StateInOS : public Command
{
public:
    explicit SetIpv6StateInOS()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::SetIpv6StateInOS debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::SetIpv6StateInOS";
    }

    bool isEnabled_;
};

class SplitTunneling : public Command
{
public:
    explicit SplitTunneling()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::SplitTunneling debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::SplitTunneling";
    }
    types::SplitTunneling splitTunneling_;
};


class DetectPacketSize : public Command
{
public:
    explicit DetectPacketSize()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::DetectPacketSize debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::DetectPacketSize";
    }
};

class UpdateVersion : public Command
{
public:
    explicit UpdateVersion()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::UpdateVersion debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::UpdateVersion";
    }

    bool cancelDownload_ = false;
    qint64 hwnd_ = 0;
};

class UpdateWindowInfo : public Command
{
public:
    explicit UpdateWindowInfo()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::UpdateWindowInfo debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::UpdateWindowInfo";
    }
    int windowCenterX_ = 2147483647;
    int windowCenterY_ = 2147483647;
};

class MakeHostsWritableWin : public Command
{
public:
    explicit MakeHostsWritableWin()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::MakeHostsWritableWin debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::MakeHostsWritableWin";
    }
};

class AdvancedParametersChanged : public Command
{
public:
    explicit AdvancedParametersChanged()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::AdvancedParametersChanged debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::AdvancedParametersChanged";
    }
};

class GetRobertFilters: public Command
{
public:
    explicit GetRobertFilters()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::GetRobertFilters debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::GetRobertFilters";
    }
};

class SetRobertFilter: public Command
{
public:
    explicit SetRobertFilter()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::SetRobertFilter debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::SetRobertFilter";
    }

    types::RobertFilter filter_;
};

class SyncRobert: public Command
{
public:
    explicit SyncRobert()
    {}

    std::vector<char> getData() const override   {  WS_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ClientCommand::SyncRobert debug string";
    }

    static std::string getCommandStringId()
    {
        return "ClientCommand::SyncRobert";
    }
};

} // namespace ClientCommands
} // namespace IPC

#endif // IPC_CLIENT_COMMANDS_H
