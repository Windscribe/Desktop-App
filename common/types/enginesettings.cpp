#include "enginesettings.h"
#include "ipc/protobufcommand.h"
#include "utils/logger.h"
#include "utils/winutils.h"
#include "types/global_consts.h"
#include "legacy_protobuf_support/legacy_protobuf.h"

const int typeIdEngineSettings = qRegisterMetaType<types::EngineSettings>("types::EngineSettings");

namespace types {

EngineSettings::EngineSettings() : d(new EngineSettingsData)
{
#if defined(Q_OS_LINUX)
    repairEngineSettings();
#endif

}

void EngineSettings::saveToSettings()
{
    QByteArray arr;
    {
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << magic_;
        ds << versionForSerialization_;
        ds << d->language << d->updateChannel << d->isIgnoreSslErrors << d->isCloseTcpSockets << d->isAllowLanTraffic <<
              d->firewallSettings << d->connectionSettings << d->dnsResolutionSettings << d->proxySettings << d->packetSize <<
              d->macAddrSpoofing << d->dnsPolicy << d->tapAdapter << d->customOvpnConfigsPath << d->isKeepAliveEnabled <<
              d->dnsWhileConnectedInfo << d->dnsManager;
    }

    QSettings settings;
    SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);
    settings.setValue("engineSettings", simpleCrypt.encryptToString(arr));
}

void EngineSettings::loadFromSettings()
{
    bool bLoaded = false;
    SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);

    QSettings settings;
    if (settings.contains("engineSettings"))
    {
        QString str = settings.value("engineSettings", "").toString();
        QByteArray arr = simpleCrypt.decryptToByteArray(str);

        QDataStream ds(&arr, QIODevice::ReadOnly);

        quint32 magic, version;
        ds >> magic;
        if (magic == magic_)
        {
            ds >> version;
            if (version <= versionForSerialization_)
            {
                ds >> d->language >> d->updateChannel >> d->isIgnoreSslErrors >> d->isCloseTcpSockets >> d->isAllowLanTraffic >>
                      d->firewallSettings >> d->connectionSettings >> d->dnsResolutionSettings >> d->proxySettings >> d->packetSize >>
                      d->macAddrSpoofing >> d->dnsPolicy >> d->tapAdapter >> d->customOvpnConfigsPath >> d->isKeepAliveEnabled >>
                      d->dnsWhileConnectedInfo >> d->dnsManager;
                if (ds.status() == QDataStream::Ok)
                {
                    bLoaded = true;
                }
            }
        }
    }
    if (!bLoaded && settings.contains("engineSettings2"))
    {
        // try load from legacy protobuf
        // todo remove this code at some point later
        QString str = settings.value("engineSettings2", "").toString();
        QByteArray arr = simpleCrypt.decryptToByteArray(str);
        if (LegacyProtobufSupport::loadEngineSettings(arr, *this))
        {
            bLoaded = true;
        }

        settings.remove("engineSettings2");
    }

    if (!bLoaded)
    {
        *this = EngineSettings();   // reset to defaults
    }

    #if defined(Q_OS_LINUX)
            repairEngineSettings();
    #endif
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

QDebug operator<<(QDebug dbg, const EngineSettings &es)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "{language:" << es.d->language << "; ";
    dbg << "updateChannel:" << UPDATE_CHANNEL_toString(es.d->updateChannel) << "; ";
    dbg << "isIgnoreSslErrors:" << es.d->isIgnoreSslErrors << "; ";
    dbg << "isCloseTcpSockets:" << es.d->isCloseTcpSockets << "; ";
    dbg << "isAllowLanTraffic:" << es.d->isAllowLanTraffic << "; ";
    dbg << "firewallSettings: " << es.d->firewallSettings << "; ";
    dbg << "connectionSettings: " << es.d->connectionSettings << "; ";
    dbg << "dnsResolutionSettings: " << es.d->dnsResolutionSettings << "; ";
    dbg << "proxySettings: " << es.d->proxySettings << "; ";
    dbg << "packetSize: " << es.d->packetSize << "; ";
    dbg << "macAddrSpoofing: " << es.d->macAddrSpoofing << "; ";
    dbg << "dnsPolicy: " << DNS_POLICY_TYPE_ToString(es.d->dnsPolicy) << "; ";
#ifdef Q_OS_WIN
    dbg << "tapAdapter: " << TAP_ADAPTER_TYPE_toString(es.d->tapAdapter) << "; ";
#endif
    dbg << "customOvpnConfigsPath: " << (es.d->customOvpnConfigsPath.isEmpty() ? "empty" : "settled") << "; ";
    dbg << "isKeepAliveEnabled:" << es.d->isKeepAliveEnabled << "; ";
    dbg << "dnsWhileConnectedInfo:" << es.d->dnsWhileConnectedInfo << "; ";
    dbg << "dnsManager:" << DNS_MANAGER_TYPE_toString(es.d->dnsManager) << "}";

    return dbg;
}

#if defined(Q_OS_LINUX)
void EngineSettings::repairEngineSettings()
{
    // IKEv2 is disabled on linux but is default protocol in ProtoTypes::ConnectionSettings.
    // UDP should be default on Linux.
    if(engineSettings_.has_connection_settings() && engineSettings_.connection_settings().protocol() == PROTOCOL::IKEV2) {
        engineSettings_.mutable_connection_settings()->set_protocol(PROTOCOL_TYPE_UDP);
        engineSettings_.mutable_connection_settings()->set_port(443);
    }
    else if(!engineSettings_.has_connection_settings()) {
        auto settings = engineSettings_.connection_settings();
        settings.set_protocol(PROTOCOL_TYPE_UDP);
        settings.set_port(443);
        *engineSettings_.mutable_connection_settings() = settings;
    }
}
#endif

} // types namespace

