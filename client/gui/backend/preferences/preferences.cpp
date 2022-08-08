#include "preferences.h"

#include <QSettings>
#include <QSystemTrayIcon>

#include "../persistentstate.h"
#include "detectlanrange.h"
#include "utils/extraconfig.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "utils/ipvalidation.h"
#include "types/global_consts.h"
#include "legacy_protobuf_support/legacy_protobuf.h"

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
    settings.set_protocol(PROTOCOL_TYPE_UDP);
    settings.set_port(443);
    *engineSettings_.mutable_connection_settings() = settings;
#endif
}

bool Preferences::isLaunchOnStartup() const
{
    return guiSettings_.isLaunchOnStartup;
}

void Preferences::setLaunchOnStartup(bool b)
{
    if (guiSettings_.isLaunchOnStartup != b)
    {
        guiSettings_.isLaunchOnStartup = b;
        saveGuiSettings();
        emit isLaunchOnStartupChanged(guiSettings_.isLaunchOnStartup);
    }
}

bool Preferences::isAutoConnect() const
{
    return guiSettings_.isAutoConnect;
}

void Preferences::setAutoConnect(bool b)
{
    if (guiSettings_.isAutoConnect != b)
    {
        guiSettings_.isAutoConnect = b;
        saveGuiSettings();
        emit isAutoConnectChanged(guiSettings_.isAutoConnect);
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
    return QSystemTrayIcon::isSystemTrayAvailable() && guiSettings_.isMinimizeAndCloseToTray;
}

void Preferences::setMinimizeAndCloseToTray(bool b)
{
    if (guiSettings_.isMinimizeAndCloseToTray != b)
    {
        guiSettings_.isMinimizeAndCloseToTray = b;
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
    return guiSettings_.isStartMinimized;
}

void Preferences::setStartMinimized(bool b)
{
    if(guiSettings_.isStartMinimized != b)
    {
        guiSettings_.isStartMinimized = b;
        saveGuiSettings();
        emit isStartMinimizedChanged(b);
    }
}


bool Preferences::isShowNotifications() const
{
    return guiSettings_.isShowNotifications;
}

void Preferences::setShowNotifications(bool b)
{
    if (guiSettings_.isShowNotifications != b)
    {
        guiSettings_.isShowNotifications = b;
        saveGuiSettings();
        emit isShowNotificationsChanged(guiSettings_.isShowNotifications);
    }
}

types::BackgroundSettings Preferences::backgroundSettings() const
{
    return guiSettings_.backgroundSettings;
}

void Preferences::setBackgroundSettings(const types::BackgroundSettings &backgroundSettings)
{
    if (guiSettings_.backgroundSettings != backgroundSettings)
    {
        guiSettings_.backgroundSettings = backgroundSettings;
        saveGuiSettings();
        emit backgroundSettingsChanged(backgroundSettings);
    }
}

bool Preferences::isDockedToTray() const
{
    return guiSettings_.isDockedToTray;
}

void Preferences::setDockedToTray(bool b)
{
    if (guiSettings_.isDockedToTray != b)
    {
        guiSettings_.isDockedToTray = b;
        saveGuiSettings();
        emit isDockedToTrayChanged(guiSettings_.isDockedToTray);
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

ORDER_LOCATION_TYPE Preferences::locationOrder() const
{
    return guiSettings_.orderLocation;
}

void Preferences::setLocationOrder(ORDER_LOCATION_TYPE o)
{
    if (guiSettings_.orderLocation != o)
    {
        guiSettings_.orderLocation = o;
        saveGuiSettings();
        emit locationOrderChanged(guiSettings_.orderLocation);
    }
}

LATENCY_DISPLAY_TYPE Preferences::latencyDisplay() const
{
    return guiSettings_.latencyDisplay;
}

void Preferences::setLatencyDisplay(LATENCY_DISPLAY_TYPE l)
{
    if (guiSettings_.latencyDisplay != l)
    {
        guiSettings_.latencyDisplay = l;
        saveGuiSettings();
        emit latencyDisplayChanged(guiSettings_.latencyDisplay);
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

const types::ShareSecureHotspot &Preferences::shareSecureHotspot() const
{
    return guiSettings_.shareSecureHotspot;
}

void Preferences::setShareSecureHotspot(const types::ShareSecureHotspot &ss)
{
    if(guiSettings_.shareSecureHotspot != ss)
    {
        guiSettings_.shareSecureHotspot = ss;
        saveGuiSettings();
        emit shareSecureHotspotChanged(guiSettings_.shareSecureHotspot);
    }
}

const types::ShareProxyGateway &Preferences::shareProxyGateway() const
{
    return guiSettings_.shareProxyGateway;
}

void Preferences::setShareProxyGateway(const types::ShareProxyGateway &sp)
{
    if(guiSettings_.shareProxyGateway != sp)
    {
        guiSettings_.shareProxyGateway = sp;
        saveGuiSettings();
        emit shareProxyGatewayChanged(guiSettings_.shareProxyGateway);
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
    return guiSettings_.splitTunneling;
}

QList<types::SplitTunnelingApp> Preferences::splitTunnelingApps()
{
    return guiSettings_.splitTunneling.apps.toList();
}

void Preferences::setSplitTunnelingApps(QList<types::SplitTunnelingApp> apps)
{
    if (guiSettings_.splitTunneling.apps.toList() != apps)
    {
        guiSettings_.splitTunneling.apps = apps.toVector();
        saveGuiSettings();
        emit splitTunnelingChanged(guiSettings_.splitTunneling);
    }
}

QList<types::SplitTunnelingNetworkRoute> Preferences::splitTunnelingNetworkRoutes()
{
    return guiSettings_.splitTunneling.networkRoutes.toList();
}

void Preferences::setSplitTunnelingNetworkRoutes(QList<types::SplitTunnelingNetworkRoute> routes)
{
    if (guiSettings_.splitTunneling.networkRoutes.toList() != routes)
    {
        guiSettings_.splitTunneling.networkRoutes = routes.toVector();
        saveGuiSettings();
        emit splitTunnelingChanged(guiSettings_.splitTunneling);
    }
}

types::SplitTunnelingSettings Preferences::splitTunnelingSettings()
{
    return guiSettings_.splitTunneling.settings;
}

void Preferences::setSplitTunnelingSettings(types::SplitTunnelingSettings settings)
{
    if (guiSettings_.splitTunneling.settings != settings)
    {
        guiSettings_.splitTunneling.settings = settings;
        saveGuiSettings();
        emit splitTunnelingChanged(guiSettings_.splitTunneling);
    }
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

types::EngineSettings Preferences::getEngineSettings() const
{
    return engineSettings_;
}

void Preferences::saveGuiSettings() const
{
    QByteArray arr;
    {
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << magic_;
        ds << versionForSerialization_;
        ds << guiSettings_;
    }

    QSettings settings;
    SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);
    settings.setValue("guiSettings", simpleCrypt.encryptToString(arr));
}

void Preferences::loadGuiSettings()
{
    bool bLoaded = false;
    SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);

    QSettings settings;
    if (settings.contains("guiSettings"))
    {
        QString str = settings.value("guiSettings").toString();
        QByteArray arr = simpleCrypt.decryptToByteArray(str);

        QDataStream ds(&arr, QIODevice::ReadOnly);

        quint32 magic, version;
        ds >> magic;
        if (magic == magic_)
        {
            ds >> version;
            if (version <= versionForSerialization_)
            {
                ds >> guiSettings_;
                if (ds.status() == QDataStream::Ok)
                {
                    bLoaded = true;
                }
            }
        }
    }
    if (!bLoaded)
    {
        // try load from legacy protobuf
        // todo remove this code at some point later
        QByteArray arr = settings.value("guiSettings").toByteArray();
        bLoaded = LegacyProtobufSupport::loadGuiSettings(arr, guiSettings_);
    }

    if (!bLoaded)
    {
        guiSettings_ = types::GuiSettings();    // reset to defaults
    }


    /*qCDebugMultiline(LOG_BASIC) << "Gui settings:" << QString::fromStdString(guiSettings_.DebugString());*/
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
        ((connSettings.protocol == PROTOCOL::WSTUNNEL) ||
         (connSettings.protocol == PROTOCOL::WIREGUARD)))
    {
        connSettings.protocol = PROTOCOL::IKEV2;
        connSettings.port = 500;
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
    return guiSettings_.isShowLocationHealth;
}

void Preferences::setShowLocationLoad(bool b)
{
    if (guiSettings_.isShowLocationHealth != b)
    {
        guiSettings_.isShowLocationHealth = b;
        saveGuiSettings();
        emit showLocationLoadChanged(guiSettings_.isShowLocationHealth);
    }
}
