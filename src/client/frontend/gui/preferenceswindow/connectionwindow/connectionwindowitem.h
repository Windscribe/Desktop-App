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
#include "decoytrafficgroup.h"
#include "utils/network_utils/dnschecker.h"

enum CONNECTION_SCREEN_TYPE { CONNECTION_SCREEN_HOME,
                              CONNECTION_SCREEN_NETWORK_OPTIONS,
                              CONNECTION_SCREEN_SPLIT_TUNNELING,
                              CONNECTION_SCREEN_ANTI_CENSORSHIP,
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
    void setClearWifiHistoryResult(bool success);

signals:
    void networkOptionsPageClick();
    void splitTunnelingPageClick();
    void antiCensorshipPageClick();
    void proxySettingsPageClick();
    void cycleMacAddressClick();
    void detectPacketSize();
    void connectedDnsDomainsClick(const QStringList &domains);
    void fetchControldDevices(const QString &apiKey);
    void clearWifiHistoryClick();

public slots:
    void checkLocalDns();
    void onControldDevicesFetched(CONTROLD_FETCH_RESULT result, const QList<QPair<QString, QString>> &devices);

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
    void onDecoyTrafficPreferencesChanged(const types::DecoyTrafficSettings &decoyTrafficSettings);
    void onSecureHotspotPreferencesChanged(const types::ShareSecureHotspot &ss);
    void onProxyGatewayAddressChanged(const QString &address);
    void onProxyGatewayPreferencesChanged(const types::ShareProxyGateway &sp);
    void onPreferencesHelperWifiSharingSupportedChanged(bool bSupported);
    void onPreferencesHelperCurrentProtocolChanged(const types::Protocol &protocol);
    void onIsExternalConfigModeChanged(bool bIsExternalConfigMode);
    void onTerminateSocketsPreferencesChanged(bool b);
    void onIsAutoConnectPreferencesChanged(bool b);
    void onIpStackPreferencesChanged(IpStack ipStackEgress);

    // slots for changes made by user
    void onFirewallPreferencesChangedByUser(const types::FirewallSettings &fm);
    void onConnectionModePreferencesChangedByUser(const types::ConnectionSettings &cm);
    void onPacketSizePreferencesChangedByUser(const types::PacketSize &ps);
    void onMacAddrSpoofingPreferencesChangedByUser(const types::MacAddrSpoofing &mas);
    void onConnectedDnsPreferencesChangedByUser(const types::ConnectedDnsInfo &dns);
    void onDecoyTrafficSettingsChangedByUser(const types::DecoyTrafficSettings &decoyTrafficSettings);
    void onIsAllowLanTrafficPreferencesChangedByUser(bool b);
    void onSecureHotspotPreferencesChangedByUser(const types::ShareSecureHotspot &ss);
    void onProxyGatewayPreferencesChangedByUser(const types::ShareProxyGateway &sp);
    void onTerminateSocketsPreferencesChangedByUser(bool isChecked);
    void onIsAutoConnectPreferencesChangedByUser(bool b);
    void onIpStackComboBoxItemChanged(QVariant o);

    void onUpdateIsSecureHotspotSupported();
    void onClearWifiHistoryClick();
    void onLanguageChanged();

protected:
    void hideOpenPopups() override;

private:
    // Sets the MAC spoofing group's description for the current platform/support state, in the current
    // language. Single source of truth shared by onLanguageChanged() and the show-time re-evaluation.
    void updateMacSpoofingDescription();

    Preferences *preferences_;
    PreferencesHelper *preferencesHelper_;
    NetworkUtils::DnsChecker dnsChecker_;
    PreferenceGroup *subpagesGroup_;
    LinkItem *networkOptionsItem_;
    LinkItem *antiCensorshipItem_;
    LinkItem *splitTunnelingItem_;
    LinkItem *proxySettingsItem_;
    FirewallGroup *firewallGroup_;
    ProtocolGroup *connectionModeGroup_;
    PacketSizeGroup *packetSizeGroup_;

    PreferenceGroup *ipStackGroup_;
    ComboBoxItem *comboBoxIpStack_;

    MacSpoofingGroup *macSpoofingGroup_;
#ifdef Q_OS_LINUX
    // Whether MAC spoofing is usable, decided once at construction (needs NetworkManager running). Reused by
    // onLanguageChanged() so it only re-translates the string and never re-probes NM state.
    bool isMacSpoofingSupported_ = true;
#endif
    PreferenceGroup *allowLanTrafficGroup_;
    PreferenceGroup *autoConnectGroup_;
    ToggleItem *checkBoxAutoConnect_;
    ToggleItem *checkBoxAllowLanTraffic_;
    ConnectedDnsGroup *connectedDnsGroup_;
    PreferenceGroup *terminateSocketsGroup_;
    ToggleItem *terminateSocketsItem_;
    SecureHotspotGroup *secureHotspotGroup_;
    ProxyGatewayGroup *proxyGatewayGroup_;
    DecoyTrafficGroup *decoyTrafficGroup_;

    PreferenceGroup *clearWifiHistoryGroup_;
    LinkItem *clearWifiHistoryItem_;
    enum class ClearWifiState {
        kNotInitiated,
        kInitiated,
        kFinished
    } clearWifiState_ = ClearWifiState::kNotInitiated;

    bool isIkev2(const types::ConnectionSettings &cs) const;

    CONNECTION_SCREEN_TYPE currentScreen_;
};

} // namespace PreferencesWindow
