#include "locationsview.h"

#include <QPainter>
#include <QEvent>

#include "dpiscalemanager.h"
#include "commongraphics/commongraphics.h"

namespace gui_locations {

LocationsView::LocationsView(QWidget *parent) : QScrollArea(parent)
  , animationScollTarget_(0)
  , lastScrollPos_(0)
{
    setFrameStyle(QFrame::NoFrame);

    // scrollbar
    scrollBar_ = new ScrollBar(this);
    setVerticalScrollBar(scrollBar_);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollBar_->setSingleStep(qCeil(LOCATION_ITEM_HEIGHT * G_SCALE)); // scroll by this many px at a time
    connect(scrollBar_, &ScrollBar::handleDragged, this, &LocationsView::onScrollBarHandleDragged);
    connect(scrollBar_, &ScrollBar::stopScroll, this, &LocationsView::onScrollBarStopScroll);

    // widget
    widget_ = new ExpandableItemsWidget();
    setWidget(widget_);
    countryItemDelegate_ = new CountryItemDelegate();
    cityItemDelegate_ = new CityItemDelegate();
    widget_->setItemDelegate(countryItemDelegate_, cityItemDelegate_);
    widget_->setItemHeight(qCeil(LOCATION_ITEM_HEIGHT * G_SCALE));
    connect(widget_, &ExpandableItemsWidget::notifyMustBeVisible, this, &LocationsView::onNotifyMustBeVisible);

    connect(&scrollAnimation_, &QVariantAnimation::valueChanged, this, &LocationsView::onScrollAnimationValueChanged);
    connect(&scrollAnimation_, &QVariantAnimation::finished, this, &LocationsView::onScrollAnimationFinished);
    //connect(&scrollAnimationForKeyPress_, SIGNAL(valueChanged(QVariant)), SLOT(onScrollAnimationForKeyPressValueChanged(QVariant)));
    //connect(&scrollAnimationForKeyPress_, SIGNAL(finished()), SLOT(onScrollAnimationForKeyPressFinished()));
}

LocationsView::~LocationsView()
{
    delete countryItemDelegate_;
    delete cityItemDelegate_;
}

void LocationsView::setModel(QAbstractItemModel *model)
{
    widget_->setModel(model);
}

void LocationsView::setShowLatencyInMs(bool isShowLatencyInMs)
{
    widget_->setShowLatencyInMs(isShowLatencyInMs);
}

void LocationsView::setShowLocationLoad(bool isShowLocationLoad)
{
    widget_->setShowLocationLoad(isShowLocationLoad);
}

void LocationsView::updateScaling()
{
    //todo
    scrollBar_->updateCustomStyleSheet();
}

bool LocationsView::eventFilter(QObject *object, QEvent *event)
{
    // order of events:
    //      scroll: prepare -> start -> scroll -> canceled
    //      tap   : prepate -> start -> finished
    if (object == viewport() && event->type() == QEvent::Gesture)
    {
        //FIXME:
        /*QGestureEvent *ge = static_cast<QGestureEvent *>(event);
        QTapGesture *g = static_cast<QTapGesture *>(ge->gesture(Qt::TapGesture));
        if (g)
        {
            if (g->state() == Qt::GestureStarted)
            {
                //qCDebug(LOG_USER) << "Tap Gesture started";
                bTapGestureStarted_ = true;
            }
            else if (g->state() == Qt::GestureFinished)
            {
                if (bTapGestureStarted_)
                {
                    // this will only run on a quick tap
                    bTapGestureStarted_ = false;
                    QPointF ptf = g->position();
                    QPoint pt(ptf.x(), ptf.y());
                    //qCDebug(LOG_USER) << "Tap Gesture finished, selecting by pt: " << pt;
                    widgetLocationsList_->selectWidgetContainingGlobalPt(pt);
                }
            }
            else if (g->state() == Qt::GestureCanceled)
            {
                // this will run if scrolling gestures start coming through
                //qCDebug(LOG_USER) << "Tap Gesture canceled - scrolling";
                bTapGestureStarted_ = false;
            }
        }
        QPanGesture *gp = static_cast<QPanGesture *>(ge->gesture(Qt::PanGesture));
        if (gp)
        {
            //qCDebug(LOG_USER) << "Pan gesture";
            if (gp->state() == Qt::GestureStarted)
            {
                //qCDebug(LOG_USER) << "Pan gesture started";
                bTapGestureStarted_ = false;
            }
        }*/
        return true;
    }
    else if (object == viewport() && event->type() == QEvent::ScrollPrepare)
    {
        // runs before tap and hold/scroll
        /*QScrollPrepareEvent *se = static_cast<QScrollPrepareEvent *>(event);
        se->setViewportSize(QSizeF(viewport()->size()));
        se->setContentPosRange(QRectF(0, 0, 1, scrollBar_->maximum()));
        se->setContentPos(QPointF(0, scrollBar_->value()));
        se->accept();*/
        return true;
    }
    else if (object == viewport() && event->type() == QEvent::Scroll)
    {
        // runs while scrolling
        /*QScrollEvent *se = static_cast<QScrollEvent *>(event);
        int scrollGesturePos = se->contentPos().y();
        gestureScrollingElapsedTimer_.restart();
        int notchedGesturePos = closestPositionIncrement(scrollGesturePos);
        gestureScrollAnimation(notchedGesturePos);
        widgetLocationsList_->accentWidgetContainingCursor();*/
        return true;
    }

    return QScrollArea::eventFilter(object, event);
}

void LocationsView::paintEvent(QPaintEvent */*event*/)
{
    QPainter painter(viewport());
    QRect bkgd(0,0,geometry().width(), geometry().height());
    painter.fillRect(bkgd, QColor(255, 0 ,0));
}

void LocationsView::resizeEvent(QResizeEvent *event)
{
    QScrollArea::resizeEvent(event);
    widget_->setFixedWidth(size().width());
    scrollBar_->setFixedWidth(SCROLL_BAR_WIDTH * G_SCALE);
}

void LocationsView::scrollContentsBy(int dx, int dy)
{
    //FIXME:
    ///TooltipController::instance().hideAllTooltips();

    if (!scrollBar_->dragging())
    {
        // cursor should not interfere when animation is running
        // prevent floating cursor from overriding a keypress animation
        // this can only occur when accessibility is disabled (since cursor will not be moved)
        //FIXME:
        /*if (cursorInViewport() && preventMouseSelectionTimer_.elapsed() > 100)
        {
            widgetLocationsList_->accentWidgetContainingCursor();
        }*/

        //if (!heightChanging_) // prevents false-positive scrolling when changing scale -- setting widgetLocationsList_ geometry in the heightChanged slot races with the manual forceSetValue
        {
            QScrollArea::scrollContentsBy(dx,dy);
            lastScrollPos_ = widget_->geometry().y();
        }
    }

}

void LocationsView::onScrollBarHandleDragged(int valuePos)
{
    animationScollTarget_ = -valuePos;
    startAnimationScrollByPosition(animationScollTarget_, scrollAnimation_);
}

void LocationsView::onScrollBarStopScroll(bool lastScrollDirectionUp)
{
    if (lastScrollDirectionUp)
    {
        int nextPos = previousPositionIncrement(-widget_->geometry().y());
        animationScollTarget_ = -nextPos;
        startAnimationScrollByPosition(animationScollTarget_, scrollAnimation_);
    }
    else
    {
        int nextPos = nextPositionIncrement(-widget_->geometry().y());
        animationScollTarget_ = -nextPos;
        startAnimationScrollByPosition(animationScollTarget_, scrollAnimation_);
    }
}

void LocationsView::onScrollAnimationValueChanged(const QVariant &value)
{
    widget_->move(0, value.toInt());
    lastScrollPos_ = widget_->geometry().y();
    viewport()->update();
}

void LocationsView::onScrollAnimationFinished()
{
    updateScrollBarWithView();
}

void LocationsView::onNotifyMustBeVisible(int topItemIndex, int bottomItemIndex)
{
    int countVisibleItems = geometry().height() / qCeil(LOCATION_ITEM_HEIGHT * G_SCALE);
    int currentTopItemInd = -lastScrollPos_ /  qCeil(LOCATION_ITEM_HEIGHT * G_SCALE);

    if (bottomItemIndex >= (currentTopItemInd + countVisibleItems))
    {
        int change = bottomItemIndex - (currentTopItemInd + countVisibleItems) + 1;
        int newPos = currentTopItemInd + change;
        if (newPos > topItemIndex) {
            newPos = topItemIndex;
        }
        startAnimationScrollByPosition(-newPos*qCeil(LOCATION_ITEM_HEIGHT * G_SCALE), scrollAnimation_);
    }
}

void LocationsView::startAnimationScrollByPosition(int positionValue, QVariantAnimation &animation)
{
    animation.stop();
    animation.setDuration(PROGRAMMATIC_SCROLL_ANIMATION_DURATION);
    animation.setStartValue(widget_->geometry().y());
    animation.setEndValue(positionValue);
    animation.setDirection(QAbstractAnimation::Forward);
    animation.start();
}

int LocationsView::nextPositionIncrement(int value)
{
    int current = 0;
    while (current < value)
    {
        current += qCeil(LOCATION_ITEM_HEIGHT * G_SCALE);
    }

    return current;
}

int LocationsView::previousPositionIncrement(int value)
{
    int current = 0;
    int last = 0;
    while (current < value)
    {
        last = current;
        current += qCeil(LOCATION_ITEM_HEIGHT * G_SCALE);
    }
    return last;
}

void LocationsView::updateScrollBarWithView()
{
    if (!scrollBar_->dragging())
    {
        scrollBar_->forceSetValue(-widget_->geometry().y());
    }
}

} // namespace gui_locations

