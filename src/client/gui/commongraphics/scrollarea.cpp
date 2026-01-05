#include "scrollarea.h"

#include <QCoreApplication>
#include <QGraphicsProxyWidget>
#include <QPainter>
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"

// #include <QDebug>

namespace CommonGraphics {

ScrollArea::ScrollArea(ScalableGraphicsObject *parent, int height, int width) : ScalableGraphicsObject(parent)
  , curItem_(nullptr)
  , height_(height)
  , width_(width)
  , curScrollBarOpacity_(OPACITY_HIDDEN)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape);

    scrollBar_ = new CommonWidgets::ScrollBar();
    connect(scrollBar_, &CommonWidgets::ScrollBar::valueChanged, this, &ScrollArea::onScrollBarValueChanged);
    connect(scrollBar_, &CommonWidgets::ScrollBar::actionTriggered, this, &ScrollArea::onScrollBarActionTriggered);

    scrollBarItem_ = new QGraphicsProxyWidget(this);
    scrollBarItem_->setWidget(scrollBar_);

    scrollBarItem_->setPos(width_*G_SCALE - SCROLL_DISTANCE_FROM_RIGHT*G_SCALE - static_cast<int>(6 * G_SCALE), SCROLL_BAR_GAP*G_SCALE);
    scrollBarItem_->setZValue(1);
    scrollBarItem_->setOpacity(OPACITY_HIDDEN);

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
    }

    scrollBar_->setValueWithoutAnimation(0);

    item->setParentItem(this);
    item->setFocus();
    item->show();
    curItem_ = item;
    connect(curItem_, &BasePage::heightChanged, this, &ScrollArea::onPageHeightChange);
    connect(curItem_, &BasePage::scrollToPosition, this, &ScrollArea::onPageScrollToPosition);

    updateScrollBarByHeight();
}

BasePage *ScrollArea::item()
{
    return curItem_;
}

double ScrollArea::screenFraction()
{
    double screenFraction = 1;

    int widgetHeight = static_cast<int>(curItem_->boundingRect().height());
    if (widgetHeight > height_) screenFraction = ( static_cast<double>(height_)/ widgetHeight);

    return screenFraction;
}

void ScrollArea::hideOpenPopups()
{
    curItem_->hideOpenPopups();
}

void ScrollArea::updateScaling()
{
    ScalableGraphicsObject::updateScaling();

    scrollBar_->setFixedWidth(kScrollBarWidth * G_SCALE);
    scrollBarItem_->setPos(width_*G_SCALE - SCROLL_DISTANCE_FROM_RIGHT*G_SCALE - static_cast<int>(6 * G_SCALE), SCROLL_BAR_GAP*G_SCALE);
}

bool ScrollArea::handleKeyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down
        || event->key() == Qt::Key_PageUp ||  event->key() == Qt::Key_PageDown) {
        QCoreApplication::sendEvent(scrollBar_, event);
        return true;
    }
    return false;
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

void ScrollArea::setScrollOffset(int amt)
{
    curItem_->setY(curItem_->y() + amt*G_SCALE);
    scrollBar_->setValueWithoutAnimation(-curItem_->y());
    update();
}


void ScrollArea::setScrollPos(int pos)
{
    int lowestY = height_ - static_cast<int>(curItem_->boundingRect().height());
    if (pos < lowestY) {
        pos = lowestY;
    }
    if (pos > 0) {
        pos = 0;
    }

    curItem_->setY(pos);
    scrollBar_->setValueWithoutAnimation(-curItem_->y());
    update();
}

void ScrollArea::onPageScrollToPosition(int itemPos)
{
    scrollBar_->setValueWithAnimation(itemPos);
}

void ScrollArea::onScrollBarValueChanged(int value)
{
    curItem_->setY(-value);
    update();
}

void ScrollArea::onScrollBarOpacityChanged(const QVariant &value)
{
    curScrollBarOpacity_ = value.toDouble();
    scrollBarItem_->setOpacity(curScrollBarOpacity_);
}

void ScrollArea::onScrollBarActionTriggered(int action)
{
    switch (action)
    {
        case QAbstractSlider::SliderSingleStepAdd:
        case QAbstractSlider::SliderSingleStepSub:
        case QAbstractSlider::SliderPageStepAdd:
        case QAbstractSlider::SliderPageStepSub:
            scrollBar_->setValueWithAnimation(scrollBar_->sliderPosition());
            break;
        case QAbstractSlider::SliderMove:
            scrollBar_->setValueWithoutAnimation(scrollBar_->sliderPosition());
            break;
    }
}

void ScrollArea::setHeight(int height)
{
    if (height_ != height) {
        int change = abs(height_ - height);

        prepareGeometryChange();
        height_ = height;

        updateScrollBarByHeight();

        // if top region occluded then bring item down with window
        if (curItem_ != nullptr && curItem_->currentPosY() < 0)
        {
            int newY = curItem_->currentPosY() - change;
            setItemPosY(newY);
        }
    }
}

void ScrollArea::setScrollBarVisibility(bool on)
{
    if (on) {
        scrollBarVisible_ = true;
        if (scrollBar_->maximum() != 0) {
            startAnAnimation<double>(scrollBarOpacityAnimation_, curScrollBarOpacity_, OPACITY_TWO_THIRDS, ANIMATION_SPEED_FAST);
        }
        //scrollBarItem_->setOpacity(OPACITY_TWO_THIRDS);
    } else {
        curScrollBarOpacity_ = OPACITY_HIDDEN;
        scrollBarItem_->setOpacity(OPACITY_HIDDEN);
        scrollBarVisible_ = false;
    }
}

bool ScrollArea::hasItemWithFocus()
{
    return curItem_->hasItemWithFocus();
}

void ScrollArea::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    // pass to the scrollbar
    QWheelEvent wheelEvent(event->pos(), event->screenPos(), event->pixelDelta(), QPoint(0, event->delta()),
                                              event->buttons(), event->modifiers(), event->phase(),
                                              event->isInverted(), Qt::MouseEventNotSynthesized, QPointingDevice::primaryPointingDevice());
    if (curItem_) {
        curItem_->hideOpenPopups();
    }
    QCoreApplication::sendEvent(scrollBar_, &wheelEvent);
}

void ScrollArea::keyPressEvent(QKeyEvent *event)
{
    if (!handleKeyPressEvent(event)) {
        QGraphicsObject::keyPressEvent(event);
    }
}

void ScrollArea::updateScrollBarByHeight()
{
    const int scrollBarHeight = height_ - static_cast<int>(SCROLL_BAR_GAP*2.5*G_SCALE);

    scrollBar_->setFixedHeight(scrollBarHeight);
    if (curItem_ != nullptr && (curItem_->boundingRect().height() - height_) > 0) {
        scrollBar_->setMinimum(0);
        scrollBar_->setMaximum(curItem_->boundingRect().height() - height_);
        scrollBar_->setPageStep(scrollBarHeight);
        scrollBar_->setSingleStep(50*G_SCALE);
        if (scrollBarVisible_) {
            scrollBarItem_->setOpacity(OPACITY_TWO_THIRDS);
        }
    } else {
        scrollBar_->setMinimum(0);
        scrollBar_->setMaximum(0);
        scrollBarItem_->setOpacity(OPACITY_HIDDEN);
    }
}

} // namespace CommonGraphics
