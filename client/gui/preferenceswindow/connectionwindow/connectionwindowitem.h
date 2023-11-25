#pragma once

#include "preferenceswindow/preferencegroup.h"
#include "backend/preferences/preferences.h"
#include "backend/preferences/preferenceshelper.h"
#include "commongraphics/basepage.h"
#include "preferenceswindow/toggleitem.h"
#include "preferenceswindow/linkitem.h"
#include "preferenceswindow/protocolgroup.h"
#include "connecteddnsgroup.h"
#include "firewallgroup.h"
#include "macspoofinggroup.h"
#include "packetsizegroup.h"
#include "proxygatewaygroup.h"
#include "securehotspotgroup.h"

enum CONNECTION_SCREEN_TYPE { CONNECTION_SCREEN_HOME,
                              CONNECTION_SCREEN_NETWORK_OPTIONS,
                              CONNECTION_SCREEN_SPLIT_TUNNELING,
                              CONNECTION_SCREEN_PROXY_SETTINGS,
                              CONNECTION_SCREEN_DNS_DOMAINS };

namespace PreferencesWindow {

class ConnectionWindowItem : public CommonGraphics::BasePage
{
    Q_OBJECT
public:
    explicit ConnectionWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper);

    QString caption() const override;
    void updateScaling() override;

    CONNECTION_SCREEN_TYPE getScreen();
    void setScreen(CONNECTION_SCREEN_TYPE subScreen);
    void setCurrentNetwork(const types::NetworkInterface &networkInterface);
    void setPacketSizeDetectionState(bool on);
    void showPacketSizeDetectionError(const QString &title, const QString &message);

signals:
    void networkOptionsPageClick();
    void splitTunnelingPageClick();
    void proxySettingsPageClick();
    void cycleMacAddressClick();
    void detectPacketSize();
    void connectedDnsDomainsClick(const QStringList &domains);

private slots:
    // slots for preference changes
    void onSplitTunnelingPreferencesChanged(const types::SplitTunneling &st);
    void onFirewallPreferencesChanged(const types::FirewallSettings &fm);
    void onConnectionModePreferencesChanged(const types::ConnectionSettings &cm);
    void onPacketSizePreferencesChanged(const types::PacketSize &ps);
    void onMacAddrSpoofingPreferencesChanged(const types::MacAddrSpoofing &mas);
    void onIsAllowLanTrafficPreferencesChanged(bool b);
    void onAllowLanTrafficButtonHoverLeave();
    void onConnectedDnsPreferencesChanged(const types::ConnectedDnsInfo &dns);
    void onSecureHotspotPreferencesChanged(const types::ShareSecureHotspot &ss);
    void onConnectionSettingsPreferencesChanged(const types::ConnectionSettings &cs);
    void onProxyGatewayAddressChanged(const QString &address);
    void onProxyGatewayPreferencesChanged(const types::ShareProxyGateway &sp);
    void onPreferencesHelperWifiSharingSupportedChanged(bool bSupported);
    void onIsExternalConfigModeChanged(bool bIsExternalConfigMode);
    void onTerminateSocketsPreferencesChanged(bool b);
    void onAntiCensorshipPreferencesChanged(bool b);
    void onIsAutoConnectPreferencesChanged(bool b);

    // slots for changes made by user
    void onFirewallPreferencesChangedByUser(const types::FirewallSettings &fm);
    void onConnectionModePreferencesChangedByUser(const types::ConnectionSettings &cm);
    void onPacketSizePreferencesChangedByUser(const types::PacketSize &ps);
    void onMacAddrSpoofingPreferencesChangedByUser(const types::MacAddrSpoofing &mas);
    void onConnectedDnsPreferencesChangedByUser(const types::ConnectedDnsInfo &dns);
    void onIsAllowLanTrafficPreferencesChangedByUser(bool b);
    void onSecureHotspotPreferencesChangedByUser(const types::ShareSecureHotspot &ss);
    void onProxyGatewayPreferencesChangedByUser(const types::ShareProxyGateway &sp);
    void onTerminateSocketsPreferencesChangedByUser(bool isChecked);
    void onAntiCensorshipPreferencesChangedByUser(bool isChecked);
    void onIsAutoConnectPreferencesChangedByUser(bool b);

    void onLanguageChanged();

protected:
    void hideOpenPopups() override;

private:
    Preferences *preferences_;
    PreferencesHelper *preferencesHelper_;
    PreferenceGroup *subpagesGroup_;
    LinkItem *networkOptionsItem_;
    LinkItem *splitTunnelingItem_;
    LinkItem *proxySettingsItem_;
    FirewallGroup *firewallGroup_;
    ProtocolGroup *connectionModeGroup_;
    PacketSizeGroup *packetSizeGroup_;
    MacSpoofingGroup *macSpoofingGroup_;
    PreferenceGroup *allowLanTrafficGroup_;
    PreferenceGroup *autoConnectGroup_;
    ToggleItem *checkBoxAutoConnect_;
    ToggleItem *checkBoxAllowLanTraffic_;
    ConnectedDnsGroup *connectedDnsGroup_;
    PreferenceGroup *terminateSocketsGroup_;
    ToggleItem *terminateSocketsItem_;
    PreferenceGroup *antiCensorshipGroup_;
    ToggleItem *antiCensorshipItem_;
    SecureHotspotGroup *secureHotspotGroup_;
    ProxyGatewayGroup *proxyGatewayGroup_;

    bool isIkev2(const types::ConnectionSettings &cs) const;
    void updateIsSupported(bool isWifiSharingSupported, bool isIkev2);

    CONNECTION_SCREEN_TYPE currentScreen_;
};

} // namespace PreferencesWindow
