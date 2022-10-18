#include "connectionwindowitem.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>
#include "dpiscalemanager.h"
#include "utils/hardcodedsettings.h"
#include "commongraphics/baseitem.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/independentpixmap.h"
#include "languagecontroller.h"
#include "utils/logger.h"
#include "tooltips/tooltiputil.h"
#include "tooltips/tooltipcontroller.h"

namespace PreferencesWindow {

ConnectionWindowItem::ConnectionWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper)
  : CommonGraphics::BasePage(parent), preferences_(preferences), preferencesHelper_(preferencesHelper), currentScreen_(CONNECTION_SCREEN_HOME)
{
    setFlag(QGraphicsItem::ItemIsFocusable);
    setSpacerHeight(PREFERENCES_MARGIN);

    connect(preferences, &Preferences::splitTunnelingChanged, this, &ConnectionWindowItem::onSplitTunnelingPreferencesChanged);
    connect(preferences, &Preferences::firewallSettingsChanged, this, &ConnectionWindowItem::onFirewallPreferencesChanged);
    connect(preferences, &Preferences::connectionSettingsChanged, this, &ConnectionWindowItem::onConnectionModePreferencesChanged);
    connect(preferences, &Preferences::packetSizeChanged, this, &ConnectionWindowItem::onPacketSizePreferencesChanged);
    connect(preferences, &Preferences::isAllowLanTrafficChanged, this, &ConnectionWindowItem::onIsAllowLanTrafficPreferencesChanged);
    connect(preferences, &Preferences::invalidLanAddressNotification, this, &ConnectionWindowItem::onInvalidLanAddressNotification);
    connect(preferences, &Preferences::macAddrSpoofingChanged, this, &ConnectionWindowItem::onMacAddrSpoofingPreferencesChanged);
    connect(preferences, &Preferences::connectedDnsInfoChanged, this, &ConnectionWindowItem::onConnectedDnsPreferencesChanged);
    connect(preferences_, &Preferences::shareSecureHotspotChanged, this, &ConnectionWindowItem::onSecureHotspotPreferencesChanged);
    connect(preferences_, &Preferences::shareProxyGatewayChanged, this, &ConnectionWindowItem::onProxyGatewayPreferencesChanged);
    connect(preferences_, &Preferences::connectionSettingsChanged, this, &ConnectionWindowItem::onConnectionSettingsPreferencesChanged);
    connect(preferencesHelper, &PreferencesHelper::wifiSharingSupportedChanged, this, &ConnectionWindowItem::onPreferencesHelperWifiSharingSupportedChanged);
    connect(preferencesHelper, &PreferencesHelper::isFirewallBlockedChanged, this, &ConnectionWindowItem::onIsFirewallBlockedChanged);
    connect(preferencesHelper, &PreferencesHelper::isExternalConfigModeChanged, this, &ConnectionWindowItem::onIsExternalConfigModeChanged);
    connect(preferencesHelper, &PreferencesHelper::proxyGatewayAddressChanged, this, &ConnectionWindowItem::onProxyGatewayAddressChanged);
    connect(preferences, &Preferences::isTerminateSocketsChanged, this, &ConnectionWindowItem::onTerminateSocketsPreferencesChanged);
    connect(preferences, &Preferences::isAutoConnectChanged, this, &ConnectionWindowItem::onIsAutoConnectPreferencesChanged);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &ConnectionWindowItem::onLanguageChanged);

    subpagesGroup_ = new PreferenceGroup(this);

    networkOptionsItem_ = new LinkItem(subpagesGroup_, LinkItem::LinkType::SUBPAGE_LINK, QT_TRANSLATE_NOOP("PreferencesWindow::LinkItem","Network Options"));
    connect(networkOptionsItem_, &LinkItem::clicked, this, &ConnectionWindowItem::networkOptionsPageClick);
    subpagesGroup_->addItem(networkOptionsItem_);

#ifndef Q_OS_LINUX
    splitTunnelingItem_ = new LinkItem(subpagesGroup_, LinkItem::LinkType::SUBPAGE_LINK, QT_TRANSLATE_NOOP("PreferencesWindow::LinkItem","Split Tunneling"));
    connect(splitTunnelingItem_, &LinkItem::clicked, this, &ConnectionWindowItem::splitTunnelingPageClick);
    onSplitTunnelingPreferencesChanged(preferences->splitTunneling());
    subpagesGroup_->addItem(splitTunnelingItem_);
