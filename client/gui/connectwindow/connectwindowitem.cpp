#include "connectwindowitem.h"

#include <QCursor>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QPainter>

#include "dpiscalemanager.h"
#include "languagecontroller.h"
#include "tooltips/tooltipcontroller.h"
#include "tooltips/tooltiputil.h"
#include "utils/ws_assert.h"

namespace ConnectWindow {

ConnectWindowItem::ConnectWindowItem(QGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper)
    : ScalableGraphicsObject (parent),
      preferences_(preferences),
      preferencesHelper_(preferencesHelper),
      networkName_(""),
      trustType_(NETWORK_TRUST_SECURED),
      interfaceType_(NETWORK_INTERFACE_NONE),
      networkActive_(false),
      connectionTime_(""),
      dataTransferred_(""),
      isFirewallAlwaysOn_(false)
{
    WS_ASSERT(preferencesHelper_);
    background_ = new Background(this, preferences);

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    closeButton_ = new IconButton(16, 16, "WINDOWS_CLOSE_ICON", "", this);
    connect(closeButton_, &IconButton::clicked, this, &ConnectWindowItem::closeClick);
    minimizeButton_ = new IconButton(16, 16, "WINDOWS_MINIMIZE_ICON", "", this);
    connect(minimizeButton_, &IconButton::clicked, this, &ConnectWindowItem::minimizeClick);
#else
    minimizeButton_ = new IconButton(14, 14, "MAC_MINIMIZE_DEFAULT", "", this);
    connect(minimizeButton_, &IconButton::clicked, this, &ConnectWindowItem::minimizeClick);
    connect(minimizeButton_, &IconButton::hoverEnter, [=](){ minimizeButton_->setIcon("MAC_MINIMIZE_HOVER"); });
    connect(minimizeButton_, &IconButton::hoverLeave, [=](){ minimizeButton_->setIcon("MAC_MINIMIZE_DEFAULT"); });
    minimizeButton_->setVisible(!preferencesHelper->isDockedToTray());
    minimizeButton_->setSelected(true);

    closeButton_ = new IconButton(14, 14, "MAC_CLOSE_DEFAULT", "", this);
    connect(closeButton_, &IconButton::clicked, this, &ConnectWindowItem::closeClick);
    connect(closeButton_, &IconButton::hoverEnter, [=](){ closeButton_->setIcon("MAC_CLOSE_HOVER"); });
    connect(closeButton_, &IconButton::hoverLeave, [=](){ closeButton_->setIcon("MAC_CLOSE_DEFAULT"); });
    closeButton_->setSelected(true);
#endif

    connectButton_ = new ConnectButton(this);
    connect(connectButton_, &ConnectButton::clicked, this, &ConnectWindowItem::connectClick);

    connectStateProtocolPort_ = new ConnectStateProtocolPort(this);
    connect(connectStateProtocolPort_, &ClickableGraphicsObject::hoverEnter, this, &ConnectWindowItem::onConnectStateTextHoverEnter);
    connect(connectStateProtocolPort_, &ClickableGraphicsObject::hoverLeave, this, &ConnectWindowItem::onConnectStateTextHoverLeave);
    connect(connectStateProtocolPort_, &ClickableGraphicsObject::clicked, this, &ConnectWindowItem::onProtocolsClick);
    connect(preferences_, &Preferences::isAntiCensorshipChanged, connectStateProtocolPort_, &ConnectStateProtocolPort::antiCensorshipChanged);

    cityName1Text_ = new CommonGraphics::TextButton("", FontDescr(21, QFont::DemiBold), Qt::white, true, this, 0, true);
    cityName1Text_->setUnhoverOpacity(OPACITY_FULL);
    cityName1Text_->setCurrentOpacity(OPACITY_FULL);
    cityName1Text_->setClickableHoverable(false, true);
    connect(cityName1Text_, &CommonGraphics::TextButton::hoverEnter, this, &ConnectWindowItem::onFirstNameHoverEnter);
    connect(cityName1Text_, &CommonGraphics::TextButton::hoverLeave, this, &ConnectWindowItem::onFirstOrSecondNameHoverLeave);

    cityName2Text_ = new CommonGraphics::TextButton("", FontDescr(21, QFont::Light), Qt::white, true, this, 0, true);
    cityName2Text_->setUnhoverOpacity(OPACITY_FULL);
    cityName2Text_->setCurrentOpacity(OPACITY_FULL);
    cityName2Text_->setClickableHoverable(false, true);
    connect(cityName2Text_, &CommonGraphics::TextButton::hoverEnter, this, &ConnectWindowItem::onSecondNameHoverEnter);
    connect(cityName2Text_, &CommonGraphics::TextButton::hoverLeave, this, &ConnectWindowItem::onFirstOrSecondNameHoverLeave);

    preferencesButton_ = new IconButton(20, 24, "MENU_ICON", "", this, 0.5);
    connect(preferencesButton_, &IconButton::clicked, this, &ConnectWindowItem::preferencesClick);

    locationsButton_ = new LocationsButton(this);
    locationsButton_->setCursor(Qt::PointingHandCursor);
    connect(locationsButton_, &LocationsButton::clicked, this, &ConnectWindowItem::locationsClick);
    connect(locationsButton_, &LocationsButton::locationTabClicked, this, &ConnectWindowItem::locationTabClicked);
    connect(locationsButton_, &LocationsButton::searchFilterChanged, this, &ConnectWindowItem::searchFilterChanged);
    connect(locationsButton_, &LocationsButton::locationsKeyPressed, this, &ConnectWindowItem::locationsKeyPressed);

    networkTrustButton_ = new IconButton(28, 22, "network/WIFI_UNSECURED", "network/WIFI_UNSECURED_SHADOW", this, OPACITY_FULL);
    networkTrustButton_->setOpacity(.2);
    connect(networkTrustButton_, &IconButton::clicked, this, &ConnectWindowItem::networkButtonClick);
    connect(networkTrustButton_, &IconButton::hoverEnter, this, &ConnectWindowItem::onNetworkHoverEnter);
    connect(networkTrustButton_, &IconButton::hoverLeave, this, &ConnectWindowItem::onNetworkHoverLeave);

    networkNameText_ = new CommonGraphics::TextButton("", FontDescr(15, QFont::Normal), Qt::white, false, this, 0, false);
    networkNameText_->setUnhoverOpacity(0.7);
    networkNameText_->setCurrentOpacity(0.7);

    ipAddressItem_ = new IPAddressItem(this);
    connect(ipAddressItem_, &IPAddressItem::widthChanged, this, &ConnectWindowItem::onIpAddressWidthChanged);

    // dotMenuButton_ = new IconButton(24, 24, "DOT_MENU", "", this);
    // dotMenuButton_->setUnhoverOpacity(0.6);

    firewallLabel_ = new CommonGraphics::TextButton(tr("FIREWALL"), FontDescr(12, QFont::Medium, 100, 2), Qt::white, false, this, 0, false);
    firewallLabel_->setUnhoverOpacity(0.6);
    firewallLabel_->setCurrentOpacity(0.6);

    firewallButton_ = new ToggleButton(this);
    // firewallButton_->setColor(QColor(0, 106, 255));
    connect(firewallButton_, &ToggleButton::hoverLeave, this, &ConnectWindowItem::onFirewallButtonHoverLeave);
    connect(firewallButton_, &ToggleButton::toggleIgnored, this, &ConnectWindowItem::onFirewallButtonToggleIgnored);
    connect(firewallButton_, &ToggleButton::stateChanged, this, &ConnectWindowItem::onFirewallButtonStateChanged);

    firewallInfo_ = new IconButton(16, 16, "INFO_ICON", "", this, OPACITY_QUARTER, OPACITY_QUARTER);
    firewallInfo_->setClickableHoverable(false, true);
    connect(firewallInfo_, &IconButton::hoverEnter, this, &ConnectWindowItem::onFirewallInfoHoverEnter);
    connect(firewallInfo_, &IconButton::hoverLeave, this, &ConnectWindowItem::onFirewallInfoHoverLeave);

    logoButton_ = new LogoNotificationsButton(this);
    connect(logoButton_, &LogoNotificationsButton::clicked, this, &ConnectWindowItem::notificationsClick);

    connect(preferencesHelper, &PreferencesHelper::isDockedModeChanged, this, &ConnectWindowItem::onDockedModeChanged);
    connect(preferences, &Preferences::appSkinChanged, this, &ConnectWindowItem::onAppSkinChanged);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &ConnectWindowItem::onLanguageChanged);

