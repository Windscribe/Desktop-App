#include "preferences.h"
#include "../persistentstate.h"
#include "detectlanrange.h"
#include "guisettingsfromver1.h"
#include "utils/extraconfig.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "utils/ipvalidation.h"
#include <QSettings>
#include <QSystemTrayIcon>

#if defined(Q_OS_WINDOWS)
#include "utils/winutils.h"
#endif

Preferences::Preferences(QObject *parent) : QObject(parent)
  , receivingEngineSettings_(false)
{
#if defined(Q_OS_LINUX)
    // ProtoTypes::ConnectionSettings has IKEv2 as default protocol in default instance.
    // But Linux doesn't support IKEv2. It is necessary to change with UDP.
    auto settings = engineSettings_.connection_settings();
    settings.set_protocol(ProtoTypes::Protocol::PROTOCOL_UDP);
    settings.set_port(443);
    *engineSettings_.mutable_connection_settings() = settings;
#endif
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
    return engineSettings_.isAllowLanTraffic();
}

void Preferences::setAllowLanTraffic(bool b)
{
    if (engineSettings_.isAllowLanTraffic() != b)
    {
        const auto address = Utils::getLocalIP();
        if (b && !DetectLanRange::isRfcLanRange(address)) {
            b = false;
            qCDebug(LOG_BASIC) << "IP address not in a valid RFC-1918 range:" << address;
            emit invalidLanAddressNotification(address);
        }

        engineSettings_.setIsAllowLanTraffic(b);
        emit isAllowLanTrafficChanged(engineSettings_.isAllowLanTraffic());
        emit updateEngineSettings();
    }
}

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
bool Preferences::isMinimizeAndCloseToTray() const
{
    return QSystemTrayIcon::isSystemTrayAvailable() && guiSettings_.is_minimize_and_close_to_tray();
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
#elif defined Q_OS_LINUX

#endif

bool Preferences::isStartMinimized() const
{
    return guiSettings_.is_start_minimized();
}

void Preferences::setStartMinimized(bool b)
{
    if(guiSettings_.is_start_minimized() != b)
    {
        guiSettings_.set_is_start_minimized(b);
        saveGuiSettings();
        emit isStartMinimizedChanged(b);
    }
}


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
    return engineSettings_.language();
}

