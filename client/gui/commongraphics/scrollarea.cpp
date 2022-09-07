#include "scrollarea.h"

#include <QPainter>
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "utils/logger.h"

// #include <QDebug>

namespace CommonGraphics {

ScrollArea::ScrollArea(ScalableGraphicsObject *parent, int height, int width) : ScalableGraphicsObject(parent)
  , curItem_(nullptr)
  , height_(height)
  , width_(width)
  , curScrollBarOpacity_(OPACITY_HIDDEN)
  , trackpadDeltaSum_(0)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape);

    const int scrollBarHeight = height_ - static_cast<int>(SCROLL_BAR_GAP*2.5*G_SCALE);

    scrollBar_ = new VerticalScrollBar(scrollBarHeight, 1.0, this);
    scrollBar_->setPos(width_*G_SCALE - SCROLL_DISTANCE_FROM_RIGHT*G_SCALE - scrollBar_->visibleWidth(), SCROLL_BAR_GAP*G_SCALE);
    scrollBar_->setZValue(1);
    scrollBar_->setOpacity(OPACITY_HIDDEN);
    connect(scrollBar_, &VerticalScrollBar::moved, this, &ScrollArea::onScrollBarMoved);
    connect(scrollBar_, &VerticalScrollBar::hoverEnter, this, &ScrollArea::onScrollBarHoverEnter);
    connect(scrollBar_, &VerticalScrollBar::hoverLeave, this, &ScrollArea::onScrollBarHoverLeave);

    connect(&itemPosAnimation_, &QVariantAnimation::valueChanged, this, &ScrollArea::onItemPosYChanged);
    connect(&scrollBarOpacityAnimation_, &QVariantAnimation::valueChanged, this, &ScrollArea::onScrollBarOpacityChanged);
}

QRectF ScrollArea::boundingRect() const
{
    return QRectF(0, 0, width_*G_SCALE, height_);
}

void ScrollArea::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter)
    Q_UNUSED(option)
    Q_UNUSED(widget)
}

void ScrollArea::setItemPosY(int newPosY)
{
    trackpadDeltaSum_ = 0;

    int lowestY = height_ - static_cast<int>(curItem_->boundingRect().height());
    if (newPosY < lowestY) newPosY = lowestY;
    if (newPosY > 0) newPosY = 0;

    curItem_->setY(newPosY);
    update();
}

void ScrollArea::setItem(BasePage *item)
{
    if (curItem_)
    {
        curItem_->hide();
        disconnect(curItem_, &BasePage::heightChanged, this, &ScrollArea::onPageHeightChange);
        disconnect(curItem_, &BasePage::scrollToPosition, this, &ScrollArea::onPageScrollToPosition);
        disconnect(curItem_, &BasePage::scrollToRect, this, &ScrollArea::onPageScrollToRect);
    }

    item->setParentItem(this);
    item->setFocus();
    item->show();
    curItem_ = item;
    connect(curItem_, &BasePage::heightChanged, this, &ScrollArea::onPageHeightChange);
    connect(curItem_, &BasePage::scrollToPosition, this, &ScrollArea::onPageScrollToPosition);
    connect(curItem_, &BasePage::scrollToRect, this, &ScrollArea::onPageScrollToRect);


    updateScrollBarByHeight();
}

double ScrollArea::screenFraction()
{
    double screenFraction = 1;

    int widgetHeight = static_cast<int>(curItem_->boundingRect().height());
    if (widgetHeight > height_) screenFraction = ( static_cast<double>(height_)/ widgetHeight);

    return screenFraction;
}

int ScrollArea::scaledThreshold()
{
    return static_cast<int>(TRACKPAD_DELTA_THRESHOLD*G_SCALE);
}

void ScrollArea::hideOpenPopups()
{
    // qCDebug(LOG_PREFERENCES) << "Hiding ScrollArea popups";

    curItem_->hideOpenPopups();
}

void ScrollArea::updateScaling()
{
    ScalableGraphicsObject::updateScaling();

    scrollBar_->setPos(width_*G_SCALE - SCROLL_DISTANCE_FROM_RIGHT*G_SCALE - scrollBar_->visibleWidth(), SCROLL_BAR_GAP*G_SCALE);
}

void ScrollArea::onScrollBarMoved(double posPercentY)
{
    int newPosY = - static_cast<int>(curItem_->boundingRect().height() * posPercentY);
    curItem_->setY(newPosY);
    update();
}

void ScrollArea::onItemPosYChanged(const QVariant &value)
{
    curItem_->setY(value.toInt());
    updateScrollBarByItem();
    update();
}

void ScrollArea::onPageHeightChange(int newHeight)
{
    Q_UNUSED(newHeight)

    const int pageHeight = curItem_->currentHeight();


    if ( pageHeight + curItem_->currentPosY() < height_)
    {
        int newPos = height_ - pageHeight;
        setItemPosY(newPos);
    }

    updateScrollBarByHeight();
}

void ScrollArea::setScrollPos(int amt)
{
    curItem_->setY(curItem_->y() + amt*G_SCALE);
    updateScrollBarByItem();
    update();
}

void ScrollArea::onPageScrollToPosition(int itemPos)
{
    trackpadDeltaSum_ = 0;

    int posWrtScrollArea = curItem_->currentPosY() + itemPos;

    if (posWrtScrollArea > 0)
    {
        int overBy = height_ - posWrtScrollArea;

        if (overBy < 0)
        {
            startAnAnimation(itemPosAnimation_, static_cast<int>(curItem_->y()), curItem_->currentPosY() + overBy, 200);
        }
    }
    else
    {
        startAnAnimation<int>(itemPosAnimation_, static_cast<int>(curItem_->y()), curItem_->currentPosY() - posWrtScrollArea, 200);
    }
}

