#ifndef CONNECTIONWINDOWITEM_H
#define CONNECTIONWINDOWITEM_H

#include "../basepage.h"
#include "subpageitem.h"
#include "backend/preferences/preferences.h"
#include "backend/preferences/preferenceshelper.h"
#include "firewallmodeitem.h"
#include "connectionmodeitem.h"
#include "packetsizeitem.h"
#include "../checkboxitem.h"
#include "macspoofingitem.h"
#include "dnswhileconnecteditem.h"

enum CONNECTION_SCREEN_TYPE { CONNECTION_SCREEN_HOME,
                              CONNECTION_SCREEN_NETWORK_WHITELIST,
                              CONNECTION_SCREEN_SPLIT_TUNNELING,
                              CONNECTION_SCREEN_PROXY_SETTINGS };

namespace PreferencesWindow {

class ConnectionWindowItem : public BasePage
{
    Q_OBJECT
public:
    explicit ConnectionWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper);

    QString caption();
    void updateScaling() override;

    CONNECTION_SCREEN_TYPE getScreen();
    void setScreen(CONNECTION_SCREEN_TYPE subScreen);
    void setCurrentNetwork(const types::NetworkInterface &networkInterface);
    void setPacketSizeDetectionState(bool on);
    void showPacketSizeDetectionError(const QString &title, const QString &message);

signals:
    void networkWhitelistPageClick();
    void splitTunnelingPageClick();
    void proxySettingsPageClick();
    void cycleMacAddressClick();
    void detectAppropriatePacketSizeButtonClicked();

private slots:
    void onFirewallModeChanged(const types::FirewallSettings &fm);
    void onFirewallModeHoverEnter();
    void onFirewallModeHoverLeave();
    void onConnectionModeChanged(const types::ConnectionSettings &cm);
    void onConnectionModeHoverEnter(ConnectionModeItem::ButtonType type);
    void onConnectionModeHoverLeave(ConnectionModeItem::ButtonType type);
    void onPacketSizeChanged(const types::PacketSize &ps);
    void onMacAddrSpoofingChanged(const types::MacAddrSpoofing &mas);
    void onIsAllowLanTrafficClicked(bool b);
    void onAllowLanTrafficButtonHoverLeave();
    void onDnsWhileConnectedItemChanged(types::DnsWhileConnectedInfo dns);

    void onFirewallModePreferencesChanged(const types::FirewallSettings &fm);
    void onConnectionModePreferencesChanged(const types::ConnectionSettings &cm);
    void onPacketSizePreferencesChanged(const types::PacketSize &ps);
    void onMacAddrSpoofingPreferencesChanged(const types::MacAddrSpoofing &mas);
    void onIsAllowLanTrafficPreferencedChanged(bool b);
    void onInvalidLanAddressNotification(QString address);
    void onIsFirewallBlockedChanged(bool bFirewallBlocked);
    void onIsExternalConfigModeChanged(bool bIsExternalConfigMode);
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    void onKillTcpSocketsPreferencesChanged(bool b);
    void onKillTcpSocketsStateChanged(bool isChecked);
#endif

    void onDnsWhileConnectedPreferencesChanged(const types::DnsWhileConnectedInfo &dns);

    void onLanguageChanged();

protected:
    void hideOpenPopups() override;

private:
    Preferences *preferences_{ nullptr };
    PreferencesHelper *preferencesHelper_{ nullptr };
    SubPageItem *networkWhitelistItem_{ nullptr };
    SubPageItem *splitTunnelingItem_{ nullptr };
    SubPageItem *proxySettingsItem_{ nullptr };
    FirewallModeItem *firewallModeItem_{ nullptr };
    ConnectionModeItem *connectionModeItem_{ nullptr };
    PacketSizeItem *packetSizeItem_{ nullptr };
    MacSpoofingItem *macSpoofingItem_{ nullptr };
    CheckBoxItem *checkBoxAllowLanTraffic_{ nullptr };
    DnsWhileConnectedItem *dnsWhileConnectedItem_{ nullptr };
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    CheckBoxItem *cbKillTcp_;
#endif

    CONNECTION_SCREEN_TYPE currentScreen_;
};

} // namespace PreferencesWindow

#endif // CONNECTIONWINDOWITEM_H
