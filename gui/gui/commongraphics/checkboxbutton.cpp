#include "checkboxbutton.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QCursor>
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

CheckBoxButton::CheckBoxButton(ScalableGraphicsObject *parent) : ClickableGraphicsObject(parent),
    curOpacity_(0.0), isChecked_(false)
{
    setAcceptHoverEvents(true);

    connect(&opacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onOpacityChanged(QVariant)));
    opacityAnimation_.setStartValue(0.0);
    opacityAnimation_.setEndValue(1.0);
    opacityAnimation_.setDuration(150);

    setClickable(true);
}

QRectF CheckBoxButton::boundingRect() const
{
    return QRectF(0, 0, 30*G_SCALE, 16*G_SCALE);
}

void CheckBoxButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initialOpacity = painter->opacity();

    QSharedPointer<IndependentPixmap> pixmapGreen = ImageResourcesSvg::instance().getIndependentPixmap("preferences/GREEN_TOGGLE_BG");
    QSharedPointer<IndependentPixmap> pixmapWhite = ImageResourcesSvg::instance().getIndependentPixmap("preferences/WHITE_TOGGLE_BG");
    QSharedPointer<IndependentPixmap> pixmapButton = ImageResourcesSvg::instance().getIndependentPixmap("preferences/TOGGLE_BUTTON_BLACK");

    pixmapWhite->draw(0, 0, painter);

    painter->setOpacity(curOpacity_ * initialOpacity);
    pixmapGreen->draw(0, 0, painter);

    painter->setOpacity(1.0 * initialOpacity);

    int w1 = 2*G_SCALE;
    int w2 = static_cast<int>(boundingRect().width() - pixmapButton->width() - 2*G_SCALE);
    int curW = static_cast<int>((w2 - w1)*curOpacity_ + w1);
    pixmapButton->draw(curW, static_cast<int>((boundingRect().height() - pixmapButton->height()) / 2), painter);
}

void CheckBoxButton::setState(bool isChecked)
{
    if (isChecked != isChecked_)
    {
        isChecked_ = isChecked;
        if (isChecked)
        {
            curOpacity_ = 1.0;
        }
        else
        {
            curOpacity_ = 0.0;
        }
        update();
    }
}

bool CheckBoxButton::isChecked() const
{
    return isChecked_;
}

void CheckBoxButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (pressed_)
        {
            pressed_ = false;
            if (contains(event->pos()))
            {
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

void CheckBoxButton::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    setCursor(Qt::PointingHandCursor);
    emit hoverEnter();
}

void CheckBoxButton::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    setCursor(Qt::ArrowCursor);
    emit hoverLeave();
}

void CheckBoxButton::onOpacityChanged(const QVariant &value)
{
    curOpacity_ = value.toDouble();
    update();
}

} // namespace PreferencesWindow
