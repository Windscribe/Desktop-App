#include "verticaldividerline.h"

#include <QPainter>
#include "dpiscalemanager.h"

namespace CommonGraphics {

VerticalDividerLine::VerticalDividerLine(ScalableGraphicsObject *parent, int height) : ScalableGraphicsObject(parent), height_(height)
{

}

QRectF CommonGraphics::VerticalDividerLine::boundingRect() const
{
    return QRectF(0,0,3*G_SCALE, height_*G_SCALE);
}

void CommonGraphics::VerticalDividerLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->fillRect(boundingRect().adjusted(0, 0, -2*G_SCALE, 0), QBrush(QColor(13, 18, 36)));
    painter->fillRect(boundingRect().adjusted(1*G_SCALE, 0, -1*G_SCALE, 0), QBrush(QColor(40, 45, 61)));
    painter->fillRect(boundingRect().adjusted(2*G_SCALE, 0,  0, 0), QBrush(QColor(30, 36, 52)));
}

}