#endif

    proxySettingsItem_ = new LinkItem(subpagesGroup_, LinkItem::LinkType::SUBPAGE_LINK, QT_TRANSLATE_NOOP("PreferencesWindow::LinkItem","Proxy Settings"));
    connect(proxySettingsItem_, &LinkItem::clicked, this, &ConnectionWindowItem::proxySettingsPageClick);
    subpagesGroup_->addItem(proxySettingsItem_);

    addItem(subpagesGroup_);

    autoConnectGroup_ = new PreferenceGroup(this, tr("Connects to last used location when the app launches or joins a network."));
    checkBoxAutoConnect_ = new CheckBoxItem(autoConnectGroup_, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "Auto-Connect"), QString());
    checkBoxAutoConnect_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/AUTOCONNECT"));
    checkBoxAutoConnect_->setState(preferences->isAutoConnect());
    connect(checkBoxAutoConnect_, &CheckBoxItem::stateChanged, this, &ConnectionWindowItem::onIsAutoConnectPreferencesChangedByUser);
    autoConnectGroup_->addItem(checkBoxAutoConnect_);
    addItem(autoConnectGroup_);

    firewallGroup_ = new FirewallGroup(this,
                                       tr("Control the mode of behavior of the Windscribe firewall."),
                                       QString("https://%1/features/firewall").arg(HardcodedSettings::instance().serverUrl()));
    connect(firewallGroup_, &FirewallGroup::firewallPreferencesChanged, this, &ConnectionWindowItem::onFirewallPreferencesChangedByUser);
    firewallGroup_->setFirewallSettings(preferences->firewallSettings());
    firewallGroup_->setBlock(preferencesHelper->isFirewallBlocked());
    addItem(firewallGroup_);

    connectionModeGroup_ = new ProtocolGroup(this,
                                             preferencesHelper,
                                             tr("Connection Mode"),
                                             "preferences/CONNECTION_MODE",
                                             ProtocolGroup::SelectionType::COMBO_BOX,
                                             tr("Automatically choose the VPN protocol, or select one manually."),
                                             QString("https://%1/features/flexible-connectivity").arg(HardcodedSettings::instance().serverUrl()));
    connectionModeGroup_->setConnectionSettings(preferences->connectionSettings());
    connect(connectionModeGroup_, &ProtocolGroup::connectionModePreferencesChanged, this, &ConnectionWindowItem::onConnectionModePreferencesChangedByUser);
    addItem(connectionModeGroup_);

#ifndef Q_OS_LINUX
    packetSizeGroup_ = new PacketSizeGroup(this,
                                           tr("Automatically determine the MTU for your connection, or manually override."),
                                           QString("https://%1/features/packet-size").arg(HardcodedSettings::instance().serverUrl()));
    packetSizeGroup_->setPacketSizeSettings(preferences->packetSize());
    connect(packetSizeGroup_, &PacketSizeGroup::packetSizeChanged, this, &ConnectionWindowItem::onPacketSizePreferencesChangedByUser);
    connect(packetSizeGroup_, &PacketSizeGroup::detectPacketSize, this, &ConnectionWindowItem::detectPacketSize);
    addItem(packetSizeGroup_);

    connectedDnsGroup_ = new ConnectedDnsGroup(this,
                                               tr("Select the DNS server while connected to Windscribe."),
                                               QString("https://%1/features/flexible-dns").arg(HardcodedSettings::instance().serverUrl()));
    connectedDnsGroup_->setConnectedDnsInfo(preferences->connectedDnsInfo());
    connect(connectedDnsGroup_, &ConnectedDnsGroup::connectedDnsInfoChanged, this, &ConnectionWindowItem::onConnectedDnsPreferencesChangedByUser);
    addItem(connectedDnsGroup_);
