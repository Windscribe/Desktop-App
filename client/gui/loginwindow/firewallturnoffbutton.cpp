#include "firewallturnoffbutton.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"

namespace LoginWindow {

FirewallTurnOffButton::FirewallTurnOffButton(const QString &text, ScalableGraphicsObject *parent)
                      : ClickableGraphicsObject(parent),
                        width_(106), height_(24), font_descr_(12, QFont::Normal),
                        text_(text), is_animating_(false), animation_()
{
    setFlag(GraphicsItemFlag::ItemIsMovable);
    connect(&animation_, &QVariantAnimation::valueChanged, this, &FirewallTurnOffButton::onPositionChanged);
}

QRectF FirewallTurnOffButton::boundingRect() const
{
    return QRectF(0, 0, width_ * G_SCALE, height_ * G_SCALE);
}

void FirewallTurnOffButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!isVisible())
        return;

    const auto kRoundness = 8.0 * G_SCALE;
    const auto rect = boundingRect();

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(FontManager::instance().getSeaGreenColor());
    painter->drawRoundedRect(rect.adjusted(0, -kRoundness, 0, 0), kRoundness, kRoundness);

    QFont font = FontManager::instance().getFont(font_descr_);
    painter->setFont(font);
    painter->setPen(Qt::black);
    painter->drawText(rect.adjusted(0, -2 * G_SCALE, 0, 0), Qt::AlignCenter | Qt::AlignVCenter,
                      tr(text_.toStdString().c_str()));
}

void FirewallTurnOffButton::setActive(bool active)
{
    setVisible(active);
    setClickable(active);

    if (active) {
        setPos(x(), -boundingRect().height());
        startAnAnimation<qreal>(animation_, y(), 0, ANIMATION_SPEED_SLOW);
        is_animating_ = true;
    } else {
        setPos(x(), 0);
        is_animating_ = false;
    }
}

void FirewallTurnOffButton::animatedHide()
{
    setClickable(false);
    startAnAnimation<qreal>(animation_, y(), -boundingRect().height(), ANIMATION_SPEED_FAST);
    is_animating_ = true;
}

void FirewallTurnOffButton::onPositionChanged(QVariant position)
{
    if (!is_animating_)
        return;

    setPos(x(), position.toReal());

    if (position == animation_.endValue()) {
        setVisible(animation_.endValue().toReal() > animation_.startValue().toReal());
        is_animating_ = false;
    }
}

void FirewallTurnOffButton::setText(const QString &text)
{
    text_ = text;
    update();
}

}  // namespace LoginWindow
