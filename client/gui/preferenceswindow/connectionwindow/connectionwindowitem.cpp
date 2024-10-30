#include "connectionwindowitem.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>

#include "dpiscalemanager.h"
#include "utils/hardcodedsettings.h"
#include "commongraphics/baseitem.h"
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/independentpixmap.h"
#include "languagecontroller.h"
#include "generalmessagecontroller.h"
#include "tooltips/tooltipcontroller.h"
#ifdef Q_OS_MACOS
#include "utils/macutils.h"
#endif

extern QWidget *g_mainWindow;

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
    connect(preferences, &Preferences::macAddrSpoofingChanged, this, &ConnectionWindowItem::onMacAddrSpoofingPreferencesChanged);
    connect(preferences, &Preferences::connectedDnsInfoChanged, this, &ConnectionWindowItem::onConnectedDnsPreferencesChanged);
    connect(preferences_, &Preferences::shareSecureHotspotChanged, this, &ConnectionWindowItem::onSecureHotspotPreferencesChanged);
    connect(preferences_, &Preferences::shareProxyGatewayChanged, this, &ConnectionWindowItem::onProxyGatewayPreferencesChanged);
    connect(preferences_, &Preferences::connectionSettingsChanged, this, &ConnectionWindowItem::onConnectionSettingsPreferencesChanged);
    connect(preferencesHelper, &PreferencesHelper::wifiSharingSupportedChanged, this, &ConnectionWindowItem::onPreferencesHelperWifiSharingSupportedChanged);
    connect(preferencesHelper, &PreferencesHelper::isExternalConfigModeChanged, this, &ConnectionWindowItem::onIsExternalConfigModeChanged);
    connect(preferencesHelper, &PreferencesHelper::proxyGatewayAddressChanged, this, &ConnectionWindowItem::onProxyGatewayAddressChanged);
    connect(preferences, &Preferences::isTerminateSocketsChanged, this, &ConnectionWindowItem::onTerminateSocketsPreferencesChanged);
    connect(preferences, &Preferences::isAntiCensorshipChanged, this, &ConnectionWindowItem::onAntiCensorshipPreferencesChanged);
    connect(preferences, &Preferences::isAutoConnectChanged, this, &ConnectionWindowItem::onIsAutoConnectPreferencesChanged);

    subpagesGroup_ = new PreferenceGroup(this);

    networkOptionsItem_ = new LinkItem(subpagesGroup_, LinkItem::LinkType::SUBPAGE_LINK);
    connect(networkOptionsItem_, &LinkItem::clicked, this, &ConnectionWindowItem::networkOptionsPageClick);
    subpagesGroup_->addItem(networkOptionsItem_);

    splitTunnelingItem_ = new LinkItem(subpagesGroup_, LinkItem::LinkType::SUBPAGE_LINK);
    connect(splitTunnelingItem_, &LinkItem::clicked, this, &ConnectionWindowItem::splitTunnelingPageClick);
    onSplitTunnelingPreferencesChanged(preferences->splitTunneling());
    subpagesGroup_->addItem(splitTunnelingItem_);

    proxySettingsItem_ = new LinkItem(subpagesGroup_, LinkItem::LinkType::SUBPAGE_LINK);
    connect(proxySettingsItem_, &LinkItem::clicked, this, &ConnectionWindowItem::proxySettingsPageClick);
    subpagesGroup_->addItem(proxySettingsItem_);

    addItem(subpagesGroup_);

    autoConnectGroup_ = new PreferenceGroup(this);
    checkBoxAutoConnect_ = new ToggleItem(autoConnectGroup_);
    checkBoxAutoConnect_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/AUTOCONNECT"));
    checkBoxAutoConnect_->setState(preferences->isAutoConnect());
    connect(checkBoxAutoConnect_, &ToggleItem::stateChanged, this, &ConnectionWindowItem::onIsAutoConnectPreferencesChangedByUser);
    autoConnectGroup_->addItem(checkBoxAutoConnect_);
    addItem(autoConnectGroup_);

    firewallGroup_ = new FirewallGroup(this,
                                       "",
                                       QString("https://%1/features/firewall").arg(HardcodedSettings::instance().windscribeServerUrl()));
    connect(firewallGroup_, &FirewallGroup::firewallPreferencesChanged, this, &ConnectionWindowItem::onFirewallPreferencesChangedByUser);
    firewallGroup_->setFirewallSettings(preferences->firewallSettings());
    addItem(firewallGroup_);

    connectionModeGroup_ = new ProtocolGroup(this,
                                             preferencesHelper,
                                             "",
                                             "preferences/CONNECTION_MODE",
                                             ProtocolGroup::SelectionType::COMBO_BOX,
                                             "",
                                             QString("https://%1/features/flexible-connectivity").arg(HardcodedSettings::instance().windscribeServerUrl()));
    connectionModeGroup_->setConnectionSettings(preferences->connectionSettings());
    connect(connectionModeGroup_, &ProtocolGroup::connectionModePreferencesChanged, this, &ConnectionWindowItem::onConnectionModePreferencesChangedByUser);
    addItem(connectionModeGroup_);

    packetSizeGroup_ = new PacketSizeGroup(this,
                                           "",
                                           QString("https://%1/features/packet-size").arg(HardcodedSettings::instance().windscribeServerUrl()));
    packetSizeGroup_->setPacketSizeSettings(preferences->packetSize());
    connect(packetSizeGroup_, &PacketSizeGroup::packetSizeChanged, this, &ConnectionWindowItem::onPacketSizePreferencesChangedByUser);
    connect(packetSizeGroup_, &PacketSizeGroup::detectPacketSize, this, &ConnectionWindowItem::detectPacketSize);
    addItem(packetSizeGroup_);

    connectedDnsGroup_ = new ConnectedDnsGroup(this,
                                               "",
                                               QString("https://%1/features/flexible-dns").arg(HardcodedSettings::instance().windscribeServerUrl()));
    connectedDnsGroup_->setConnectedDnsInfo(preferences->connectedDnsInfo());
    connect(connectedDnsGroup_, &ConnectedDnsGroup::connectedDnsInfoChanged, this, &ConnectionWindowItem::onConnectedDnsPreferencesChangedByUser);
    connect(connectedDnsGroup_, &ConnectedDnsGroup::domainsClick, this, &ConnectionWindowItem::connectedDnsDomainsClick);
    addItem(connectedDnsGroup_);
    allowLanTrafficGroup_ = new PreferenceGroup(this,
                                                "",
                                                QString("https://%1/features/lan-traffic").arg(HardcodedSettings::instance().windscribeServerUrl()));
    checkBoxAllowLanTraffic_ = new ToggleItem(allowLanTrafficGroup_, tr("Allow LAN Traffic"));
    checkBoxAllowLanTraffic_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/ALLOW_LAN_TRAFFIC"));
    checkBoxAllowLanTraffic_->setState(preferences->isAllowLanTraffic());
    connect(checkBoxAllowLanTraffic_, &ToggleItem::stateChanged, this, &ConnectionWindowItem::onIsAllowLanTrafficPreferencesChangedByUser);
    connect(checkBoxAllowLanTraffic_, &ToggleItem::buttonHoverLeave, this, &ConnectionWindowItem::onAllowLanTrafficButtonHoverLeave);
    allowLanTrafficGroup_->addItem(checkBoxAllowLanTraffic_);
    addItem(allowLanTrafficGroup_);

    macSpoofingGroup_ = new MacSpoofingGroup(this,
                                             "",
                                             QString("https://%1/features/mac-spoofing").arg(HardcodedSettings::instance().windscribeServerUrl()));
    connect(macSpoofingGroup_, &MacSpoofingGroup::macAddrSpoofingChanged, this, &ConnectionWindowItem::onMacAddrSpoofingPreferencesChangedByUser);
    connect(macSpoofingGroup_, &MacSpoofingGroup::cycleMacAddressClick, this, &ConnectionWindowItem::cycleMacAddressClick);
    macSpoofingGroup_->setMacSpoofingSettings(preferences->macAddrSpoofing());
    addItem(macSpoofingGroup_);
