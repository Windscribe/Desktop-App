#include "bottomresizeitem.h"

#include <QPainter>
#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include "dpiscalemanager.h"

namespace PreferencesWindow {

BottomResizeItem::BottomResizeItem(ScalableGraphicsObject *parent) : ScalableGraphicsObject(parent),
    bDragPressed_(false)
{
    setCursor(Qt::SizeVerCursor);
}

QRectF BottomResizeItem::boundingRect() const
{
    return QRectF(0, 0, 30*G_SCALE, 9*G_SCALE);
}

void BottomResizeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QRectF rc = boundingRect().adjusted(3*G_SCALE, 3*G_SCALE, -3*G_SCALE, -3*G_SCALE);

    painter->fillRect(rc.adjusted(0, 0, 0, -2*G_SCALE), QBrush(QColor(53, 61, 74)));
    painter->fillRect(rc.adjusted(0, 1*G_SCALE, 0, -1*G_SCALE), QBrush(QColor(93, 100, 110)));
    painter->fillRect(rc.adjusted(0, 2*G_SCALE, 0, 0), QBrush(QColor(79, 87, 97)));
}

void BottomResizeItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    bDragPressed_ = true;
    dragPressPt_ = event->screenPos();
    emit resizeStarted();
}

void BottomResizeItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (bDragPressed_)
    {
        QPoint pt = event->screenPos();
        int y_offs = (pt.y() - dragPressPt_.y());
        emit resizeChange(y_offs);
    }
}

void BottomResizeItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);

    if (bDragPressed_)
    {
        emit resizeFinished();
    }
    bDragPressed_ = false;
}

} // namespace PreferencesWindow
