#include "preferences.h"
#include "../persistentstate.h"
#include "detectlanrange.h"
#include "guisettingsfromver1.h"
#include "utils/extraconfig.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "utils/ipvalidation.h"
#include <QSettings>

Preferences::Preferences(QObject *parent) : QObject(parent)
  , receivingEngineSettings_(false)
{

}

bool Preferences::isLaunchOnStartup() const
{
    return guiSettings_.is_launch_on_startup();
}

void Preferences::setLaunchOnStartup(bool b)
{
    if (guiSettings_.is_launch_on_startup() != b)
    {
        guiSettings_.set_is_launch_on_startup(b);
        saveGuiSettings();
        emit isLaunchOnStartupChanged(guiSettings_.is_launch_on_startup());
    }
}

bool Preferences::isAutoConnect() const
{
    return guiSettings_.is_auto_connect();
}

void Preferences::setAutoConnect(bool b)
{
    if (guiSettings_.is_auto_connect() != b)
    {
        guiSettings_.set_is_auto_connect(b);
        saveGuiSettings();
        emit isAutoConnectChanged(guiSettings_.is_auto_connect());
    }
}

bool Preferences::isAllowLanTraffic() const
{
    return engineSettings_.is_allow_lan_traffic();
}

void Preferences::setAllowLanTraffic(bool b)
{
    if (engineSettings_.is_allow_lan_traffic() != b)
    {
        const auto address = Utils::getLocalIP();
        if (b && !DetectLanRange::isRfcLanRange(address)) {
            b = false;
            qCDebug(LOG_BASIC) << "IP address not in a valid RFC-1918 range:" << address;
            emit invalidLanAddressNotification(address);
        }

        engineSettings_.set_is_allow_lan_traffic(b);
        emit isAllowLanTrafficChanged(engineSettings_.is_allow_lan_traffic());
        emit updateEngineSettings();
    }
}

#ifdef Q_OS_WIN
bool Preferences::isMinimizeAndCloseToTray() const
{
    return guiSettings_.is_minimize_and_close_to_tray();
}

void Preferences::setMinimizeAndCloseToTray(bool b)
{
    if (guiSettings_.is_minimize_and_close_to_tray() != b)
    {
        guiSettings_.set_is_minimize_and_close_to_tray(b);
        saveGuiSettings();
        emit minimizeAndCloseToTrayChanged(b);
    }
}

#elif defined Q_OS_MAC
bool Preferences::isHideFromDock() const
{
    return guiSettings_.is_hide_from_dock();
}

void Preferences::setHideFromDock(bool b)
{
    if (guiSettings_.is_hide_from_dock() != b)
    {
        guiSettings_.set_is_hide_from_dock(b);
        saveGuiSettings();
        emit hideFromDockChanged(guiSettings_.is_hide_from_dock());
    }
}
#endif


bool Preferences::isShowNotifications() const
{
    return guiSettings_.is_show_notifications();
}

void Preferences::setShowNotifications(bool b)
{
    if (guiSettings_.is_show_notifications() != b)
    {
        guiSettings_.set_is_show_notifications(b);
        saveGuiSettings();
        emit isShowNotificationsChanged(guiSettings_.is_show_notifications());
    }
}

ProtoTypes::BackgroundSettings Preferences::backgroundSettings() const
{
    return guiSettings_.background_settings();
}

void Preferences::setBackgroundSettings(const ProtoTypes::BackgroundSettings &backgroundSettings)
{
    if (!google::protobuf::util::MessageDifferencer::Equals(guiSettings_.background_settings(), backgroundSettings))
    {
        *guiSettings_.mutable_background_settings() = backgroundSettings;
        saveGuiSettings();
        emit backgroundSettingsChanged(backgroundSettings);
    }
}

bool Preferences::isDockedToTray() const
{
    return guiSettings_.is_docked_to_tray();
}

void Preferences::setDockedToTray(bool b)
{
    if (guiSettings_.is_docked_to_tray() != b)
    {
        guiSettings_.set_is_docked_to_tray(b);
        saveGuiSettings();
        emit isDockedToTrayChanged(guiSettings_.is_docked_to_tray());
    }
}

const QString Preferences::language() const
{
    return QString::fromStdString(engineSettings_.language());
}