#endif

    allowLanTrafficGroup_ = new PreferenceGroup(this,
                                                tr("Allow access to local services and printers while connected to Windscribe."),
                                                QString("https://%1/features/lan-traffic").arg(HardcodedSettings::instance().serverUrl()));
    checkBoxAllowLanTraffic_ = new CheckBoxItem(allowLanTrafficGroup_, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "Allow LAN Traffic"), QString());
    checkBoxAllowLanTraffic_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/ALLOW_LAN_TRAFFIC"));
    checkBoxAllowLanTraffic_->setState(preferences->isAllowLanTraffic());
    connect(checkBoxAllowLanTraffic_, &CheckBoxItem::stateChanged, this, &ConnectionWindowItem::onIsAllowLanTrafficPreferencesChangedByUser);
    connect(checkBoxAllowLanTraffic_, &CheckBoxItem::buttonHoverLeave, this, &ConnectionWindowItem::onAllowLanTrafficButtonHoverLeave);
    allowLanTrafficGroup_->addItem(checkBoxAllowLanTraffic_);
    addItem(allowLanTrafficGroup_);

#ifndef Q_OS_LINUX
    macSpoofingGroup_ = new MacSpoofingGroup(this,
                                             tr("Spoof your device's physical address (MAC address)."),
                                             QString("https://%1/features/mac-spoofing").arg(HardcodedSettings::instance().serverUrl()));
    connect(macSpoofingGroup_, &MacSpoofingGroup::macAddrSpoofingChanged, this, &ConnectionWindowItem::onMacAddrSpoofingPreferencesChangedByUser);
    connect(macSpoofingGroup_, &MacSpoofingGroup::cycleMacAddressClick, this, &ConnectionWindowItem::cycleMacAddressClick);
    macSpoofingGroup_->setMacSpoofingSettings(preferences->macAddrSpoofing());
    addItem(macSpoofingGroup_);
#endif

#if defined(Q_OS_WIN)
    terminateSocketsGroup_ = new PreferenceGroup(this,
                                                 tr("Close all active TCP sockets when the VPN tunnel is established."),
                                                 QString("https://%1/features/tcp-socket-termination").arg(HardcodedSettings::instance().serverUrl()));
    terminateSocketsItem_ = new CheckBoxItem(terminateSocketsGroup_, QT_TRANSLATE_NOOP("PreferencesWindow::CheckBoxItem", "Terminate Sockets"), QString());
    terminateSocketsItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/TERMINATE_SOCKETS"));
    terminateSocketsItem_->setState(preferences->isTerminateSockets());
    connect(terminateSocketsItem_, &CheckBoxItem::stateChanged, this, &ConnectionWindowItem::onTerminateSocketsPreferencesChangedByUser);
    terminateSocketsGroup_->addItem(terminateSocketsItem_);
    addItem(terminateSocketsGroup_);
#endif

#ifndef Q_OS_LINUX
    secureHotspotGroup_ = new SecureHotspotGroup(this,
                                                 "",
                                                 QString("https://%1/features/secure-hotspot").arg(HardcodedSettings::instance().serverUrl()));
    connect(secureHotspotGroup_, &SecureHotspotGroup::secureHotspotPreferencesChanged, this, &ConnectionWindowItem::onSecureHotspotPreferencesChangedByUser);
    secureHotspotGroup_->setSecureHotspotSettings(preferences->shareSecureHotspot());
    updateIsSupported(preferencesHelper_->isWifiSharingSupported(),
                      isIkev2OrAutomaticConnectionMode(preferences_->connectionSettings()));
    addItem(secureHotspotGroup_);
#endif

    proxyGatewayGroup_ = new ProxyGatewayGroup(this,
                                               tr("Configure your TV, gaming console, or other devices that support proxy servers."),
                                               QString("https://%1/features/proxy-gateway").arg(HardcodedSettings::instance().serverUrl()));
    connect(proxyGatewayGroup_, &ProxyGatewayGroup::proxyGatewayPreferencesChanged, this, &ConnectionWindowItem::onProxyGatewayPreferencesChangedByUser);
    proxyGatewayGroup_->setProxyGatewaySettings(preferences->shareProxyGateway());
    addItem(proxyGatewayGroup_);
}