    updatePositions();
}

QRectF ConnectWindowItem::boundingRect() const
{
    return background_->boundingRect();
}

void ConnectWindowItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // painter->fillRect(boundingRect(), Qt::blue);
}

void ConnectWindowItem::setClickable(bool isClickable)
{
    minimizeButton_->setClickable(isClickable);
    closeButton_->setClickable(isClickable);

    connectButton_->setClickable(isClickable);
    preferencesButton_->setClickable(isClickable);
    firewallButton_->setClickable(isClickable);
    locationsButton_->setClickable(isClickable);
    networkTrustButton_->setClickable(isClickable);
    logoButton_->setClickable(isClickable);

    firewallInfo_->setClickableHoverable(false, isClickable);
    connectStateProtocolPort_->setHoverable(isClickable);
    connectStateProtocolPort_->setClickable(isClickable);
}

QRegion ConnectWindowItem::getMask()
{
    QPolygon polygon;

    int windowWidth = WINDOW_WIDTH * G_SCALE;
    int xOffset = (WINDOW_WIDTH - 250) * G_SCALE;

    polygon << QPoint(0, 0)
            << QPoint(windowWidth, 0)
            << QPoint(windowWidth, (WINDOW_HEIGHT - 16) * G_SCALE)
            << QPoint(xOffset,     (WINDOW_HEIGHT - 16) * G_SCALE)
            << QPoint(xOffset,     (WINDOW_HEIGHT     ) * G_SCALE)
            << QPoint(0,           (WINDOW_HEIGHT     ) * G_SCALE);
    return QRegion(polygon);
}

