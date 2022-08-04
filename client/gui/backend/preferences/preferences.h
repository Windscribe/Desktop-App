#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QObject>
#include "types/dnswhileconnectedinfo.h"
#include "utils/protobuf_includes.h"
#include "types/enginesettings.h"
#include "types/splittunneling.h"

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

    ProtoTypes::BackgroundSettings backgroundSettings() const;
    void setBackgroundSettings(const ProtoTypes::BackgroundSettings &backgroundSettings);

    bool isDockedToTray() const;
    void setDockedToTray(bool b);

    const QString language() const;
    void setLanguage(const QString &lang);

    ProtoTypes::OrderLocationType locationOrder() const;
    void setLocationOrder(ProtoTypes::OrderLocationType o);

    ProtoTypes::LatencyDisplayType latencyDisplay() const;
    void setLatencyDisplay(ProtoTypes::LatencyDisplayType l);

    UPDATE_CHANNEL updateChannel() const;
    void setUpdateChannel(UPDATE_CHANNEL c);

    QVector<types::NetworkInterface> networkWhiteList();
    void setNetworkWhiteList(const QVector<types::NetworkInterface> &l);

    //const CustomRouting &customRouting() const;
    //void setCustomRouting(const CustomRouting &cr);

    const types::ProxySettings &proxySettings() const;
    void setProxySettings(const types::ProxySettings &ps);

    const types::FirewallSettings &firewalSettings() const;
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
    bool isKillTcpSockets() const;
    void setKillTcpSockets(bool b);
#endif

    const ProtoTypes::ShareSecureHotspot &shareSecureHotspot() const;
    void setShareSecureHotspot(const ProtoTypes::ShareSecureHotspot &ss);

    const ProtoTypes::ShareProxyGateway &shareProxyGateway() const;
    void setShareProxyGateway(const ProtoTypes::ShareProxyGateway &sp);

    const QString debugAdvancedParameters() const;
    void setDebugAdvancedParameters(const QString &p);

#ifdef Q_OS_WIN
    TAP_ADAPTER_TYPE tapAdapter() const;
    void setTapAdapter(TAP_ADAPTER_TYPE tapAdapter);
#endif
    DNS_POLICY_TYPE dnsPolicy() const;
    void setDnsPolicy(DNS_POLICY_TYPE d);

#ifdef Q_OS_LINUX
    ProtoTypes::DnsManagerType dnsManager() const;
    void setDnsManager(ProtoTypes::DnsManagerType d);
#endif


    types::DnsWhileConnectedInfo dnsWhileConnectedInfo() const;
    void setDnsWhileConnectedInfo(types::DnsWhileConnectedInfo d);

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
    //void setGuiSettings(const ProtoTypes::GuiSettings &gs);

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
    void backgroundSettingsChanged(const ProtoTypes::BackgroundSettings &backgroundSettings);

    void isStartMinimizedChanged(bool b);
    void isDockedToTrayChanged(bool b);
    void languageChanged(const QString &lang);
    void locationOrderChanged(ProtoTypes::OrderLocationType o);
    void latencyDisplayChanged(ProtoTypes::LatencyDisplayType l);
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

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    void minimizeAndCloseToTrayChanged(bool b);
    void isKillTcpSocketsChanged(bool b);
    void tapAdapterChanged(TAP_ADAPTER_TYPE tapAdapter);
#elif defined Q_OS_MAC
    void hideFromDockChanged(bool b);
#endif

    void shareSecureHotspotChanged(const ProtoTypes::ShareSecureHotspot &ss);
    void shareProxyGatewayChanged(const ProtoTypes::ShareProxyGateway &sp);
    void debugAdvancedParametersChanged(const QString &pars);
    void dnsPolicyChanged(DNS_POLICY_TYPE d);

#if defined(Q_OS_LINUX)
    void dnsManagerChanged(ProtoTypes::DnsManagerType d);
#endif

    void dnsWhileConnectedInfoChanged(types::DnsWhileConnectedInfo dnsWcInfo);
    void networkWhiteListChanged(QVector<types::NetworkInterface> l);
    void splitTunnelingChanged(types::SplitTunneling st);
    void keepAliveChanged(bool b);
    void updateEngineSettings();

    void reportErrorToUser(QString title, QString desc);

private:
    types::EngineSettings engineSettings_;
    ProtoTypes::GuiSettings guiSettings_;
    types::SplitTunneling splitTunneling_; //todo remove

    bool receivingEngineSettings_;
};

#endif // PREFERENCES_H
