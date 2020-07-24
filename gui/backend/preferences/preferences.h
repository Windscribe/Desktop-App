#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QObject>
#include "../types/types.h"
#include "ipc/generated_proto/types.pb.h"

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

#ifdef Q_OS_WIN
    bool isMinimizeAndCloseToTray() const;
    void setMinimizeAndCloseToTray(bool b);
#elif defined Q_OS_MAC
    bool isHideFromDock() const;
    void setHideFromDock(bool b);
#endif

    bool isShowNotifications() const;
    void setShowNotifications(bool b);

    bool isDockedToTray() const;
    void setDockedToTray(bool b);

    const QString language() const;
    void setLanguage(const QString &lang);

    ProtoTypes::OrderLocationType locationOrder() const;
    void setLocationOrder(ProtoTypes::OrderLocationType o);

    ProtoTypes::LatencyDisplayType latencyDisplay() const;
    void setLatencyDisplay(ProtoTypes::LatencyDisplayType l);

    ProtoTypes::UpdateChannel updateChannel() const;
    void setUpdateChannel(ProtoTypes::UpdateChannel c);

    ProtoTypes::NetworkWhiteList networkWhiteList();
    void setNetworkWhiteList(ProtoTypes::NetworkWhiteList l);

    //const CustomRouting &customRouting() const;
    //void setCustomRouting(const CustomRouting &cr);

    const ProtoTypes::ProxySettings &proxySettings() const;
    void setProxySettings(const ProtoTypes::ProxySettings &ps);

    const ProtoTypes::FirewallSettings &firewalSettings() const;
    void setFirewallSettings(const ProtoTypes::FirewallSettings &fm);

    const ProtoTypes::ConnectionSettings &connectionSettings() const;
    void setConnectionSettings(const ProtoTypes::ConnectionSettings &cm);

    const ProtoTypes::ApiResolution &apiResolution() const;
    void setApiResolution(const ProtoTypes::ApiResolution &ar);

    const ProtoTypes::PacketSize &packetSize() const;
    void setPacketSize(const ProtoTypes::PacketSize &ps);

    const ProtoTypes::MacAddrSpoofing &macAddrSpoofing() const;
    void setMacAddrSpoofing(const ProtoTypes::MacAddrSpoofing &mas);

    bool isIgnoreSslErrors() const;
    void setIgnoreSslErrors(bool b);

#ifdef Q_OS_WIN
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
    ProtoTypes::TapAdapterType tapAdapter() const;
    void setTapAdapter(ProtoTypes::TapAdapterType tapAdapter);
#endif
    ProtoTypes::DnsPolicy dnsPolicy() const;
    void setDnsPolicy(ProtoTypes::DnsPolicy d);

    bool keepAlive() const;
    void setKeepAlive(bool bEnabled);

    ProtoTypes::SplitTunneling splitTunneling();
    QList<ProtoTypes::SplitTunnelingApp> splitTunnelingApps();
    void setSplitTunnelingApps(QList<ProtoTypes::SplitTunnelingApp> apps);
    QList<ProtoTypes::SplitTunnelingNetworkRoute> splitTunnelingNetworkRoutes();
    void setSplitTunnelingNetworkRoutes(QList<ProtoTypes::SplitTunnelingNetworkRoute> routes);
    ProtoTypes::SplitTunnelingSettings splitTunnelingSettings();
    void setSplitTunnelingSettings(ProtoTypes::SplitTunnelingSettings settings);

    void setCustomOvpnConfigsPath(const QString &path);

    void setEngineSettings(const ProtoTypes::EngineSettings &es);
    //void setGuiSettings(const ProtoTypes::GuiSettings &gs);

    ProtoTypes::EngineSettings getEngineSettings() const;

    void saveGuiSettings();
    void loadGuiSettings();

    bool isReceivingEngineSettings();

signals:
    void isLaunchOnStartupChanged(bool b);
    void isAutoConnectChanged(bool b);
    void isAllowLanTrafficChanged(bool b);
    void isShowNotificationsChanged(bool b);
    void isDockedToTrayChanged(bool b);
    void languageChanged(const QString &lang);
    void locationOrderChanged(ProtoTypes::OrderLocationType o);
    void latencyDisplayChanged(ProtoTypes::LatencyDisplayType l);
    void updateChannelChanged(ProtoTypes::UpdateChannel c);
    void proxySettingsChanged(const ProtoTypes::ProxySettings &ps);
    void firewallSettingsChanged(const ProtoTypes::FirewallSettings &fm);
    void connectionSettingsChanged(const ProtoTypes::ConnectionSettings &cm);
    void apiResolutionChanged(const ProtoTypes::ApiResolution &ar);
    void packetSizeChanged(const ProtoTypes::PacketSize &ps);
    void macAddrSpoofingChanged(const ProtoTypes::MacAddrSpoofing &mas);
    void isIgnoreSslErrorsChanged(bool b);

#ifdef Q_OS_WIN
    void minimizeAndCloseToTrayChanged(bool b);
    void isKillTcpSocketsChanged(bool b);
    void tapAdapterChanged(ProtoTypes::TapAdapterType tapAdapter);
#elif defined Q_OS_MAC
    void hideFromDockChanged(bool b);
#endif

    void shareSecureHotspotChanged(const ProtoTypes::ShareSecureHotspot &ss);
    void shareProxyGatewayChanged(const ProtoTypes::ShareProxyGateway &sp);
    void debugAdvancedParametersChanged(const QString &pars);
    void dnsPolicyChanged(ProtoTypes::DnsPolicy d);
    void networkWhiteListChanged(ProtoTypes::NetworkWhiteList l);
    void splitTunnelingChanged(ProtoTypes::SplitTunneling st);
    void keepAliveChanged(bool b);
    void updateEngineSettings();

private:
    ProtoTypes::EngineSettings engineSettings_;
    ProtoTypes::GuiSettings guiSettings_;

    bool receivingEngineSettings_;
};

#endif // PREFERENCES_H