QPixmap ConnectWindowItem::getShadowPixmap()
{
    return background_->getShadowPixmap();
}

void ConnectWindowItem::setConnectionTimeAndData(QString connectionTime, QString dataTransferred)
{
    connectionTime_ = connectionTime;
    dataTransferred_ = dataTransferred;
}

void ConnectWindowItem::setFirewallAlwaysOn(bool isFirewallAlwaysOn)
{
    isFirewallAlwaysOn_ = isFirewallAlwaysOn;
    firewallButton_->setToggleable(!isFirewallAlwaysOn_);
}

void ConnectWindowItem::setTestTunnelResult(bool success)
{
    connectStateProtocolPort_->setTestTunnelResult(success);
}

void ConnectWindowItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    updateShortenedText();
    updatePositions();
}

void ConnectWindowItem::updateLocationInfo(const QString &firstName, const QString &secondName, const QString &countryCode, PingTime pingTime, bool isCustomConfig)
{
    if (fullFirstName_ != firstName || fullSecondName_ != secondName)
    {
        fullFirstName_ = firstName;
        fullSecondName_ = secondName;
        updateShortenedText();
    }
    background_->onLocationSelected(countryCode);

    // if custom but button is visible, make it invisible
    // if not custom but button is invisible, make it visible
    connectStateProtocolPort_->setProtocolButtonVisible(!isCustomConfig);

    // If the user is in custom config mode, ensure the protocol line is not clickable to prevent display of the protocol selection screen.
    connectStateProtocolPort_->setClickableHoverable(!isCustomConfig, true);
}