QString ConnectionWindowItem::caption()
{
    return QT_TRANSLATE_NOOP("PreferencesWindow::PreferencesWindowItem", "Connection");
}

void ConnectionWindowItem::updateScaling()
{
    CommonGraphics::BasePage::updateScaling();
}

CONNECTION_SCREEN_TYPE ConnectionWindowItem::getScreen()
{
    return currentScreen_;
}

void ConnectionWindowItem::setScreen(CONNECTION_SCREEN_TYPE subScreen)
{
    currentScreen_ = subScreen;
}

void ConnectionWindowItem::setCurrentNetwork(const types::NetworkInterface &networkInterface)
{
#ifndef Q_OS_LINUX
    macSpoofingGroup_->setCurrentNetwork(networkInterface);
#endif
}

void ConnectionWindowItem::setPacketSizeDetectionState(bool on)
{
#ifndef Q_OS_LINUX
    packetSizeGroup_->setPacketSizeDetectionState(on);
#endif
}

void ConnectionWindowItem::showPacketSizeDetectionError(const QString &title,
                                                        const QString &message)
{
#ifndef Q_OS_LINUX
    packetSizeGroup_->showPacketSizeDetectionError(title, message);
#endif
}

void ConnectionWindowItem::onFirewallPreferencesChangedByUser(const types::FirewallSettings &fm)
{
    preferences_->setFirewallSettings(fm);
}

void ConnectionWindowItem::onConnectionModePreferencesChangedByUser(const types::ConnectionSettings &cm)
{
    preferences_->setConnectionSettings(cm);
}

void ConnectionWindowItem::onPacketSizePreferencesChangedByUser(const types::PacketSize &ps)
{
#ifndef Q_OS_LINUX
    preferences_->setPacketSize(ps);
#endif
}

void ConnectionWindowItem::onMacAddrSpoofingPreferencesChangedByUser(const types::MacAddrSpoofing &mas)
{
#ifndef Q_OS_LINUX
    preferences_->setMacAddrSpoofing(mas);
#endif
}

void ConnectionWindowItem::onTerminateSocketsPreferencesChangedByUser(bool isChecked)
{
#if defined(Q_OS_WIN)
    preferences_->setTerminateSockets(isChecked);
#endif
}

void ConnectionWindowItem::onSplitTunnelingPreferencesChanged(const types::SplitTunneling &st)
{
#ifndef Q_OS_LINUX
    QString splitTunnelStatus;

    if (st.settings.active)
    {
        switch(st.settings.mode)
        {
            case SPLIT_TUNNELING_MODE_EXCLUDE:
                splitTunnelStatus = tr("Exclusive");
                break;
            case SPLIT_TUNNELING_MODE_INCLUDE:
                splitTunnelStatus = tr("Inclusive");
                break;
        }
    }
    else
    {
        splitTunnelStatus = tr("Off");
    }
    splitTunnelingItem_->setLinkText(splitTunnelStatus);
#endif
}

void ConnectionWindowItem::onFirewallPreferencesChanged(const types::FirewallSettings &fm)
{
    firewallGroup_->setFirewallSettings(fm);
}

void ConnectionWindowItem::onConnectionModePreferencesChanged(const types::ConnectionSettings &cm)
{
    connectionModeGroup_->setConnectionSettings(cm);
}

void ConnectionWindowItem::onPacketSizePreferencesChanged(const types::PacketSize &ps)
{
#ifndef Q_OS_LINUX
    packetSizeGroup_->setPacketSizeSettings(ps);
#endif
}

void ConnectionWindowItem::onMacAddrSpoofingPreferencesChanged(const types::MacAddrSpoofing &mas)
{
#ifndef Q_OS_LINUX
    macSpoofingGroup_->setMacSpoofingSettings(mas);
#endif
}

void ConnectionWindowItem::onTerminateSocketsPreferencesChanged(bool b)
{
#if defined(Q_OS_WIN)
    terminateSocketsItem_->setState(b);
#endif
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
    firewallGroup_->setBlock(bFirewallBlocked);
}

void ConnectionWindowItem::onIsExternalConfigModeChanged(bool bIsExternalConfigMode)
{
    connectionModeGroup_->setVisible(!bIsExternalConfigMode);
    CommonGraphics::BaseItem *spacer = item(indexOf(connectionModeGroup_) + 1);
    spacer->setVisible(!bIsExternalConfigMode);
}

