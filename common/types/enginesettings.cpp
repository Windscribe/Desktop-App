#include "enginesettings.h"
#include "ipc/protobufcommand.h"
#include "utils/logger.h"
#include "utils/winutils.h"

extern "C" {
    #include "legacy_protobuf_support/types.pb-c.h"
}

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
    SimpleCrypt simpleCrypt(simpleCryptKey_);

    QByteArray arr;
    {
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds.setVersion(QDataStream::Qt_6_2);
        ds << magic_;
        ds << versionForSerialization_;
        ds << d->language << d->updateChannel << d->isIgnoreSslErrors << d->isCloseTcpSockets << d->isAllowLanTraffic <<
              d->firewallSettings << d->connectionSettings << d->dnsResolutionSettings << d->proxySettings << d->packetSize <<
              d->macAddrSpoofing << d->dnsPolicy << d->tapAdapter << d->customOvpnConfigsPath << d->isKeepAliveEnabled <<
              d->dnsWhileConnectedInfo << d->dnsManager;
    }

    QSettings settings;
    settings.setValue("engineSettings", simpleCrypt.encryptToString(arr));
}

void EngineSettings::loadFromSettings()
{
    SimpleCrypt simpleCrypt(simpleCryptKey_);
    QSettings settings;
    if (settings.contains("engineSettings"))
    {
        QString str = settings.value("engineSettings", "").toString();
        QByteArray arr = simpleCrypt.decryptToByteArray(str);
        QDataStream ds(&arr, QIODevice::ReadOnly);
        ds.setVersion(QDataStream::Qt_6_2);

        quint32 magic, version;
        ds >> magic;
        if (magic != magic_)
        {
            return;
        }
        ds >> version;
        if (version > versionForSerialization_)
        {
            return;
        }
        ds >> d->language >> d->updateChannel >> d->isIgnoreSslErrors >> d->isCloseTcpSockets >> d->isAllowLanTraffic >>
              d->firewallSettings >> d->connectionSettings >> d->dnsResolutionSettings >> d->proxySettings >> d->packetSize >>
              d->macAddrSpoofing >> d->dnsPolicy >> d->tapAdapter >> d->customOvpnConfigsPath >> d->isKeepAliveEnabled >>
              d->dnsWhileConnectedInfo >> d->dnsManager;
    }
    else if (settings.contains("engineSettings2"))
    {
        // try load from legacy protobuf
        // todo remove this code at some point later
        QString str = settings.value("engineSettings2", "").toString();
        QByteArray arr = simpleCrypt.decryptToByteArray(str);
        ProtoTypes__EngineSettings *es = proto_types__engine_settings__unpack(NULL, arr.size(), (const uint8_t *)arr.data());
        if (es)
        {
            d->language = es->language;
            if (es->has_update_channel) d->updateChannel = (UPDATE_CHANNEL)es->update_channel;
            if (es->has_is_ignore_ssl_errors) d->isIgnoreSslErrors = es->is_ignore_ssl_errors;
            if (es->has_is_close_tcp_sockets) d->isCloseTcpSockets = es->is_close_tcp_sockets;
            if (es->has_is_allow_lan_traffic) d->isAllowLanTraffic = es->is_allow_lan_traffic;

            if (es->firewall_settings)
            {
                if (es->firewall_settings->has_mode) d->firewallSettings.mode = (FIREWALL_MODE)es->firewall_settings->mode;
                if (es->firewall_settings->has_when)  d->firewallSettings.when = (FIREWALL_WHEN)es->firewall_settings->when;
            }

            if (es->connection_settings)
            {
                if (es->connection_settings->has_is_automatic) d->connectionSettings.setIsAutomatic(es->connection_settings->is_automatic);
                if (es->connection_settings->has_port) d->connectionSettings.setPort(es->connection_settings->port);
                if (es->connection_settings->has_protocol) d->connectionSettings.setProtocol((ProtocolType::PROTOCOL_TYPE)es->connection_settings->protocol);
            }

            if (es->api_resolution)
            {
                if (es->api_resolution->has_is_automatic) d->dnsResolutionSettings.setIsAutomatic(es->api_resolution->is_automatic);
                d->dnsResolutionSettings.set(es->api_resolution->is_automatic, es->api_resolution->manual_ip);
            }

            if (es->proxy_settings)
            {
                d->proxySettings.setAddress(es->proxy_settings->address);
                d->proxySettings.setUsername(es->proxy_settings->username);
                d->proxySettings.setPassword(es->proxy_settings->password);
                if (es->proxy_settings->has_port) d->proxySettings.setPort(es->proxy_settings->port);
                if (es->proxy_settings->has_proxy_option) d->proxySettings.setOption((PROXY_OPTION)es->proxy_settings->proxy_option);
            }

            if (es->packet_size)
            {
                if (es->packet_size->has_is_automatic) d->packetSize.isAutomatic = es->packet_size->is_automatic;
                if (es->packet_size->has_mtu) d->packetSize.mtu = es->packet_size->mtu;
            }

            if (es->mac_addr_spoofing)
            {
                if (es->mac_addr_spoofing->has_is_enabled) d->macAddrSpoofing.isEnabled = es->mac_addr_spoofing->is_enabled;
                d->macAddrSpoofing.macAddress = es->mac_addr_spoofing->mac_address;
                if (es->mac_addr_spoofing->has_is_auto_rotate) d->macAddrSpoofing.isAutoRotate = es->mac_addr_spoofing->is_auto_rotate;


                // NetworkInterface
                if (es->mac_addr_spoofing->selected_network_interface)
                {
                    if (es->mac_addr_spoofing->selected_network_interface->has_interface_index) {
                        d->macAddrSpoofing.selectedNetworkInterface.interfaceIndex = es->mac_addr_spoofing->selected_network_interface->interface_index;
                    }
                    d->macAddrSpoofing.selectedNetworkInterface.interfaceName = es->mac_addr_spoofing->selected_network_interface->interface_name;
                    d->macAddrSpoofing.selectedNetworkInterface.interfaceGuid = es->mac_addr_spoofing->selected_network_interface->interface_guid;
                    d->macAddrSpoofing.selectedNetworkInterface.networkOrSSid = es->mac_addr_spoofing->selected_network_interface->network_or_ssid;
                    if (es->mac_addr_spoofing->selected_network_interface->has_interface_type) {
                        d->macAddrSpoofing.selectedNetworkInterface.interfaceType = (NETWORK_INTERACE_TYPE)es->mac_addr_spoofing->selected_network_interface->interface_type;
                    }
                    if (es->mac_addr_spoofing->selected_network_interface->has_trust_type) {
                        d->macAddrSpoofing.selectedNetworkInterface.trustType = (NETWORK_TRUST_TYPE)es->mac_addr_spoofing->selected_network_interface->trust_type;
                    }
                    if (es->mac_addr_spoofing->selected_network_interface->has_active) {
                        d->macAddrSpoofing.selectedNetworkInterface.active = es->mac_addr_spoofing->selected_network_interface->active;
                    }
                    d->macAddrSpoofing.selectedNetworkInterface.friendlyName = es->mac_addr_spoofing->selected_network_interface->friendly_name;
                    if (es->mac_addr_spoofing->selected_network_interface->has_requested) {
                        d->macAddrSpoofing.selectedNetworkInterface.requested = es->mac_addr_spoofing->selected_network_interface->requested;
                    }
                    if (es->mac_addr_spoofing->selected_network_interface->has_metric) {
                        d->macAddrSpoofing.selectedNetworkInterface.metric = es->mac_addr_spoofing->selected_network_interface->metric;
                    }
                    d->macAddrSpoofing.selectedNetworkInterface.physicalAddress = es->mac_addr_spoofing->selected_network_interface->physical_address;
                    if (es->mac_addr_spoofing->selected_network_interface->has_mtu) {
                        d->macAddrSpoofing.selectedNetworkInterface.mtu = es->mac_addr_spoofing->selected_network_interface->mtu;
                    }
                    if (es->mac_addr_spoofing->selected_network_interface->has_state) {
                        d->macAddrSpoofing.selectedNetworkInterface.state = es->mac_addr_spoofing->selected_network_interface->state;
                    }
                    if (es->mac_addr_spoofing->selected_network_interface->has_dw_type) {
                        d->macAddrSpoofing.selectedNetworkInterface.dwType = es->mac_addr_spoofing->selected_network_interface->dw_type;
                    }
                    d->macAddrSpoofing.selectedNetworkInterface.deviceName = es->mac_addr_spoofing->selected_network_interface->device_name;
                    if (es->mac_addr_spoofing->selected_network_interface->has_connector_present) {
                        d->macAddrSpoofing.selectedNetworkInterface.connectorPresent = es->mac_addr_spoofing->selected_network_interface->connector_present;
                    }
                    if (es->mac_addr_spoofing->selected_network_interface->has_end_point_interface) {
                        d->macAddrSpoofing.selectedNetworkInterface.endPointInterface = es->mac_addr_spoofing->selected_network_interface->end_point_interface;
                    }
                }

                if (es->mac_addr_spoofing->network_interfaces)
                {
                    d->macAddrSpoofing.networkInterfaces.clear();
                    for (size_t i = 0; i < es->mac_addr_spoofing->network_interfaces->n_networks; ++i)
                    {
                        NetworkInterface networkInterface;
                        ProtoTypes__NetworkInterface *ni = es->mac_addr_spoofing->network_interfaces->networks[i];

                        if (ni->has_interface_index) {
                            networkInterface.interfaceIndex = ni->interface_index;
                        }
                        networkInterface.interfaceName = ni->interface_name;
                        networkInterface.interfaceGuid = ni->interface_guid;
                        networkInterface.networkOrSSid = ni->network_or_ssid;
                        if (ni->has_interface_type) {
                            networkInterface.interfaceType = (NETWORK_INTERACE_TYPE)ni->interface_type;
                        }
                        if (ni->has_trust_type) {
                            networkInterface.trustType = (NETWORK_TRUST_TYPE)ni->trust_type;
                        }
                        if (ni->has_active) {
                            networkInterface.active = ni->active;
                        }
                        networkInterface.friendlyName = ni->friendly_name;
                        if (ni->has_requested) {
                            networkInterface.requested = ni->requested;
                        }
                        if (ni->has_metric) {
                            networkInterface.metric = ni->metric;
                        }
                        networkInterface.physicalAddress = ni->physical_address;
                        if (ni->has_mtu) {
                            networkInterface.mtu = ni->mtu;
                        }
                        if (ni->has_state) {
                            networkInterface.state = ni->state;
                        }
                        if (ni->has_dw_type) {
                            networkInterface.dwType = ni->dw_type;
                        }
                        networkInterface.deviceName = ni->device_name;
                        if (ni->has_connector_present) {
                            networkInterface.connectorPresent = ni->connector_present;
                        }
                        if (ni->has_end_point_interface) {
                            networkInterface.endPointInterface = ni->end_point_interface;
                        }

                        d->macAddrSpoofing.networkInterfaces << networkInterface;
                    }
                }
            }

            if (es->has_dns_policy) d->dnsPolicy = (DNS_POLICY_TYPE)es->dns_policy;
            if (es->has_tap_adapter) d->tapAdapter = (TAP_ADAPTER_TYPE)es->tap_adapter;
            d->customOvpnConfigsPath = es->customovpnconfigspath;
            if (es->has_is_keep_alive_enabled) d->isKeepAliveEnabled = es->is_keep_alive_enabled;

            if (es->dns_while_connected_info)
            {
                if (es->dns_while_connected_info->has_type) d->dnsWhileConnectedInfo.setType((DNS_WHILE_CONNECTED_TYPE)es->dns_while_connected_info->type);
                d->dnsWhileConnectedInfo.setIpAddress(es->dns_while_connected_info->ip_address);
            }

            if (es->has_dns_manager) d->dnsManager = (DNS_MANAGER_TYPE)es->dns_manager;

            proto_types__engine_settings__free_unpacked(es, NULL);
        }
        settings.remove("engineSettings2");
    }

    #if defined(Q_OS_LINUX)
            repairEngineSettings();
    #endif

    qCDebugMultiline(LOG_BASIC) << "Engine settings debug info: todo";
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


#if defined(Q_OS_LINUX)
void EngineSettings::repairEngineSettings()
{
    // IKEv2 is disabled on linux but is default protocol in ProtoTypes::ConnectionSettings.
    // UDP should be default on Linux.
    if(engineSettings_.has_connection_settings() && engineSettings_.connection_settings().protocol() == types::ProtocolType::PROTOCOL_IKEV2) {
        engineSettings_.mutable_connection_settings()->set_protocol(types::ProtocolType::PROTOCOL_UDP);
        engineSettings_.mutable_connection_settings()->set_port(443);
    }
    else if(!engineSettings_.has_connection_settings()) {
        auto settings = engineSettings_.connection_settings();
        settings.set_protocol(types::ProtocolType::PROTOCOL_UDP);
        settings.set_port(443);
        *engineSettings_.mutable_connection_settings() = settings;
    }
}
#endif

} // types namespace