void Preferences::setLanguage(const QString &lang)
{
    if (QString::fromStdString(engineSettings_.language()) != lang)
    {
        engineSettings_.set_language(lang.toStdString());
        emit languageChanged(lang);
    }
}

ProtoTypes::OrderLocationType Preferences::locationOrder() const
{
    return guiSettings_.order_location();
}

void Preferences::setLocationOrder(ProtoTypes::OrderLocationType o)
{
    if (guiSettings_.order_location() != o)
    {
        guiSettings_.set_order_location(o);
        saveGuiSettings();
        emit locationOrderChanged(guiSettings_.order_location());
    }
}

ProtoTypes::LatencyDisplayType Preferences::latencyDisplay() const
{
    return guiSettings_.latency_display();
}

void Preferences::setLatencyDisplay(ProtoTypes::LatencyDisplayType l)
{
    if (guiSettings_.latency_display() != l)
    {
        guiSettings_.set_latency_display(l);
        saveGuiSettings();
        emit latencyDisplayChanged(guiSettings_.latency_display());
    }
}

ProtoTypes::UpdateChannel Preferences::updateChannel() const
{
    return engineSettings_.update_channel();
}

void Preferences::setUpdateChannel(ProtoTypes::UpdateChannel c)
{
    if (engineSettings_.update_channel() != c)
    {
        engineSettings_.set_update_channel(c);
        emit updateChannelChanged(engineSettings_.update_channel());
        emit updateEngineSettings();
    }
}

ProtoTypes::NetworkWhiteList Preferences::networkWhiteList()
{
    return PersistentState::instance().networkWhitelist();
}

void Preferences::setNetworkWhiteList(ProtoTypes::NetworkWhiteList l)
{
    if (!google::protobuf::util::MessageDifferencer::Equals(PersistentState::instance().networkWhitelist(), l))
    {
        PersistentState::instance().setNetworkWhitelist(l);
        emit networkWhiteListChanged(l);
    }
}

const ProtoTypes::ProxySettings &Preferences::proxySettings() const
{
    return engineSettings_.proxy_settings();
}

void Preferences::setProxySettings(const ProtoTypes::ProxySettings &ps)
{
    if(!google::protobuf::util::MessageDifferencer::Equals(engineSettings_.proxy_settings(), ps))
    {
        *engineSettings_.mutable_proxy_settings() = ps;
        emit proxySettingsChanged(engineSettings_.proxy_settings());
        emit updateEngineSettings();
    }
}

const ProtoTypes::FirewallSettings &Preferences::firewalSettings() const
{
    return engineSettings_.firewall_settings();
}

void Preferences::setFirewallSettings(const ProtoTypes::FirewallSettings &fm)
{
    if(!google::protobuf::util::MessageDifferencer::Equals(engineSettings_.firewall_settings(), fm))
    {
        *engineSettings_.mutable_firewall_settings() = fm;
        emit firewallSettingsChanged(engineSettings_.firewall_settings());
        emit updateEngineSettings();
    }
}

const ProtoTypes::ConnectionSettings &Preferences::connectionSettings() const
{
    return engineSettings_.connection_settings();
}

void Preferences::setConnectionSettings(const ProtoTypes::ConnectionSettings &cm)
{
    if(!google::protobuf::util::MessageDifferencer::Equals(engineSettings_.connection_settings(), cm))
    {
        *engineSettings_.mutable_connection_settings() = cm;
        emit connectionSettingsChanged(engineSettings_.connection_settings());
    }
}

const ProtoTypes::ApiResolution &Preferences::apiResolution() const
{
    return engineSettings_.api_resolution();
}

void Preferences::setApiResolution(const ProtoTypes::ApiResolution &ar)
{
    if(!google::protobuf::util::MessageDifferencer::Equals(engineSettings_.api_resolution(), ar))
    {
        *engineSettings_.mutable_api_resolution() = ar;
        emit apiResolutionChanged(engineSettings_.api_resolution());
        emit updateEngineSettings();
    }
}

const ProtoTypes::PacketSize &Preferences::packetSize() const
{
    return engineSettings_.packet_size();
}

void Preferences::setPacketSize(const ProtoTypes::PacketSize &ps)
{
    if(!google::protobuf::util::MessageDifferencer::Equals(engineSettings_.packet_size(), ps))
    {
        *engineSettings_.mutable_packet_size() = ps;
        emit packetSizeChanged(engineSettings_.packet_size());
        emit updateEngineSettings();
    }
}

