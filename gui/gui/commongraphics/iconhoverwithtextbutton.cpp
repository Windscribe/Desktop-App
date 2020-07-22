#include "iconhoverwithtextbutton.h"

IconHoverWithTextButton::IconHoverWithTextButton(int width, int height, const QString &imagePath, QGraphicsObject * parent)
{

}

QRectF IconHoverWithTextButton::boundingRect() const
{
    return QRect(0, 0, width_, height_);
}

void IconHoverWithTextButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // paint pixmap

    // paint text
}

void IconHoverWithTextButton::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // change cursor

    emit clicked();
}

void IconHoverWithTextButton::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    // animate opacity button change

    // animation opacity text change
}

void IconHoverWithTextButton::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    // animate opacity button change

    // animate opacity text change
}