void ScrollArea::onPageScrollToRect(QRect r)
{
    trackpadDeltaSum_ = 0;

    int posWrtScrollArea = curItem_->currentPosY() + r.y();

    if (posWrtScrollArea > 0) // item in or below the view
    {
        int overBy = height_ - posWrtScrollArea - r.height();

        if (overBy < 0) // item is below the view
        {
            startAnAnimation(itemPosAnimation_, static_cast<int>(curItem_->y()), curItem_->currentPosY() + overBy, 200);
        }
    }
    else // item is above the view
    {
        startAnAnimation<int>(itemPosAnimation_, static_cast<int>(curItem_->y()), curItem_->currentPosY() - posWrtScrollArea, 200);
    }
}

void ScrollArea::onScrollBarOpacityChanged(const QVariant &value)
{
    curScrollBarOpacity_ = value.toDouble();
    scrollBar_->setOpacity(curScrollBarOpacity_);
}

void ScrollArea::onScrollBarHoverEnter()
{
    if (scrollBarVisible_)
    {
        startAnAnimation<double>(scrollBarOpacityAnimation_, curScrollBarOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
    }
}

void ScrollArea::onScrollBarHoverLeave()
{
    if (scrollBarVisible_)
    {
        startAnAnimation<double>(scrollBarOpacityAnimation_, curScrollBarOpacity_, OPACITY_TWO_THIRDS, ANIMATION_SPEED_FAST);
    }
}

void ScrollArea::setHeight(int height)
{
    int change = abs(height_ - height);

    prepareGeometryChange();
    height_ = height;

    updateScrollBarByHeight();

    // if top region occluded then bring item down with window
    if (curItem_->currentPosY() < 0)
    {
        int newY = curItem_->currentPosY() - change;
        setItemPosY(newY);
    }
}

void ScrollArea::setScrollBarVisibility(bool on)
{
    if (on)
    {
        scrollBarVisible_ = true;
        if (screenFraction() < 1)
        {
            startAnAnimation<double>(scrollBarOpacityAnimation_, curScrollBarOpacity_, OPACITY_TWO_THIRDS, ANIMATION_SPEED_FAST);
        }

    }
    else
    {
        curScrollBarOpacity_ = OPACITY_HIDDEN;
        scrollBar_->setOpacity(curScrollBarOpacity_);

        scrollBarVisible_ = false;
    }

}

bool ScrollArea::hasItemWithFocus()
{
    return curItem_->hasItemWithFocus();
}

void ScrollArea::wheelEvent(QGraphicsSceneWheelEvent *event)
{
//    qDebug() << "ScrollArea::wheelEvent,"
//             << " spontaneous: " << event->spontaneous()
//             << ", mouse buttons: " << event->buttons()
//             << ", delta: " << event->delta()
//             << ", scenePos: " << event->scenePos()
//             << ", screenPos: " << event->screenPos()
//             << ", pos: " << event->pos()
//             << ", wheel orietnation: " << event->orientation()
//             << ", widget: " << event->widget();

    // event->delta() may have different value depending on source device
    // mouse -> always will have a single +/- 120 value come through
    // trackpad -> single +/-120 or many +/-smallValues (depending on device driver)
    // Either behaviour is fine, just needs to be handled differently
    if (abs(event->delta()) == 120) // probably mouseWheel, but some trackpads too
    {
        int change = scaledThreshold();
        if (event->delta() < 0) change = -scaledThreshold();

        int newY = static_cast<int>(curItem_->y() + change);
        int lowestY = height_ - static_cast<int>(curItem_->boundingRect().height());
        if (newY < lowestY) newY = lowestY;
        if (newY > 0) newY = 0;

        startAnAnimation(itemPosAnimation_, static_cast<int>(curItem_->y()), newY, ANIMATION_SPEED_FAST);
    }
    else // definitely trackpad
    {
        trackpadDeltaSum_ += event->delta();

        int change = 0;
        if (trackpadDeltaSum_ > scaledThreshold())
        {
            change = scaledThreshold();
            trackpadDeltaSum_ -= scaledThreshold();
        }
        else if (trackpadDeltaSum_ < -scaledThreshold())
        {
            change = -scaledThreshold();
            trackpadDeltaSum_ += scaledThreshold();
        }

        if (change != 0)
        {
            int newY = static_cast<int>(curItem_->y() + change);

            int lowestY = height_ - static_cast<int>(curItem_->boundingRect().height());
            if (newY < lowestY) newY = lowestY;
            if (newY > 0) newY = 0;

            startAnAnimation(itemPosAnimation_, static_cast<int>(curItem_->y()), newY, ANIMATION_SPEED_FAST);
        }
    }

    curItem_->hideOpenPopups();
}

void ScrollArea::updateScrollBarByHeight()
{
    trackpadDeltaSum_ = 0;
    double sf = screenFraction();

    if (scrollBarVisible_)
    {
        if (sf < 1)
        {
            scrollBar_->setOpacity(OPACITY_TWO_THIRDS);
        }
        else
        {
            scrollBar_->setOpacity(OPACITY_HIDDEN);
        }
    }

    const int scrollBarHeight = height_ - static_cast<int>(SCROLL_BAR_GAP*2.5*G_SCALE);
    scrollBar_->setHeight(scrollBarHeight, sf);
    updateScrollBarByItem();
}

void ScrollArea::updateScrollBarByItem()
{
    double posYPercent = static_cast<double>(curItem_->y()) / curItem_->boundingRect().height();
    scrollBar_->moveBarToPercentPos(posYPercent);
}

} // namespace CommonGraphics