const ProtoTypes::MacAddrSpoofing &Preferences::macAddrSpoofing() const
{
    return engineSettings_.mac_addr_spoofing();
}

void Preferences::setMacAddrSpoofing(const ProtoTypes::MacAddrSpoofing &mas)
{
    // qDebug() << "Preferences::SetMacAddrSpoofing()";
    if(!google::protobuf::util::MessageDifferencer::Equals(engineSettings_.mac_addr_spoofing(), mas))
    {
        // qDebug() << "Actually updating GUI copy of engine settings with mac_addr_spoofing";
        *engineSettings_.mutable_mac_addr_spoofing() = mas;
        emit macAddrSpoofingChanged(engineSettings_.mac_addr_spoofing());
    }
}

bool Preferences::isIgnoreSslErrors() const
{
    return engineSettings_.is_ignore_ssl_errors();
}

void Preferences::setIgnoreSslErrors(bool b)
{
    if (engineSettings_.is_ignore_ssl_errors() != b)
    {
        engineSettings_.set_is_ignore_ssl_errors(b);
        emit isIgnoreSslErrorsChanged(engineSettings_.is_ignore_ssl_errors());
        emit updateEngineSettings();
    }
}

#ifdef Q_OS_WIN
bool Preferences::isKillTcpSockets() const
{
    return engineSettings_.is_close_tcp_sockets();
}

void Preferences::setKillTcpSockets(bool b)
{
    if (engineSettings_.is_close_tcp_sockets() != b)
    {
        engineSettings_.set_is_close_tcp_sockets(b);
        emit isKillTcpSocketsChanged(engineSettings_.is_close_tcp_sockets());
        emit updateEngineSettings();
    }
}
#endif

const ProtoTypes::ShareSecureHotspot &Preferences::shareSecureHotspot() const
{
    return guiSettings_.share_secure_hotspot();
}

void Preferences::setShareSecureHotspot(const ProtoTypes::ShareSecureHotspot &ss)
{
    if(!google::protobuf::util::MessageDifferencer::Equals(guiSettings_.share_secure_hotspot(), ss))
    {
        *guiSettings_.mutable_share_secure_hotspot() = ss;
        saveGuiSettings();
        emit shareSecureHotspotChanged(guiSettings_.share_secure_hotspot());
    }
}

const ProtoTypes::ShareProxyGateway &Preferences::shareProxyGateway() const
{
    return guiSettings_.share_proxy_gateway();
}

void Preferences::setShareProxyGateway(const ProtoTypes::ShareProxyGateway &sp)
{
    if(!google::protobuf::util::MessageDifferencer::Equals(guiSettings_.share_proxy_gateway(), sp))
    {
        *guiSettings_.mutable_share_proxy_gateway() = sp;
        saveGuiSettings();
        emit shareProxyGatewayChanged(guiSettings_.share_proxy_gateway());
    }
}

const QString Preferences::debugAdvancedParameters() const
{
    return ExtraConfig::instance().getExtraConfig(false);
}

void Preferences::setDebugAdvancedParameters(const QString &p)
{
    ExtraConfig::instance().writeConfig(p);
    emit debugAdvancedParametersChanged(p);
}

#ifdef Q_OS_WIN
ProtoTypes::TapAdapterType Preferences::tapAdapter() const
{
    return engineSettings_.tap_adapter();
}

void Preferences::setTapAdapter(ProtoTypes::TapAdapterType tapAdapter)
{
    if (engineSettings_.tap_adapter() != tapAdapter)
    {
        engineSettings_.set_tap_adapter(tapAdapter);
        emit tapAdapterChanged(tapAdapter);
        emit updateEngineSettings();
    }
}
#endif

ProtoTypes::DnsPolicy Preferences::dnsPolicy() const
{
    return engineSettings_.dns_policy();
}

void Preferences::setDnsPolicy(ProtoTypes::DnsPolicy d)
{
    if (engineSettings_.dns_policy() != d)
    {
        engineSettings_.set_dns_policy(d);
        emit dnsPolicyChanged(d);
        emit updateEngineSettings();
    }
}

