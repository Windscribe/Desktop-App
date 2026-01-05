#include "resizebar.h"

#include <QPainter>
#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "graphicresources/imageresourcessvg.h"

namespace CommonGraphics {

ResizeBar::ResizeBar(ScalableGraphicsObject *parent) : ScalableGraphicsObject(parent),
    bDragPressed_(false),
    iconOpacity_(0.5)
{
    setCursor(Qt::SizeVerCursor);
    setAcceptHoverEvents(true);

    connect(&opacityAnimation_, &QVariantAnimation::valueChanged, this, &ResizeBar::onOpacityChanged);
}

QRectF ResizeBar::boundingRect() const
{
    return QRectF(0, 0, 30*G_SCALE, 9*G_SCALE);
}

QRectF ResizeBar::getIconRect() const
{
    QSharedPointer<IndependentPixmap> footerIcon = ImageResourcesSvg::instance().getIndependentPixmap("FOOTER");
    qreal x = boundingRect().center().x() - footerIcon->width() / 2;
    qreal y = boundingRect().center().y() - footerIcon->height() / 2;
    return QRectF(x, y, footerIcon->width(), footerIcon->height());
}

void ResizeBar::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QSharedPointer<IndependentPixmap> footerIcon = ImageResourcesSvg::instance().getIndependentPixmap("FOOTER");
    double initOpacity = painter->opacity();
    painter->setOpacity(painter->opacity() * iconOpacity_);
    footerIcon->draw(boundingRect().center().x() - footerIcon->width() / 2,
                     boundingRect().center().y() - footerIcon->height() / 2,
                     painter);
    painter->setOpacity(initOpacity);
}

void ResizeBar::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    bDragPressed_ = true;
    dragPressPt_ = event->screenPos();
    emit resizeStarted();
}

void ResizeBar::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (bDragPressed_)
    {
        QPoint pt = event->screenPos();
        int y_offs = (pt.y() - dragPressPt_.y());
        emit resizeChange(y_offs);
    }
}

void ResizeBar::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);

    if (bDragPressed_)
    {
        emit resizeFinished();
    }
    bDragPressed_ = false;
}

void ResizeBar::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    startAnAnimation<double>(opacityAnimation_, iconOpacity_, 1.0, ANIMATION_SPEED_FAST);
}

void ResizeBar::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
}

void ResizeBar::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    startAnAnimation<double>(opacityAnimation_, iconOpacity_, 0.5, ANIMATION_SPEED_FAST);
}

void ResizeBar::onOpacityChanged(const QVariant &value)
{
    iconOpacity_ = value.toDouble();
    update();
}

} // namespace CommonGraphics
