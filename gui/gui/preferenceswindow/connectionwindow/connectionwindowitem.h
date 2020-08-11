#ifndef CONNECTIONWINDOWITEM_H
#define CONNECTIONWINDOWITEM_H

#include "../basepage.h"
#include "subpageitem.h"
#include "../backend/preferences/preferences.h"
#include "../backend/preferences/preferenceshelper.h"
#include "firewallmodeitem.h"
#include "connectionmodeitem.h"
#include "packetsizeitem.h"
#include "../checkboxitem.h"
#include "macspoofingitem.h"
#include "ipc/generated_proto/types.pb.h"
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
    void setCurrentNetwork(const ProtoTypes::NetworkInterface &networkInterface);
    void setPacketSizeDetectionState(bool on);

signals:
    void networkWhitelistPageClick();
    void splitTunnelingPageClick();
    void proxySettingsPageClick();
    void cycleMacAddressClick();
    void detectPacketMssButtonClicked();
    void showTooltip(TooltipInfo info);
    void hideTooltip(TooltipId id);

private slots:
    void onFirewallModeChanged(const ProtoTypes::FirewallSettings &fm);
    void onFirewallModeHoverEnter();
    void onFirewallModeHoverLeave();
    void onConnectionModeChanged(const ProtoTypes::ConnectionSettings &cm);
    void onPacketSizeChanged(const ProtoTypes::PacketSize &ps);
    void onMacAddrSpoofingChanged(const ProtoTypes::MacAddrSpoofing &mas);
    void onIsAllowLanTrafficClicked(bool b);
    void onAllowLanTrafficButtonHoverLeave();
    //void onDNSWhileConnectedItemChanged(DNSWhileConnected dns);

    void onFirewallModePreferencesChanged(const ProtoTypes::FirewallSettings &fm);
    void onConnectionModePreferencesChanged(const ProtoTypes::ConnectionSettings &cm);
    void onPacketSizePreferencesChanged(const ProtoTypes::PacketSize &ps);
    void onMacAddrSpoofingPreferencesChanged(const ProtoTypes::MacAddrSpoofing &mas);
    void onIsAllowLanTrafficPreferencedChanged(bool b);
    void onInvalidLanAddressNotification(QString address);
    void onIsFirewallBlockedChanged(bool bFirewallBlocked);
#ifdef Q_OS_WIN
    void onKillTcpSocketsPreferencesChanged(bool b);
    void onKillTcpSocketsStateChanged(bool isChecked);
#endif

    //void onDNSWhileConnectedPreferencesChanged(const DNSWhileConnected &dns);

    void onLanguageChanged();

protected:
    void hideOpenPopups() override;

private:
    Preferences *preferences_;
    PreferencesHelper *preferencesHelper_;
    SubPageItem *networkWhitelistItem_;
    SubPageItem *splitTunnelingItem_;
    SubPageItem *proxySettingsItem_;
    FirewallModeItem *firewallModeItem_;
    ConnectionModeItem *connectionModeItem_;
    PacketSizeItem *packetSizeItem_;
    MacSpoofingItem *macSpoofingItem_;
    CheckBoxItem *checkBoxAllowLanTraffic_;
    // DNSWhileConnectedItem *dnsWhileConnectedItem_;
#ifdef Q_OS_WIN
    CheckBoxItem *cbKillTcp_;
#endif

    CONNECTION_SCREEN_TYPE currentScreen_;
};

} // namespace PreferencesWindow

#endif // CONNECTIONWINDOWITEM_H