DnsWhileConnectedInfo Preferences::dnsWhileConnectedInfo() const
{
   return DnsWhileConnectedInfo(engineSettings_.dns_while_connected_info());
}

void Preferences::setDnsWhileConnectedInfo(DnsWhileConnectedInfo d)
{
    ProtoTypes::DnsWhileConnectedInfo protoDNS = d.toProtobuf();
    if (!google::protobuf::util::MessageDifferencer::Equals(engineSettings_.dns_while_connected_info(), protoDNS))
    {
        *engineSettings_.mutable_dns_while_connected_info() = protoDNS;
        emit dnsWhileConnectedInfoChanged(d);
        emit updateEngineSettings();
    }
}

bool Preferences::keepAlive() const
{
    return engineSettings_.is_keep_alive_enabled();
}

void Preferences::setKeepAlive(bool bEnabled)
{
    if (engineSettings_.is_keep_alive_enabled() != bEnabled)
    {
        engineSettings_.set_is_keep_alive_enabled(bEnabled);
        emit keepAliveChanged(bEnabled);
        emit updateEngineSettings();
    }
}

ProtoTypes::SplitTunneling Preferences::splitTunneling()
{
    return guiSettings_.split_tunneling();
}

QList<ProtoTypes::SplitTunnelingApp> Preferences::splitTunnelingApps()
{
    QList<ProtoTypes::SplitTunnelingApp> apps;
    ProtoTypes::SplitTunneling splitTunneling = guiSettings_.split_tunneling();
    for (int i = 0; i < splitTunneling.apps_size(); i++)
    {
        apps.append(splitTunneling.apps(i));
    }

    return apps;
}

void Preferences::setSplitTunnelingApps(QList<ProtoTypes::SplitTunnelingApp> apps)
{
    ProtoTypes::SplitTunneling st;
    for (ProtoTypes::SplitTunnelingApp app : qAsConst(apps))
    {
        *st.add_apps() = app;
    }

    *st.mutable_settings() = guiSettings_.split_tunneling().settings();
    *st.mutable_network_routes() = guiSettings_.split_tunneling().network_routes();

    *guiSettings_.mutable_split_tunneling() = st;
    saveGuiSettings();
    emit splitTunnelingChanged(st);
}

QList<ProtoTypes::SplitTunnelingNetworkRoute> Preferences::splitTunnelingNetworkRoutes()
{
    QList<ProtoTypes::SplitTunnelingNetworkRoute> routes;
    ProtoTypes::SplitTunneling splitTunneling = guiSettings_.split_tunneling();
    for (int i = 0; i < splitTunneling.network_routes_size(); i++)
    {
        routes.append(splitTunneling.network_routes(i));
    }
    return routes;
}

void Preferences::setSplitTunnelingNetworkRoutes(QList<ProtoTypes::SplitTunnelingNetworkRoute> routes)
{
    ProtoTypes::SplitTunneling st;
    for (ProtoTypes::SplitTunnelingNetworkRoute app : qAsConst(routes))
    {
        *st.add_network_routes() = app;
    }

    *st.mutable_settings() = guiSettings_.split_tunneling().settings();
    *st.mutable_apps() = guiSettings_.split_tunneling().apps();

    *guiSettings_.mutable_split_tunneling() = st;
    saveGuiSettings();
    emit splitTunnelingChanged(st);
}

ProtoTypes::SplitTunnelingSettings Preferences::splitTunnelingSettings()
{
    return guiSettings_.split_tunneling().settings();
}

void Preferences::setSplitTunnelingSettings(ProtoTypes::SplitTunnelingSettings settings)
{
    ProtoTypes::SplitTunneling st;
    *st.mutable_settings() = settings;

    *st.mutable_apps() = guiSettings_.split_tunneling().apps();
    *st.mutable_network_routes() = guiSettings_.split_tunneling().network_routes();
    *guiSettings_.mutable_split_tunneling() = st;
    saveGuiSettings();
    emit splitTunnelingChanged(st);
}

QString Preferences::customOvpnConfigsPath() const
{
    return QString::fromStdString(engineSettings_.customovpnconfigspath());
}

void Preferences::setCustomOvpnConfigsPath(const QString &path)
{
    if (QString::fromStdString(engineSettings_.customovpnconfigspath()) != path)
    {
        engineSettings_.set_customovpnconfigspath(path.toStdString());
        emit customConfigsPathChanged(path);
    }
}

