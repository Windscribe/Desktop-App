#include "enginesettings.h"
#include "ipc/protobufcommand.h"
#include "ipc/generated_proto/servercommands.pb.h"
#include <google/protobuf/util/message_differencer.h>

const int typeIdEngineSettings = qRegisterMetaType<EngineSettings>("EngineSettings");

EngineSettings::EngineSettings()
{
}

EngineSettings::EngineSettings(const ProtoTypes::EngineSettings &s)
{
    engineSettings_ = s;
}

void EngineSettings::saveToSettings()
{
    QSettings settings("Windscribe", "Windscribe");

    size_t size = engineSettings_.ByteSizeLong();
    QByteArray arr(size, Qt::Uninitialized);
    engineSettings_.SerializeToArray(arr.data(), size);

    settings.setValue("engineSettings", arr);
}

void EngineSettings::loadFromSettings()
{
    QSettings settings("Windscribe", "Windscribe");

    if (settings.contains("engineSettings"))
    {
        QByteArray arr = settings.value("engineSettings").toByteArray();
        engineSettings_.ParseFromArray(arr.data(), arr.size());
    }
    // try load settings from version 1
    else
    {
        loadFromVersion1(settings);
    }
    qDebug() << "Engine settings:" << QString::fromStdString(engineSettings_.DebugString());
}

const ProtoTypes::EngineSettings &EngineSettings::getProtoBufEngineSettings() const
{
    return engineSettings_;
}

/*IPC::Command *EngineSettings::transformToProtoBufCommand(unsigned int cmdUid) const
{
    IPC::ProtobufCommand<IPCServerCommands::EngineSettingsChanged> *cmdEngineSettings = new IPC::ProtobufCommand<IPCServerCommands::EngineSettingsChanged>();

    *cmdEngineSettings->getProtoObj().mutable_enginesettings() = engineSettings_;

    return cmdEngineSettings;
}*/

bool EngineSettings::isEqual(const ProtoTypes::EngineSettings &s) const
{
    return google::protobuf::util::MessageDifferencer::Equals(engineSettings_, s);
}

QString EngineSettings::language() const
{
    return QString::fromStdString(engineSettings_.language());
}

bool EngineSettings::isBetaChannel() const
{
    // todo add support for other channels
    return engineSettings_.update_channel() != ProtoTypes::UPDATE_CHANNEL_RELEASE;
}

bool EngineSettings::isIgnoreSslErrors() const
{
    return engineSettings_.is_ignore_ssl_errors();
}

bool EngineSettings::isCloseTcpSockets() const
{
    return engineSettings_.is_close_tcp_sockets();
}

bool EngineSettings::isAllowLanTraffic() const
{
    return engineSettings_.is_allow_lan_traffic();
}

const ProtoTypes::FirewallSettings &EngineSettings::firewallSettings() const
{
    return engineSettings_.firewall_settings();
}

ConnectionSettings EngineSettings::connectionSettings() const
{
    ConnectionSettings cs(engineSettings_.connection_settings());
    return cs;
}

DnsResolutionSettings EngineSettings::dnsResolutionSettings() const
{
    DnsResolutionSettings drs(engineSettings_.api_resolution());
    return drs;
}

ProxySettings EngineSettings::proxySettings() const
{
    ProxySettings ps(engineSettings_.proxy_settings());
    return ps;
}

DNS_POLICY_TYPE EngineSettings::getDnsPolicy() const
{
    return (DNS_POLICY_TYPE)engineSettings_.dns_policy();
}

ProtoTypes::MacAddrSpoofing EngineSettings::getMacAddrSpoofing() const
{
    return engineSettings_.mac_addr_spoofing();
}

ProtoTypes::PacketSize EngineSettings::getPacketSize() const
{
    return engineSettings_.packet_size();
}

bool EngineSettings::isUseWintun() const
{
    return engineSettings_.tap_adapter() == ProtoTypes::WINTUN_ADAPTER;
}

QString EngineSettings::getCustomOvpnConfigsPath() const
{
    return QString::fromStdString(engineSettings_.customovpnconfigspath());
}

bool EngineSettings::isKeepAliveEnabled() const
{
    return engineSettings_.is_keep_alive_enabled();
}

void EngineSettings::setMacAddrSpoofing(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing)
{
    *engineSettings_.mutable_mac_addr_spoofing() = macAddrSpoofing;
}

void EngineSettings::setPacketSize(const ProtoTypes::PacketSize &packetSize)
{
    *engineSettings_.mutable_packet_size() = packetSize;
}

void EngineSettings::loadFromVersion1(QSettings &settings)
{
    if (settings.contains("allowLanTraffic"))
    {
        engineSettings_.set_is_allow_lan_traffic(settings.value("allowLanTraffic").toBool());
    }

    if (settings.contains("firewallOption"))
    {
        engineSettings_.mutable_firewall_settings()->set_mode((ProtoTypes::FirewallMode)settings.value("firewallOption").toInt());
    }

    engineSettings_.mutable_api_resolution()->set_is_automatic(settings.value("automaticDnsResolution", true).toBool());
    engineSettings_.mutable_api_resolution()->set_manual_ip(settings.value("manualIp", "").toString().toStdString());

    if (settings.contains("dnsPolicyType"))
    {
        engineSettings_.set_dns_policy((ProtoTypes::DnsPolicy)settings.value("dnsPolicyType").toInt());
    }

    if (settings.contains("language"))
    {
        engineSettings_.set_language(settings.value("language").toString().toStdString());
    }

    if (settings.contains("betaChannel"))
    {
        engineSettings_.set_update_channel(settings.value("betaChannel").toBool() ? ProtoTypes::UPDATE_CHANNEL_BETA : ProtoTypes::UPDATE_CHANNEL_RELEASE);
    }

    if (settings.contains("ignoreSslErrors"))
    {
        engineSettings_.set_is_ignore_ssl_errors(settings.value("ignoreSslErrors").toBool());
    }

    if (settings.contains("closeTcpSockets"))
    {
        engineSettings_.set_is_close_tcp_sockets(settings.value("closeTcpSockets").toBool());
    }

    //vpnShareSettings_.readFromSettings(settings);
    ConnectionSettings cs;
    if (cs.readFromSettings(settings))
    {
        *engineSettings_.mutable_connection_settings() = cs.convertToProtobuf();
    }

    ProxySettings ps;
    ps.readFromSettings(settings);
    *engineSettings_.mutable_proxy_settings() = ps.convertToProtobuf();

}
