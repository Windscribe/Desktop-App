#include "connectionwindowitem.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>
#include "dpiscalemanager.h"
#include "languagecontroller.h"
#include "utils/protoenumtostring.h"
#include "tooltips/tooltiputil.h"
#include "tooltips/tooltipcontroller.h"

namespace PreferencesWindow {

ConnectionWindowItem::ConnectionWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper) : BasePage(parent)
    , preferences_(preferences)
    , preferencesHelper_(preferencesHelper)
    , currentScreen_(CONNECTION_SCREEN_HOME)
{
    setFlag(QGraphicsItem::ItemIsFocusable);

    connect(preferences, SIGNAL(firewallSettingsChanged(ProtoTypes::FirewallSettings)), SLOT(onFirewallModePreferencesChanged(ProtoTypes::FirewallSettings)));
    connect(preferences, SIGNAL(connectionSettingsChanged(ProtoTypes::ConnectionSettings)), SLOT(onConnectionModePreferencesChanged(ProtoTypes::ConnectionSettings)));
    connect(preferences, SIGNAL(packetSizeChanged(ProtoTypes::PacketSize)), SLOT(onPacketSizePreferencesChanged(ProtoTypes::PacketSize)));
    connect(preferences, SIGNAL(isAllowLanTrafficChanged(bool)), SLOT(onIsAllowLanTrafficPreferencedChanged(bool)));
    connect(preferences, SIGNAL(invalidLanAddressNotification(QString)), SLOT(onInvalidLanAddressNotification(QString)));
    connect(preferences, SIGNAL(macAddrSpoofingChanged(ProtoTypes::MacAddrSpoofing)), SLOT(onMacAddrSpoofingPreferencesChanged(ProtoTypes::MacAddrSpoofing)));
    //connect(preferences, SIGNAL(dnsWhileConnectedChanged(DNSWhileConnected)), SLOT(onDNSWhileConnectedPreferencesChanged(DNSWhileConnected)));
    connect(preferencesHelper, SIGNAL(isFirewallBlockedChanged(bool)), SLOT(onIsFirewallBlockedChanged(bool)));
    connect(preferencesHelper, SIGNAL(isExternalConfigModeChanged(bool)), SLOT(onIsExternalConfigModeChanged(bool)));
#ifdef Q_OS_WIN
    connect(preferences, SIGNAL(isKillTcpSocketsChanged(bool)), SLOT(onKillTcpSocketsPreferencesChanged(bool)));
#endif

    connect(&LanguageController::instance(), SIGNAL(languageChanged()), SLOT(onLanguageChanged()));

    networkWhitelistItem_ = new SubPageItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::SubPageItem","Network Whitelist"), true);
    connect(networkWhitelistItem_, SIGNAL(clicked()), SIGNAL(networkWhitelistPageClick()));
    addItem(networkWhitelistItem_);

    splitTunnelingItem_ = new SubPageItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::SubPageItem","Split Tunneling"), true);
    connect(splitTunnelingItem_, SIGNAL(clicked()), SIGNAL(splitTunnelingPageClick()));
    addItem(splitTunnelingItem_);

    proxySettingsItem_ = new SubPageItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::SubPageItem","Proxy Settings"), true);
    connect(proxySettingsItem_, SIGNAL(clicked()), SIGNAL(proxySettingsPageClick()));
    addItem(proxySettingsItem_);

    firewallModeItem_ = new FirewallModeItem(this);
    connect(firewallModeItem_, SIGNAL(firewallModeChanged(ProtoTypes::FirewallSettings)), SLOT(onFirewallModeChanged(ProtoTypes::FirewallSettings)));
    connect(firewallModeItem_, SIGNAL(buttonHoverEnter()), SLOT(onFirewallModeHoverEnter()));
    connect(firewallModeItem_, SIGNAL(buttonHoverLeave()), SLOT(onFirewallModeHoverLeave()));
    firewallModeItem_->setFirewallMode(preferences->firewalSettings());
    firewallModeItem_->setFirewallBlock(preferencesHelper->isFirewallBlocked());
    addItem(firewallModeItem_);

    connectionModeItem_ = new ConnectionModeItem(this, preferencesHelper);
    connectionModeItem_->setConnectionMode(preferences->connectionSettings());
    connect(connectionModeItem_, SIGNAL(connectionlModeChanged(ProtoTypes::ConnectionSettings)),
        SLOT(onConnectionModeChanged(ProtoTypes::ConnectionSettings)));
    connect(connectionModeItem_, SIGNAL(buttonHoverEnter(ConnectionModeItem::ButtonType)),
        SLOT(onConnectionModeHoverEnter(ConnectionModeItem::ButtonType)));
    connect(connectionModeItem_, SIGNAL(buttonHoverLeave(ConnectionModeItem::ButtonType)),
        SLOT(onConnectionModeHoverLeave(ConnectionModeItem::ButtonType)));
    addItem(connectionModeItem_);

    packetSizeItem_ = new PacketSizeItem(this);
    packetSizeItem_->setPacketSize(preferences->packetSize());
    connect(packetSizeItem_, SIGNAL(packetSizeChanged(ProtoTypes::PacketSize)), SLOT(onPacketSizeChanged(ProtoTypes::PacketSize)));
    connect(packetSizeItem_, SIGNAL(detectAppropriatePacketSizeButtonClicked()), SIGNAL(detectAppropriatePacketSizeButtonClicked()));
    addItem(packetSizeItem_);

    checkBoxAllowLanTraffic_ = new CheckBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "Allow LAN traffic"), QString());
    checkBoxAllowLanTraffic_->setState(preferences->isAllowLanTraffic());
    connect(checkBoxAllowLanTraffic_, SIGNAL(stateChanged(bool)), SLOT(onIsAllowLanTrafficClicked(bool)));
    connect(checkBoxAllowLanTraffic_, SIGNAL(buttonHoverLeave()), SLOT(onAllowLanTrafficButtonHoverLeave()));
    addItem(checkBoxAllowLanTraffic_);

    macSpoofingItem_ = new MacSpoofingItem(this);
    connect(macSpoofingItem_, SIGNAL(macAddrSpoofingChanged(ProtoTypes::MacAddrSpoofing)), SLOT(onMacAddrSpoofingChanged(ProtoTypes::MacAddrSpoofing)));
    connect(macSpoofingItem_, SIGNAL(cycleMacAddressClick()), SIGNAL(cycleMacAddressClick()));
    addItem(macSpoofingItem_);

