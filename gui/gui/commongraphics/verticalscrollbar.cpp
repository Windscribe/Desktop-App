#include "verticalscrollbar.h"

#include <QPainter>
#include <QGraphicsSceneEvent>
#include <QCursor>
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"

// #include <QDebug>

VerticalScrollBar::VerticalScrollBar(int height, double barPortion, ScalableGraphicsObject *parent)
    : ScalableGraphicsObject(parent),
      height_(height),
      curBarPosY_(0),
      curBarHeight_(static_cast<int>(height * barPortion)),
      mouseHold_(false),
      mouseOnClickY_(0)
{
    setAcceptHoverEvents(true);
    connect(&barPosYAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onBarPosYChanged(QVariant)));
}

QRectF VerticalScrollBar::boundingRect() const
{
    return QRectF(0, 0, SCROLL_WIDTH*G_SCALE, height_);
}

void VerticalScrollBar::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    qreal initOpacity = painter->opacity();
    painter->setRenderHint(QPainter::Antialiasing, true);

    // draw background line
    painter->setPen(Qt::white);
    painter->setOpacity(OPACITY_FULL * initOpacity);
    QRectF bgRect(0*G_SCALE, 1*G_SCALE, visibleWidth(), height_);
    painter->fillRect(bgRect, FontManager::instance().getScrollBarBackgroundColor());

    // Foreground handle
    painter->setOpacity(OPACITY_FULL * initOpacity);
    QRectF fgRect(0*G_SCALE, curBarPosY_+1*G_SCALE, visibleWidth(), curBarHeight_);
    painter->fillRect(fgRect, Qt::white);
}

void VerticalScrollBar::setHeight(int height, double barPortion)
{
    prepareGeometryChange();
    height_ = height;

    curBarHeight_ = static_cast<int>( (height * barPortion) );
}

void VerticalScrollBar::moveBarToPercentPos(double posPercentY)
{
    int newPos = static_cast<int>(-posPercentY * height_);

    // keep bar in window
    if (newPos < 0) newPos = 0;
    if (newPos > height_ - curBarHeight_) newPos = height_ - curBarHeight_;

    curBarPosY_ = newPos;
    update();
}

int VerticalScrollBar::visibleWidth()
{
    return static_cast<int>(static_cast<double>(SCROLL_WIDTH)/2 * G_SCALE);
}

void VerticalScrollBar::onBarPosYChanged(const QVariant &value)
{
    curBarPosY_ = value.toInt();

    double percentPos = static_cast<double>( curBarPosY_) / height_;
    emit moved(percentPos);
    update();
}

void VerticalScrollBar::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    int y = static_cast<int>( event->pos().y() );

    mouseOnClickY_ = y;

    if (inBarRegion(y))
    {
        mouseHold_ = true;
    }
    else
    {
        int diff = - height_/8;
        if (y > curBarPosY_) diff = height_/8;

        int newPos = diff + curBarPosY_;

        // keep bar in window
        if (newPos < 0) newPos = 0;
        if (newPos > height_ - curBarHeight_) newPos = height_ - curBarHeight_;

        startAnAnimation(barPosYAnimation_, curBarPosY_, newPos, ANIMATION_SPEED_FAST);
    }
}

void VerticalScrollBar::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);

    mouseHold_ = false;

}

void VerticalScrollBar::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (mouseHold_)
    {
        int y = static_cast<int>( event->pos().y() );
        int delta =  y - mouseOnClickY_;

        int potentialNewPos = curBarPosY_ + delta;

        // keep bar in window
        if (potentialNewPos < 0) potentialNewPos = 0;
        if (potentialNewPos > height_ - curBarHeight_) potentialNewPos = height_ - curBarHeight_;

        if (potentialNewPos != curBarPosY_)
        {
            curBarPosY_ = potentialNewPos;

            double percentPos = static_cast<double>( curBarPosY_ ) / height_;
            emit moved(percentPos);
        }

        mouseOnClickY_ = y;

        update();
    }
}

void VerticalScrollBar::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event)
    setCursor(Qt::PointingHandCursor);
    emit hoverEnter();
}

void VerticalScrollBar::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event)
    setCursor(Qt::ArrowCursor);
    emit hoverLeave();
}

bool VerticalScrollBar::inBarRegion( int y)
{
    bool inside = false;
    if (y >= curBarPosY_ && y <= (curBarPosY_ + curBarHeight_))
    {
        inside = true;
    }

    return inside;
}