void ConnectWindowItem::updateConnectState(const types::ConnectState &newConnectState)
{
    if (newConnectState != prevConnectState_)
    {
        firewallButton_->setEnabled(newConnectState.connectState != CONNECT_STATE_CONNECTING && newConnectState.connectState != CONNECT_STATE_DISCONNECTING);

        background_->onConnectStateChanged(newConnectState.connectState, prevConnectState_.connectState);
        connectButton_->onConnectStateChanged(newConnectState.connectState, prevConnectState_.connectState);
        connectStateProtocolPort_->onConnectStateChanged(newConnectState, prevConnectState_);

        prevConnectState_ = newConnectState;
        updateFirewallInfo();
    }
}

void ConnectWindowItem::updateFirewallState(bool isFirewallEnabled)
{
    firewallButton_->setState(isFirewallEnabled);
    updateFirewallInfo();

}

void ConnectWindowItem::updateFirewallInfo()
{
    if (prevConnectState_.connectState == CONNECT_STATE_DISCONNECTED && firewallButton_->isChecked()) {
        firewallInfo_->setTintColor(QColor(254, 188, 46));
        firewallInfo_->setUnhoverOpacity(OPACITY_FULL);
        firewallInfo_->setHoverOpacity(OPACITY_FULL);
        firewallInfo_->unhover();
    } else {
        firewallInfo_->setTintColor(Qt::white);
        firewallInfo_->setUnhoverOpacity(OPACITY_UNHOVER_ICON_STANDALONE);
        firewallInfo_->setHoverOpacity(OPACITY_FULL);
        firewallInfo_->unhover();
    }
}

void ConnectWindowItem::updateLocationsState(bool isExpanded)
{
    locationsButton_->onLocationsExpandStateChanged(isExpanded);
}

void ConnectWindowItem::updateMyIp(const QString &ip)
{
    ipAddressItem_->setIpAddress(ip);
    updatePositions();
}

void ConnectWindowItem::updateNotificationsState(int totalMessages, int unread)
{
    logoButton_->setCountState(totalMessages, unread);
}

void ConnectWindowItem::updateNetworkState(types::NetworkInterface network)
{
    NETWORK_TRUST_TYPE trusted = network.trustType;
    NETWORK_INTERFACE_TYPE type = network.interfaceType;
    bool networkActive = network.active;

    if (trustType_ != trusted || interfaceType_ != type || networkActive_ != networkActive) {
        QString icon = "";
        if (type == NETWORK_INTERFACE_WIFI || type == NETWORK_INTERFACE_MOBILE_BROADBAND) {
            icon += "network/WIFI_";
        } else if (type == NETWORK_INTERFACE_ETH) {
            icon += "network/ETHERNET_";
        }

        if (networkActive) {
            networkTrustButton_->setOpacity(1);
        } else {
            networkTrustButton_->setOpacity(.2);
        }

        if (icon != "") {
            if (trusted == NETWORK_TRUST_UNSECURED) {
                icon += "UNSECURED";
            } else {
                icon += "SECURED";
            }

            networkTrustButton_->setIcon(icon, icon + "_SHADOW");
        }
    }

    networkName_ = network.friendlyName;
    networkNameText_->setText(networkName_);
    trustType_ = trusted;
    interfaceType_ = type;
    networkActive_ = networkActive;

}

void ConnectWindowItem::setSplitTunnelingState(bool on)
{
    connectButton_->setSplitRouting(on);
}

void ConnectWindowItem::setInternetConnectivity(bool connectivity)
{
    connectStateProtocolPort_->setInternetConnectivity(connectivity);
    connectButton_->setInternetConnectivity(connectivity);
}

void ConnectWindowItem::setProtocolPort(const types::Protocol &protocol, const uint port)
{
    connectStateProtocolPort_->setProtocolPort(protocol, port);
    preferencesHelper_->setCurrentProtocol(protocol);
}

