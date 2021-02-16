#include "scrollareaitem.h"

#include <QPainter>
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"

#include <QDebug>

namespace PreferencesWindow {

ScrollAreaItem::ScrollAreaItem(ScalableGraphicsObject *parent, int height) : ScalableGraphicsObject(parent)
  , curItem_(nullptr)
  , height_(height)
  , curScrollBarOpacity_(OPACITY_HIDDEN)
  , trackpadDeltaSum_(0)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape);

    const int scrollDistanceFromRight = 6;

    const int scrollBarHeight = height_ - static_cast<int>(SCROLL_BAR_GAP*2.5);

    scrollBar_ = new VerticalScrollBar(scrollBarHeight, 1.0, this);
    scrollBar_->setPos(PAGE_WIDTH - VerticalScrollBar::SCROLL_WIDTH - scrollDistanceFromRight, SCROLL_BAR_GAP);
    scrollBar_->setZValue(1);
    scrollBar_->setOpacity(OPACITY_HIDDEN);
    connect(scrollBar_, SIGNAL(moved(double)), SLOT(onScrollBarMoved(double)));

    connect(&itemPosAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onItemPosYChanged(QVariant)));
    connect(&scrollBarOpacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onScrollBarOpacityChanged(QVariant)));
}

QRectF ScrollAreaItem::boundingRect() const
{
    return QRectF(0, 0, WIDTH*G_SCALE, height_);
}

void ScrollAreaItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter)
    Q_UNUSED(option)
    Q_UNUSED(widget)
}

void ScrollAreaItem::setItemPosY(int newPosY)
{
    trackpadDeltaSum_ = 0;

    int lowestY = height_ - static_cast<int>(curItem_->boundingRect().height());
    if (newPosY < lowestY) newPosY = lowestY;
    if (newPosY > 0) newPosY = 0;

    curItem_->setY(newPosY);
    update();
}

void ScrollAreaItem::setItem(BasePage *item)
{
    if (curItem_)
    {
        curItem_->hide();
        disconnect(curItem_, SIGNAL(heightChanged(int)), this, SLOT(onPageHeightChange(int)));
        disconnect(curItem_, SIGNAL(scrollToPosition(int)), this, SLOT(onPageScrollToPosition(int)));
        disconnect(curItem_, SIGNAL(scrollToRect(QRect)), this, SLOT(onPageScrollToRect(QRect)));
    }

    item->setParentItem(this);
    item->setFocus();
    item->show();
    curItem_ = item;
    connect(curItem_, SIGNAL(heightChanged(int)), this, SLOT(onPageHeightChange(int)));
    connect(curItem_, SIGNAL(scrollToPosition(int)), this, SLOT(onPageScrollToPosition(int)));
    connect(curItem_, SIGNAL(scrollToRect(QRect)), this, SLOT(onPageScrollToRect(QRect)));


    updateScrollBarByHeight();
}

double ScrollAreaItem::screenFraction()
{
    double screenFraction = 1;

    int widgetHeight = static_cast<int>(curItem_->boundingRect().height());
    if (widgetHeight > height_) screenFraction = ( static_cast<double>(height_)/ widgetHeight);

    return screenFraction;
}

int ScrollAreaItem::scaledThreshold()
{
    return static_cast<int>(TRACKPAD_DELTA_THRESHOLD*G_SCALE);
}

void ScrollAreaItem::hideOpenPopups()
{
    curItem_->hideOpenPopups();
}

void ScrollAreaItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();

    const int scrollDistanceFromRight = 6;
    scrollBar_->setPos((PAGE_WIDTH - VerticalScrollBar::SCROLL_WIDTH - scrollDistanceFromRight)*G_SCALE, SCROLL_BAR_GAP*G_SCALE);
}

void ScrollAreaItem::onScrollBarMoved(double posPercentY)
{
    int newPosY = - static_cast<int>(curItem_->boundingRect().height() * posPercentY);
    curItem_->setY(newPosY);
    update();
}

void ScrollAreaItem::onItemPosYChanged(const QVariant &value)
{
    curItem_->setY(value.toInt());
    updateScrollBarByItem();
    update();
}

void ScrollAreaItem::onPageHeightChange(int newHeight)
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

void ScrollAreaItem::onPageScrollToPosition(int itemPos)
{
    trackpadDeltaSum_ = 0;

    int posWrtScrollArea = curItem_->currentPosY() + itemPos;

    if (posWrtScrollArea > 0)
    {
        int overBy = height_ - posWrtScrollArea;

        if (overBy < 0)
        {
            startAnAnimation(itemPosAnimation_, static_cast<int>(curItem_->y()), curItem_->currentPosY() + overBy, 150);
        }
    }
    else
    {
        startAnAnimation<int>(itemPosAnimation_, static_cast<int>(curItem_->y()), curItem_->currentPosY() - posWrtScrollArea, 150);
    }
}

void ScrollAreaItem::onPageScrollToRect(QRect r)
{
    trackpadDeltaSum_ = 0;

    int posWrtScrollArea = curItem_->currentPosY() + r.y();

    if (posWrtScrollArea > 0) // item in or below the view
    {
        int overBy = height_ - posWrtScrollArea - r.height();

        if (overBy < 0) // item is below the view
        {
            startAnAnimation(itemPosAnimation_, static_cast<int>(curItem_->y()), curItem_->currentPosY() + overBy, 150);
        }
    }
    else // item is above the view
    {
        startAnAnimation<int>(itemPosAnimation_, static_cast<int>(curItem_->y()), curItem_->currentPosY() - posWrtScrollArea, 150);
    }
}

void ScrollAreaItem::onScrollBarOpacityChanged(const QVariant &value)
{
    curScrollBarOpacity_ = value.toDouble();
    scrollBar_->setOpacity(curScrollBarOpacity_);
}

void ScrollAreaItem::setHeight(int height)
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

void ScrollAreaItem::setScrollBarVisibility(bool on)
{
    if (on)
    {
        scrollBarVisible_ = true;
        if (screenFraction() < 1)
        {
            startAnAnimation<double>(scrollBarOpacityAnimation_, curScrollBarOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
        }

    }
    else
    {
        curScrollBarOpacity_ = OPACITY_HIDDEN;
        scrollBar_->setOpacity(curScrollBarOpacity_);

        scrollBarVisible_ = false;
    }

}

void ScrollAreaItem::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    qDebug() << "ScrollAreaItem::wheelEvent,"
             << " spontaneous: " << event->spontaneous()
             << ", mouse buttons: " << event->buttons()
             << ", delta: " << event->delta()
             << ", scenePos: " << event->scenePos()
             << ", screenPos: " << event->screenPos()
             << ", pos: " << event->pos()
             << ", wheel orietnation: " << event->orientation()
             << ", widget: " << event->widget();

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

void ScrollAreaItem::updateScrollBarByHeight()
{
    trackpadDeltaSum_ = 0;
    double sf = screenFraction();

    if (scrollBarVisible_)
    {
        if (sf < 1)
        {
            scrollBar_->setOpacity(OPACITY_FULL);
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

void ScrollAreaItem::updateScrollBarByItem()
{
    double posYPercent = static_cast<double>(curItem_->y()) / curItem_->boundingRect().height();
    scrollBar_->moveBarToPercentPos(posYPercent);
}

} // namespace PreferencesWindow
