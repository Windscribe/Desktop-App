#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QObject>
#include "types/connecteddnsinfo.h"
#include "types/enginesettings.h"
#include "types/guisettings.h"

// all preferences with the ability to receive signals when certain preferences are changed
class Preferences : public QObject
{
    Q_OBJECT
public:
    explicit Preferences(QObject *parent = nullptr);

    bool isLaunchOnStartup() const;
    void setLaunchOnStartup(bool b);

    bool isAutoConnect() const;
    void setAutoConnect(bool b);

    bool isAllowLanTraffic() const;
    void setAllowLanTraffic(bool b);

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    bool isMinimizeAndCloseToTray() const;
    void setMinimizeAndCloseToTray(bool b);
#elif defined Q_OS_MAC
    bool isHideFromDock() const;
    void setHideFromDock(bool b);
#endif

    bool isStartMinimized() const;
    void setStartMinimized(bool b);

    bool isShowNotifications() const;
    void setShowNotifications(bool b);

    types::BackgroundSettings backgroundSettings() const;
    void setBackgroundSettings(const types::BackgroundSettings &backgroundSettings);

    bool isDockedToTray() const;
    void setDockedToTray(bool b);

    const QString language() const;
    void setLanguage(const QString &lang);

    ORDER_LOCATION_TYPE locationOrder() const;
    void setLocationOrder(ORDER_LOCATION_TYPE o);

    LATENCY_DISPLAY_TYPE latencyDisplay() const;
    void setLatencyDisplay(LATENCY_DISPLAY_TYPE l);

    UPDATE_CHANNEL updateChannel() const;
    void setUpdateChannel(UPDATE_CHANNEL c);

    QVector<types::NetworkInterface> networkWhiteList();
    void setNetworkWhiteList(const QVector<types::NetworkInterface> &l);

    const types::ProxySettings &proxySettings() const;
    void setProxySettings(const types::ProxySettings &ps);

    bool isAutoSecureNetworks();
    void setAutoSecureNetworks(bool b);

    const types::FirewallSettings &firewallSettings() const;
    void setFirewallSettings(const types::FirewallSettings &fs);

    const types::ConnectionSettings &connectionSettings() const;
    void setConnectionSettings(const types::ConnectionSettings &cs);

    const types::DnsResolutionSettings &apiResolution() const;
    void setApiResolution(const types::DnsResolutionSettings &s);

    const types::PacketSize &packetSize() const;
    void setPacketSize(const types::PacketSize &ps);

    const types::MacAddrSpoofing &macAddrSpoofing() const;
    void setMacAddrSpoofing(const types::MacAddrSpoofing &mas);

    bool isIgnoreSslErrors() const;
    void setIgnoreSslErrors(bool b);

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    bool isTerminateSockets() const;
    void setTerminateSockets(bool b);
#endif

    const types::ShareSecureHotspot &shareSecureHotspot() const;
    void setShareSecureHotspot(const types::ShareSecureHotspot &ss);

    const types::ShareProxyGateway &shareProxyGateway() const;
    void setShareProxyGateway(const types::ShareProxyGateway &sp);

    const QString debugAdvancedParameters() const;
    void setDebugAdvancedParameters(const QString &p);

#ifdef Q_OS_WIN
    TAP_ADAPTER_TYPE tapAdapter() const;
    void setTapAdapter(TAP_ADAPTER_TYPE tapAdapter);
#endif
    DNS_POLICY_TYPE dnsPolicy() const;
    void setDnsPolicy(DNS_POLICY_TYPE d);

#ifdef Q_OS_LINUX
    DNS_MANAGER_TYPE dnsManager() const;
    void setDnsManager(DNS_MANAGER_TYPE d);
#endif

    types::ConnectedDnsInfo connectedDnsInfo() const;
    void setConnectedDnsInfo(types::ConnectedDnsInfo d);

    bool keepAlive() const;
    void setKeepAlive(bool bEnabled);

    types::SplitTunneling splitTunneling();
    QList<types::SplitTunnelingApp> splitTunnelingApps();
    void setSplitTunnelingApps(QList<types::SplitTunnelingApp> apps);
    QList<types::SplitTunnelingNetworkRoute> splitTunnelingNetworkRoutes();
    void setSplitTunnelingNetworkRoutes(QList<types::SplitTunnelingNetworkRoute> routes);
    types::SplitTunnelingSettings splitTunnelingSettings();
    void setSplitTunnelingSettings(types::SplitTunnelingSettings settings);

    QString customOvpnConfigsPath() const;
    void setCustomOvpnConfigsPath(const QString &path);

    void setEngineSettings(const types::EngineSettings &es);
    types::EngineSettings getEngineSettings() const;

    void saveGuiSettings() const;
    void loadGuiSettings();
    void validateAndUpdateIfNeeded();

    bool isReceivingEngineSettings() const;

    bool isShowLocationLoad() const;
    void setShowLocationLoad(bool b);

signals:
    void isLaunchOnStartupChanged(bool b);
    void isAutoConnectChanged(bool b);
    void isAllowLanTrafficChanged(bool b);
    void isShowNotificationsChanged(bool b);
    void backgroundSettingsChanged(const types::BackgroundSettings &backgroundSettings);

    void isStartMinimizedChanged(bool b);
    void isDockedToTrayChanged(bool b);
    void languageChanged(const QString &lang);
    void locationOrderChanged(ORDER_LOCATION_TYPE o);
    void latencyDisplayChanged(LATENCY_DISPLAY_TYPE l);
    void updateChannelChanged(UPDATE_CHANNEL c);
    void proxySettingsChanged(const types::ProxySettings &ps);
    void firewallSettingsChanged(const types::FirewallSettings &fs);
    void connectionSettingsChanged(const types::ConnectionSettings &cm);
    void apiResolutionChanged(const types::DnsResolutionSettings &s);
    void packetSizeChanged(const types::PacketSize &ps);
    void macAddrSpoofingChanged(const types::MacAddrSpoofing &mas);
    void isIgnoreSslErrorsChanged(bool b);
    void invalidLanAddressNotification(QString address);
    void customConfigsPathChanged(QString path);
    void showLocationLoadChanged(bool b);
    void isAutoSecureNetworksChanged(bool b);
    void minimizeAndCloseToTrayChanged(bool b);
    void isTerminateSocketsChanged(bool b);
    void tapAdapterChanged(TAP_ADAPTER_TYPE tapAdapter);
    void hideFromDockChanged(bool b);
    void shareSecureHotspotChanged(const types::ShareSecureHotspot &ss);
    void shareProxyGatewayChanged(const types::ShareProxyGateway &sp);
    void debugAdvancedParametersChanged(const QString &pars);
    void dnsPolicyChanged(DNS_POLICY_TYPE d);
    void dnsManagerChanged(DNS_MANAGER_TYPE d);

    void connectedDnsInfoChanged(types::ConnectedDnsInfo dnsWcInfo);
    void networkWhiteListChanged(QVector<types::NetworkInterface> l);
    void splitTunnelingChanged(types::SplitTunneling st);
    void keepAliveChanged(bool b);
    void updateEngineSettings();

    void reportErrorToUser(QString title, QString desc);

private:
    types::EngineSettings engineSettings_;
    types::GuiSettings guiSettings_;

    bool receivingEngineSettings_;

    // for serialization
    static constexpr quint32 magic_ = 0x7715C211;
    static constexpr int versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

#endif // PREFERENCES_H
