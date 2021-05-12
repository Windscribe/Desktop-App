#include "connectstateprotocolport.h"
#include "graphicresources/fontmanager.h"
#include "utils/protoenumtostring.h"

#include <QFontMetrics>
#include <QPainter>
#include "graphicresources/imageresourcessvg.h"
#include "commongraphics/commongraphics.h"
#include "languagecontroller.h"
#include "dpiscalemanager.h"

const int PROTOCOL_OPACITY_ANIMATION_DURATION = 500;

namespace ConnectWindow {

ConnectStateProtocolPort::ConnectStateProtocolPort(ScalableGraphicsObject *parent) : ScalableGraphicsObject(parent)
    , fontDescr_(11,true, 100)
    , hoverable_(false)
    , connectivity_(false)
    , protocol_(ProtoTypes::PROTOCOL_IKEV2)
    , port_(500)
    , textColor_(QColor(255, 255, 255))
    , textOpacity_(0.5)
    , badgeIconBg_("CONNECTION_BADGE_BG_OFF")
    , badgeIconFg_("CONNECTION_BADGE_TEXT_OFF")
    , receivedTunnelTestResult_  (false)
{
    setAcceptHoverEvents(true);
    protocolTestTunnelTimer_.setInterval(PROTOCOL_OPACITY_ANIMATION_DURATION);
    connect(&protocolTestTunnelTimer_, SIGNAL(timeout()), SLOT(onProtocolTestTunnelTimerTick()));
    connect(&protocolOpacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onProtocolOpacityAnimationChanged(QVariant)));

    connectionBadgeDots_ = new ConnectionBadgeDots(this);

    recalcSize();
}

QRectF ConnectStateProtocolPort::boundingRect() const
{
    return QRectF(0, 0, width_*G_SCALE, height_*G_SCALE);
}

void ConnectStateProtocolPort::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    //painter->fillRect(boundingRect(), QBrush(QColor(0, 255, 255)));

    qreal initOpacity = painter->opacity();

    // badge
    QSharedPointer<IndependentPixmap> badgeBgPixmap = ImageResourcesSvg::instance().getIndependentPixmap(badgeIconBg_);
    badgeBgPixmap->draw(0,0, painter);

    // connection badge dots made to show during connecting
    if (badgeIconFg_ != "")
    {
        QSharedPointer<IndependentPixmap> badgeFgPixmap = ImageResourcesSvg::instance().getIndependentPixmap(badgeIconFg_);
        int widthOffset = badgeBgPixmap->width()/2 - badgeFgPixmap->width()/2;
        badgeFgPixmap->draw(widthOffset, badgeBgPixmap->height()/2 - badgeFgPixmap->height()/2, painter);
    }

    QFont font = *FontManager::instance().getFont(fontDescr_);
    painter->setFont(font);
    painter->setOpacity(textOpacity_ * initOpacity);
    painter->setPen(textColor_);

    const int textHeight = 8*G_SCALE;
    const int posYTop = badgeBgPixmap->height()/2 - textHeight/2;
    const int posYBot = posYTop + textHeight;
    const int separatorMinusProtocolPosX = badgeBgPixmap->width() + (protocolSeparatorPadding + badgeProtocolPadding)*G_SCALE;

    // protocol
    const QString protocolString = ProtoEnumToString::instance().toString(protocol_);
    painter->drawText(QPoint(badgeBgPixmap->width() + badgeProtocolPadding * G_SCALE, posYBot), protocolString);

    // qDebug() << "G_SCALE: " << G_SCALE;
    // qDebug() << "Protocol width: " << protocolWidth;

    // port
    QFontMetrics fm = painter->fontMetrics();
    const int protocolWidth = fm.horizontalAdvance(protocolString);
    const QString portString = QString::number(port_);
    const int portPosX = separatorMinusProtocolPosX + (separatorPortPadding + 1) * G_SCALE + protocolWidth;
    painter->drawText(QPoint(portPosX, posYBot), portString);

    // separator
    painter->setPen(Qt::white);
    painter->setOpacity(0.15 * initOpacity);
    painter->drawLine(QPoint(separatorMinusProtocolPosX + protocolWidth, posYTop),
                      QPoint(separatorMinusProtocolPosX + protocolWidth, posYBot));
}