#ifdef Q_OS_WIN
    cbKillTcp_ = new CheckBoxItem(this, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "Kill TCP sockets after connection"), "");
    cbKillTcp_->setState(preferences->isKillTcpSockets());
    connect(cbKillTcp_, SIGNAL(stateChanged(bool)), SLOT(onKillTcpSocketsStateChanged(bool)));
    addItem(cbKillTcp_);
#endif

    /* dnsWhileConnectedItem_ = new DNSWhileConnectedItem(this);
     connect(dnsWhileConnectedItem_, SIGNAL(dnsWhileConnectedChanged(DNSWhileConnected)), SLOT(onDNSWhileConnectedItemChanged(DNSWhileConnected)));
     dnsWhileConnectedItem_->setDNSWhileConnected(preferences->dnsWhileConnected());
     addItem(dnsWhileConnectedItem_);*/
}

QString ConnectionWindowItem::caption()
{
    return QT_TRANSLATE_NOOP("PreferencesWindow::PreferencesWindowItem", "Connection");
}

void ConnectionWindowItem::updateScaling()
{
    BasePage::updateScaling();
    packetSizeItem_->updateScaling();
}

CONNECTION_SCREEN_TYPE ConnectionWindowItem::getScreen()
{
    return currentScreen_;
}

void ConnectionWindowItem::setScreen(CONNECTION_SCREEN_TYPE subScreen)
{
    currentScreen_ = subScreen;
}