#ifdef Q_OS_MACOS
    if (MacUtils::isOsVersionAtLeast(14, 4)) {
        macSpoofingGroup_->setEnabled(false);
    }
#endif

#if defined(Q_OS_WIN)
    terminateSocketsGroup_ = new PreferenceGroup(this,
                                                 "",
                                                 QString("https://%1/features/tcp-socket-termination").arg(HardcodedSettings::instance().windscribeServerUrl()));
    terminateSocketsItem_ = new ToggleItem(terminateSocketsGroup_, tr("Terminate Sockets"));
    terminateSocketsItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/TERMINATE_SOCKETS"));
    terminateSocketsItem_->setState(preferences->isTerminateSockets());
    connect(terminateSocketsItem_, &ToggleItem::stateChanged, this, &ConnectionWindowItem::onTerminateSocketsPreferencesChangedByUser);
    terminateSocketsGroup_->addItem(terminateSocketsItem_);
    addItem(terminateSocketsGroup_);
#endif

#if defined(Q_OS_WIN)
    secureHotspotGroup_ = new SecureHotspotGroup(this,
                                                 "",
                                                 QString("https://%1/features/secure-hotspot").arg(HardcodedSettings::instance().windscribeServerUrl()));
    connect(secureHotspotGroup_, &SecureHotspotGroup::secureHotspotPreferencesChanged, this, &ConnectionWindowItem::onSecureHotspotPreferencesChangedByUser);
    secureHotspotGroup_->setSecureHotspotSettings(preferences->shareSecureHotspot());
    updateIsSupported(preferencesHelper_->isWifiSharingSupported(), isIkev2(preferences_->connectionSettings()));
    addItem(secureHotspotGroup_);
