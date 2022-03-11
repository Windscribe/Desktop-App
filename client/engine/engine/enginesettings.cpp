#include "enginesettings.h"
#include "ipc/protobufcommand.h"
#include "utils/logger.h"
#include "utils/winutils.h"

const int typeIdEngineSettings = qRegisterMetaType<EngineSettings>("EngineSettings");

EngineSettings::EngineSettings() : simpleCrypt_(0x4572A4ACF31A31BA)
{
#if defined(Q_OS_LINUX)
    repairEngineSettings();
#endif
}

EngineSettings::EngineSettings(const ProtoTypes::EngineSettings &s) :
    engineSettings_(s)
  , simpleCrypt_(0x4572A4ACF31A31BA)
{
#if defined(Q_OS_LINUX)
    repairEngineSettings();
#endif
}

void EngineSettings::saveToSettings()
{
    QSettings settings;

    size_t size = engineSettings_.ByteSizeLong();
    QByteArray arr(size, Qt::Uninitialized);
    engineSettings_.SerializeToArray(arr.data(), size);

    // Changed engineSettings to engineSettings2 when settings enrcyption was added.
    settings.setValue("engineSettings2", simpleCrypt_.encryptToString(arr));
}

void EngineSettings::loadFromSettings()
{
    QSettings settings;

    const bool containsEncryptedSettings = settings.contains("engineSettings2");
    const bool containsSettings = settings.contains("engineSettings");

    if (containsEncryptedSettings || containsSettings)
    {
        if(containsEncryptedSettings) {
            qCDebug(LOG_BASIC) << "EngineSettings::loadFromSettings Engine Settings are encrypted. Decrypting Engine Settings...";

            const QString s = settings.value("engineSettings2", "").toString();
            const QByteArray arr = simpleCrypt_.decryptToByteArray(s);

            if (!engineSettings_.ParseFromArray(arr.data(), arr.size()))
            {
                qCDebug(LOG_BASIC) << "Deserialization of EngineSettings has failed";
            }
        }
        else if(containsSettings) {
            qCDebug(LOG_BASIC) << "EngineSettings::loadFromSettings Engine Settings are not encrypted. Loading Engine Settings...";

            const QByteArray arr = settings.value("engineSettings", "").toByteArray();
            if (!engineSettings_.ParseFromArray(arr.data(), arr.size()))
            {
                qCDebug(LOG_BASIC) << "EngineSettings::loadFromSettings Loading of EngineSettings failed.";
            }
            settings.remove("engineSettings");
        }

#if defined(Q_OS_LINUX)
        repairEngineSettings();
#elif defined(Q_OS_WINDOWS)
        // Wireguard connection mode was disabled on Windows 7 32-bit since 2.3.12 13th build.
        // If it was saved in settings since the last build it is necessary to reset it.
        if(engineSettings_.has_connection_settings() && engineSettings_.connection_settings().protocol() == ProtoTypes::Protocol::PROTOCOL_WIREGUARD) {
            if(WinUtils::isWindows7() && !WinUtils::isWindows64Bit()) {
                engineSettings_.mutable_connection_settings()->set_protocol(ProtoTypes::Protocol::PROTOCOL_IKEV2);
                engineSettings_.mutable_connection_settings()->set_port(500);
            }
        }
#endif
    }
    // try load settings from version 1
    else
    {
        loadFromVersion1();
    }
    qCDebugMultiline(LOG_BASIC) << "Engine settings:"
        << Utils::cleanSensitiveInfo(QString::fromStdString(engineSettings_.DebugString()));
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

ProtoTypes::DnsManagerType EngineSettings::getDnsManager() const
{
    return engineSettings_.dns_manager();
}

ProtoTypes::MacAddrSpoofing EngineSettings::getMacAddrSpoofing() const
{
    return engineSettings_.mac_addr_spoofing();
}

ProtoTypes::PacketSize EngineSettings::getPacketSize() const
{
    return engineSettings_.packet_size();
}

ProtoTypes::UpdateChannel EngineSettings::getUpdateChannel() const
{
    return engineSettings_.update_channel();
}

ProtoTypes::DnsWhileConnectedInfo EngineSettings::getDnsWhileConnectedInfo() const
{
    return engineSettings_.dns_while_connected_info();
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

void EngineSettings::loadFromVersion1()
{
    QSettings settings("Windscribe", "Windscribe");
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
    if (cs.readFromSettingsV1(settings))
    {
        *engineSettings_.mutable_connection_settings() = cs.convertToProtobuf();
    }

    ProxySettings ps;
    ps.readFromSettingsV1(settings);
    *engineSettings_.mutable_proxy_settings() = ps.convertToProtobuf();

}

#if defined(Q_OS_LINUX)
void EngineSettings::repairEngineSettings()
{
    // IKEv2 is dissabled on linux but is default protocol in ProtoTypes::ConnectionSettings.
    // UDP should be default on Linux.
    if(engineSettings_.has_connection_settings() && engineSettings_.connection_settings().protocol() == ProtoTypes::Protocol::PROTOCOL_IKEV2) {
        engineSettings_.mutable_connection_settings()->set_protocol(ProtoTypes::Protocol::PROTOCOL_UDP);
        engineSettings_.mutable_connection_settings()->set_port(443);
    }
    else if(!engineSettings_.has_connection_settings()) {
        auto settings = engineSettings_.connection_settings();
        settings.set_protocol(ProtoTypes::Protocol::PROTOCOL_UDP);
        settings.set_port(443);
        *engineSettings_.mutable_connection_settings() = settings;
    }
}
#endif
