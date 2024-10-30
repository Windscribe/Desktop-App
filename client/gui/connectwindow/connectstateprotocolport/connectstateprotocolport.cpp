#include "connectstateprotocolport.h"
#include "graphicresources/fontmanager.h"

#include <QFontMetrics>
#include <QPainter>
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"

const int PROTOCOL_OPACITY_ANIMATION_DURATION = 500;

namespace ConnectWindow {

ConnectStateProtocolPort::ConnectStateProtocolPort(ScalableGraphicsObject *parent) : ClickableGraphicsObject(parent)
    , fontDescr_(11, true, 100, 0.5)
    , connectivity_(false)
    , textColor_(QColor(255, 255, 255))
    , textOpacity_(0.5)
    , receivedTunnelTestResult_  (false)
    , badgePixmap_(QSize(36*G_SCALE, 20*G_SCALE), 10*G_SCALE)
    , preferredProtocolBadge_(new IconButton(8, 10, "PREFERRED_PROTOCOL_BADGE", "", this, OPACITY_FULL, OPACITY_FULL))
    , isPreferredProtocol_(false)
    , protocolArrow_(new IconButton(16, 16, "PROTOCOL_ARROW", "", this, OPACITY_HALF, OPACITY_FULL))
    , arrowShift_(0)
{
    protocol_ = types::Protocol::WIREGUARD;
    port_ = 443;

    badgeFgImage_.reset(new ImageWithShadow("connection-badge/OFF", "connection-badge/OFF_SHADOW"));
    setAcceptHoverEvents(true);
    protocolTestTunnelTimer_.setInterval(PROTOCOL_OPACITY_ANIMATION_DURATION);
    connect(&protocolTestTunnelTimer_, &QTimer::timeout, this, &ConnectStateProtocolPort::onProtocolTestTunnelTimerTick);
    connect(&protocolOpacityAnimation_, &QVariantAnimation::valueChanged, this, &ConnectStateProtocolPort::onProtocolOpacityAnimationChanged);
    connect(protocolArrow_, &IconButton::clicked, this, &ConnectStateProtocolPort::clicked);

    connectionBadgeDots_ = new ConnectionBadgeDots(this);

    connect(this, &ClickableGraphicsObject::hoverEnter, this, &ConnectStateProtocolPort::onHoverEnter);
    connect(this, &ClickableGraphicsObject::hoverLeave, this, &ConnectStateProtocolPort::onHoverLeave);
    connect(&protocolArrowAnimation_, &QVariantAnimation::valueChanged, this, &ConnectStateProtocolPort::onProtocolArrowAnimationChanged);

    preferredProtocolBadge_->setClickableHoverable(false, false);

    antiCensorshipBadge_ = new IconButton(8, 16, "preferences/CIRCUMVENT_CENSORSHIP", "", this, OPACITY_FULL, OPACITY_FULL);
    antiCensorshipBadge_->setClickableHoverable(false, false);

    setClickableHoverable(true, false);
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
        int widthOffset = (badgePixmap_.width() - badgeFgImage_->width())/2;
        badgeFgImage_->draw(painter, widthOffset, (badgePixmap_.height() - badgeFgImage_->height())/2);
    }

    QFont font = FontManager::instance().getFont(fontDescr_);
    painter->setOpacity(textOpacity_ * initOpacity);
    painter->setPen(textColor_);

    const int textHeight = CommonGraphics::textHeight(font);
    const int posYTop = badgePixmap_.height()/2 - textHeight/2;
    const int posYBot = posYTop + textHeight;
    const int separatorMinusProtocolPosX = badgePixmap_.width() + 2*kSpacerWidth*G_SCALE;

    // types::Protocol and port string
    QString protocolString = protocol_.toLongString();
    textShadowProtocol_.drawText(painter, QRect(badgePixmap_.width() + kSpacerWidth*G_SCALE, 0, INT_MAX, badgePixmap_.height()), Qt::AlignLeft | Qt::AlignVCenter, protocolString, font, textColor_ );

