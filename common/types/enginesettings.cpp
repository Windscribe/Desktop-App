#include "enginesettings.h"
#include "ipc/protobufcommand.h"
#include "utils/logger.h"
#include "utils/winutils.h"

const int typeIdEngineSettings = qRegisterMetaType<types::EngineSettings>("types::EngineSettings");

/*EngineSettings::EngineSettings() : simpleCrypt_(0x4572A4ACF31A31BA)
{
#if defined(Q_OS_LINUX)
    repairEngineSettings();
#endif
}*/

namespace types {

EngineSettings::EngineSettings() : d(new EngineSettingsData)
{

}

void EngineSettings::saveToSettings()
{
    /*QSettings settings;

    size_t size = engineSettings_.ByteSizeLong();
    QByteArray arr(size, Qt::Uninitialized);
    engineSettings_.SerializeToArray(arr.data(), size);

    // Changed engineSettings to engineSettings2 when settings enrcyption was added.
    settings.setValue("engineSettings2", simpleCrypt_.encryptToString(arr));*/
}

void EngineSettings::loadFromSettings()
{
    /*QSettings settings;

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
        // Wireguard connection mode disabled on 32-bit Windows as of build 2.4.4.
        // If it was saved in settings since the last build it is necessary to reset it.
        if (!WinUtils::isWindows64Bit() && engineSettings_.has_connection_settings() &&
            (engineSettings_.connection_settings().protocol() == ProtoTypes::Protocol::PROTOCOL_WIREGUARD))
        {
            engineSettings_.mutable_connection_settings()->set_protocol(ProtoTypes::Protocol::PROTOCOL_IKEV2);
            engineSettings_.mutable_connection_settings()->set_port(500);
        }
#endif
    }
    // try load settings from version 1
    else
    {
        loadFromVersion1();
    }
    qCDebugMultiline(LOG_BASIC) << "Engine settings:"
        << Utils::cleanSensitiveInfo(QString::fromStdString(engineSettings_.DebugString()));*/
}

QString EngineSettings::language() const
{
    return d->language;
}

void EngineSettings::setLanguage(const QString &lang)
{
    d->language = lang;
}

bool EngineSettings::isIgnoreSslErrors() const
{
    return d->isIgnoreSslErrors;
}

void EngineSettings::setIsIgnoreSslErrors(bool ignore)
{
    d->isIgnoreSslErrors = ignore;
}

bool EngineSettings::isCloseTcpSockets() const
{
    return d->isCloseTcpSockets;
}

void EngineSettings::setIsCloseTcpSockets(bool close)
{
    d->isCloseTcpSockets = close;
}

bool EngineSettings::isAllowLanTraffic() const
{
    return d->isAllowLanTraffic;
}

void EngineSettings::setIsAllowLanTraffic(bool isAllowLanTraffic)
{
    d->isAllowLanTraffic = isAllowLanTraffic;
}

const types::FirewallSettings &EngineSettings::firewallSettings() const
{
    return d->firewallSettings;
}

void EngineSettings::setFirewallSettings(const FirewallSettings &fs)
{
    d->firewallSettings = fs;
}

const types::ConnectionSettings &EngineSettings::connectionSettings() const
{
    return d->connectionSettings;
}

void EngineSettings::setConnectionSettings(const ConnectionSettings &cs)
{
    d->connectionSettings = cs;
}

const types::DnsResolutionSettings &EngineSettings::dnsResolutionSettings() const
{
    return d->dnsResolutionSettings;
}

void EngineSettings::setDnsResolutionSettings(const DnsResolutionSettings &drs)
{
    d->dnsResolutionSettings = drs;
}

const types::ProxySettings &EngineSettings::proxySettings() const
{
    return d->proxySettings;
}

void EngineSettings::setProxySettings(const ProxySettings &ps)
{
    d->proxySettings = ps;
}

DNS_POLICY_TYPE EngineSettings::dnsPolicy() const
{
    return d->dnsPolicy;
}

void EngineSettings::setDnsPolicy(DNS_POLICY_TYPE policy)
{
    d->dnsPolicy = policy;
}

DNS_MANAGER_TYPE EngineSettings::dnsManager() const
{
    return d->dnsManager;
}

void EngineSettings::setDnsManager(DNS_MANAGER_TYPE dnsManager)
{
    d->dnsManager = dnsManager;
}

const types::MacAddrSpoofing &EngineSettings::macAddrSpoofing() const
{
    return d->macAddrSpoofing;
}

const types::PacketSize &EngineSettings::packetSize() const
{
    return d->packetSize;
}

UPDATE_CHANNEL EngineSettings::updateChannel() const
{
    return d->updateChannel;
}

void EngineSettings::setUpdateChannel(UPDATE_CHANNEL channel)
{
    d->updateChannel = channel;
}

const types::DnsWhileConnectedInfo &EngineSettings::dnsWhileConnectedInfo() const
{
    return d->dnsWhileConnectedInfo;
}

void EngineSettings::setDnsWhileConnectedInfo(const DnsWhileConnectedInfo &info)
{
    d->dnsWhileConnectedInfo = info;
}

/*IPC::Command *EngineSettings::transformToProtoBufCommand(unsigned int cmdUid) const
{
    IPC::ProtobufCommand<IPCServerCommands::EngineSettingsChanged> *cmdEngineSettings = new IPC::ProtobufCommand<IPCServerCommands::EngineSettingsChanged>();

    *cmdEngineSettings->getProtoObj().mutable_enginesettings() = engineSettings_;

    return cmdEngineSettings;
}*/

/*bool EngineSettings::isEqual(const ProtoTypes::EngineSettings &s) const
{
    return google::protobuf::util::MessageDifferencer::Equals(engineSettings_, s);
}*/

bool EngineSettings::isUseWintun() const
{
    return d->tapAdapter == WINTUN_ADAPTER;
}

TAP_ADAPTER_TYPE EngineSettings::tapAdapter() const
{
    return d->tapAdapter;
}

void EngineSettings::setTapAdapter(TAP_ADAPTER_TYPE tap)
{
    d->tapAdapter = tap;
}

QString EngineSettings::customOvpnConfigsPath() const
{
    return d->customOvpnConfigsPath;
}

void EngineSettings::setCustomOvpnConfigsPath(const QString &path)
{
    d->customOvpnConfigsPath = path;
}

bool EngineSettings::isKeepAliveEnabled() const
{
    return d->isKeepAliveEnabled;
}

void EngineSettings::setIsKeepAliveEnabled(bool enabled)
{
    d->isKeepAliveEnabled = enabled;
}

void EngineSettings::setMacAddrSpoofing(const MacAddrSpoofing &macAddrSpoofing)
{
    d->macAddrSpoofing = macAddrSpoofing;
}

void EngineSettings::setPacketSize(const PacketSize &packetSize)
{
    d->packetSize = packetSize;
}

bool EngineSettings::operator==(const EngineSettings &other) const
{
    return  other.d->language == d->language &&
            other.d->updateChannel == d->updateChannel &&
            other.d->isIgnoreSslErrors == d->isIgnoreSslErrors &&
            other.d->isCloseTcpSockets == d->isCloseTcpSockets &&
            other.d->isAllowLanTraffic == d->isAllowLanTraffic &&
            other.d->firewallSettings == d->firewallSettings &&
            other.d->connectionSettings == d->connectionSettings &&
            other.d->dnsResolutionSettings == d->dnsResolutionSettings &&
            other.d->proxySettings == d->proxySettings &&
            other.d->packetSize == d->packetSize &&
            other.d->macAddrSpoofing == d->macAddrSpoofing &&
            other.d->dnsPolicy == d->dnsPolicy &&
            other.d->tapAdapter == d->tapAdapter &&
            other.d->customOvpnConfigsPath == d->customOvpnConfigsPath &&
            other.d->isKeepAliveEnabled == d->isKeepAliveEnabled &&
            other.d->dnsWhileConnectedInfo == d->dnsWhileConnectedInfo &&
            other.d->dnsManager == d->dnsManager;
}

bool EngineSettings::operator!=(const EngineSettings &other) const
{
    return !(*this == other);
}

/*void EngineSettings::setMacAddrSpoofing(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing)
{
    *engineSettings_.mutable_mac_addr_spoofing() = macAddrSpoofing;
}

void EngineSettings::setPacketSize(const ProtoTypes::PacketSize &packetSize)
{
    *engineSettings_.mutable_packet_size() = packetSize;
}*/

/*void EngineSettings::loadFromVersion1()
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
    types::ConnectionSettings cs;
    if (cs.readFromSettingsV1(settings))
    {
        *engineSettings_.mutable_connection_settings() = cs.convertToProtobuf();
    }

    types::ProxySettings ps;
    ps.readFromSettingsV1(settings);
    *engineSettings_.mutable_proxy_settings() = ps.convertToProtobuf();
}*/

#if defined(Q_OS_LINUX)
void EngineSettings::repairEngineSettings()
{
    // IKEv2 is disabled on linux but is default protocol in ProtoTypes::ConnectionSettings.
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

} // types namespace

