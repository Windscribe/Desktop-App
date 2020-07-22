#include "connectwindowitem.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QCursor>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <google/protobuf/util/message_differencer.h>
#include "GraphicResources/fontmanager.h"
#include "Utils/logger.h"
#include "dpiscalemanager.h"


namespace ConnectWindow {

ConnectWindowItem::ConnectWindowItem(QGraphicsObject  *parent) : ScalableGraphicsObject (parent)
  , networkName_("")
  , trustType_(ProtoTypes::NETWORK_SECURED)
  , interfaceType_(ProtoTypes::NETWORK_INTERFACE_NONE)
  , networkActive_(false)
  , connectionTime_("")
  , dataTransferred_("")
  , isFirewallBlocked_(false)
{
    background_ = new Background(this);

#ifdef Q_OS_WIN
    closeButton_ = new IconButton(10, 10, "WINDOWS_CLOSE_ICON", this);
    connect(closeButton_, SIGNAL(clicked()), SIGNAL(closeClick()));
    minimizeButton_ = new IconButton(10, 10, "WINDOWS_MINIMIZE_ICON", this);
    connect(minimizeButton_, SIGNAL(clicked()), SIGNAL(minimizeClick()));
#else

    //fullScreenButton_ = new IconButton(14,14,"MAC_EMPTY_DEFAULT", this);

    //fullScreenButton_->setSelected(true);
    //fullScreenButton_->setClickable(false);
    minimizeButton_ = new IconButton(14,14,"MAC_MINIMIZE_DEFAULT", this);
    connect(minimizeButton_, SIGNAL(clicked()), SIGNAL(minimizeClick()));
    connect(minimizeButton_, &IconButton::hoverEnter, [=](){ minimizeButton_->setIcon("MAC_MINIMIZE_HOVER"); });
    connect(minimizeButton_, &IconButton::hoverLeave, [=](){ minimizeButton_->setIcon("MAC_MINIMIZE_DEFAULT"); });
    minimizeButton_->setSelected(true);

    closeButton_ = new IconButton(14,14, "MAC_CLOSE_DEFAULT", this);
    connect(closeButton_, SIGNAL(clicked()), SIGNAL(closeClick()));
    connect(closeButton_, &IconButton::hoverEnter, [=](){ closeButton_->setIcon("MAC_CLOSE_HOVER"); });
    connect(closeButton_, &IconButton::hoverLeave, [=](){ closeButton_->setIcon("MAC_CLOSE_DEFAULT"); });
    closeButton_->setSelected(true);

#endif

    connectButton_ = new ConnectButton(this);
    connect(connectButton_, SIGNAL(clicked()), SIGNAL(connectClick()));

    connectStateProtocolPort_ = new ConnectStateProtocolPort(this);
    connect(connectStateProtocolPort_, SIGNAL(hoverEnter()), SLOT(onConnectStateTextHoverEnter()));
    connect(connectStateProtocolPort_, SIGNAL(hoverLeave()), SLOT(onConnectStateTextHoverLeave()));

    cityName1Text_ = new QGraphicsSimpleTextItem(this);
    cityName1Text_->setPen(Qt::NoPen);
    cityName1Text_->setBrush(QColor(255, 255, 255));

    cityName2Text_ = new CommonGraphics::TextButton("", FontDescr(16, false), Qt::white, true, this);
    cityName2Text_->setUnhoverOpacity(OPACITY_FULL);
    cityName2Text_->setCurrentOpacity(OPACITY_FULL);
    cityName2Text_->setClickableHoverable(false, true);
    connect(cityName2Text_, SIGNAL(hoverEnter()), this, SLOT(onSecondNameHoverEnter()));
    connect(cityName2Text_, SIGNAL(hoverLeave()), this, SLOT(onSecondNameHoverLeave()));

    preferencesButton_ = new IconButton(20, 24, "MENU_ICON", this, 0.5);
    connect(preferencesButton_, SIGNAL(clicked()), SIGNAL(preferencesClick()));

    locationsButton_ = new LocationsButton(this);
    locationsButton_->setCursor(Qt::PointingHandCursor);
    connect(locationsButton_, SIGNAL(clicked()), SIGNAL(locationsClick()));

    serverRatingIndicator_ = new ServerRatingIndicator(this);
    serverRatingIndicator_->setClickableHoverable(false, true);
    connect(serverRatingIndicator_, SIGNAL(hoverEnter()), SLOT(onServerRatingIndicatorHoverEnter()));
    connect(serverRatingIndicator_, SIGNAL(hoverLeave()), SLOT(onServerRatingIndicatorHoverLeave()));

    networkTrustButton_ = new IconButton(28, 22, "NETWORK_WIFI_UNSECURED", this, OPACITY_FULL);
    networkTrustButton_->setOpacity(.2);
    connect(networkTrustButton_, SIGNAL(clicked()), SIGNAL(networkButtonClick()));
    connect(networkTrustButton_, SIGNAL(hoverEnter()), SLOT(onNetworkHoverEnter()));
    connect(networkTrustButton_, SIGNAL(hoverLeave()), SLOT(onNetworkHoverLeave()));

    middleItem_ = new MiddleItem(this, "N/A");

    firewallButton_ = new FirewallButton(this);
    connect(firewallButton_, SIGNAL(clicked()), SIGNAL(firewallClick()));

    firewallInfo_ = new IconButton(16, 16, "INFO_ICON", this, 0.25, 0.25);
    firewallInfo_->setClickableHoverable(false, true);
    connect(firewallInfo_, SIGNAL(hoverEnter()), SLOT(onFirewallInfoHoverEnter()));
    connect(firewallInfo_, SIGNAL(hoverLeave()), SLOT(onFirewallInfoHoverLeave()));

    logoButton_ = new LogoNotificationsButton(this);
    connect(logoButton_, SIGNAL(clicked()), this, SIGNAL(notificationsClick()));

    QFont descFont = *FontManager::instance().getFont(11, false);

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

QGraphicsObject *ConnectWindowItem::getGraphicsObject()
{
    return this;
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
    serverRatingIndicator_->setClickableHoverable(false, isClickable);
    connectStateProtocolPort_->setHoverable(isClickable);
}

QRegion ConnectWindowItem::getMask()
{
    QPolygon polygon;

    int windowWidth = WINDOW_WIDTH * G_SCALE;
    int xOffset = (WINDOW_WIDTH - 235) * G_SCALE;
    int xOffset2 = (WINDOW_WIDTH - 235 - 20) * G_SCALE;

    polygon << QPoint(0, 0)
            << QPoint(windowWidth, 0)
            << QPoint(windowWidth, (WINDOW_HEIGHT - 25) * G_SCALE)
            << QPoint(xOffset,     (WINDOW_HEIGHT - 25) * G_SCALE)
            << QPoint(xOffset2,    (WINDOW_HEIGHT     ) * G_SCALE)
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
    firewallButton_->setFirewallAlwaysOn(isFirewallAlwaysOn);
}

void ConnectWindowItem::setFirewallBlock(bool isFirewallBlocked)
{
    isFirewallBlocked_ = isFirewallBlocked;
    firewallButton_->setDisabled(isFirewallBlocked_);
}

void ConnectWindowItem::setTestTunnelResult(bool success)
{
    connectStateProtocolPort_->setTestTunnelResult(success);
}

void ConnectWindowItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    updatePositions();
}

void ConnectWindowItem::updateLocationInfo(LocationID id, const QString &firstName, const QString &secondName, const QString &countryCode, PingTime pingTime, bool isFavorite)
{
    locationID_ = id;

    QString shortenedSecondName = CommonGraphics::truncateText(secondName, *FontManager::instance().getFont(16, false), 175);
    fullSecondName_ = secondName;

    cityName1Text_->setText(firstName);
    cityName2Text_->setText(shortenedSecondName);
    background_->onLocationSelected(countryCode);
    serverRatingIndicator_->setPingTime(pingTime);

    updateFavoriteState(id, isFavorite);
}

void ConnectWindowItem::updateLocationSpeed(LocationID id, PingTime speed)
{
    if (locationID_ == id)
        serverRatingIndicator_->setPingTime(speed);
}

void ConnectWindowItem::updateConnectState(const ProtoTypes::ConnectState &newConnectState)
{
    if (!google::protobuf::util::MessageDifferencer::Equals(newConnectState, prevConnectState_))
    {
        middleItem_->setIsSecured(newConnectState.connect_state_type() == ProtoTypes::CONNECTED);

        firewallButton_->setDisabled(isFirewallBlocked_ || (newConnectState.connect_state_type() == ProtoTypes::CONNECTING || newConnectState.connect_state_type() == ProtoTypes::DISCONNECTING));

        background_->onConnectStateChanged(newConnectState.connect_state_type(), prevConnectState_.connect_state_type());
        connectButton_->onConnectStateChanged(newConnectState.connect_state_type(), prevConnectState_.connect_state_type());
        serverRatingIndicator_->onConnectStateChanged(newConnectState.connect_state_type(), prevConnectState_.connect_state_type());
        connectStateProtocolPort_->onConnectStateChanged(newConnectState, prevConnectState_);

        prevConnectState_ = newConnectState;
    }
}

void ConnectWindowItem::updateFirewallState(bool isFirewallEnabled)
{
    firewallButton_->setFirewallState(isFirewallEnabled);
}

void ConnectWindowItem::updateLocationsState(bool isExpanded)
{
    locationsButton_->onLocationsExpandStateChanged(isExpanded);
}

void ConnectWindowItem::updateFavoriteState(LocationID id, bool isFavorite)
{
    if (id == locationID_)
    {
        favorite_ = isFavorite;
        if (isFavorite)
        {
            //favoriteButton_->setIcon("FAV_SELECTED");
        }
        else
        {
            //favoriteButton_->setIcon("FAV_DESELECTED");
        }
    }
}


void ConnectWindowItem::updateMyIp(const QString &ip)
{
    middleItem_->setIpAddress(ip);
}

void ConnectWindowItem::updateNotificationsState(int totalMessages, int unread)
{
    logoButton_->setCountState(totalMessages, unread);
}

void ConnectWindowItem::updateNetworkState(ProtoTypes::NetworkInterface network)
{
    ProtoTypes::NetworkTrustType trusted = network.trust_type();
    ProtoTypes::NetworkInterfaceType type = network.interface_type();
    bool networkActive = network.active();

    if (trustType_ != trusted || interfaceType_ != type || networkActive_ != networkActive)
    {
        QString icon = "";
        if (type == ProtoTypes::NETWORK_INTERFACE_WIFI)
        {
            icon += "NETWORK_WIFI_";
        }
        else if (type == ProtoTypes::NETWORK_INTERFACE_ETH)
        {
            icon += "NETWORK_ETHERNET_";
        }

        if (networkActive)
        {
            networkTrustButton_->setOpacity(1);
        }
        else
        {
            networkTrustButton_->setOpacity(.2);
        }

        if (icon != "")
        {
            if (trusted == ProtoTypes::NETWORK_UNSECURED)
            {
                icon += "UNSECURED";
            }
            else
            {
                icon += "SECURED";
            }

            networkTrustButton_->setIcon(icon);
        }
    }

    networkName_ = QString::fromStdString(network.friendly_name());
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

void ConnectWindowItem::setProtocolPort(const ProtoTypes::Protocol &protocol, const uint port)
{
    connectStateProtocolPort_->setProtocolPort(protocol, port);
}

void ConnectWindowItem::onNetworkHoverEnter()
{
    QString title = "";
    if (networkName_ == "")
    {
        title = tr("No Network Info");
    }
    else
    {
        title = networkName_ + ": ";

       if (trustType_)
       {
           title += tr("Unsecured");
       }
       else
       {
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
    emit showTooltip(ti);
}

void ConnectWindowItem::onNetworkHoverLeave()
{
    emit hideTooltip(TOOLTIP_ID_NETWORK_INFO);
}

void ConnectWindowItem::onConnectStateTextHoverEnter()
{
    if (prevConnectState_.connect_state_type() == ProtoTypes::CONNECTED || prevConnectState_.connect_state_type() == ProtoTypes::DISCONNECTED)
    {
        if (connectionTime_ != "") // only show if connection has been started at least once
        {

            QGraphicsView *view = scene()->views().first();
            QPoint globalPt = view->mapToGlobal(view->mapFromScene(connectStateProtocolPort_->scenePos()));

            int posX = globalPt.x() + 16 * G_SCALE;
            int posY = globalPt.y() - 3 * G_SCALE;

            if (prevConnectState_.connect_state_type() == ProtoTypes::DISCONNECTED)
            {
                QString text = QT_TRANSLATE_NOOP("CommonWidgets::ToolTipWidget", "Connection to Windscribe has been terminated.") +
                        dataTransferred_ + QT_TRANSLATE_NOOP("CommonWidgets::ToolTipWidget", " transferred in ") +
                        connectionTime_;

                TooltipInfo ti(TOOLTIP_TYPE_DESCRIPTIVE, TOOLTIP_ID_CONNECTION_INFO);
                ti.x = posX;
                ti.y = posY;
                ti.tailtype = TOOLTIP_TAIL_BOTTOM;
                ti.tailPosPercent = 0.05;
                ti.title = "";
                ti.desc = text;
                ti.width = 275 * G_SCALE;
                emit showTooltip(ti);
            }
            else // CONNECTED
            {
                QString text = QT_TRANSLATE_NOOP("CommonWidgets::ToolTipWidget", "Connected for ") +
                        connectionTime_ + ", " +
                        dataTransferred_ + QT_TRANSLATE_NOOP("CommonWidgets::ToolTipWidget", " transferred");

                TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_CONNECTION_INFO);
                ti.x = posX;
                ti.y = posY;
                ti.tailtype = TOOLTIP_TAIL_BOTTOM;
                ti.tailPosPercent = 0.05;
                ti.title = text;
                emit showTooltip(ti);
            }
        }
    }
}

void ConnectWindowItem::onConnectStateTextHoverLeave()
{
    emit hideTooltip(TOOLTIP_ID_CONNECTION_INFO);
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
    ti.desc = tr("Blocks all connectivity in the event of a sudden disconnect");
    ti.width = width;

    emit showTooltip(ti);
}

void ConnectWindowItem::onFirewallInfoHoverLeave()
{
    emit hideTooltip(TOOLTIP_ID_FIREWALL_INFO);
}

void ConnectWindowItem::onSecondNameHoverEnter()
{
    if (fullSecondName_ != cityName2Text_->text())
    {
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
        emit showTooltip(ti);
    }
}

void ConnectWindowItem::onSecondNameHoverLeave()
{
    emit hideTooltip(TOOLTIP_ID_FULL_SERVER_NAME);
}

void ConnectWindowItem::onServerRatingIndicatorHoverEnter()
{
    if (prevConnectState_.connect_state_type() == ProtoTypes::CONNECTED)
    {
        QGraphicsView *view = scene()->views().first();
        QPoint globalPt = view->mapToGlobal(serverRatingIndicator_->scenePos().toPoint());
        int posX = globalPt.x() + 7 * G_SCALE;
        int posY = globalPt.y() - 3 * G_SCALE;

        TooltipInfo ti(TOOLTIP_TYPE_INTERACTIVE, TOOLTIP_ID_SERVER_RATINGS);
        ti.x = posX;
        ti.y = posY;
        emit showTooltip(ti);
    }
    else
    {
        QGraphicsView *view = scene()->views().first();
        QPoint globalPt = view->mapToGlobal(serverRatingIndicator_->scenePos().toPoint());
        int posX = globalPt.x() + 8 * G_SCALE;
        int posY = globalPt.y() - 3 * G_SCALE;

        TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_CONNECT_TO_RATE);
        ti.id = TOOLTIP_ID_CONNECT_TO_RATE;
        ti.x = posX;
        ti.y = posY;
        ti.tailtype = TOOLTIP_TAIL_BOTTOM;
        ti.tailPosPercent = 0.9;
        ti.title = tr("Connect to rate");
        ti.type = TOOLTIP_TYPE_BASIC;
        emit showTooltip(ti);
    }
}

void ConnectWindowItem::onServerRatingIndicatorHoverLeave()
{
    emit hideTooltip(TOOLTIP_ID_SERVER_RATINGS);
    emit hideTooltip(TOOLTIP_ID_CONNECT_TO_RATE);
}

void ConnectWindowItem::updatePositions()
{
#ifdef Q_OS_WIN
    int closePosY = WINDOW_WIDTH*G_SCALE - closeButton_->boundingRect().width() - WINDOW_MARGIN*G_SCALE;
    closeButton_->setPos(closePosY, 14*G_SCALE);
    minimizeButton_->setPos(closePosY - minimizeButton_->boundingRect().width() - 26*G_SCALE, 14*G_SCALE);
#else
    int rightMostPosX = static_cast<int>(boundingRect().width()) - static_cast<int>(closeButton_->boundingRect().width()) - 8*G_SCALE;
    int middlePosX = rightMostPosX - static_cast<int>(closeButton_->boundingRect().width()) - 8*G_SCALE;
    closeButton_->setPos(rightMostPosX, 8*G_SCALE);
    minimizeButton_->setPos(middlePosX, 8*G_SCALE);
#endif
    connectButton_->setPos(235*G_SCALE, 50*G_SCALE);

    connectStateProtocolPort_->setPos(16*G_SCALE, 90*G_SCALE);

    cityName1Text_->setFont(*FontManager::instance().getFont(28, true));
    cityName1Text_->setPos(14*G_SCALE, 112*G_SCALE);

    cityName2Text_->recalcBoundingRect();
    cityName2Text_->setPos(16*G_SCALE, 155*G_SCALE);


    preferencesButton_->setPos(WINDOW_MARGIN*G_SCALE, 45*G_SCALE);
    locationsButton_->setPos(93*G_SCALE, 242*G_SCALE);
    serverRatingIndicator_->setPos(296*G_SCALE, 153*G_SCALE);
    networkTrustButton_->setPos(boundingRect().width() - 100*G_SCALE, 156*G_SCALE);
    middleItem_->setPos(0, 208*G_SCALE);
    firewallButton_->setPos(16*G_SCALE, 276*G_SCALE);
    firewallInfo_->setPos(35*G_SCALE, 241*G_SCALE);
    logoButton_->setPos(56*G_SCALE, 34*G_SCALE);
}

} //namespace ConnectWindow
