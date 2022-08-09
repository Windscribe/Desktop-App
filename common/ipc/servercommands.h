#ifndef IPC_SERVER_COMMANDS_H
#define IPC_SERVER_COMMANDS_H

#include "command.h"
#include "types/sessionstatus.h"
#include "types/enums.h"
#include "types/enginesettings.h"
#include "types/portmap.h"
#include "types/notification.h"
#include "types/checkupdate.h"
#include "types/connectstate.h"
#include "types/proxysharinginfo.h"
#include "types/wifisharinginfo.h"
#include "types/locationitem.h"

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
        return "ServerCommand::SessionStatusUpdated debug string";
    }
    static std::string getCommandStringId()
    {
        return "ServerCommand::SessionStatusUpdated";
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
        return "ServerCommand::AuthReply debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::AuthReply";
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
    explicit InitFinished(INIT_STATE initState) :
        initState_(initState)
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::InitFinished debug string";
    }
    static std::string getCommandStringId()
    {
        return "ServerCommand::InitFinished";
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
        return "ServerCommand::EngineSettingsChanged debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::EngineSettingsChanged";
    }
    types::EngineSettings engineSettings_;
};

class NetworkChanged : public Command
{
public:
    explicit NetworkChanged(const types::NetworkInterface &networkInterface) : networkInterface_(networkInterface)  {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::NetworkChanged debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::NetworkChanged";
    }
    types::NetworkInterface networkInterface_;
};

class LoginFinished : public Command
{
public:
    explicit LoginFinished(bool isLoginFromSettings, const types::PortMap &portMap, const QString &authHash) :
        isLoginFromSettings_(isLoginFromSettings), portMap_(portMap), authHash_(authHash)
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return QString("ServerCommand::LoginFinished isLoginFromSettings %1").arg(isLoginFromSettings_).toStdString();
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::LoginFinished";
    }
    bool isLoginFromSettings_;
    types::PortMap portMap_;
    QString authHash_;
};


class BfeEnableFinished : public Command
{
public:
    explicit BfeEnableFinished(INIT_STATE initState) : initState_(initState)
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::BfeEnableFinished debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::BfeEnableFinished";
    }

    INIT_STATE initState_;
};

class CleanupFinished : public Command
{
public:
    explicit CleanupFinished() {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::CleanupFinished debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::CleanupFinished";
    }
};


class WebSessionToken : public Command
{
public:
    explicit WebSessionToken(WEB_SESSION_PURPOSE purpose, const QString &tempSessionToken) :
        purpose_(purpose), tempSessionToken_(tempSessionToken)
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::WebSessionToken debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::WebSessionToken";
    }

    WEB_SESSION_PURPOSE purpose_;
    QString tempSessionToken_;
};


class SignOutFinished : public Command
{
public:
    explicit SignOutFinished() {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::SignOutFinished debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::SignOutFinished";
    }
};

class LoginStepMessage : public Command
{
public:
    explicit LoginStepMessage(LOGIN_MESSAGE message) : message_(message)
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::LoginStepMessage debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::LoginStepMessage";
    }

    LOGIN_MESSAGE message_;
};


class LoginError : public Command
{
public:
    explicit LoginError(LOGIN_RET error, const QString &errorMessage) : error_(error), errorMessage_(errorMessage)
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::LoginError debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::LoginError";
    }

    LOGIN_RET error_;
    QString errorMessage_;
};

class NotificationsUpdated : public Command
{
public:
    explicit NotificationsUpdated(const QVector<types::Notification> &notifications) : notifications_(notifications)
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::NotificationsUpdated debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::NotificationsUpdated";
    }

    QVector<types::Notification> notifications_;
};

class CheckUpdateInfoUpdated : public Command
{
public:
    explicit CheckUpdateInfoUpdated(const types::CheckUpdate &checkUpdateInfo) : checkUpdateInfo_(checkUpdateInfo)
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::CheckUpdateInfoUpdated debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::CheckUpdateInfoUpdated";
    }

    types::CheckUpdate checkUpdateInfo_;
};

class MyIpUpdated : public Command
{
public:
    explicit MyIpUpdated(const QString &ip, bool isDisconnectedState) : ip_(ip), isDisconnectedState_(isDisconnectedState)
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        QString censoredIp = ip_;
        // censor user IP for log
        if (isDisconnectedState_)
        {
            int index = ip_.lastIndexOf('.');
            censoredIp = ip_.mid(0,index+1) + "###"; // censor last octet
        }

        return QString("[Server::MyIpUpdated] ip:%1 isDisconnectedState:%2").arg(censoredIp, isDisconnectedState_).toStdString();
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::MyIpUpdated";
    }

    QString ip_;
    bool isDisconnectedState_;
};

class LocationsUpdated : public Command
{
public:
    explicit LocationsUpdated()
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::LocationsUpdated debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::LocationsUpdated";
    }
    LocationID bestLocation_;
    QVector<types::LocationItem> locations_;
    QString staticIpDeviceName_;
};

class CustomConfigLocationsUpdated : public Command
{
public:
    explicit CustomConfigLocationsUpdated()
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::CustomConfigLocationsUpdated debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::CustomConfigLocationsUpdated";
    }
    QVector<types::LocationItem> locations_;
};