void ConnectionWindowItem::setCurrentNetwork(const ProtoTypes::NetworkInterface &networkInterface)
{
    macSpoofingItem_->setCurrentNetwork(networkInterface);
}

void ConnectionWindowItem::setPacketSizeDetectionState(bool on)
{
    packetSizeItem_->setPacketSizeDetectionState(on);
}

void ConnectionWindowItem::showPacketSizeDetectionError(const QString &title,
                                                        const QString &message)
{
    packetSizeItem_->showPacketSizeDetectionError(title, message);
}

void ConnectionWindowItem::onFirewallModeChanged(const ProtoTypes::FirewallSettings &fm)
{
    preferences_->setFirewallSettings(fm);
}

void ConnectionWindowItem::onFirewallModeHoverEnter()
{
    if (!preferencesHelper_->isFirewallBlocked())
        return;

    QGraphicsView *view = scene()->views().first();
    QPoint globalPt = view->mapToGlobal(view->mapFromScene(firewallModeItem_->getButtonScenePos()));

    TooltipInfo ti(TOOLTIP_TYPE_DESCRIPTIVE, TOOLTIP_ID_FIREWALL_BLOCKED);
    ti.tailtype = TOOLTIP_TAIL_BOTTOM;
    ti.tailPosPercent = 0.5;
    ti.x = globalPt.x() + 8 * G_SCALE;
    ti.y = globalPt.y() - 4 * G_SCALE;
    ti.width = 200 * G_SCALE;
    TooltipUtil::getFirewallBlockedTooltipInfo(&ti.title, &ti.desc);
    TooltipController::instance().showTooltipDescriptive(ti);
}

void ConnectionWindowItem::onFirewallModeHoverLeave()
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_FIREWALL_BLOCKED);
}

void ConnectionWindowItem::onConnectionModeHoverEnter(ConnectionModeItem::ButtonType type)
{
    if (connectionModeItem_->isPortMapInitialized())
        return;

    QGraphicsView *view = scene()->views().first();
    QPoint globalPt =
        view->mapToGlobal(view->mapFromScene(connectionModeItem_->getButtonScenePos(type)));

    TooltipInfo ti(TOOLTIP_TYPE_DESCRIPTIVE, TOOLTIP_ID_CONNECTION_MODE_LOGIN);
    ti.tailtype = TOOLTIP_TAIL_BOTTOM;
    ti.tailPosPercent = 0.5;
    ti.x = globalPt.x() + 8 * G_SCALE;
    ti.y = globalPt.y() - 4 * G_SCALE;
    ti.width = 200 * G_SCALE;
    ti.title = tr("Not Logged In");
    ti.desc = tr("Please login to modify connection settings.");
    TooltipController::instance().showTooltipDescriptive(ti);
}

void ConnectionWindowItem::onConnectionModeHoverLeave(ConnectionModeItem::ButtonType /*type*/)
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_CONNECTION_MODE_LOGIN);
}

void ConnectionWindowItem::onConnectionModeChanged(const ProtoTypes::ConnectionSettings &cm)
{
    preferences_->setConnectionSettings(cm);

    if (!cm.is_automatic()) // only scroll when opening
    {
        // magic number is expanded height
        emit scrollToPosition(static_cast<int>(connectionModeItem_->y()) + 50 + 43 + 43 );
    }
}

void ConnectionWindowItem::onPacketSizeChanged(const ProtoTypes::PacketSize &ps)
{
    preferences_->setPacketSize(ps);
}

void ConnectionWindowItem::onMacAddrSpoofingChanged(const ProtoTypes::MacAddrSpoofing &mas)
{
    preferences_->setMacAddrSpoofing(mas);
}

#ifdef Q_OS_WIN
void ConnectionWindowItem::onKillTcpSocketsStateChanged(bool isChecked)
{
    preferences_->setKillTcpSockets(isChecked);
}
#endif