void Preferences::setLanguage(const QString &lang)
{
    if (engineSettings_.language() != lang)
    {
        engineSettings_.setLanguage(lang);
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

UPDATE_CHANNEL Preferences::updateChannel() const
{
    return engineSettings_.updateChannel();
}

void Preferences::setUpdateChannel(UPDATE_CHANNEL c)
{
    if (engineSettings_.updateChannel() != c)
    {
        engineSettings_.setUpdateChannel(c);
        emit updateChannelChanged(engineSettings_.updateChannel());
        emit updateEngineSettings();
    }
}

QVector<types::NetworkInterface> Preferences::networkWhiteList()
{
    return PersistentState::instance().networkWhitelist();
}

void Preferences::setNetworkWhiteList(const QVector<types::NetworkInterface> &l)
{
    if (PersistentState::instance().networkWhitelist() != l)
    {
        PersistentState::instance().setNetworkWhitelist(l);
        emit networkWhiteListChanged(l);
    }
}

const types::ProxySettings &Preferences::proxySettings() const
{
    return engineSettings_.proxySettings();
}

void Preferences::setProxySettings(const types::ProxySettings &ps)
{
    if (engineSettings_.proxySettings() != ps)
    {
        engineSettings_.setProxySettings(ps);
        emit proxySettingsChanged(engineSettings_.proxySettings());
        emit updateEngineSettings();
    }
}

const types::FirewallSettings &Preferences::firewalSettings() const
{
    return engineSettings_.firewallSettings();
}

void Preferences::setFirewallSettings(const types::FirewallSettings &fs)
{
    if(engineSettings_.firewallSettings() != fs)
    {
        engineSettings_.setFirewallSettings(fs);
        emit firewallSettingsChanged(engineSettings_.firewallSettings());
        emit updateEngineSettings();
    }
}

const types::ConnectionSettings &Preferences::connectionSettings() const
{
    return engineSettings_.connectionSettings();
}

void Preferences::setConnectionSettings(const types::ConnectionSettings &cs)
{
    if (engineSettings_.connectionSettings() != cs)
    {
        engineSettings_.setConnectionSettings(cs);
        emit connectionSettingsChanged(engineSettings_.connectionSettings());
        emit updateEngineSettings();
    }
}

const types::DnsResolutionSettings &Preferences::apiResolution() const
{
    return engineSettings_.dnsResolutionSettings();
}

void Preferences::setApiResolution(const types::DnsResolutionSettings &s)
{
    if(engineSettings_.dnsResolutionSettings() != s)
    {
        engineSettings_.setDnsResolutionSettings(s);
        emit apiResolutionChanged(engineSettings_.dnsResolutionSettings());
        emit updateEngineSettings();
    }
}

const types::PacketSize &Preferences::packetSize() const
{
    return engineSettings_.packetSize();
}

void Preferences::setPacketSize(const types::PacketSize &ps)
{
    if(engineSettings_.packetSize() != ps)
    {
        engineSettings_.setPacketSize(ps);
        emit packetSizeChanged(engineSettings_.packetSize());
        emit updateEngineSettings();
    }
}

const types::MacAddrSpoofing &Preferences::macAddrSpoofing() const
{
    return engineSettings_.macAddrSpoofing();
}

void Preferences::setMacAddrSpoofing(const types::MacAddrSpoofing &mas)
{
    if(engineSettings_.macAddrSpoofing() != mas)
    {
        engineSettings_.setMacAddrSpoofing(mas);
        emit macAddrSpoofingChanged(engineSettings_.macAddrSpoofing());
    }
}

bool Preferences::isIgnoreSslErrors() const
{
    return engineSettings_.isIgnoreSslErrors();
}

void Preferences::setIgnoreSslErrors(bool b)
{
    if (engineSettings_.isIgnoreSslErrors() != b)
    {
        engineSettings_.setIsIgnoreSslErrors(b);
        emit isIgnoreSslErrorsChanged(engineSettings_.isIgnoreSslErrors());
        emit updateEngineSettings();
    }
}

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
bool Preferences::isKillTcpSockets() const
{
    return engineSettings_.isCloseTcpSockets();
}

void Preferences::setKillTcpSockets(bool b)
{
    if (engineSettings_.isCloseTcpSockets() != b)
    {
        engineSettings_.setIsCloseTcpSockets(b);
        emit isKillTcpSocketsChanged(engineSettings_.isCloseTcpSockets());
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
TAP_ADAPTER_TYPE Preferences::tapAdapter() const
{
    return engineSettings_.tapAdapter();
}

void Preferences::setTapAdapter(TAP_ADAPTER_TYPE tapAdapter)
{
    if (engineSettings_.tapAdapter() != tapAdapter)
    {
        engineSettings_.setTapAdapter(tapAdapter);
        emit tapAdapterChanged(tapAdapter);
        emit updateEngineSettings();
    }
}
#endif

DNS_POLICY_TYPE Preferences::dnsPolicy() const
{
    return engineSettings_.dnsPolicy();
}

void Preferences::setDnsPolicy(DNS_POLICY_TYPE d)
{
    if (engineSettings_.dnsPolicy() != d)
    {
        engineSettings_.setDnsPolicy(d);
        emit dnsPolicyChanged(d);
        emit updateEngineSettings();
    }
}

#ifdef Q_OS_LINUX
ProtoTypes::DnsManagerType Preferences::dnsManager() const
{
    return engineSettings_.dns_manager();
}
void Preferences::setDnsManager(ProtoTypes::DnsManagerType d)
{
    if (engineSettings_.dns_manager() != d)
    {
        engineSettings_.set_dns_manager(d);
        emit dnsManagerChanged(d);
        emit updateEngineSettings();
    }
}
#endif

types::DnsWhileConnectedInfo Preferences::dnsWhileConnectedInfo() const
{
   return engineSettings_.dnsWhileConnectedInfo();
}

void Preferences::setDnsWhileConnectedInfo(types::DnsWhileConnectedInfo d)
{
    if (engineSettings_.dnsWhileConnectedInfo() != d)
    {
        engineSettings_.setDnsWhileConnectedInfo(d);
        emit dnsWhileConnectedInfoChanged(d);
        emit updateEngineSettings();
    }
}

bool Preferences::keepAlive() const
{
    return engineSettings_.isKeepAliveEnabled();
}

void Preferences::setKeepAlive(bool bEnabled)
{
    if (engineSettings_.isKeepAliveEnabled() != bEnabled)
    {
        engineSettings_.setIsKeepAliveEnabled(bEnabled);
        emit keepAliveChanged(bEnabled);
        emit updateEngineSettings();
    }
}

types::SplitTunneling Preferences::splitTunneling()
{
    return splitTunneling_;
}

QList<types::SplitTunnelingApp> Preferences::splitTunnelingApps()
{
    return splitTunneling_.apps.toList();
}

void Preferences::setSplitTunnelingApps(QList<types::SplitTunnelingApp> apps)
{
    //todo move to gui settings
    types::SplitTunneling st = splitTunneling_;
    st.apps = apps.toVector();
    saveGuiSettings();
    emit splitTunnelingChanged(st);
}

QList<types::SplitTunnelingNetworkRoute> Preferences::splitTunnelingNetworkRoutes()
{
    return splitTunneling_.networkRoutes.toList();
}

void Preferences::setSplitTunnelingNetworkRoutes(QList<types::SplitTunnelingNetworkRoute> routes)
{
    //todo move to guiSettings
    types::SplitTunneling st = splitTunneling_;
    st.networkRoutes = routes.toVector();
    saveGuiSettings();
    emit splitTunnelingChanged(st);
}

types::SplitTunnelingSettings Preferences::splitTunnelingSettings()
{
    //return guiSettings_.split_tunneling().settings();
    return splitTunneling_.settings;
}

void Preferences::setSplitTunnelingSettings(types::SplitTunnelingSettings settings)
{
    //todo move to guiSettings
    types::SplitTunneling st = splitTunneling_;
    st.settings = settings;
    saveGuiSettings();
    emit splitTunnelingChanged(st);
}

QString Preferences::customOvpnConfigsPath() const
{
    return engineSettings_.customOvpnConfigsPath();
}

void Preferences::setCustomOvpnConfigsPath(const QString &path)
{
    if (engineSettings_.customOvpnConfigsPath() != path)
    {
        engineSettings_.setCustomOvpnConfigsPath(path);
        emit customConfigsPathChanged(path);
    }
}

void Preferences::setEngineSettings(const types::EngineSettings &es)
{
    receivingEngineSettings_ = true;
    setLanguage(es.language());
    setUpdateChannel(es.updateChannel());
    setIgnoreSslErrors(es.isIgnoreSslErrors());
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    setKillTcpSockets(es.isCloseTcpSockets());
#endif
#if defined(Q_OS_WIN)
    setTapAdapter(es.tapAdapter());
#endif
    setAllowLanTraffic(es.isAllowLanTraffic());
    setFirewallSettings(es.firewallSettings());
    setConnectionSettings(es.connectionSettings());
    setApiResolution(es.dnsResolutionSettings());
    setProxySettings(es.proxySettings());
    setPacketSize(es.packetSize());
    setMacAddrSpoofing(es.macAddrSpoofing());
    setDnsPolicy(es.dnsPolicy());
    setKeepAlive(es.isKeepAliveEnabled());
    setCustomOvpnConfigsPath(es.customOvpnConfigsPath());
    setDnsWhileConnectedInfo(es.dnsWhileConnectedInfo());
#ifdef Q_OS_LINUX
    setDnsManager(es.dnsManager());
#endif
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

types::EngineSettings Preferences::getEngineSettings() const
{
    return engineSettings_;
}

void Preferences::saveGuiSettings() const
{
    QSettings settings;

    int size = (int)guiSettings_.ByteSizeLong();
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
    if (!engineSettings_.dnsResolutionSettings().getIsAutomatic() &&
        engineSettings_.dnsResolutionSettings().getManualIp().isEmpty())
    {
        types::DnsResolutionSettings ds = engineSettings_.dnsResolutionSettings();
        ds.set(true, ds.getManualIp());
        engineSettings_.setDnsResolutionSettings(ds);
        emit apiResolutionChanged(engineSettings_.dnsResolutionSettings());
        is_update_needed = true;
    }

    if (engineSettings_.dnsWhileConnectedInfo().type() == DNS_WHILE_CONNECTED_TYPE_CUSTOM &&
            !IpValidation::instance().isIp(engineSettings_.dnsWhileConnectedInfo().ipAddress()))
    {
        types::DnsWhileConnectedInfo dns;
        dns.setType(DNS_WHILE_CONNECTED_TYPE_ROBERT);
        dns.setIpAddress("");
        engineSettings_.setDnsWhileConnectedInfo(dns);
        emit dnsWhileConnectedInfoChanged(engineSettings_.dnsWhileConnectedInfo());
        emit reportErrorToUser("Invalid DNS Settings", "'DNS while connected' was not configured with a valid IP Address. DNS was reverted to ROBERT (default).");
        is_update_needed = true;
    }

    #if defined(Q_OS_WINDOWS)
    types::ConnectionSettings connSettings = engineSettings_.connectionSettings();
    if (!WinUtils::isWindows64Bit() &&
        ((connSettings.protocol() == types::ProtocolType::PROTOCOL_WSTUNNEL) ||
         (connSettings.protocol() == types::ProtocolType::PROTOCOL_WIREGUARD)))
    {
        connSettings.set(types::ProtocolType::PROTOCOL_IKEV2, 500, connSettings.isAutomatic());
        engineSettings_.setConnectionSettings(connSettings);
        emit connectionSettingsChanged(engineSettings_.connectionSettings());
        emit reportErrorToUser("WireGuard and WStunnel Not Supported", "The WireGuard and WStunnel protocols are no longer supported on 32-bit Windows. The 'Connection Mode' protocol has been changed to IKEv2.");
        is_update_needed = true;
    }
    #endif

    if (is_update_needed)
        emit updateEngineSettings();
}

bool Preferences::isReceivingEngineSettings() const
{
    return receivingEngineSettings_;
}

bool Preferences::isShowLocationLoad() const
{
    return guiSettings_.is_show_location_health();
}

void Preferences::setShowLocationLoad(bool b)
{
    if (guiSettings_.is_show_location_health() != b)
    {
        guiSettings_.set_is_show_location_health(b);
        saveGuiSettings();
        emit showLocationLoadChanged(guiSettings_.is_show_location_health());
    }
}