void ConnectWindowItem::onNetworkHoverEnter()
{
    QString title = "";
    if (networkName_ == "") {
        title = tr("No Network Info");
    } else {
        title = networkName_ + ": ";

        if (trustType_) {
            title += tr("Unsecured");
        } else {
            title += tr("Secured");
        }
    }

    QGraphicsView *view = scene()->views().first();
    QPoint globalPt = view->mapToGlobal(view->mapFromScene(networkTrustButton_->scenePos()));

    int posX = globalPt.x() + 12 * G_SCALE;
    int posY = globalPt.y() - 5 * G_SCALE;

    TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_NETWORK_INFO);
    ti.x = posX;
    ti.y = posY;
    ti.tailtype = TOOLTIP_TAIL_BOTTOM;
    ti.tailPosPercent = 0.5;
    ti.title = title;
    TooltipController::instance().showTooltipBasic(ti);
}

void ConnectWindowItem::onNetworkHoverLeave()
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_NETWORK_INFO);
}

void ConnectWindowItem::onConnectStateTextHoverEnter()
{
    if (prevConnectState_.connectState == CONNECT_STATE_CONNECTED || prevConnectState_.connectState == CONNECT_STATE_DISCONNECTED) {
        if (connectionTime_ != "") { // only show if connection has been started at least once
            QGraphicsView *view = scene()->views().first();
            QPoint globalPt = view->mapToGlobal(view->mapFromScene(connectStateProtocolPort_->scenePos()));

            int posX = globalPt.x() + 16 * G_SCALE;
            int posY = globalPt.y() - 3 * G_SCALE;

            if (prevConnectState_.connectState == CONNECT_STATE_DISCONNECTED) {
                QString text = tr("Connection to Windscribe has been terminated. ") +
                        dataTransferred_ + tr(" transferred in ") +
                        connectionTime_;

                TooltipInfo ti(TOOLTIP_TYPE_DESCRIPTIVE, TOOLTIP_ID_CONNECTION_INFO);
                ti.x = posX;
                ti.y = posY;
                ti.tailtype = TOOLTIP_TAIL_BOTTOM;
                ti.tailPosPercent = 0.05;
                ti.title = "";
                ti.desc = text;
                ti.width = 275 * G_SCALE;
                TooltipController::instance().showTooltipDescriptive(ti);
            } else { // CONNECTED
                QString text = tr("Connected for ") +
                        connectionTime_ + ", " + dataTransferred_ + tr(" transferred");

                TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_CONNECTION_INFO);
                ti.x = posX;
                ti.y = posY;
                ti.tailtype = TOOLTIP_TAIL_BOTTOM;
                ti.tailPosPercent = 0.05;
                ti.title = text;
                TooltipController::instance().showTooltipBasic(ti);
            }
        }
    }
}

void ConnectWindowItem::onConnectStateTextHoverLeave()
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_CONNECTION_INFO);
}

void ConnectWindowItem::onFirewallButtonHoverLeave()
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_FIREWALL_BLOCKED);
}

void ConnectWindowItem::onFirewallButtonStateChanged(bool isFirewallEnabled)
{
    updateFirewallInfo();
    if (!isFirewallAlwaysOn_) {
        emit firewallClick();
    }
}

void ConnectWindowItem::onFirewallButtonToggleIgnored()
{
    if (!isFirewallAlwaysOn_) {
        return;
    }

    QGraphicsView *view = scene()->views().first();
    QPoint globalPt = view->mapToGlobal(view->mapFromScene(firewallButton_->scenePos()));

    TooltipInfo ti(TOOLTIP_TYPE_DESCRIPTIVE, TOOLTIP_ID_FIREWALL_BLOCKED);
    ti.tailtype = TOOLTIP_TAIL_BOTTOM;
    ti.tailPosPercent = 0.2;
    ti.x = globalPt.x() + firewallButton_->boundingRect().width() / 2;
    ti.y = globalPt.y() - 4 * G_SCALE;
    ti.width = 200 * G_SCALE;
    ti.delay = 100;
    TooltipUtil::getFirewallAlwaysOnTooltipInfo(&ti.title, &ti.desc);
    TooltipController::instance().showTooltipDescriptive(ti);
}

