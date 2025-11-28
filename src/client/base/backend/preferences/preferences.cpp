#include "preferences.h"

#include <QIODevice>
#include <QSettings>
#ifndef CLI_ONLY
#include <QSystemTrayIcon>
#endif

#include "../persistentstate.h"
#include "detectlanrange.h"
#include "utils/extraconfig.h"
#include "utils/log/categories.h"
#include "utils/utils.h"
#include "utils/ipvalidation.h"
#include "utils/simplecrypt.h"
#include "types/global_consts.h"


Preferences::Preferences(QObject *parent) : QObject(parent)
  , isSettingEngineSettings_(false)
{
}

Preferences::~Preferences()
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
        engineSettings_.setIsAllowLanTraffic(b);
        emitEngineSettingsChanged();
        emit isAllowLanTrafficChanged(engineSettings_.isAllowLanTraffic());
    }
}

bool Preferences::isMinimizeAndCloseToTray() const
{
#ifndef CLI_ONLY
    return QSystemTrayIcon::isSystemTrayAvailable() && guiSettings_.isMinimizeAndCloseToTray;
#else
    return false;
#endif
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

#if defined Q_OS_MACOS
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
#endif

bool Preferences::isStartMinimized() const
{
    return guiSettings_.isStartMinimized;
}

void Preferences::setStartMinimized(bool b)
{
    if (guiSettings_.isStartMinimized != b)
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

types::SoundSettings Preferences::soundSettings() const
{
    return guiSettings_.soundSettings;
}

void Preferences::setSoundSettings(const types::SoundSettings &soundSettings)
{
    if (guiSettings_.soundSettings != soundSettings)
    {
        guiSettings_.soundSettings = soundSettings;
        saveGuiSettings();
        emit soundSettingsChanged(soundSettings);
    }
}

bool Preferences::isDockedToTray() const
{
    // no longer supported on Linux
#ifdef Q_OS_LINUX
    return false;
#else
    return guiSettings_.isDockedToTray;
#endif
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
        emitEngineSettingsChanged();
        emit languageChanged(lang);
    }

    // Also update the language in the general settings file
    // This is so a future installer (update or reinstall) can determine the correct language to use
    QSettings settings;
    settings.setValue("language", lang);
    settings.sync();
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

UPDATE_CHANNEL Preferences::updateChannel() const
{
    return engineSettings_.updateChannel();
}

void Preferences::setUpdateChannel(UPDATE_CHANNEL c)
{
    if (engineSettings_.updateChannel() != c)
    {
        engineSettings_.setUpdateChannel(c);
        emitEngineSettingsChanged();
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

const QMap<QString, types::ConnectionSettings> Preferences::networkPreferredProtocols() const
{
    return engineSettings_.networkPreferredProtocols();
}

bool Preferences::hasNetworkPreferredProtocol(QString networkOrSsid) const
{
    return engineSettings_.networkPreferredProtocols().contains(networkOrSsid) &&
           !engineSettings_.networkPreferredProtocols()[networkOrSsid].isAutomatic();
}

void Preferences::setNetworkPreferredProtocols(const QMap<QString, types::ConnectionSettings> &preferredProtocols)
{
    if (engineSettings_.networkPreferredProtocols() != preferredProtocols)
    {
        engineSettings_.setNetworkPreferredProtocols(preferredProtocols);
        emitEngineSettingsChanged();
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
        emitEngineSettingsChanged();
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
        emitEngineSettingsChanged();
        emit proxySettingsChanged(engineSettings_.proxySettings());
    }
}

const types::FirewallSettings &Preferences::firewallSettings() const
{
    return engineSettings_.firewallSettings();
}

void Preferences::setFirewallSettings(const types::FirewallSettings &fs)
{
    if (engineSettings_.firewallSettings() != fs)
    {
        engineSettings_.setFirewallSettings(fs);
        emitEngineSettingsChanged();
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
        emitEngineSettingsChanged();
        emit connectionSettingsChanged(engineSettings_.connectionSettings());
    }
}

const types::PacketSize &Preferences::packetSize() const
{
    return engineSettings_.packetSize();
}

void Preferences::setPacketSize(const types::PacketSize &ps)
{
    if (engineSettings_.packetSize() != ps)
    {
        engineSettings_.setPacketSize(ps);
        emitEngineSettingsChanged();
        emit packetSizeChanged(engineSettings_.packetSize());
    }
}

const types::MacAddrSpoofing &Preferences::macAddrSpoofing() const
{
    return engineSettings_.macAddrSpoofing();
}

void Preferences::setMacAddrSpoofing(const types::MacAddrSpoofing &mas)
{
    if (engineSettings_.macAddrSpoofing() != mas)
    {
        engineSettings_.setMacAddrSpoofing(mas);
        emitEngineSettingsChanged();
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
        emitEngineSettingsChanged();
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
        emitEngineSettingsChanged();
        emit isTerminateSocketsChanged(engineSettings_.isTerminateSockets());
    }
}
#endif

bool Preferences::isAntiCensorship() const
{
    return engineSettings_.isAntiCensorship();
}

void Preferences::setAntiCensorship(bool bEnabled)
{
    if (engineSettings_.isAntiCensorship() != bEnabled)
    {
        engineSettings_.setIsAntiCensorship(bEnabled);
        emitEngineSettingsChanged();
        emit isAntiCensorshipChanged(engineSettings_.isAntiCensorship());
    }
}

const types::ShareSecureHotspot &Preferences::shareSecureHotspot() const
{
    return guiSettings_.shareSecureHotspot;
}

void Preferences::setShareSecureHotspot(const types::ShareSecureHotspot &ss)
{
    if (guiSettings_.shareSecureHotspot != ss)
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
    if (guiSettings_.shareProxyGateway != sp)
    {
        guiSettings_.shareProxyGateway = sp;
        saveGuiSettings();
        emit shareProxyGatewayChanged(guiSettings_.shareProxyGateway);
    }
}

const QString Preferences::debugAdvancedParameters() const
{
    return ExtraConfig::instance().getExtraConfig();
}

void Preferences::setDebugAdvancedParameters(const QString &p)
{
    ExtraConfig::instance().writeConfig(p);
    emit debugAdvancedParametersChanged(p);
}

DNS_POLICY_TYPE Preferences::dnsPolicy() const
{
    return engineSettings_.dnsPolicy();
}

void Preferences::setDnsPolicy(DNS_POLICY_TYPE d)
{
    if (engineSettings_.dnsPolicy() != d)
    {
        engineSettings_.setDnsPolicy(d);
        emitEngineSettingsChanged();
        emit dnsPolicyChanged(d);
    }
}

types::DecoyTrafficSettings Preferences::decoyTrafficSettings() const
{
    return engineSettings_.decoyTrafficSettings();
}

void Preferences::setDecoyTrafficSettings(const types::DecoyTrafficSettings &d)
{
    if (engineSettings_.decoyTrafficSettings() != d)
    {
        engineSettings_.setDecoyTrafficSettings(d);
        emitEngineSettingsChanged();
        emit decoyTrafficSettingsChanged(d);
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
        emitEngineSettingsChanged();
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
        emitEngineSettingsChanged();
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
        emitEngineSettingsChanged();
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

#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
TRAY_ICON_COLOR Preferences::trayIconColor() const
{
    return guiSettings_.trayIconColor;
}

void Preferences::setTrayIconColor(TRAY_ICON_COLOR c)
{
    if (guiSettings_.trayIconColor != c)
    {
        guiSettings_.trayIconColor = c;
        saveGuiSettings();
        emit trayIconColorChanged(guiSettings_.trayIconColor);
    }
}
#endif

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
        emitEngineSettingsChanged();
        emit customConfigsPathChanged(path);
    }
}

QJsonObject Preferences::toJson() const
{
    QJsonObject json;
    json[kJsonGuiSettingsProp] = guiSettings_.toJson(false);
    json[kJsonEngineSettingsProp] = engineSettings_.toJson(false);
    json[kJsonPersistentStateProp] = PersistentState::instance().toJson();
    const auto extraConfigJson = ExtraConfig::instance().toJson();
    if (!extraConfigJson.isEmpty()) {
        json[kJsonAdvancedParamsProp] = extraConfigJson;
    }
    return json;
}

void Preferences::updateFromJson(const QJsonObject& json)
{
    if (json.contains(kJsonEngineSettingsProp) && json[kJsonEngineSettingsProp].isObject()) {
        setEngineSettings(json[kJsonEngineSettingsProp].toObject(), true);
        emitEngineSettingsChanged();
    }

    if (json.contains(kJsonGuiSettingsProp) && json[kJsonGuiSettingsProp].isObject()) {
        setGuiSettings(json[kJsonGuiSettingsProp].toObject());
    }

    if (json.contains(kJsonPersistentStateProp) && json[kJsonPersistentStateProp].isObject()) {
        QVector<types::NetworkInterface> oldList = PersistentState::instance().networkWhitelist();
        PersistentState::instance().fromJson(json[kJsonPersistentStateProp].toObject());
        QVector<types::NetworkInterface> newList = PersistentState::instance().networkWhitelist();
        if (oldList != newList) {
            emit networkWhiteListChanged(newList);
        }
    }

    if (json.contains(kJsonAdvancedParamsProp) && json[kJsonAdvancedParamsProp].isObject()) {
        ExtraConfig::instance().fromJson(json[kJsonAdvancedParamsProp].toObject());
    }
}

#if defined(Q_OS_MACOS)
MULTI_DESKTOP_BEHAVIOR Preferences::multiDesktopBehavior() const
{
    return guiSettings_.multiDesktopBehavior;
}

void Preferences::setMultiDesktopBehavior(MULTI_DESKTOP_BEHAVIOR m)
{
    if (guiSettings_.multiDesktopBehavior != m) {
        guiSettings_.multiDesktopBehavior = m;
        saveGuiSettings();
        emit multiDesktopBehaviorChanged(m);
    }
}
#endif

void Preferences::emitEngineSettingsChanged()
{
    if (!isSettingEngineSettings_)
        emit engineSettingsChanged();

#ifdef CLI_ONLY
    saveIni();
#endif
}

void Preferences::setEngineSettings(const types::EngineSettings &es, bool fromJson)
{
    isSettingEngineSettings_ = true;
    setLanguage(es.language());
    setUpdateChannel(es.updateChannel());
    setIgnoreSslErrors(es.isIgnoreSslErrors());
#if defined(Q_OS_WIN)
    setTerminateSockets(es.isTerminateSockets());
#endif
    setAntiCensorship(es.isAntiCensorship());
    setAllowLanTraffic(es.isAllowLanTraffic());
    setFirewallSettings(es.firewallSettings());
    setConnectionSettings(es.connectionSettings());
    setProxySettings(es.proxySettings());
    setPacketSize(es.packetSize());
    setMacAddrSpoofing(es.macAddrSpoofing());
    setDnsPolicy(es.dnsPolicy());
    setKeepAlive(es.isKeepAliveEnabled());
    if (fromJson) {
        if (es.customOvpnConfigsPath() != engineSettings_.customOvpnConfigsPath()) {
            if (es.customOvpnConfigsPath().isEmpty()) {
                // if the imported path is empty, we don't need to check permissions, just remove it
                setCustomOvpnConfigsPath(es.customOvpnConfigsPath());
            } else {
                // let the gui handle checking the path
                emit customConfigNeedsUpdate(es.customOvpnConfigsPath());
            }
        }
    } else {
        setCustomOvpnConfigsPath(es.customOvpnConfigsPath());
    }
    setConnectedDnsInfo(es.connectedDnsInfo());
    setDecoyTrafficSettings(es.decoyTrafficSettings());
#ifdef Q_OS_LINUX
    setDnsManager(es.dnsManager());
#endif
    setNetworkPreferredProtocols(es.networkPreferredProtocols());
    isSettingEngineSettings_ = false;
}

types::EngineSettings Preferences::getEngineSettings() const
{
    return engineSettings_;
}

void Preferences::setGuiSettings(const types::GuiSettings &gs)
{
    setAppSkin(gs.appSkin);
    setBackgroundSettings(gs.backgroundSettings);
    setSoundSettings(gs.soundSettings);
    setAutoConnect(gs.isAutoConnect);
    setDockedToTray(gs.isDockedToTray);

#ifdef Q_OS_MACOS
    setHideFromDock(gs.isHideFromDock);
    setMultiDesktopBehavior(gs.multiDesktopBehavior);
#endif

    setLaunchOnStartup(gs.isLaunchOnStartup);
    setMinimizeAndCloseToTray(gs.isMinimizeAndCloseToTray);
    setShowLocationLoad(gs.isShowLocationHealth);
    setShowNotifications(gs.isShowNotifications);
    setStartMinimized(gs.isStartMinimized);
    setLocationOrder(gs.orderLocation);

#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
    setTrayIconColor(gs.trayIconColor);
#endif

    setShareSecureHotspot(gs.shareSecureHotspot);
    setShareProxyGateway(gs.shareProxyGateway);

    QList<types::SplitTunnelingApp> apps;
    for (const auto& app : gs.splitTunneling.apps) {
        apps.append(app);
    }
    setSplitTunnelingApps(apps);

    QList<types::SplitTunnelingNetworkRoute> networkRoutes;
    for (const auto& route : gs.splitTunneling.networkRoutes) {
        networkRoutes.append(route);
    }
    setSplitTunnelingNetworkRoutes(networkRoutes);

    setSplitTunnelingSettings(gs.splitTunneling.settings);
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

#ifdef CLI_ONLY
    saveIni();
#endif
}

void Preferences::loadGuiSettings()
{
    bool bLoaded = false;
    SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);

    QSettings settings;
    if (settings.contains("guiSettings2")) {
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

#ifdef CLI_ONLY
    QSettings ini("Windscribe", "windscribe_cli");
    guiSettings_.fromIni(ini);
    PersistentState::instance().fromIni(ini);
#endif

    qCInfo(LOG_BASIC) << "GUI settings" << guiSettings_;
}

void Preferences::loadIni()
{
    QSettings settings("Windscribe", "windscribe_cli");

    // PersistentState can be updated directly as there are no signals to worry about
    PersistentState::instance().fromIni(settings);

    // Copy settings and apply the ini to the copy, then call the setXYZ functions on the actual settings.
    // This is a bit roundabount but ensures that all the correct signals are emitted.
    types::GuiSettings gs = guiSettings_;
    gs.fromIni(settings);

    setAutoConnect(gs.isAutoConnect);
    setAutoSecureNetworks(gs.isAutoSecureNetworks);
    setLaunchOnStartup(gs.isLaunchOnStartup);
#if defined(Q_OS_MACOS)
    setMultiDesktopBehavior(gs.multiDesktopBehavior);
#endif
    setShareProxyGateway(gs.shareProxyGateway);
    setSplitTunnelingApps(gs.splitTunneling.apps);
    setSplitTunnelingNetworkRoutes(gs.splitTunneling.networkRoutes);
    setSplitTunnelingSettings(gs.splitTunneling.settings);

    // Same for engine settings
    types::EngineSettings es = engineSettings_;
    es.fromIni(settings);

    isSettingEngineSettings_ = true;
    setLanguage(es.language());
    setUpdateChannel(es.updateChannel());
    setIgnoreSslErrors(es.isIgnoreSslErrors());
    setAntiCensorship(es.isAntiCensorship());
    setAllowLanTraffic(es.isAllowLanTraffic());
    setFirewallSettings(es.firewallSettings());
    setConnectionSettings(es.connectionSettings());
    setProxySettings(es.proxySettings());
    setPacketSize(es.packetSize());
    setMacAddrSpoofing(es.macAddrSpoofing());
    setDnsPolicy(es.dnsPolicy());
    setConnectedDnsInfo(es.connectedDnsInfo());
#ifdef Q_OS_LINUX
    setDnsManager(es.dnsManager());
#endif
    setDecoyTrafficSettings(es.decoyTrafficSettings());
    isSettingEngineSettings_ = false;

    emitEngineSettingsChanged();
}

void Preferences::validateAndUpdateIfNeeded()
{
    bool is_update_needed = false;

    // Validate Connected Dns settings
    types::ConnectedDnsInfo cdi = engineSettings_.connectedDnsInfo();
    if (cdi.type == CONNECTED_DNS_TYPE_CUSTOM || cdi.type == CONNECTED_DNS_TYPE_CONTROLD) {
        bool bCorrect = true;
        if (!IpValidation::isCtrldCorrectAddress(cdi.upStream1)) {
            bCorrect = false;
            emit reportErrorToUser(tr("Invalid DNS Settings"), tr("'Connected DNS' was not configured with a valid Upstream 1 (IP/DNS-over-HTTPS/TLS). DNS was reverted to ROBERT (default)."));
        }
        if (cdi.isSplitDns && !IpValidation::isCtrldCorrectAddress(cdi.upStream2)) {
            bCorrect = false;
            emit reportErrorToUser(tr("Invalid DNS Settings"), tr("'Connected DNS' was not configured with a valid Upstream 2 (IP/DNS-over-HTTPS/TLS). DNS was reverted to ROBERT (default)."));
        }
        if (!bCorrect)
        {
            cdi.type = CONNECTED_DNS_TYPE_AUTO;
            cdi.isSplitDns = false;
            engineSettings_.setConnectedDnsInfo(cdi);
            emit connectedDnsInfoChanged(engineSettings_.connectedDnsInfo());
            is_update_needed = true;
        }
    }

    if (is_update_needed)
        emitEngineSettingsChanged();
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

void Preferences::saveIni() const
{
    QSettings settings("Windscribe", "windscribe_cli");

    guiSettings_.toIni(settings);
    engineSettings_.toIni(settings);
    PersistentState::instance().toIni(settings);

    settings.sync();
}
