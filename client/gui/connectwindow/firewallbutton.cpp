#include "firewallbutton.h"
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"

namespace ConnectWindow {


FirewallButton::FirewallButton(ScalableGraphicsObject *parent) : ClickableGraphicsObject(parent),
    isFirewallEnabled_(false), isDisabled_(false)
{
    animation_.setStartValue(0.0);
    animation_.setEndValue(1.0);
    animation_.setDuration(150);
    connect(&animation_, &QVariantAnimation::valueChanged, this, &FirewallButton::onAnimationValueChanged);
    setClickable(true);
    setResetHoverOnClick(false);
}

QRectF FirewallButton::boundingRect() const
{
    return QRectF(0, 0, 44*G_SCALE, 24*G_SCALE);
}

void FirewallButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initOpacity = painter->opacity();

    //painter->fillRect(boundingRect(), QBrush(QColor(0, 255, 255)));
    {
        QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap("firewall/BLACK_TOGGLE_BG");
        p->draw(0, 0, painter);
        painter->setOpacity(animation_.currentValue().toDouble() * initOpacity);
        p = ImageResourcesSvg::instance().getIndependentPixmap("firewall/BLUE_TOGGLE_BG");
        p->draw(0, 0, painter);
    }

    painter->setOpacity(1.0 * initOpacity);
    {
        QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap("firewall/TOGGLE_BUTTON_WHITE");
        double d = animation_.currentValue().toDouble();
        p->draw(static_cast<int>((2 + d*20.0)*G_SCALE), 2*G_SCALE, painter);
    }
}

void FirewallButton::setDisabled(bool isDisabled)
{
    isDisabled_ = isDisabled;
}

void FirewallButton::setFirewallState(bool isFirewallEnabled)
{
    if (isFirewallEnabled_ != isFirewallEnabled)
    {
        isFirewallEnabled_ = isFirewallEnabled;

        if (isFirewallEnabled_)
        {
            animation_.setDirection(QVariantAnimation::Forward);
        }
        else
        {
            animation_.setDirection(QVariantAnimation::Backward);
        }
        if (animation_.state() != QVariantAnimation::Running)
        {
            animation_.start();
        }

        update();
    }
}

void FirewallButton::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (isDisabled_)
    {
        event->ignore();
    }
    else
    {
        ClickableGraphicsObject::mousePressEvent(event);
    }
}

void FirewallButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (isDisabled_)
    {
        event->ignore();
    }
    else
    {
        ClickableGraphicsObject::mouseReleaseEvent(event);
    }
}

void FirewallButton::onAnimationValueChanged(const QVariant &value)
{
    Q_UNUSED(value);
    update();
}

} //namespace ConnectWindow