void Preferences::setEngineSettings(const ProtoTypes::EngineSettings &es)
{
    receivingEngineSettings_ = true;
    setLanguage(QString::fromStdString(es.language()));
    setUpdateChannel(es.update_channel());
    setIgnoreSslErrors(es.is_ignore_ssl_errors());
#ifdef Q_OS_WIN
    setKillTcpSockets(es.is_close_tcp_sockets());
    setTapAdapter(es.tap_adapter());
#endif
    setAllowLanTraffic(es.is_allow_lan_traffic());
    setFirewallSettings(es.firewall_settings());
    setConnectionSettings(es.connection_settings());
    setApiResolution(es.api_resolution());
    setProxySettings(es.proxy_settings());
    setPacketSize(es.packet_size());
    setMacAddrSpoofing(es.mac_addr_spoofing());
    setDnsPolicy(es.dns_policy());
    setKeepAlive(es.is_keep_alive_enabled());
    setCustomOvpnConfigsPath(QString::fromStdString(es.customovpnconfigspath()));
    setDnsWhileConnectedInfo(DnsWhileConnectedInfo(es.dns_while_connected_info()));
    receivingEngineSettings_ = false;
}

/*void Preferences::setGuiSettings(const ProtoTypes::GuiSettings &gs)
{
    setLaunchOnStartup(gs.is_launch_on_startup());
    setAutoConnect(gs.is_auto_connect());
    setHideFromDockOrMinimizeToTray(gs.is_hide_from_dock_or_minimize_to_tray());
    setShowNotifications(gs.is_show_notifications());
    setLocationOrder(gs.order_location());
    setLatencyDisplay(gs.latency_display());
}*/

ProtoTypes::EngineSettings Preferences::getEngineSettings() const
{
    return engineSettings_;
}

void Preferences::saveGuiSettings() const
{
    QSettings settings;

    int size = guiSettings_.ByteSizeLong();
    QByteArray arr(size, Qt::Uninitialized);
    guiSettings_.SerializeToArray(arr.data(), size);

    settings.setValue("guiSettings", arr);
}

void Preferences::loadGuiSettings()
{
    QSettings settings;

    if (settings.contains("guiSettings"))
    {
        QByteArray arr = settings.value("guiSettings").toByteArray();
        guiSettings_.ParseFromArray(arr.data(), arr.size());
    }
    // if can't load from version 2
    // then try load from version 1
    else
    {
        guiSettings_ = GuiSettingsFromVer1::read();
    }
    qCDebugMultiline(LOG_BASIC) << "Gui settings:" << QString::fromStdString(guiSettings_.DebugString());
}

void Preferences::validateAndUpdateIfNeeded()
{
    bool is_update_needed = false;

    // Reset API resolution to automatic if the ip address hasn't been specified.
    if (!engineSettings_.api_resolution().is_automatic() &&
        engineSettings_.api_resolution().manual_ip().empty()) {
        engineSettings_.mutable_api_resolution()->set_is_automatic(true);
        emit apiResolutionChanged(engineSettings_.api_resolution());
        is_update_needed = true;
    }

    if (engineSettings_.dns_while_connected_info().type() == ProtoTypes::DNS_WHILE_CONNECTED_TYPE_CUSTOM &&
            !IpValidation::instance().isIp(QString::fromStdString(engineSettings_.dns_while_connected_info().ip_address())))
    {
        ProtoTypes::DnsWhileConnectedInfo protoDns;
        protoDns.set_type(ProtoTypes::DNS_WHILE_CONNECTED_TYPE_ROBERT);
        protoDns.set_ip_address("");
        *engineSettings_.mutable_dns_while_connected_info() = protoDns;
        emit dnsWhileConnectedInfoChanged(DnsWhileConnectedInfo(engineSettings_.dns_while_connected_info()));
        emit reportErrorToUser("Invalid DNS Settings", "'DNS while connected' was not configured with a valid IP Address. DNS was reverted to ROBERT (default).");
        is_update_needed = true;
    }

    if (is_update_needed)
        emit updateEngineSettings();
}

bool Preferences::isReceivingEngineSettings() const
{
    return receivingEngineSettings_;
}