void ConnectWindowItem::onFirewallInfoHoverEnter()
{
    QGraphicsView *view = scene()->views().first();
    QPoint globalPt = view->mapToGlobal(view->mapFromScene(firewallInfo_->scenePos()));

    int width = 200 * G_SCALE;
    int posX = globalPt.x() + 8 * G_SCALE;
    int posY = globalPt.y() - 4  * G_SCALE ;

    TooltipInfo ti(TOOLTIP_TYPE_DESCRIPTIVE, TOOLTIP_ID_FIREWALL_INFO);
    ti.x = posX;
    ti.y = posY;
    ti.tailtype = TOOLTIP_TAIL_BOTTOM;
    ti.tailPosPercent = 0.1;
    ti.title = tr("Firewall");
    if (prevConnectState_.connectState == CONNECT_STATE_DISCONNECTED && firewallButton_->isChecked()) {
        ti.desc = tr("Keeping the firewall on while disconnected may break internet connectivity");
    } else {
        ti.desc = tr("Blocks all connectivity in the event of a sudden disconnect");
    }
    ti.width = width;

    TooltipController::instance().showTooltipDescriptive(ti);
}

void ConnectWindowItem::onFirewallInfoHoverLeave()
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_FIREWALL_INFO);
}

void ConnectWindowItem::onDockedModeChanged(bool bIsDockedToTray)
{
#if defined(Q_OS_MACOS)
    minimizeButton_->setVisible(!bIsDockedToTray);
#else
    Q_UNUSED(bIsDockedToTray);
#endif
}

void ConnectWindowItem::onFirstNameHoverEnter()
{
    if (fullFirstName_ != cityName1Text_->text())
    {
        QGraphicsView *view = scene()->views().first();
        QPoint globalPt = view->mapToGlobal(view->mapFromScene(cityName1Text_->scenePos()));

        int posX = globalPt.x() + 10 * G_SCALE;
        int posY = globalPt.y() + 2 * G_SCALE;

        TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_FULL_SERVER_NAME);
        ti.x = posX;
        ti.y = posY;
        ti.tailtype = TOOLTIP_TAIL_BOTTOM;
        ti.tailPosPercent = 0.1;
        ti.title = fullFirstName_;
        TooltipController::instance().showTooltipBasic(ti);
    }
}

void ConnectWindowItem::onSecondNameHoverEnter()
{
    if (fullSecondName_ != cityName2Text_->text()) {
        QGraphicsView *view = scene()->views().first();
        QPoint globalPt = view->mapToGlobal(view->mapFromScene(cityName2Text_->scenePos()));

        int posX = globalPt.x() + 10 * G_SCALE;
        int posY = globalPt.y() + 2 * G_SCALE;

        TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_FULL_SERVER_NAME);
        ti.x = posX;
        ti.y = posY;
        ti.tailtype = TOOLTIP_TAIL_BOTTOM;
        ti.tailPosPercent = 0.1;
        ti.title = fullSecondName_;
        TooltipController::instance().showTooltipBasic(ti);
    }
}

void ConnectWindowItem::onFirstOrSecondNameHoverLeave()
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_FULL_SERVER_NAME);
}