void ConnectionWindowItem::onFirewallModePreferencesChanged(const ProtoTypes::FirewallSettings &fm)
{
    firewallModeItem_->setFirewallMode(fm);
}

void ConnectionWindowItem::onConnectionModePreferencesChanged(const ProtoTypes::ConnectionSettings &cm)
{
    connectionModeItem_->setConnectionMode(cm);
}

void ConnectionWindowItem::onPacketSizePreferencesChanged(const ProtoTypes::PacketSize &ps)
{
    packetSizeItem_->setPacketSize(ps);
}

void ConnectionWindowItem::onMacAddrSpoofingPreferencesChanged(const ProtoTypes::MacAddrSpoofing &mas)
{
    macSpoofingItem_->setMacAddrSpoofing(mas);

    if (mas.is_enabled()) // only scroll when opening
    {
        // magic number is expanded height
        emit scrollToPosition(static_cast<int>(macSpoofingItem_->y()) + 50 + 43 + 43);
    }
}

#ifdef Q_OS_WIN
void ConnectionWindowItem::onKillTcpSocketsPreferencesChanged(bool b)
{
    cbKillTcp_->setState(b);
}
#endif

void ConnectionWindowItem::onIsAllowLanTrafficPreferencedChanged(bool b)
{
    checkBoxAllowLanTraffic_->setState(b);
}

void ConnectionWindowItem::onInvalidLanAddressNotification(QString address)
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_INVALID_LAN_ADDRESS);

    const auto *the_scene = scene();
    if (!the_scene || !isVisible())
        return;
    const auto *view = the_scene->views().first();
    if (!view)
        return;

    const auto global_pt = view->mapToGlobal(
        view->mapFromScene(checkBoxAllowLanTraffic_->getButtonScenePos()));

    const int width = 150 * G_SCALE;
    const int posX = global_pt.x() + 15 * G_SCALE;
    const int posY = global_pt.y() - 5 * G_SCALE;

    TooltipInfo ti(TOOLTIP_TYPE_DESCRIPTIVE, TOOLTIP_ID_INVALID_LAN_ADDRESS);
    ti.x = posX;
    ti.y = posY;
    ti.tailtype = TOOLTIP_TAIL_BOTTOM;
    ti.tailPosPercent = 0.9;
    ti.title = address;
    ti.desc = tr("Your IP address is not in a LAN address range. Please reconfigure your LAN with "
                 "a valid RFC-1918 IP range.");
    ti.width = width;
    ti.delay = 100;
    TooltipController::instance().showTooltipDescriptive(ti);
}

void ConnectionWindowItem::onAllowLanTrafficButtonHoverLeave()
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_INVALID_LAN_ADDRESS);
}

void ConnectionWindowItem::onIsFirewallBlockedChanged(bool bFirewallBlocked)
{
    firewallModeItem_->setFirewallBlock(bFirewallBlocked);
}

void ConnectionWindowItem::onIsExternalConfigModeChanged(bool bIsExternalConfigMode)
{
    connectionModeItem_->setVisible(!bIsExternalConfigMode);
}

/*
void ConnectionWindowItem::onDNSWhileConnectedPreferencesChanged(const DNSWhileConnected &dns)
{
    dnsWhileConnectedItem_->setDNSWhileConnected(dns);

    if (dns.type == DNSWhileConnectedType::CUSTOM)
    {
        // magic number is expanded height
        emit scrollToItemBottom(static_cast<int>(dnsWhileConnectedItem_->y()) + 50 + 43);
    }
}*/

void ConnectionWindowItem::onLanguageChanged()
{

}

void ConnectionWindowItem::onIsAllowLanTrafficClicked(bool b)
{
    preferences_->setAllowLanTraffic(b);
}

/*void ConnectionWindowItem::onDNSWhileConnectedItemChanged(DNSWhileConnected dns)
{
    preferences_->setDNSWhileConnected(dns);
}*/

void ConnectionWindowItem::hideOpenPopups()
{
    BasePage::hideOpenPopups();
}


} // namespace PreferencesWindow
