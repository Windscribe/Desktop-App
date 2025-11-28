#pragma once

#include <QJsonObject>
#include <QObject>
#include <QMap>
#include <QTimer>
#include "types/connecteddnsinfo.h"
#include "types/enginesettings.h"
#include "types/guisettings.h"
#include "types/locationid.h"

// all preferences with the ability to receive signals when certain preferences are changed
class Preferences : public QObject
{
    Q_OBJECT
public:
    explicit Preferences(QObject *parent = nullptr);
    ~Preferences();

    bool isLaunchOnStartup() const;
    void setLaunchOnStartup(bool b);

    bool isAutoConnect() const;
    void setAutoConnect(bool b);

    bool isAllowLanTraffic() const;
    void setAllowLanTraffic(bool b);

    bool isMinimizeAndCloseToTray() const;
    void setMinimizeAndCloseToTray(bool b);

#if defined Q_OS_MACOS
    bool isHideFromDock() const;
    void setHideFromDock(bool b);
#endif

    bool isStartMinimized() const;
    void setStartMinimized(bool b);

    bool isShowNotifications() const;
    void setShowNotifications(bool b);

    types::BackgroundSettings backgroundSettings() const;
    void setBackgroundSettings(const types::BackgroundSettings &backgroundSettings);

    types::SoundSettings soundSettings() const;
    void setSoundSettings(const types::SoundSettings &soundSettings);

    bool isDockedToTray() const;
    void setDockedToTray(bool b);

    const QString language() const;
    void setLanguage(const QString &lang);

    ORDER_LOCATION_TYPE locationOrder() const;
    void setLocationOrder(ORDER_LOCATION_TYPE o);

    UPDATE_CHANNEL updateChannel() const;
    void setUpdateChannel(UPDATE_CHANNEL c);

    QVector<types::NetworkInterface> networkWhiteList();
    void setNetworkWhiteList(const QVector<types::NetworkInterface> &l);

    const types::ConnectionSettings networkPreferredProtocol(QString networkOrSsid) const;
    const QMap<QString, types::ConnectionSettings> networkPreferredProtocols() const;
    bool hasNetworkPreferredProtocol(QString networkOrSsid) const;
    void setNetworkPreferredProtocol(QString networkOrSsid, const types::ConnectionSettings &preferredProtocol);
    void setNetworkPreferredProtocols(const QMap<QString, types::ConnectionSettings> &preferredProtocols);

    const types::ProxySettings &proxySettings() const;
    void setProxySettings(const types::ProxySettings &ps);

    bool isAutoSecureNetworks();
    void setAutoSecureNetworks(bool b);

    const types::FirewallSettings &firewallSettings() const;
    void setFirewallSettings(const types::FirewallSettings &fs);

    const types::ConnectionSettings &connectionSettings() const;
    void setConnectionSettings(const types::ConnectionSettings &cs);

    const types::PacketSize &packetSize() const;
    void setPacketSize(const types::PacketSize &ps);

    const types::MacAddrSpoofing &macAddrSpoofing() const;
    void setMacAddrSpoofing(const types::MacAddrSpoofing &mas);

    bool isIgnoreSslErrors() const;
    void setIgnoreSslErrors(bool b);

#if defined(Q_OS_WIN)
    bool isTerminateSockets() const;
    void setTerminateSockets(bool b);
#endif

    bool isAntiCensorship() const;
    void setAntiCensorship(bool bEnabled);

    const types::ShareSecureHotspot &shareSecureHotspot() const;
    void setShareSecureHotspot(const types::ShareSecureHotspot &ss);

    const types::ShareProxyGateway &shareProxyGateway() const;
    void setShareProxyGateway(const types::ShareProxyGateway &sp);

    const QString debugAdvancedParameters() const;
    void setDebugAdvancedParameters(const QString &p);

    DNS_POLICY_TYPE dnsPolicy() const;
    void setDnsPolicy(DNS_POLICY_TYPE d);

    types::DecoyTrafficSettings decoyTrafficSettings() const;
    void setDecoyTrafficSettings(const types::DecoyTrafficSettings &d);

#ifdef Q_OS_LINUX
    DNS_MANAGER_TYPE dnsManager() const;
    void setDnsManager(DNS_MANAGER_TYPE d);
#endif

    types::ConnectedDnsInfo connectedDnsInfo() const;
    void setConnectedDnsInfo(types::ConnectedDnsInfo d);

    bool keepAlive() const;
    void setKeepAlive(bool bEnabled);

    APP_SKIN appSkin() const;
    void setAppSkin(APP_SKIN s);

#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
    TRAY_ICON_COLOR trayIconColor() const;
    void setTrayIconColor(TRAY_ICON_COLOR c);
#endif

