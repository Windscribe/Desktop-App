#include "verticalscrollbar.h"

#include <QPainter>
#include <QGraphicsSceneEvent>
#include "CommonGraphics/commongraphics.h"

#include "dpiscalemanager.h"

VerticalScrollBar::VerticalScrollBar(int height, double barPortion, ScalableGraphicsObject *parent) : ScalableGraphicsObject(parent)
{
    height_ = height;

    drawWidth_ = SCROLL_WIDTH/2;
    drawWidthPosY_ = static_cast<int>( (static_cast<double>( SCROLL_WIDTH )/ 2 - static_cast<double>(drawWidth_) / 2) );

    curBarPosY_ = 0;
    curBarHeight_ = static_cast<int>(height * barPortion);

    mouseOnClickY_ = 0;

    connect(&barPosYAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onBarPosYChanged(QVariant)));
}

QRectF VerticalScrollBar::boundingRect() const
{
    return QRectF(0, 0, SCROLL_WIDTH*G_SCALE, height_);
}

void VerticalScrollBar::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initOpacity = painter->opacity();

    painter->setRenderHint(QPainter::Antialiasing, true);

    // draw background line
    painter->setPen(Qt::white);
    painter->setOpacity(OPACITY_UNHOVER_ICON_TEXT * initOpacity);
    QRectF rect(drawWidthPosY_*G_SCALE, 1*G_SCALE, drawWidth_*G_SCALE, height_);
    painter->drawRoundedRect(rect, 5*G_SCALE, 5*G_SCALE);

    // Foreground lin
    painter->setPen(Qt::white);
    painter->setBrush(Qt::white);
    painter->setOpacity(OPACITY_FULL * initOpacity);
    QRectF rect2(drawWidthPosY_*G_SCALE, curBarPosY_+1*G_SCALE, drawWidth_*G_SCALE, curBarHeight_);
    painter->drawRoundedRect(rect2, 5*G_SCALE, 5*G_SCALE);

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

bool VerticalScrollBar::inBarRegion( int y)
{
    bool inside = false;
    if (y >= curBarPosY_ && y <= (curBarPosY_ + curBarHeight_))
    {
        inside = true;
    }

    return inside;
}