void ConnectWindowItem::updatePositions()
{
    cityName1Text_->recalcBoundingRect();
    cityName2Text_->recalcBoundingRect();

    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH) {
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
        int closePosY = WINDOW_WIDTH*G_SCALE - closeButton_->boundingRect().width() - WINDOW_MARGIN*G_SCALE;
        closeButton_->setPos(closePosY, 10*G_SCALE);
        minimizeButton_->setPos(closePosY - minimizeButton_->boundingRect().width() - 20*G_SCALE, 10*G_SCALE);
#else
        int rightMostPosX = WINDOW_WIDTH*G_SCALE - closeButton_->boundingRect().width() - 8*G_SCALE;
        int middlePosX = rightMostPosX - closeButton_->boundingRect().width() - 8*G_SCALE;
        closeButton_->setPos(rightMostPosX, 8*G_SCALE);
        minimizeButton_->setPos(middlePosX, 8*G_SCALE);
#endif
        preferencesButton_->setPos(22*G_SCALE, 16*G_SCALE);
        logoButton_->setPos(54*G_SCALE, 10*G_SCALE);
#if defined(Q_OS_MACOS)
        connectButton_->setPos(254*G_SCALE, 26*G_SCALE);
        cityName1Text_->setPos(16*G_SCALE, 101*G_SCALE);
        cityName2Text_->setPos(24*G_SCALE + cityName1Text_->boundingRect().width(), 101*G_SCALE);
#else
        // Shift down slightly on Windows/Linux since the minimize/close buttons take up more space, and the
        // connect button is too close to them.
        connectButton_->setPos(254*G_SCALE, 30*G_SCALE);
        cityName1Text_->setPos(16*G_SCALE, 105*G_SCALE);
        cityName2Text_->setPos(24*G_SCALE + cityName1Text_->boundingRect().width(), 105*G_SCALE);
#endif
        connectStateProtocolPort_->setPos(13*G_SCALE, 74*G_SCALE);
        locationsButton_->setPos(97*G_SCALE, 183*G_SCALE);
        networkTrustButton_->setPos(16*G_SCALE, 147*G_SCALE);
        networkNameText_->setPos(48*G_SCALE, 142*G_SCALE);
        // If FreshScribe API available, IP should shift 32*G_SCALE to the left and dotMenuButton_ should be visible
        ipAddressItem_->setPos(boundingRect().width() - ipAddressItem_->boundingRect().width() - 16*G_SCALE, 146*G_SCALE);
        // dotMenuButton_->setPos(boundingRect().width() - 40*G_SCALE, 144*G_SCALE);
        firewallLabel_->setPos(14*G_SCALE, 177*G_SCALE);
        firewallButton_->setPos(16*G_SCALE, 202*G_SCALE);
        firewallInfo_->setPos(69*G_SCALE, 205*G_SCALE);
    } else {
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
        int closePosY = WINDOW_WIDTH*G_SCALE - closeButton_->boundingRect().width() - WINDOW_MARGIN*G_SCALE;
        closeButton_->setPos(closePosY, 10*G_SCALE);
        minimizeButton_->setPos(closePosY - minimizeButton_->boundingRect().width() - 20*G_SCALE, 10*G_SCALE);
#else
        int rightMostPosX = static_cast<int>(boundingRect().width()) - static_cast<int>(closeButton_->boundingRect().width()) - 8*G_SCALE;
        int middlePosX = rightMostPosX - static_cast<int>(closeButton_->boundingRect().width()) - 8*G_SCALE;
        closeButton_->setPos(rightMostPosX, 8*G_SCALE);
        minimizeButton_->setPos(middlePosX, 8*G_SCALE);
#endif

        preferencesButton_->setPos(22*G_SCALE, 33*G_SCALE);
        logoButton_->setPos(54*G_SCALE, 27*G_SCALE);
        connectButton_->setPos(254*G_SCALE, 41*G_SCALE);
        connectStateProtocolPort_->setPos(13*G_SCALE, 90*G_SCALE);
        cityName1Text_->setPos(16*G_SCALE, 118*G_SCALE);
        cityName2Text_->setPos(24*G_SCALE + cityName1Text_->boundingRect().width(), 118*G_SCALE);
        locationsButton_->setPos(98*G_SCALE, 198*G_SCALE);
        networkTrustButton_->setPos(16*G_SCALE, 162*G_SCALE);
        networkNameText_->setPos(48*G_SCALE, 158*G_SCALE);
        // If FreshScribe API available, IP should shift 32*G_SCALE to the left and dotMenuButton_ should be visible
        ipAddressItem_->setPos(boundingRect().width() - ipAddressItem_->boundingRect().width() - 16*G_SCALE, 161*G_SCALE);
        // dotMenuButton_->setPos(boundingRect().width() - 40*G_SCALE, 159*G_SCALE);
        firewallLabel_->setPos(14*G_SCALE, 210*G_SCALE);
        firewallButton_->setPos(16*G_SCALE, 235*G_SCALE);
        firewallInfo_->setPos(69*G_SCALE, 238*G_SCALE);
    }
}