void ConnectionWindowItem::onConnectedDnsPreferencesChanged(const types::ConnectedDnsInfo &dns)
{
#ifndef Q_OS_LINUX
    connectedDnsGroup_->setConnectedDnsInfo(dns);
#endif
}

void ConnectionWindowItem::onProxyGatewayAddressChanged(const QString &address)
{
    proxyGatewayGroup_->setProxyGatewayAddress(address);
}

void ConnectionWindowItem::onIsAllowLanTrafficPreferencesChanged(bool b)
{
    checkBoxAllowLanTraffic_->setState(b);
}

void ConnectionWindowItem::onLanguageChanged()
{

}

void ConnectionWindowItem::onIsAllowLanTrafficPreferencesChangedByUser(bool b)
{
    preferences_->setAllowLanTraffic(b);
}

void ConnectionWindowItem::onConnectedDnsPreferencesChangedByUser(const types::ConnectedDnsInfo &dns)
{
#ifndef Q_OS_LINUX
    preferences_->setConnectedDnsInfo(dns);
#endif
}

void ConnectionWindowItem::hideOpenPopups()
{
    // qCDebug(LOG_PREFERENCES) << "Hiding Connection popups";

    CommonGraphics::BasePage::hideOpenPopups();
}

void ConnectionWindowItem::onSecureHotspotPreferencesChangedByUser(const types::ShareSecureHotspot &ss)
{
#ifndef Q_OS_LINUX
    preferences_->setShareSecureHotspot(ss);
#endif
}

void ConnectionWindowItem::onSecureHotspotPreferencesChanged(const types::ShareSecureHotspot &ss)
{
#ifndef Q_OS_LINUX
    secureHotspotGroup_->setSecureHotspotSettings(ss);
#endif
}

void ConnectionWindowItem::onConnectionSettingsPreferencesChanged(const types::ConnectionSettings &cs)
{
    updateIsSupported(preferencesHelper_->isWifiSharingSupported(), isIkev2OrAutomaticConnectionMode(cs));
}

void ConnectionWindowItem::onProxyGatewayPreferencesChangedByUser(const types::ShareProxyGateway &sp)
{
    preferences_->setShareProxyGateway(sp);
}

void ConnectionWindowItem::onProxyGatewayPreferencesChanged(const types::ShareProxyGateway &sp)
{
    proxyGatewayGroup_->setProxyGatewaySettings(sp);
}

void ConnectionWindowItem::onPreferencesHelperWifiSharingSupportedChanged(bool bSupported)
{
    updateIsSupported(bSupported, isIkev2OrAutomaticConnectionMode(preferences_->connectionSettings()));
}

bool ConnectionWindowItem::isIkev2OrAutomaticConnectionMode(const types::ConnectionSettings &cs) const
{
    return cs.protocol() == types::Protocol::IKEV2 || cs.protocol() == types::Protocol::WIREGUARD || cs.isAutomatic();
}

void ConnectionWindowItem::updateIsSupported(bool isWifiSharingSupported, bool isIkev2OrAutomatic)
{
#ifndef Q_OS_LINUX
    if (secureHotspotGroup_)
    {
        if (!isWifiSharingSupported)
        {
            secureHotspotGroup_->setSupported(SecureHotspotGroup::HOTSPOT_NOT_SUPPORTED);
        }
        else if (isIkev2OrAutomatic)
        {
            secureHotspotGroup_->setSupported(SecureHotspotGroup::HOTSPOT_NOT_SUPPORTED_BY_IKEV2);
        }
        else
        {
            secureHotspotGroup_->setSupported(SecureHotspotGroup::HOTSPOT_SUPPORTED);
        }
    }
#endif
}

void ConnectionWindowItem::onIsAutoConnectPreferencesChangedByUser(bool on)
{
    preferences_->setAutoConnect(on);
}

void ConnectionWindowItem::onIsAutoConnectPreferencesChanged(bool b)
{
    checkBoxAutoConnect_->setState(b);
}

} // namespace PreferencesWindow