#endif

    proxyGatewayGroup_ = new ProxyGatewayGroup(this,
                                               "",
                                               QString("https://%1/features/proxy-gateway").arg(HardcodedSettings::instance().windscribeServerUrl()));
    connect(proxyGatewayGroup_, &ProxyGatewayGroup::proxyGatewayPreferencesChanged, this, &ConnectionWindowItem::onProxyGatewayPreferencesChangedByUser);
    proxyGatewayGroup_->setProxyGatewaySettings(preferences->shareProxyGateway());
    addItem(proxyGatewayGroup_);

    antiCensorshipGroup_ = new PreferenceGroup(this,
                                              "",
                                              QString("https://%1/features/circumvent-censorship").arg(HardcodedSettings::instance().windscribeServerUrl()));
    antiCensorshipItem_ = new ToggleItem(antiCensorshipGroup_, tr("Circumvent Censorship"));
    antiCensorshipItem_->setIcon(ImageResourcesSvg::instance().getIndependentPixmap("preferences/CIRCUMVENT_CENSORSHIP"));
    antiCensorshipItem_->setState(preferences->isAntiCensorship());
    connect(antiCensorshipItem_, &ToggleItem::stateChanged, this, &ConnectionWindowItem::onAntiCensorshipPreferencesChangedByUser);
    antiCensorshipGroup_->addItem(antiCensorshipItem_);
    addItem(antiCensorshipGroup_);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &ConnectionWindowItem::onLanguageChanged);
    onLanguageChanged();
}

QString ConnectionWindowItem::caption() const
{
    return tr("Connection");
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
    macSpoofingGroup_->setCurrentNetwork(networkInterface);
}

void ConnectionWindowItem::setPacketSizeDetectionState(bool on)
{
    packetSizeGroup_->setPacketSizeDetectionState(on);
}

void ConnectionWindowItem::showPacketSizeDetectionError(const QString &title,
                                                        const QString &message)
{
    packetSizeGroup_->showPacketSizeDetectionError(title, message);
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
    preferences_->setPacketSize(ps);
}

void ConnectionWindowItem::onMacAddrSpoofingPreferencesChangedByUser(const types::MacAddrSpoofing &mas)
{
    preferences_->setMacAddrSpoofing(mas);
}

void ConnectionWindowItem::onTerminateSocketsPreferencesChangedByUser(bool isChecked)
{
#if defined(Q_OS_WIN)
    preferences_->setTerminateSockets(isChecked);
#endif
}

void ConnectionWindowItem::onAntiCensorshipPreferencesChangedByUser(bool isChecked)
{
    preferences_->setAntiCensorship(isChecked);
}

void ConnectionWindowItem::onSplitTunnelingPreferencesChanged(const types::SplitTunneling &st)
{
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
    packetSizeGroup_->setPacketSizeSettings(ps);
}

void ConnectionWindowItem::onMacAddrSpoofingPreferencesChanged(const types::MacAddrSpoofing &mas)
{
    macSpoofingGroup_->setMacSpoofingSettings(mas);
}

void ConnectionWindowItem::onTerminateSocketsPreferencesChanged(bool b)
{
#if defined(Q_OS_WIN)
    terminateSocketsItem_->setState(b);
#endif
}

