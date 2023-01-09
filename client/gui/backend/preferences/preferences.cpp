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

Preferences::Preferences(QObject *parent) : QObject(parent)
  , receivingEngineSettings_(false)
{
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
        emit updateEngineSettings();
        emit isAllowLanTrafficChanged(engineSettings_.isAllowLanTraffic());
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
    return guiSettings_.isHideFromDock;
}

void Preferences::setHideFromDock(bool b)
{
    if (guiSettings_.isHideFromDock != b)
    {
        guiSettings_.isHideFromDock = b;
        saveGuiSettings();
        emit hideFromDockChanged(guiSettings_.isHideFromDock);
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
        emit updateEngineSettings();
        emit updateChannelChanged(engineSettings_.updateChannel());
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

const types::ConnectionSettings Preferences::networkPreferredProtocol(QString networkOrSsid) const
{
    return engineSettings_.networkPreferredProtocols()[networkOrSsid];
}

void Preferences::setNetworkPreferredProtocols(const QMap<QString, types::ConnectionSettings> &preferredProtocols)
{
    if (engineSettings_.networkPreferredProtocols() != preferredProtocols)
    {
        engineSettings_.setNetworkPreferredProtocols(preferredProtocols);
        emit updateEngineSettings();
        emit networkPreferredProtocolsChanged(engineSettings_.networkPreferredProtocols());
    }
}

void Preferences::setNetworkPreferredProtocol(QString networkOrSsid, const types::ConnectionSettings &settings)
{
    QMap<QString, types::ConnectionSettings> map = engineSettings_.networkPreferredProtocols();
    if (map[networkOrSsid] != settings)
    {
        map[networkOrSsid] = settings;
        engineSettings_.setNetworkPreferredProtocols(map);
        emit updateEngineSettings();
        emit networkPreferredProtocolsChanged(engineSettings_.networkPreferredProtocols());
    }
}

bool Preferences::isAutoSecureNetworks()
{
    return guiSettings_.isAutoSecureNetworks;
}

void Preferences::setAutoSecureNetworks(bool b)
{
    if (guiSettings_.isAutoSecureNetworks != b)
    {
        guiSettings_.isAutoSecureNetworks = b;
        emit isAutoSecureNetworksChanged(guiSettings_.isAutoSecureNetworks);
        saveGuiSettings();
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
        emit updateEngineSettings();
        emit proxySettingsChanged(engineSettings_.proxySettings());
    }
}

const types::FirewallSettings &Preferences::firewallSettings() const
{
    return engineSettings_.firewallSettings();
}

void Preferences::setFirewallSettings(const types::FirewallSettings &fs)
{
    if(engineSettings_.firewallSettings() != fs)
    {
        engineSettings_.setFirewallSettings(fs);
        emit updateEngineSettings();
        emit firewallSettingsChanged(engineSettings_.firewallSettings());
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
        emit updateEngineSettings();
        emit connectionSettingsChanged(engineSettings_.connectionSettings());
    }
}

const types::ApiResolutionSettings &Preferences::apiResolution() const
{
    return engineSettings_.apiResolutionSettings();
}

void Preferences::setApiResolution(const types::ApiResolutionSettings &s)
{
    if(engineSettings_.apiResolutionSettings() != s)
    {
        engineSettings_.setApiResolutionSettings(s);
        emit updateEngineSettings();
        emit apiResolutionChanged(engineSettings_.apiResolutionSettings());
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
        emit updateEngineSettings();
        emit packetSizeChanged(engineSettings_.packetSize());
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
        emit updateEngineSettings();
        emit isIgnoreSslErrorsChanged(engineSettings_.isIgnoreSslErrors());
    }
}

#if defined(Q_OS_WIN)
bool Preferences::isTerminateSockets() const
{
    return engineSettings_.isTerminateSockets();
}

void Preferences::setTerminateSockets(bool b)
{
    if (engineSettings_.isTerminateSockets() != b)
    {
        engineSettings_.setIsTerminateSockets(b);
        emit updateEngineSettings();
        emit isTerminateSocketsChanged(engineSettings_.isTerminateSockets());
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
        emit updateEngineSettings();
        emit tapAdapterChanged(tapAdapter);
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
        emit updateEngineSettings();
        emit dnsPolicyChanged(d);
    }
}

#ifdef Q_OS_LINUX
DNS_MANAGER_TYPE Preferences::dnsManager() const
{
    return engineSettings_.dnsManager();
}
void Preferences::setDnsManager(DNS_MANAGER_TYPE d)
{
    if (engineSettings_.dnsManager() != d)
    {
        engineSettings_.setDnsManager(d);
        emit updateEngineSettings();
        emit dnsManagerChanged(d);
    }
}
#endif

types::ConnectedDnsInfo Preferences::connectedDnsInfo() const
{
   return engineSettings_.connectedDnsInfo();
}

void Preferences::setConnectedDnsInfo(types::ConnectedDnsInfo d)
{
    if (engineSettings_.connectedDnsInfo() != d)
    {
        engineSettings_.setConnectedDnsInfo(d);
        emit updateEngineSettings();
        emit connectedDnsInfoChanged(d);
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
        emit updateEngineSettings();
        emit keepAliveChanged(bEnabled);
    }
}

APP_SKIN Preferences::appSkin() const
{
    return guiSettings_.appSkin;
}

void Preferences::setAppSkin(APP_SKIN appSkin)
{
    if (guiSettings_.appSkin != appSkin)
    {
        guiSettings_.appSkin = appSkin;
        saveGuiSettings();
        emit appSkinChanged(guiSettings_.appSkin);
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
#if defined(Q_OS_WIN)
    setTerminateSockets(es.isTerminateSockets());
#endif
#if defined(Q_OS_WIN)
    setTapAdapter(es.tapAdapter());
#endif
    setAllowLanTraffic(es.isAllowLanTraffic());
    setFirewallSettings(es.firewallSettings());
    setConnectionSettings(es.connectionSettings());
    setApiResolution(es.apiResolutionSettings());
    setProxySettings(es.proxySettings());
    setPacketSize(es.packetSize());
    setMacAddrSpoofing(es.macAddrSpoofing());
    setDnsPolicy(es.dnsPolicy());
    setKeepAlive(es.isKeepAliveEnabled());
    setCustomOvpnConfigsPath(es.customOvpnConfigsPath());
    setConnectedDnsInfo(es.connectedDnsInfo());
#ifdef Q_OS_LINUX
    setDnsManager(es.dnsManager());
#endif
    setNetworkPreferredProtocols(es.networkPreferredProtocols());
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
    settings.setValue("guiSettings2", simpleCrypt.encryptToString(arr));
    settings.sync();
}

void Preferences::loadGuiSettings()
{
    bool bLoaded = false;
    SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);

    QSettings settings;
    if (settings.contains("guiSettings"))
    {
        // try load from legacy protobuf
        // todo remove this code at some point later
        QByteArray arr = settings.value("guiSettings").toByteArray();
        bLoaded = LegacyProtobufSupport::loadGuiSettings(arr, guiSettings_);
        if (bLoaded)
        {
            settings.remove("guiSettings");
        }
    }
    if (!bLoaded && settings.contains("guiSettings2"))
    {
        QString str = settings.value("guiSettings2").toString();
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
        qCDebug(LOG_BASIC) << "Could not load GUI settings -- resetting to defaults";
        guiSettings_ = types::GuiSettings();    // reset to defaults
    }


    qCDebug(LOG_BASIC) << "Gui settings" << guiSettings_;
}

void Preferences::validateAndUpdateIfNeeded()
{
    bool is_update_needed = false;

    // Reset API resolution to automatic if the ip address hasn't been specified.
    if (!engineSettings_.apiResolutionSettings().getIsAutomatic() &&
        engineSettings_.apiResolutionSettings().getManualAddress().isEmpty())
    {
        types::ApiResolutionSettings ds = engineSettings_.apiResolutionSettings();
        ds.set(true, ds.getManualAddress());
        engineSettings_.setApiResolutionSettings(ds);
        emit apiResolutionChanged(engineSettings_.apiResolutionSettings());
        is_update_needed = true;
    }

    if (engineSettings_.connectedDnsInfo().type() == CONNECTED_DNS_TYPE_CUSTOM &&
            !IpValidation::isIp(engineSettings_.connectedDnsInfo().ipAddress()))
    {
        types::ConnectedDnsInfo dns;
        dns.setType(CONNECTED_DNS_TYPE_ROBERT);
        dns.setIpAddress("");
        engineSettings_.setConnectedDnsInfo(dns);
        emit connectedDnsInfoChanged(engineSettings_.connectedDnsInfo());
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
