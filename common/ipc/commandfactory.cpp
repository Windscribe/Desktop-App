#include <QObject>

#include "commandfactory.h"
#include "protobufcommand.h"
#include "generated_proto/clientcommands.pb.h"
#include "generated_proto/servercommands.pb.h"

namespace IPC
{

Command *CommandFactory::makeCommand(const std::string strId, char *buf, int size)
{
    // client commands
    if (strId == IPCClientCommands::ClientAuth::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::ClientAuth>(buf, size);
    }
    else if (strId == IPCClientCommands::ClientPing::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::ClientPing>(buf, size);
    }
    else if (strId == IPCClientCommands::Init::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::Init>(buf, size);
    }
    else if (strId == IPCClientCommands::GetState::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::GetState>(buf, size);
    }
    else if (strId == IPCClientCommands::Cleanup::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::Cleanup>(buf, size);
    }
    else if (strId == IPCClientCommands::GetSettings::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::GetSettings>(buf, size);
    }
    else if (strId == IPCClientCommands::SetSettings::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::SetSettings>(buf, size);
    }
    else if (strId == IPCClientCommands::Login::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::Login>(buf, size);
    }
    else if (strId == IPCClientCommands::SignOut::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::SignOut>(buf, size);
    }
    else if (strId == IPCClientCommands::Connect::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::Connect>(buf, size);
    }
    else if (strId == IPCClientCommands::Disconnect::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::Disconnect>(buf, size);
    }
    else if (strId == IPCClientCommands::Firewall::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::Firewall>(buf, size);
    }
    else if (strId == IPCClientCommands::EnableBfe_win::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::EnableBfe_win>(buf, size);
    }
    else if (strId == IPCClientCommands::ApplicationActivated::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::ApplicationActivated>(buf, size);
    }
    else if (strId == IPCClientCommands::RecordInstall::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::RecordInstall>(buf, size);
    }
    else if (strId == IPCClientCommands::SendConfirmEmail::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::SendConfirmEmail>(buf, size);
    }
    else if (strId == IPCClientCommands::SendDebugLog::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::SendDebugLog>(buf, size);
    }
    else if (strId == IPCClientCommands::SetBlockConnect::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::SetBlockConnect>(buf, size);
    }
    else if (strId == IPCClientCommands::ClearCredentials::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::ClearCredentials>(buf, size);
    }
    else if (strId == IPCClientCommands::StartWifiSharing::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::StartWifiSharing>(buf, size);
    }
    else if (strId == IPCClientCommands::StopWifiSharing::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::StopWifiSharing>(buf, size);
    }
    else if (strId == IPCClientCommands::StartProxySharing::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::StartProxySharing>(buf, size);
    }
    else if (strId == IPCClientCommands::StopProxySharing::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::StopProxySharing>(buf, size);
    }
    else if (strId == IPCClientCommands::EmergencyConnect::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::EmergencyConnect>(buf, size);
    }
    else if (strId == IPCClientCommands::EmergencyDisconnect::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::EmergencyDisconnect>(buf, size);
    }
    else if (strId == IPCClientCommands::SpeedRating::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::SpeedRating>(buf, size);
    }
    else if (strId == IPCClientCommands::GotoCustomOvpnConfigMode::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::GotoCustomOvpnConfigMode>(buf, size);
    }
    else if (strId == IPCClientCommands::ContinueWithCredentialsForOvpnConfig::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::ContinueWithCredentialsForOvpnConfig>(buf, size);
    }
    else if (strId == IPCClientCommands::GetIpv6StateInOS::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::GetIpv6StateInOS>(buf, size);
    }
    else if (strId == IPCClientCommands::SetIpv6StateInOS::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::SetIpv6StateInOS>(buf, size);
    }
    else if (strId == IPCClientCommands::SplitTunneling::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::SplitTunneling>(buf, size);
    }
    else if (strId == IPCClientCommands::ForceCliStateUpdate::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::ForceCliStateUpdate>(buf, size);
    }
    else if (strId == IPCClientCommands::DetectPacketSize::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::DetectPacketSize>(buf, size);
    }
    else if (strId == IPCClientCommands::UpdateVersion::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCClientCommands::UpdateVersion>(buf,size);
    }
    // servers commands
    else if (strId == IPCServerCommands::AuthReply::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::AuthReply>(buf, size);
    }
    else if (strId == IPCServerCommands::InitFinished::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::InitFinished>(buf, size);
    }
    else if (strId == IPCServerCommands::FirewallStateChanged::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::FirewallStateChanged>(buf, size);
    }
    else if (strId == IPCServerCommands::LoginFinished::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::LoginFinished>(buf, size);
    }
    else if (strId == IPCServerCommands::LoginStepMessage::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::LoginStepMessage>(buf, size);
    }
    else if (strId == IPCServerCommands::LoginError::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::LoginError>(buf, size);
    }
    else if (strId == IPCServerCommands::EngineSettingsChanged::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::EngineSettingsChanged>(buf, size);
    }
    else if (strId == IPCServerCommands::LocationSpeedChanged::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::LocationSpeedChanged>(buf, size);
    }
    else if (strId == IPCServerCommands::NetworkChanged::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::NetworkChanged>(buf, size);
    }
    else if (strId == IPCServerCommands::ConfirmEmailResult::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::ConfirmEmailResult>(buf, size);
    }
    else if (strId == IPCServerCommands::DebugLogResult::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::DebugLogResult>(buf, size);
    }
    else if (strId == IPCServerCommands::StatisticsUpdated::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::StatisticsUpdated>(buf, size);
    }
    else if (strId == IPCServerCommands::CleanupFinished::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::CleanupFinished>(buf, size);
    }
    else if (strId == IPCServerCommands::RequestCredentialsForOvpnConfig::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::RequestCredentialsForOvpnConfig>(buf, size);
    }
    else if (strId == IPCServerCommands::Ipv6StateInOS::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::Ipv6StateInOS>(buf, size);
    }
    else if (strId == IPCServerCommands::SessionStatusUpdated::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::SessionStatusUpdated>(buf, size);
    }
    else if (strId == IPCServerCommands::LocationsUpdated::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::LocationsUpdated>(buf, size);
    }
    else if (strId == IPCServerCommands::BestLocationUpdated::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::BestLocationUpdated>(buf, size);
    }
    else if (strId == IPCServerCommands::CustomConfigLocationsUpdated::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::CustomConfigLocationsUpdated>(buf, size);
    }
    else if (strId == IPCServerCommands::ConnectStateChanged::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::ConnectStateChanged>(buf, size);
    }
    else if (strId == IPCServerCommands::ProxySharingInfoChanged::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::ProxySharingInfoChanged>(buf, size);
    }
    else if (strId == IPCServerCommands::WifiSharingInfoChanged::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::WifiSharingInfoChanged>(buf, size);
    }
    else if (strId == IPCServerCommands::EmergencyConnectStateChanged::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::EmergencyConnectStateChanged>(buf, size);
    }
    else if (strId == IPCServerCommands::SignOutFinished::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::SignOutFinished>(buf, size);
    }
    else if (strId == IPCServerCommands::NotificationsUpdated::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::NotificationsUpdated>(buf, size);
    }
    else if (strId == IPCServerCommands::CheckUpdateInfoUpdated::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::CheckUpdateInfoUpdated>(buf, size);
    }
    else if (strId == IPCServerCommands::MyIpUpdated::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::MyIpUpdated>(buf, size);
    }
    else if (strId == IPCServerCommands::CustomOvpnConfigModeInitFinished::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::CustomOvpnConfigModeInitFinished>(buf, size);
    }
    else if (strId == IPCServerCommands::SessionDeleted::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::SessionDeleted>(buf, size);
    }
    else if (strId == IPCServerCommands::TestTunnelResult::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::TestTunnelResult>(buf, size);
    }
    else if (strId == IPCServerCommands::LostConnectionToHelper::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::LostConnectionToHelper>(buf, size);
    }
    else if (strId == IPCServerCommands::HighCpuUsage::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::HighCpuUsage>(buf, size);
    }
    else if (strId == IPCServerCommands::BackendPing::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::BackendPing>(buf, size);
    }
    else if (strId == IPCServerCommands::UserWarning::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::UserWarning>(buf, size);
    }
    else if (strId == IPCServerCommands::InternetConnectivityChanged::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::InternetConnectivityChanged>(buf, size);
    }
    else if (strId == IPCServerCommands::ProtocolPortChanged::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::ProtocolPortChanged>(buf, size);
    }
    else if (strId == IPCServerCommands::PacketSizeDetectionState::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::PacketSizeDetectionState>(buf, size);
    }
    else if (strId == IPCServerCommands::UpdateVersionChanged::descriptor()->full_name())
    {
        return new ProtobufCommand<IPCServerCommands::UpdateVersionChanged>(buf, size);
    }
    Q_ASSERT(false);
    return NULL;
}

} // namespace IPC