void ConnectWindowItem::updateShortenedText()
{
    // Network name
    QFont networkFont = networkNameText_->getFont();
    QFontMetrics fmNetwork(networkFont);
    if (fmNetwork.horizontalAdvance(networkName_) > boundingRect().width()/2 - 48*G_SCALE) {
        networkNameText_->setText(fmNetwork.elidedText(networkName_, Qt::ElideMiddle, boundingRect().width()/2 - 48*G_SCALE));
    }

    // City name
    QString shortenedFirstName;
    QString shortenedSecondName;

    QFont font1 = cityName1Text_->getFont();
    QFontMetrics fm1(font1);
    QFont font2 = cityName2Text_->getFont();
    QFontMetrics fm2(font2);

    // 16px on either end and 8px between the two names
    int availableWidth = boundingRect().width() - 40*G_SCALE;

    // Everything fits, no need to elide
    if (fm1.horizontalAdvance(fullFirstName_) + fm2.horizontalAdvance(fullSecondName_) <= availableWidth) {
        cityName1Text_->setText(fullFirstName_);
        cityName2Text_->setText(fullSecondName_);
        updatePositions();
        return;
    }

    // If both names are >50% of the available width, elide them both to 50%
    if (fm1.horizontalAdvance(fullFirstName_) >= availableWidth/2 &&
        fm2.horizontalAdvance(fullSecondName_) >= availableWidth/2)
    {
        shortenedFirstName = fm1.elidedText(fullFirstName_, Qt::ElideMiddle, availableWidth/2);
        shortenedSecondName = fm2.elidedText(fullSecondName_, Qt::ElideMiddle, availableWidth/2);
        cityName1Text_->setText(shortenedFirstName);
        cityName2Text_->setText(shortenedSecondName);
        updatePositions();
        return;
    } else {
        // Elide the longer name
        if (fm1.horizontalAdvance(fullFirstName_) >= fm2.horizontalAdvance(fullSecondName_)) {
            shortenedFirstName = fm1.elidedText(fullFirstName_, Qt::ElideMiddle, availableWidth - fm2.horizontalAdvance(fullSecondName_));
            cityName1Text_->setText(shortenedFirstName);
            cityName2Text_->setText(fullSecondName_);
        } else {
            shortenedSecondName = fm2.elidedText(fullSecondName_, Qt::ElideMiddle, availableWidth - fm1.horizontalAdvance(fullFirstName_));
            cityName1Text_->setText(fullFirstName_);
            cityName2Text_->setText(shortenedSecondName);
        }
    }
    updatePositions();
}

void ConnectWindowItem::onAppSkinChanged(APP_SKIN s)
{
    Q_UNUSED(s);
    updatePositions();
}

types::ProtocolStatus ConnectWindowItem::getProtocolStatus()
{
    return connectStateProtocolPort_->getProtocolStatus();
}

void ConnectWindowItem::onProtocolsClick()
{
    if (connectStateProtocolPort_->isProtocolButtonVisible()) {
        emit protocolsClick();
    }
}

void ConnectWindowItem::setIsPreferredProtocol(bool on)
{
    connectStateProtocolPort_->setIsPreferredProtocol(on);
}

void ConnectWindowItem::onLanguageChanged()
{
    firewallLabel_->setText(tr("FIREWALL"));
    updatePositions();
}

void ConnectWindowItem::setIsPremium(bool isPremium)
{
    logoButton_->setIsPremium(isPremium);
}

void ConnectWindowItem::onIpAddressWidthChanged(int width)
{
    updatePositions();
}

void ConnectWindowItem::setCustomConfigMode(bool isCustomConfig)
{
    // TBD: change locations tab etc
}

void ConnectWindowItem::onLocationsCollapsed()
{
    // TBD: change locations tab etc
}

bool ConnectWindowItem::handleKeyPressEvent(QKeyEvent *event)
{
    return locationsButton_->handleKeyPressEvent(event);
}

} //namespace ConnectWindow