void ConnectionWindowItem::onAntiCensorshipPreferencesChanged(bool b)
{
    antiCensorshipItem_->setState(b);
}

void ConnectionWindowItem::onAllowLanTrafficButtonHoverLeave()
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_INVALID_LAN_ADDRESS);
}

void ConnectionWindowItem::onIsExternalConfigModeChanged(bool bIsExternalConfigMode)
{
    connectionModeGroup_->setVisible(!bIsExternalConfigMode);
    CommonGraphics::BaseItem *spacer = item(indexOf(connectionModeGroup_) + 1);
    spacer->setVisible(!bIsExternalConfigMode);
}

void ConnectionWindowItem::onConnectedDnsPreferencesChanged(const types::ConnectedDnsInfo &dns)
{
    connectedDnsGroup_->setConnectedDnsInfo(dns);
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
    networkOptionsItem_->setTitle(tr("Network Options"));
    splitTunnelingItem_->setTitle(tr("Split Tunneling"));
    onSplitTunnelingPreferencesChanged(preferences_->splitTunneling());
    proxySettingsItem_->setTitle(tr("Proxy Settings"));

    autoConnectGroup_->setDescription(tr("Connects to last used location when the app launches or joins a network."));
    checkBoxAutoConnect_->setCaption(tr("Auto-Connect"));
    firewallGroup_->setDescription(tr("Control the mode of behavior of the Windscribe firewall."));
    connectionModeGroup_->setTitle(tr("Connection Mode"));
    connectionModeGroup_->setDescription(tr("Automatically choose the VPN protocol, or select one manually. NOTE: \"Preferred Protocol\" will override this setting."));
    packetSizeGroup_->setDescription(tr("Automatically determine the MTU for your connection, or manually override.  This has no effect on TCP-based protocols."));
    connectedDnsGroup_->setDescription(tr("Select the DNS server while connected to Windscribe."));
    allowLanTrafficGroup_->setDescription(tr("Allow access to local services and printers while connected to Windscribe."));
    checkBoxAllowLanTraffic_->setCaption(tr("Allow LAN Traffic"));

    macSpoofingGroup_->setDescription(tr("Spoof your device's physical address (MAC address)."));
#ifdef Q_OS_MACOS
    if (MacUtils::isOsVersionAtLeast(14, 4)) {
        macSpoofingGroup_->setDescription(tr("MAC spoofing is not supported on your version of MacOS."));
    }
#endif

#if defined(Q_OS_WIN)
    terminateSocketsItem_->setCaption(tr("Terminate Sockets"));
    terminateSocketsGroup_->setDescription(tr("Close all active TCP sockets when the VPN tunnel is established."));
#endif

    //secureHotspotGroup_ sets its own description

    proxyGatewayGroup_->setDescription(tr("Configure your TV, gaming console, or other devices that support proxy servers."));
    antiCensorshipItem_->setCaption(tr("Circumvent Censorship"));
    antiCensorshipGroup_->setDescription(tr("Connect to the VPN even in hostile environment."));
}

void ConnectionWindowItem::onIsAllowLanTrafficPreferencesChangedByUser(bool b)
{
    preferences_->setAllowLanTraffic(b);

    if (preferences_->shareProxyGateway().isEnabled && !b) {
        GeneralMessageController::instance().showMessage(
            "WARNING_WHITE",
            tr("Settings Conflict"),
            tr("Disabling Allow LAN Traffic will cause your proxy gateway to stop working.  Do you want to disable the proxy?"),
            GeneralMessageController::tr(GeneralMessageController::kYes),
            GeneralMessageController::tr(GeneralMessageController::kNo),
            "",
            [this](bool b) {
                types::ShareProxyGateway sp = preferences_->shareProxyGateway();
                sp.isEnabled = false;
                preferences_->setShareProxyGateway(sp);
            },
            std::function<void(bool b)>(nullptr),
            std::function<void(bool b)>(nullptr),
            GeneralMessage::kFromPreferences);
    } else if (preferences_->shareSecureHotspot().isEnabled && !b) {
        GeneralMessageController::instance().showMessage(
            "WARNING_WHITE",
            tr("Settings Conflict"),
            tr("Disabling Allow LAN Traffic will cause your secure hotspot to stop working.  Do you want to disable the hotspot?"),
            GeneralMessageController::tr(GeneralMessageController::kYes),
            GeneralMessageController::tr(GeneralMessageController::kNo),
            "",
            [this](bool b) {
                types::ShareSecureHotspot sh = preferences_->shareSecureHotspot();
                sh.isEnabled = false;
                preferences_->setShareSecureHotspot(sh);
            },
            std::function<void(bool b)>(nullptr),
            std::function<void(bool b)>(nullptr),
            GeneralMessage::kFromPreferences);
    }
}

