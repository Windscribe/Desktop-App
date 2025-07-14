#include "togglebutton.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QCursor>
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"

ToggleButton::ToggleButton(ScalableGraphicsObject *parent) : ClickableGraphicsObject(parent),
    animationProgress_(0.0), isChecked_(false), isToggleable_(true), enabled_(true)
{
    setAcceptHoverEvents(true);

    color_ = FontManager::instance().getSeaGreenColor();

    connect(&opacityAnimation_, &QVariantAnimation::valueChanged, this, &ToggleButton::onOpacityChanged);
    opacityAnimation_.setStartValue(0.0);
    opacityAnimation_.setEndValue(1.0);
    opacityAnimation_.setDuration(150);

    setClickable(true);
}

QRectF ToggleButton::boundingRect() const
{
    return QRectF(0, 0, 40*G_SCALE, 22*G_SCALE);
}

void ToggleButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setOpacity(OPACITY_FULL);
    if (!enabled_)
    {
        painter->setOpacity(OPACITY_HALF);
    }

    painter->setRenderHint(QPainter::Antialiasing);
    // At 100%, this would be FontManager::instance().getSeaGreenColor()
    QColor bgColor = QColor(255 - (255 - color_.red())*animationProgress_,
                            255 - (255 - color_.green())*animationProgress_,
                            255 - (255 - color_.blue())*animationProgress_);
    painter->setBrush(bgColor);
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(boundingRect().toRect(), TOGGLE_RADIUS*G_SCALE, TOGGLE_RADIUS*G_SCALE);

    int w1 = BUTTON_OFFSET*G_SCALE;
    int w2 = static_cast<int>(boundingRect().width() - BUTTON_WIDTH*G_SCALE - BUTTON_OFFSET*G_SCALE);
    int curW = static_cast<int>((w2 - w1)*animationProgress_ + w1);

    QColor fgColor = QColor(24, 34, 47);
    painter->setBrush(fgColor);
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(QRectF(curW, boundingRect().height()/2 - BUTTON_HEIGHT/2*G_SCALE, BUTTON_WIDTH*G_SCALE, BUTTON_HEIGHT*G_SCALE));
}

void ToggleButton::setState(bool isChecked)
{
    if (isChecked != isChecked_) {
        opacityAnimation_.stop();

        isChecked_ = isChecked;
        if (isChecked) {
            animationProgress_= 1.0;
        } else {
            animationProgress_ = 0.0;
        }
        update();
    }
}

bool ToggleButton::isChecked() const
{
    return isChecked_;
}

void ToggleButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (pressed_)
        {
            pressed_ = false;
            if (contains(event->pos()))
            {
                if (!isToggleable_) {
                    emit toggleIgnored();
                    return;
                }

                isChecked_ = !isChecked_;
                emit stateChanged(isChecked_);

                if (isChecked_)
                {
                    opacityAnimation_.setDirection(QVariantAnimation::Forward);
                    if (opacityAnimation_.state() != QVariantAnimation::Running)
                    {
                        opacityAnimation_.start();
                    }
                }
                else
                {
                    opacityAnimation_.setDirection(QVariantAnimation::Backward);
                    if (opacityAnimation_.state() != QVariantAnimation::Running)
                    {
                        opacityAnimation_.start();
                    }
                }
            }
        }
    }
}

void ToggleButton::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    setCursor(Qt::PointingHandCursor);
    emit hoverEnter();
}

void ToggleButton::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    setCursor(Qt::ArrowCursor);
    emit hoverLeave();
}

void ToggleButton::onOpacityChanged(const QVariant &value)
{
    animationProgress_ = value.toDouble();
    update();
}

void ToggleButton::setEnabled(bool enabled)
{
    ClickableGraphicsObject::setEnabled(enabled);
    enabled_ = enabled;
    update();
}

void ToggleButton::setColor(const QColor &color)
{
    color_ = color;
    update();
}

void ToggleButton::setToggleable(bool toggleable)
{
    isToggleable_ = toggleable;
}