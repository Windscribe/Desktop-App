#include "networktrustbutton.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>

#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"
#include "tooltips/tooltipcontroller.h"
#include "tooltips/tooltiptypes.h"

namespace ConnectWindow {

NetworkTrustButton::NetworkTrustButton(ScalableGraphicsObject *parent) : ClickableGraphicsObject(parent), trustIcon_(nullptr), animProgress_(OPACITY_SEVENTY), width_(318), arrowShift_(0), isTextElided_(false)
{
    connect(&opacityAnimation_, &QVariantAnimation::valueChanged, this, &NetworkTrustButton::onOpacityValueChanged);

    connect(this, &ClickableGraphicsObject::hoverEnter, this, &NetworkTrustButton::onHoverEnter);
    connect(this, &ClickableGraphicsObject::hoverLeave, this, &NetworkTrustButton::onHoverLeave);

    // This is technically the same asset as protocol arrow, but we use a different copy of it because ImageResourcesSvg will cache the tint color as well,
    // which makes this arrow green when connected.  We don't want that.
    arrow_ = new ImageItem(this, "NETWORK_ARROW");
    arrow_->setOpacity(0.5);

    setClickableHoverable(true, true);
    updatePositions();
}

QRectF NetworkTrustButton::boundingRect() const
{
    return QRectF(0, 0, width_, 22*G_SCALE);
}

void NetworkTrustButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setOpacity(animProgress_);
    painter->setPen(Qt::white);
    painter->setFont(FontManager::instance().getFont(15, QFont::Medium));
    painter->drawText(boundingRect().adjusted(32*G_SCALE, 0, -20*G_SCALE, 0), Qt::AlignLeft | Qt::AlignVCenter, networkText_);
}

void NetworkTrustButton::setNetwork(const types::NetworkInterface &network)
{
    network_ = network;

    QString iconPath;

    // Determine the appropriate icon based on interface type and trust type
    if (network.interfaceType == NETWORK_INTERFACE_NONE) {
        // If network is not available, don't show an icon
        iconPath = "";
    } else if (network_.interfaceType == NETWORK_INTERFACE_WIFI || network_.interfaceType == NETWORK_INTERFACE_MOBILE_BROADBAND) {
        if (network_.trustType == NETWORK_TRUST_SECURED) {
            iconPath = "network/WIFI_SECURED";
        } else {
            iconPath = "network/WIFI_UNSECURED";
        }
    } else {
        if (network_.trustType == NETWORK_TRUST_SECURED) {
            iconPath = "network/ETHERNET_SECURED";
        } else {
            iconPath = "network/ETHERNET_UNSECURED";
        }
    }

    if (network_.active) {
        arrow_->show();
        setClickableHoverable(true, true);
    } else {
        arrow_->hide();
        setClickableHoverable(false, false);
    }

    // Create or update the trust icon
    if (trustIcon_) {
        delete trustIcon_;
        trustIcon_ = nullptr;
    }

    if (!iconPath.isEmpty()) {
        trustIcon_ = new ImageItem(this, iconPath);
        trustIcon_->setOpacity(animProgress_);
    }

    updateNetworkText();
    updatePositions();
}

void NetworkTrustButton::updateNetworkText()
{
    // width minus left text offset and space for arrow on the right
    int availableWidth = kMaxWidth*G_SCALE - 72*G_SCALE - arrow_->boundingRect().width();

    QFont networkFont = FontManager::instance().getFont(15, QFont::Medium);
    QFontMetrics fmNetwork(networkFont);
    if (fmNetwork.horizontalAdvance(network_.friendlyName) > availableWidth) {
        networkText_ = fmNetwork.elidedText(network_.friendlyName, Qt::ElideMiddle, availableWidth);
        isTextElided_ = true;
    } else {
        networkText_ = network_.friendlyName;
        isTextElided_ = false;
    }

    updatePositions();
}

void NetworkTrustButton::updateScaling()
{
    ClickableGraphicsObject::updateScaling();
    updateNetworkText();
    updatePositions();
}

void NetworkTrustButton::onOpacityValueChanged(const QVariant &value)
{
    animProgress_ = value.toDouble();
    if (trustIcon_) {
        trustIcon_->setOpacity(animProgress_);
    }
    // The arrow is slightly less opaque than the icon/text, otherwise it appears different than the protocol arrow.
    // When animProgress is OPACITY_SEVENTY (base), opacity should be 0.5.
    arrow_->setOpacity((animProgress_ - OPACITY_SEVENTY) / 0.6 + 0.5);
    // When animProgress_ is OPACITY_SEVENTY (base), arrowShift_ is 0.  When it's 1.0 (full), arrowShift_ is 4
    arrowShift_ = (animProgress_ - OPACITY_SEVENTY) * 4.0 / 0.3;
    updatePositions();
}

void NetworkTrustButton::updatePositions()
{
    if (trustIcon_) {
        trustIcon_->setPos(0, (boundingRect().height() - trustIcon_->boundingRect().height())/2);
    }

    QFont networkFont = FontManager::instance().getFont(15, QFont::Medium);
    QFontMetrics fmNetwork(networkFont);
    int textWidth = fmNetwork.horizontalAdvance(networkText_);

    arrow_->setPos(36*G_SCALE + textWidth + arrowShift_*G_SCALE, (boundingRect().height() - arrow_->boundingRect().height())/2);

    prepareGeometryChange();
    width_ = 40*G_SCALE + arrow_->boundingRect().width() + textWidth;

    update();
}

void NetworkTrustButton::onHoverEnter()
{
    startAnAnimation(opacityAnimation_, animProgress_, 1.0, ANIMATION_SPEED_FAST);

    // Show tooltip if text is elided
    if (isTextElided_) {
        QGraphicsView *view = scene()->views().first();
        QPoint globalPos = view->mapToGlobal(view->mapFromScene(scenePos()));

        TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_ELIDED_TEXT);
        ti.x = globalPos.x() + boundingRect().width() / 2;
        ti.y = globalPos.y();
        ti.title = network_.friendlyName;
        ti.tailtype = TOOLTIP_TAIL_BOTTOM;
        ti.tailPosPercent = 0.5;
        if (scene() && !scene()->views().isEmpty()) {
            ti.parent = scene()->views().first()->viewport();
        }

        TooltipController::instance().showTooltipBasic(ti);
    }
}

void NetworkTrustButton::onHoverLeave()
{
    startAnAnimation(opacityAnimation_, animProgress_, OPACITY_SEVENTY, ANIMATION_SPEED_FAST);

    // Hide tooltip if it was shown
    if (isTextElided_) {
        TooltipController::instance().hideTooltip(TOOLTIP_ID_ELIDED_TEXT);
    }
}

} // namespace ConnectWindow