void ConnectionWindowItem::onConnectedDnsPreferencesChangedByUser(const types::ConnectedDnsInfo &dns)
{
    preferences_->setConnectedDnsInfo(dns);
}

void ConnectionWindowItem::hideOpenPopups()
{
    // qCDebug(LOG_PREFERENCES) << "Hiding Connection popups";

    CommonGraphics::BasePage::hideOpenPopups();
}

void ConnectionWindowItem::onSecureHotspotPreferencesChangedByUser(const types::ShareSecureHotspot &ss)
{
#if defined(Q_OS_WIN)
    if (ss.isEnabled && !preferences_->isAllowLanTraffic()) {
        GeneralMessageController::instance().showMessage(
            "WARNING_WHITE",
            tr("Settings Conflict"),
            tr("LAN traffic is currently blocked by the Windscribe firewall.  Do you want to allow LAN traffic to bypass the firewall in order for this feature to work?"),
            GeneralMessageController::tr(GeneralMessageController::kYes),
            GeneralMessageController::tr(GeneralMessageController::kNo),
            "",
            [this](bool b) { preferences_->setAllowLanTraffic(true); },
            std::function<void(bool)>(nullptr),
            std::function<void(bool)>(nullptr),
            GeneralMessage::kFromPreferences);
    }

    preferences_->setShareSecureHotspot(ss);
#endif
}

void ConnectionWindowItem::onSecureHotspotPreferencesChanged(const types::ShareSecureHotspot &ss)
{
#if defined(Q_OS_WIN)
    secureHotspotGroup_->setSecureHotspotSettings(ss);
#endif
}

void ConnectionWindowItem::onConnectionSettingsPreferencesChanged(const types::ConnectionSettings &cs)
{
    updateIsSupported(preferencesHelper_->isWifiSharingSupported(), isIkev2(cs));
}

void ConnectionWindowItem::onProxyGatewayPreferencesChangedByUser(const types::ShareProxyGateway &sp)
{
    preferences_->setShareProxyGateway(sp);

    if (sp.isEnabled && !preferences_->isAllowLanTraffic()) {
        GeneralMessageController::instance().showMessage(
            "WARNING_WHITE",
            tr("Settings Conflict"),
            tr("LAN traffic is currently blocked by the Windscribe firewall.  Do you want to allow LAN traffic to bypass the firewall in order for this feature to work?"),
            GeneralMessageController::tr(GeneralMessageController::kYes),
            GeneralMessageController::tr(GeneralMessageController::kNo),
            "",
            [this](bool b) { preferences_->setAllowLanTraffic(true); },
            std::function<void(bool)>(nullptr),
            std::function<void(bool)>(nullptr),
            GeneralMessage::kFromPreferences);
    }
}

void ConnectionWindowItem::onProxyGatewayPreferencesChanged(const types::ShareProxyGateway &sp)
{
    proxyGatewayGroup_->setProxyGatewaySettings(sp);
}

void ConnectionWindowItem::onPreferencesHelperWifiSharingSupportedChanged(bool bSupported)
{
    updateIsSupported(bSupported, isIkev2(preferences_->connectionSettings()));
}

bool ConnectionWindowItem::isIkev2(const types::ConnectionSettings &cs) const
{
    return cs.protocol() == types::Protocol::IKEV2;
}

void ConnectionWindowItem::updateIsSupported(bool isWifiSharingSupported, bool isIkev2)
{
#if defined(Q_OS_WIN)
    if (secureHotspotGroup_) {
        if (!isWifiSharingSupported) {
            secureHotspotGroup_->setSupported(SecureHotspotGroup::HOTSPOT_NOT_SUPPORTED);
        } else if (isIkev2) {
            secureHotspotGroup_->setSupported(SecureHotspotGroup::HOTSPOT_NOT_SUPPORTED_BY_IKEV2);
        } else {
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