class BestLocationUpdated : public Command
{
public:
    explicit BestLocationUpdated()
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::BestLocationUpdated debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::BestLocationUpdated";
    }
    LocationID bestLocation_;
};

class StatisticsUpdated : public Command
{
public:
    explicit StatisticsUpdated()
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::StatisticsUpdated debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::StatisticsUpdated";
    }

    quint64 bytesIn_;
    quint64 bytesOut_;
    bool isTotalBytes_;
};

class RequestCredentialsForOvpnConfig : public Command
{
public:
    explicit RequestCredentialsForOvpnConfig()
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::RequestCredentialsForOvpnConfig debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::RequestCredentialsForOvpnConfig";
    }
};

class ConnectStateChanged : public Command
{
public:
    explicit ConnectStateChanged()
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::ConnectStateChanged debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::ConnectStateChanged";
    }

    types::ConnectState connectState_;
};

class EmergencyConnectStateChanged : public Command
{
public:
    explicit EmergencyConnectStateChanged()
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::EmergencyConnectStateChanged debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::EmergencyConnectStateChanged";
    }

    types::ConnectState emergencyConnectState_;
};

class DebugLogResult : public Command
{
public:
    explicit DebugLogResult()
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::DebugLogResult debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::DebugLogResult";
    }
    bool success_;
};

class ConfirmEmailResult : public Command
{
public:
    explicit ConfirmEmailResult()
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::ConfirmEmailResult debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::ConfirmEmailResult";
    }
    bool success_;
};

class FirewallStateChanged : public Command
{
public:
    explicit FirewallStateChanged()
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::FirewallStateChanged debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::FirewallStateChanged";
    }
    bool isFirewallEnabled_;
};

class Ipv6StateInOS : public Command
{
public:
    explicit Ipv6StateInOS()
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return QString("ServerCommand::Ipv6StateInOS isEnabled: %1").arg(isEnabled_).toStdString();
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::Ipv6StateInOS";
    }
    bool isEnabled_;
};

class LocationSpeedChanged : public Command
{
public:
    explicit LocationSpeedChanged()
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::LocationSpeedChanged debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::LocationSpeedChanged";
    }
    LocationID id_;
    PingTime pingTime_;
};

class CustomOvpnConfigModeInitFinished : public Command
{
public:
    explicit CustomOvpnConfigModeInitFinished()
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::CustomOvpnConfigModeInitFinished debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::CustomOvpnConfigModeInitFinished";
    }
};

class ProxySharingInfoChanged : public Command
{
public:
    explicit ProxySharingInfoChanged()
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::ProxySharingInfoChanged debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::ProxySharingInfoChanged";
    }
    types::ProxySharingInfo proxySharingInfo_;
};

class WifiSharingInfoChanged : public Command
{
public:
    explicit WifiSharingInfoChanged()
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::WifiSharingInfoChanged debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::WifiSharingInfoChanged";
    }
    types::WifiSharingInfo wifiSharingInfo_;
};

class SessionDeleted : public Command
{
public:
    explicit SessionDeleted()
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::SessionDeleted debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::SessionDeleted";
    }
};

class TestTunnelResult : public Command
{
public:
    explicit TestTunnelResult()
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::TestTunnelResult debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::TestTunnelResult";
    }

    bool success_;
};

class LostConnectionToHelper : public Command
{
public:
    explicit LostConnectionToHelper()
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::LostConnectionToHelper debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::LostConnectionToHelper";
    }
};

class HighCpuUsage : public Command
{
public:
    explicit HighCpuUsage()
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::HighCpuUsage debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::HighCpuUsage";
    }

    QStringList processes_;
};

class UserWarning : public Command
{
public:
    explicit UserWarning()
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::UserWarning debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::UserWarning";
    }
    USER_WARNING_TYPE type_;
};

class InternetConnectivityChanged : public Command
{
public:
    explicit InternetConnectivityChanged()
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::InternetConnectivityChanged debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::InternetConnectivityChanged";
    }
    bool connectivity_;
};

class ProtocolPortChanged : public Command
{
public:
    explicit ProtocolPortChanged()
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::ProtocolPortChanged debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::ProtocolPortChanged";
    }
    PROTOCOL protocol_;
    uint port_;
};

class PacketSizeDetectionState : public Command
{
public:
    explicit PacketSizeDetectionState()
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::PacketSizeDetectionState debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::PacketSizeDetectionState";
    }
    bool on_;
    bool isError_;
};

class UpdateVersionChanged : public Command
{
public:
    explicit UpdateVersionChanged()
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::UpdateVersionChanged debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::UpdateVersionChanged";
    }

    uint progressPercent;
    UPDATE_VERSION_STATE state;
    UPDATE_VERSION_ERROR error;
};

class HostsFileBecameWritable : public Command
{
public:
    explicit HostsFileBecameWritable()
    {}

    std::vector<char> getData() const override   {  Q_ASSERT(false); return std::vector<char>(); }
    std::string getStringId() const override { return getCommandStringId(); }
    std::string getDebugString() const override
    {
        return "ServerCommand::HostsFileBecameWritable debug string";
    }

    static std::string getCommandStringId()
    {
        return "ServerCommand::HostsFileBecameWritable";
    }
};



} // namespace ServerCommands
} // namespace IPC

#endif // IPC_SERVER_COMMANDS_H
