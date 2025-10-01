#include "iconhoverengagebutton.h"

#include <QGraphicsSceneMouseEvent>
#include <QImage>
#include <QPixmap>
#include <QPainter>
#include <QCursor>
#include "graphicresources/imageresourcessvg.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"

namespace LoginWindow {

IconHoverEngageButton::IconHoverEngageButton(const QString &imagePathDisabled, const QString &imagePathEnabled, ScalableGraphicsObject *parent) : ClickableGraphicsObject(parent)
{
    setScale(1.0);
    imagePath_ = imagePathDisabled;

    imagePathEnabled_ = imagePathEnabled;
    imagePathDisabled_ = imagePathDisabled;

    active_ = false;

    curOpacity_ = OPACITY_UNHOVER_ICON_STANDALONE; // 40% opacity for standalone icons

    connect(&imageOpacityAnimation_, &QVariantAnimation::valueChanged, this, &IconHoverEngageButton::onImageHoverOpacityChanged);
    connect(this, &ClickableGraphicsObject::hoverEnter, this, &IconHoverEngageButton::onHoverEnter);
    connect(this, &ClickableGraphicsObject::hoverLeave, this, &IconHoverEngageButton::onHoverLeave);
    setClickable(true);
}

QRectF IconHoverEngageButton::boundingRect() const
{
    return QRectF(0, 0, WIDTH*G_SCALE, HEIGHT*G_SCALE);
}

void IconHoverEngageButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initialOpacity = painter->opacity();

    painter->setOpacity(curOpacity_ * initialOpacity);
    QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap(imagePath_);
    p->draw(0, 0, painter);
}

void IconHoverEngageButton::setActive(bool active)
{
    active_ = active;

    double targetOpacity = OPACITY_FULL;
    imagePath_ = imagePathEnabled_;
    if (!active)
    {
        imagePath_ = imagePathDisabled_;
        targetOpacity = OPACITY_UNHOVER_ICON_STANDALONE;
    }

    // the only way to change active is by clicking the button, so you must be hovering -> FULL
    startAnAnimation<double>(imageOpacityAnimation_, curOpacity_, targetOpacity, ANIMATION_SPEED_FAST);
}

bool IconHoverEngageButton::active()
{
    return active_;
}

void IconHoverEngageButton::onHoverEnter()
{
    if (clickable_)
    {
        startAnAnimation<double>(imageOpacityAnimation_, curOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
    }
}

void IconHoverEngageButton::onHoverLeave()
{
    startAnAnimation<double>(imageOpacityAnimation_, curOpacity_, unhoverOpacity(), ANIMATION_SPEED_FAST);
}

void IconHoverEngageButton::onImageHoverOpacityChanged(const QVariant &value)
{
    curOpacity_ = value.toDouble();
    update();
}

double IconHoverEngageButton::unhoverOpacity()
{
    double opacity = OPACITY_UNHOVER_ICON_STANDALONE;
    if (active_)
    {
        opacity = OPACITY_FULL;
    }

    return opacity;
}

}