    types::SplitTunneling splitTunneling();
    QList<types::SplitTunnelingApp> splitTunnelingApps();
    void setSplitTunnelingApps(QList<types::SplitTunnelingApp> apps);
    QList<types::SplitTunnelingNetworkRoute> splitTunnelingNetworkRoutes();
    void setSplitTunnelingNetworkRoutes(QList<types::SplitTunnelingNetworkRoute> routes);
    types::SplitTunnelingSettings splitTunnelingSettings();
    void setSplitTunnelingSettings(types::SplitTunnelingSettings settings);

    QString customOvpnConfigsPath() const;
    void setCustomOvpnConfigsPath(const QString &path);

#if defined(Q_OS_MACOS)
    MULTI_DESKTOP_BEHAVIOR multiDesktopBehavior() const;
    void setMultiDesktopBehavior(MULTI_DESKTOP_BEHAVIOR m);
#endif

    void setEngineSettings(const types::EngineSettings &es, bool fromJson = false);
    types::EngineSettings getEngineSettings() const;

    void setGuiSettings(const types::GuiSettings& gs);

    void saveGuiSettings() const;
    void loadGuiSettings();
    void validateAndUpdateIfNeeded();

    bool isReceivingEngineSettings() const;

    bool isShowLocationLoad() const;
    void setShowLocationLoad(bool b);

    QJsonObject toJson() const;
    void updateFromJson(const QJsonObject& ob);

    void loadIni();
    void saveIni() const;

signals:
    void isLaunchOnStartupChanged(bool b);
    void isAutoConnectChanged(bool b);
    void isAllowLanTrafficChanged(bool b);
    void isShowNotificationsChanged(bool b);
    void backgroundSettingsChanged(const types::BackgroundSettings &backgroundSettings);
    void soundSettingsChanged(const types::SoundSettings &soundSettings);

    void isStartMinimizedChanged(bool b);
    void isDockedToTrayChanged(bool b);
    void languageChanged(const QString &lang);
    void locationOrderChanged(ORDER_LOCATION_TYPE o);
    void updateChannelChanged(UPDATE_CHANNEL c);
    void proxySettingsChanged(const types::ProxySettings &ps);
    void firewallSettingsChanged(const types::FirewallSettings &fs);
    void connectionSettingsChanged(const types::ConnectionSettings &cm);
    void packetSizeChanged(const types::PacketSize &ps);
    void macAddrSpoofingChanged(const types::MacAddrSpoofing &mas);
    void isIgnoreSslErrorsChanged(bool b);
    void customConfigsPathChanged(QString path);
    void customConfigNeedsUpdate(const QString &path);
    void showLocationLoadChanged(bool b);
    void isAutoSecureNetworksChanged(bool b);
    void minimizeAndCloseToTrayChanged(bool b);
    void isTerminateSocketsChanged(bool b);
    void isAntiCensorshipChanged(bool b);
    void hideFromDockChanged(bool b);
    void shareSecureHotspotChanged(const types::ShareSecureHotspot &ss);
    void shareProxyGatewayChanged(const types::ShareProxyGateway &sp);
    void debugAdvancedParametersChanged(const QString &pars);
    void dnsPolicyChanged(DNS_POLICY_TYPE d);
    void decoyTrafficSettingsChanged(const types::DecoyTrafficSettings &d);
    void dnsManagerChanged(DNS_MANAGER_TYPE d);
    void appSkinChanged(APP_SKIN s);

#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
    void trayIconColorChanged(TRAY_ICON_COLOR c);
#elif defined(Q_OS_MACOS)
    void multiDesktopBehaviorChanged(MULTI_DESKTOP_BEHAVIOR m);
#endif

    void connectedDnsInfoChanged(types::ConnectedDnsInfo dnsWcInfo);
    void networkWhiteListChanged(QVector<types::NetworkInterface> l);
    void networkPreferredProtocolsChanged(QMap<QString, types::ConnectionSettings> p);
    void splitTunnelingChanged(types::SplitTunneling st);
    void keepAliveChanged(bool b);

    // emit if any of the engine options have changed
    // don't emit in setEngineSettings()
    void engineSettingsChanged();

    void reportErrorToUser(QString title, QString desc);

private:
    types::EngineSettings engineSettings_;
    types::GuiSettings guiSettings_;

    bool isSettingEngineSettings_;

    void emitEngineSettingsChanged();

    static const inline QString kJsonEngineSettingsProp = "engineSettings";
    static const inline QString kJsonGuiSettingsProp = "guiSettings";
    static const inline QString kJsonPersistentStateProp = "persistentState";
    static const inline QString kJsonAdvancedParamsProp = "advancedParameters";

    // for serialization
    static constexpr quint32 magic_ = 0x7715C211;
    static constexpr int versionForSerialization_ = 1;  // should increment the version if the data format is changed
};
