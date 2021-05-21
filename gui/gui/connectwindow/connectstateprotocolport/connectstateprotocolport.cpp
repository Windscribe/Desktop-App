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
    , receivedTunnelTestResult_  (false)
    , badgePixmap_(QSize(36*G_SCALE, 20*G_SCALE), 10*G_SCALE)
{
    badgeFgImage_.reset(new ImageWithShadow("connection-badge/OFF", "connection-badge/OFF_SHADOW"));
    setAcceptHoverEvents(true);
    protocolTestTunnelTimer_.setInterval(PROTOCOL_OPACITY_ANIMATION_DURATION);
    connect(&protocolTestTunnelTimer_, SIGNAL(timeout()), SLOT(onProtocolTestTunnelTimerTick()));
    connect(&protocolOpacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onProtocolOpacityAnimationChanged(QVariant)));

    connectionBadgeDots_ = new ConnectionBadgeDots(this);

    recalcSize();
}

QRectF ConnectStateProtocolPort::boundingRect() const
{
    return QRectF(0, 0, width_, height_);
}

void ConnectStateProtocolPort::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->save();
    //painter->fillRect(boundingRect(), QBrush(QColor(0, 255, 255)));

    qreal initOpacity = painter->opacity();

    // draw badge
    badgePixmap_.draw(painter, 0, 0);

    // connection badge dots made to show during connecting
    if (badgeFgImage_)
    {
        int widthOffset = badgePixmap_.width()/2 - badgeFgImage_->width()/2;
        badgeFgImage_->draw(painter, widthOffset, badgePixmap_.height()/2 - badgeFgImage_->height()/2);
    }

    QFont font = *FontManager::instance().getFont(fontDescr_);
    painter->setFont(font);
    painter->setOpacity(textOpacity_ * initOpacity);
    painter->setPen(textColor_);

    const int textHeight = 8*G_SCALE;
    const int posYTop = badgePixmap_.height()/2 - textHeight/2;
    const int posYBot = posYTop + textHeight;
    const int separatorMinusProtocolPosX = badgePixmap_.width() + (protocolSeparatorPadding + badgeProtocolPadding)*G_SCALE;

    // protocol and port string
    QString protocolString = ProtoEnumToString::instance().toString(protocol_);
    textShadowProtocol_.drawText(painter, QRect(badgePixmap_.width() + badgeProtocolPadding * G_SCALE, 0, INT_MAX, badgePixmap_.height()), Qt::AlignLeft | Qt::AlignVCenter, protocolString, &font, textColor_ );

    // port
    const QString portString = QString::number(port_);
    const int portPosX = separatorMinusProtocolPosX + (separatorPortPadding + 1) * G_SCALE + textShadowProtocol_.width();
    textShadowPort_.drawText(painter, QRect(portPosX, 0, INT_MAX, badgePixmap_.height()), Qt::AlignLeft | Qt::AlignVCenter, portString, &font, textColor_ );

    // separator
    painter->setPen(Qt::white);
    painter->setOpacity(0.15 * initOpacity);
    painter->drawLine(QPoint(separatorMinusProtocolPosX + textShadowProtocol_.width(), posYTop),
                      QPoint(separatorMinusProtocolPosX + textShadowProtocol_.width(), posYBot));

    painter->setPen(QColor(0x02, 0x0D, 0x1C));
    painter->drawLine(QPoint(separatorMinusProtocolPosX + textShadowProtocol_.width()+1, posYTop),
                      QPoint(separatorMinusProtocolPosX + textShadowProtocol_.width()+1, posYBot));


    painter->restore();
}

void ConnectStateProtocolPort::updateStateDisplay(ProtoTypes::ConnectState connectState)
{
    if (connectivity_)
    {
        if (connectState.connect_state_type() == ProtoTypes::CONNECTED)
        {
            badgeBgColor_ = QColor(0x02, 0x0D, 0x1C, 255*0.25);
            badgeFgImage_.reset(new ImageWithShadow("connection-badge/ON", "connection-badge/ON_SHADOW"));
            textOpacity_ = 1.0;
            textColor_ = QColor(0x55, 0xFF, 0x8A);
            connectionBadgeDots_->hide();
            connectionBadgeDots_->stop();
        }
        else if (connectState.connect_state_type() == ProtoTypes::CONNECTING)
        {
            badgeBgColor_ = QColor(0x02, 0x0D, 0x1C, 255*0.25);
            badgeFgImage_.reset();
            textOpacity_ = 1.0;
            textColor_ = QColor(0xA0, 0xFE, 0xDA);

            connectionBadgeDots_->start();
            connectionBadgeDots_->show();
        }
        else if (connectState.connect_state_type() == ProtoTypes::DISCONNECTING)
        {
            badgeBgColor_ = QColor(0x02, 0x0D, 0x1C, 255*0.25);
            badgeFgImage_.reset();
            textOpacity_ = 1.0;
            textColor_ = QColor(0xA0, 0xFE, 0xDA);
            connectionBadgeDots_->start();
            connectionBadgeDots_->show();
        }
        else if (connectState.connect_state_type() == ProtoTypes::DISCONNECTED)
        {
            badgeBgColor_ = QColor(255, 255, 255, 39);
            badgeFgImage_.reset(new ImageWithShadow("connection-badge/OFF", "connection-badge/OFF_SHADOW"));
            textOpacity_ = 0.5;
            textColor_ = Qt::white;
            connectionBadgeDots_->hide();
            connectionBadgeDots_->stop();
        }
    }
    else
    {
        protocolTestTunnelTimer_.stop();

        badgeFgImage_.reset(new ImageWithShadow("connection-badge/NO_INT", "connection-badge/NO_INT_SHADOW"));
        connectionBadgeDots_->hide();
        connectionBadgeDots_->stop();

        if (connectState.connect_state_type() == ProtoTypes::DISCONNECTED)
        {
            badgeBgColor_ = QColor(255, 255, 255, 39);
            textOpacity_ = 0.5;
            textColor_ = Qt::white;
        }
        else
        {
            badgeBgColor_ = QColor(0x02, 0x0D, 0x1C, 255*0.25);

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
    badgePixmap_.setSize(QSize(36*G_SCALE, 20*G_SCALE), 10*G_SCALE);
    if (badgeFgImage_)
    {
        badgeFgImage_->updatePixmap();
    }
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
    const int badgeWidth = 36*G_SCALE;
    const int separatorWidthSum = (badgeProtocolPadding + protocolSeparatorPadding * 2 + separatorPortPadding) * G_SCALE;
    const int badgeHeight = 20*G_SCALE;

    width_ = protocolWidth + portWidth + badgeWidth + separatorWidthSum;
    height_ = badgeHeight;
}


} //namespace ConnectWindow