void ConnectStateProtocolPort::updateStateDisplay(ProtoTypes::ConnectState connectState)
{
    if (connectivity_)
    {
        if (connectState.connect_state_type() == ProtoTypes::CONNECTED)
        {
            badgeIconBg_ = "CONNECTION_BADGE_BG_ON";
            badgeIconFg_ = "CONNECTION_BADGE_FG_ON";
            textOpacity_ = 1.0;
            textColor_ = QColor(0x55, 0xFF, 0x8A);
            connectionBadgeDots_->hide();
            connectionBadgeDots_->stop();
        }
        else if (connectState.connect_state_type() == ProtoTypes::CONNECTING)
        {
            badgeIconBg_ = "CONNECTION_BADGE_BG_CONNECTING";
            badgeIconFg_ = ""; // hide fg
            textOpacity_ = 1.0;
            textColor_ = QColor(0xA0, 0xFE, 0xDA);

            connectionBadgeDots_->start();
            connectionBadgeDots_->show();
        }
        else if (connectState.connect_state_type() == ProtoTypes::DISCONNECTING)
        {
            badgeIconBg_ = "CONNECTION_BADGE_BG_CONNECTING";
            badgeIconFg_ = ""; // hide fg
            textOpacity_ = 1.0;
            textColor_ = QColor(0xA0, 0xFE, 0xDA);
            connectionBadgeDots_->start();
            connectionBadgeDots_->show();
        }
        else if (connectState.connect_state_type() == ProtoTypes::DISCONNECTED)
        {
            badgeIconBg_ = "CONNECTION_BADGE_BG_OFF";
            badgeIconFg_ = "CONNECTION_BADGE_FG_OFF";
            textOpacity_ = 0.5;
            textColor_ = Qt::white;
            connectionBadgeDots_->hide();
            connectionBadgeDots_->stop();
        }
    }
    else
    {
        protocolTestTunnelTimer_.stop();

        badgeIconFg_ = "CONNECTION_BADGE_FG_NO_INT";
        connectionBadgeDots_->hide();
        connectionBadgeDots_->stop();

        if (connectState.connect_state_type() == ProtoTypes::DISCONNECTED)
        {
            badgeIconBg_ = "CONNECTION_BADGE_BG_OFF";
            textOpacity_ = 0.5;
            textColor_ = Qt::white;
        }
        else
        {
            badgeIconBg_ = "CONNECTION_BADGE_BG_CONNECTING";

            textOpacity_ = 1.0;
            textColor_ = FontManager::instance().getBrightYellowColor();
        }
    }
    recalcSize();
}

void ConnectStateProtocolPort::onConnectStateChanged(ProtoTypes::ConnectState newConnectState, ProtoTypes::ConnectState prevConnectState)
{
    Q_UNUSED(prevConnectState);

    if (newConnectState.connect_state_type() == ProtoTypes::CONNECTING)
    {
        receivedTunnelTestResult_ = false;
    }

    if (newConnectState.connect_state_type() == ProtoTypes::CONNECTED)
    {
        protocolTestTunnelTimer_.start();
    }

    if (newConnectState.connect_state_type() == ProtoTypes::DISCONNECTING || newConnectState.connect_state_type() == ProtoTypes::DISCONNECTED)
    {
        protocolTestTunnelTimer_.stop();
    }

    connectState_ = newConnectState;
    updateStateDisplay(newConnectState);
}

void ConnectStateProtocolPort::setHoverable(bool hoverable)
{
    hoverable_ = hoverable;
}

void ConnectStateProtocolPort::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    recalcSize();
}

void ConnectStateProtocolPort::setInternetConnectivity(bool connectivity)
{
    connectivity_ = connectivity;
    updateStateDisplay(connectState_);
}

void ConnectStateProtocolPort::setProtocolPort(const ProtoTypes::Protocol &protocol, const uint port)
{
    protocol_ = protocol;
    port_ = port;
    updateStateDisplay(connectState_);
}

void ConnectStateProtocolPort::setTestTunnelResult(bool success)
{
    Q_UNUSED(success)

    if (!receivedTunnelTestResult_)
    {
        receivedTunnelTestResult_ = true;
        protocolTestTunnelTimer_.stop();
        startAnAnimation<double>(protocolOpacityAnimation_, textOpacity_, OPACITY_FULL, PROTOCOL_OPACITY_ANIMATION_DURATION);
    }
}

void ConnectStateProtocolPort::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);

    if (hoverable_)
    {
        emit hoverEnter();
    }
}

void ConnectStateProtocolPort::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    emit hoverLeave();
}

void ConnectStateProtocolPort::onProtocolTestTunnelTimerTick()
{
    if (connectState_.connect_state_type() == ProtoTypes::CONNECTED)
    {
        if (textOpacity_ > .5)
        {
            startAnAnimation<double>(protocolOpacityAnimation_, textOpacity_, OPACITY_HIDDEN, PROTOCOL_OPACITY_ANIMATION_DURATION);
        }
        else
        {
            startAnAnimation<double>(protocolOpacityAnimation_, textOpacity_, OPACITY_FULL, PROTOCOL_OPACITY_ANIMATION_DURATION);
        }
    }
    else
    {
        protocolTestTunnelTimer_.stop();
        textOpacity_ = OPACITY_HIDDEN;
    }
}

void ConnectStateProtocolPort::onProtocolOpacityAnimationChanged(const QVariant &value)
{
    if (connectState_.connect_state_type() == ProtoTypes::CONNECTED)
    {
        textOpacity_ = value.toDouble();
        update();
    }
}

void ConnectStateProtocolPort::recalcSize()
{
    prepareGeometryChange();

    QFont font = *FontManager::instance().getFont(fontDescr_);
    QFontMetrics fm(font);

    const QString protocolString = ProtoEnumToString::instance().toString(protocol_);
    const int protocolWidth = fm.horizontalAdvance(protocolString);
    const QString portString = QString::number(port_);
    const int portWidth = fm.horizontalAdvance(portString);
    const int badgeWidth = 36;
    const int separatorWidthSum = badgeProtocolPadding + protocolSeparatorPadding + separatorPortPadding;
    const int badgeHeight = 20;

    width_ = protocolWidth + portWidth + badgeWidth + separatorWidthSum;
    height_ = badgeHeight;
}


} //namespace ConnectWindow