    // port
    const QString portString = QString::number(port_);
    const int portPosX = separatorMinusProtocolPosX + kSpacerWidth*G_SCALE + textShadowProtocol_.width();
    textShadowPort_.drawText(painter, QRect(portPosX, 0, INT_MAX, badgePixmap_.height()), Qt::AlignLeft | Qt::AlignVCenter, portString, font, textColor_ );

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

void ConnectStateProtocolPort::updateStateDisplay(const types::ConnectState &connectState)
{
    if (connectivity_)
    {
        if (connectState.connectState == CONNECT_STATE_CONNECTED)
        {
            badgeBgColor_ = QColor(0x02, 0x0D, 0x1C, 255*0.25);
            badgeFgImage_.reset(new ImageWithShadow("connection-badge/ON", "connection-badge/ON_SHADOW"));
            textOpacity_ = 1.0;
            textColor_ = QColor(0x55, 0xFF, 0x8A);
            connectionBadgeDots_->hide();
            connectionBadgeDots_->stop();
        }
        else if (connectState.connectState == CONNECT_STATE_CONNECTING)
        {
            badgeBgColor_ = QColor(0x02, 0x0D, 0x1C, 255*0.25);
            badgeFgImage_.reset();
            textOpacity_ = 1.0;
            textColor_ = QColor(0xA0, 0xFE, 0xDA);
            connectionBadgeDots_->start();
            connectionBadgeDots_->show();
        }
        else if (connectState.connectState == CONNECT_STATE_DISCONNECTING)
        {
            badgeBgColor_ = QColor(0x02, 0x0D, 0x1C, 255*0.25);
            badgeFgImage_.reset();
            textOpacity_ = 1.0;
            textColor_ = QColor(0xA0, 0xFE, 0xDA);
            connectionBadgeDots_->start();
            connectionBadgeDots_->show();
        }
        else if (connectState.connectState == CONNECT_STATE_DISCONNECTED)
        {
            badgeBgColor_ = QColor(255, 255, 255, 39);
            badgeFgImage_.reset(new ImageWithShadow("connection-badge/OFF", "connection-badge/OFF_SHADOW"));
            textOpacity_ = 0.8;
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

        if (connectState.connectState == CONNECT_STATE_DISCONNECTED)
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

    badgePixmap_.setColor(badgeBgColor_);
    protocolArrow_->setTintColor(textColor_);
    preferredProtocolBadge_->setTintColor(textColor_);
    antiCensorshipBadge_->setTintColor(textColor_);

    recalcSize();
}

void ConnectStateProtocolPort::onConnectStateChanged(const types::ConnectState &newConnectState, const types::ConnectState &prevConnectState)
{
    Q_UNUSED(prevConnectState);

    if (newConnectState.connectState == CONNECT_STATE_CONNECTING)
    {
        receivedTunnelTestResult_ = false;
    }

    if (newConnectState.connectState == CONNECT_STATE_CONNECTED)
    {
        protocolTestTunnelTimer_.start();
    }

    if (newConnectState.connectState == CONNECT_STATE_DISCONNECTING || newConnectState.connectState == CONNECT_STATE_DISCONNECTED)
    {
        protocolTestTunnelTimer_.stop();
    }

    connectState_ = newConnectState;
    updateStateDisplay(newConnectState);
}

void ConnectStateProtocolPort::setHoverable(bool hoverable)
{
    setClickableHoverable(true, hoverable);
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

void ConnectStateProtocolPort::setProtocolPort(const types::Protocol &protocol, const uint port)
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

void ConnectStateProtocolPort::onProtocolTestTunnelTimerTick()
{
    if (connectState_.connectState == CONNECT_STATE_CONNECTED)
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
        updateStateDisplay(connectState_);
    }
}

void ConnectStateProtocolPort::onProtocolOpacityAnimationChanged(const QVariant &value)
{
    if (connectState_.connectState == CONNECT_STATE_CONNECTED)
    {
        textOpacity_ = value.toDouble();
        update();
    }
}

void ConnectStateProtocolPort::onProtocolArrowAnimationChanged(const QVariant &value)
{
    arrowShift_ = value.toInt();
    recalcSize();
}

void ConnectStateProtocolPort::recalcSize()
{
    prepareGeometryChange();

    QFont font = FontManager::instance().getFont(fontDescr_);
    QFontMetrics fm(font);

    const QString protocolString = protocol_.toLongString();
    const int protocolWidth = fm.horizontalAdvance(protocolString);
    const QString portString = QString::number(port_);
    const int portWidth = fm.horizontalAdvance(portString);
    const int badgeWidth = 36*G_SCALE;
    const int separatorWidth = kSpacerWidth*G_SCALE;
    const int badgeHeight = 20*G_SCALE;
    const int protocolArrowWidth = 16*G_SCALE;

    width_ = protocolWidth + portWidth + badgeWidth + protocolArrowWidth + 4*separatorWidth + 8*G_SCALE;
    if (isPreferredProtocol_) {
        width_ += preferredProtocolBadge_->boundingRect().width() + separatorWidth;
    }
    if (isAntiCensorshipEnabled_) {
        width_ += antiCensorshipBadge_->boundingRect().width() + separatorWidth;
    }
    height_ = badgeHeight;

    protocolArrow_->setPos(width_ - protocolArrowWidth - 4*G_SCALE + arrowShift_*G_SCALE, 2*G_SCALE);

    preferredProtocolBadge_->setVisible(isPreferredProtocol_);
    if (isPreferredProtocol_) {
        preferredProtocolBadge_->setPos(protocolWidth + portWidth + badgeWidth + 4*separatorWidth + 4*G_SCALE,
                                        boundingRect().height()/2 - preferredProtocolBadge_->boundingRect().height()/2);
    }

    antiCensorshipBadge_->setVisible(isAntiCensorshipEnabled_);
    if (isAntiCensorshipEnabled_) {
        qreal antiCensorshipBadgeLeft;
        if (isPreferredProtocol_) {
            antiCensorshipBadgeLeft = preferredProtocolBadge_->pos().x() + preferredProtocolBadge_->boundingRect().width() + separatorWidth;
        }
        else {
            antiCensorshipBadgeLeft = protocolWidth + portWidth + badgeWidth + 4*separatorWidth + 4*G_SCALE;
        }

        antiCensorshipBadge_->setPos(antiCensorshipBadgeLeft, boundingRect().height()/2 - antiCensorshipBadge_->boundingRect().height()/2);
    }
}

types::ProtocolStatus ConnectStateProtocolPort::getProtocolStatus()
{
    types::ProtocolStatus ps;

    ps.protocol = protocol_;
    ps.port = port_;
    if (connectState_.connectState == CONNECT_STATE_CONNECTED) {
        ps.status = types::ProtocolStatus::Status::kConnected;
    } else {
        ps.status = types::ProtocolStatus::Status::kDisconnected;
    }
    return ps;
}

bool ConnectStateProtocolPort::isProtocolButtonVisible() const
{
    return protocolArrow_->isVisible();
}

void ConnectStateProtocolPort::setProtocolButtonVisible(bool visible)
{
    protocolArrow_->setVisible(visible);
}

void ConnectStateProtocolPort::onHoverEnter()
{
    protocolArrow_->hover();
    startAnAnimation(protocolArrowAnimation_, arrowShift_, 4, ANIMATION_SPEED_FAST);
}

void ConnectStateProtocolPort::onHoverLeave()
{
    protocolArrow_->unhover();
    startAnAnimation(protocolArrowAnimation_, arrowShift_, 0, ANIMATION_SPEED_FAST);
}

void ConnectStateProtocolPort::setIsPreferredProtocol(bool on)
{
    isPreferredProtocol_ = on;
    recalcSize();
}

void ConnectStateProtocolPort::antiCensorshipChanged(bool enabled)
{
    isAntiCensorshipEnabled_ = enabled;
    recalcSize();
}

} //namespace ConnectWindow
