#include "locationsview.h"

#include <QPainter>
#include <QEvent>
#include <QScroller>
#include <QWheelEvent>

#include "dpiscalemanager.h"
#include "commongraphics/commongraphics.h"

namespace gui_locations {

LocationsView::LocationsView(QWidget *parent, QAbstractItemModel *model) : QScrollArea(parent)
  , lastScrollTargetPos_(0)
  , isExpandingAnimationForbidden_(false)
{
    setFrameStyle(QFrame::NoFrame);

    // scrollbar
    scrollBar_ = new ScrollBar(this);
    setVerticalScrollBar(scrollBar_);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(scrollBar_, &ScrollBar::sliderMoved, [this](int value) {
        lastScrollTargetPos_ = value;
    });
    connect(scrollBar_, &ScrollBar::actionTriggered, this, &LocationsView::onScrollBarActionTriggered);
    connect(scrollBar_, &ScrollBar::rangeChanged, [this](int min, int max) {
        adjustPosBetweenMinAndMax(lastScrollTargetPos_);
    });
    connect(scrollBar_, &ScrollBar::valueChanged, [this]() {
        widget_->updateSelectedItem();
    });

    scrollBar_->setSingleStep(qCeil(LOCATION_ITEM_HEIGHT * G_SCALE));

    // widget
    widget_ = new ExpandableItemsWidget(nullptr, model);
    setWidget(widget_);
    countryItemDelegate_ = new CountryItemDelegate();
    cityItemDelegate_ = new CityItemDelegate();
    widget_->setItemDelegate(countryItemDelegate_, cityItemDelegate_);
    widget_->setItemHeight(qCeil(LOCATION_ITEM_HEIGHT * G_SCALE));
    connect(widget_, &ExpandableItemsWidget::expandingAnimationStarted, this, &LocationsView::onExpandingAnimationStarted);
    connect(widget_, &ExpandableItemsWidget::expandingAnimationProgress, this, &LocationsView::onExpandingAnimationProgress);
    connect(widget_, &ExpandableItemsWidget::emptyListStateChanged, [this](bool isEmptyList) {
       emit  emptyListStateChanged(isEmptyList);
    });
}

LocationsView::~LocationsView()
{
    delete countryItemDelegate_;
    delete cityItemDelegate_;
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

void LocationsView::scrollToTop()
{
    lastScrollTargetPos_ = 0;
    scrollBar_->setValue(lastScrollTargetPos_, true);
}

bool LocationsView::eventFilter(QObject *object, QEvent *event)
{
    if (object == scrollBar_ && event->type() == QEvent::Wheel)
    {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent *>(event);
        lastScrollTargetPos_ -= wheelEvent->angleDelta().y();
        adjustPosBetweenMinAndMax(lastScrollTargetPos_);
        scrollBar_->setValue(lastScrollTargetPos_);
        isExpandingAnimationForbidden_ = true;
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
    scrollBar_->setFixedWidth(kScrollBarWidth * G_SCALE);
}


void LocationsView::ensureVisible(int top, int bottom)
{
    if (bottom > (lastScrollTargetPos_ + viewport()->height())) {
        lastScrollTargetPos_ = qMin(bottom - viewport()->height(), scrollBar_->maximum());
        lastScrollTargetPos_ = qMin(lastScrollTargetPos_, top);
        adjustPosBetweenMinAndMax(lastScrollTargetPos_);
        scrollBar_->setValue(lastScrollTargetPos_, true);
    }
}

void LocationsView::onScrollBarActionTriggered(int action)
{
    switch (action)
    {
        case QAbstractSlider::SliderSingleStepAdd:
        case QAbstractSlider::SliderSingleStepSub:
        case QAbstractSlider::SliderPageStepAdd:
        case QAbstractSlider::SliderPageStepSub:
        case QAbstractSlider::SliderToMinimum:
        case QAbstractSlider::SliderToMaximum:
            lastScrollTargetPos_ = scrollBar_->sliderPosition();
            scrollBar_->setValue(lastScrollTargetPos_);
            isExpandingAnimationForbidden_ = true;
            break;
        case QAbstractSlider::SliderMove:
            isExpandingAnimationForbidden_ = true;
            break;
    }
}

void LocationsView::onExpandingAnimationStarted(int top, int height)
{
    isExpandingAnimationForbidden_ = false;
    expandingAnimationParams_.top = top;
    expandingAnimationParams_.height = height;
    int topItemInvisiblePart = (top + qCeil(LOCATION_ITEM_HEIGHT * G_SCALE)) - (lastScrollTargetPos_ + viewport()->height());
    expandingAnimationParams_.topItemInvisiblePart = topItemInvisiblePart > 0 ? topItemInvisiblePart : 0;
}

void LocationsView::onExpandingAnimationProgress(qreal progress)
{
    if (!isExpandingAnimationForbidden_)
    {
        int topItemVisibleHeight = qCeil(LOCATION_ITEM_HEIGHT * G_SCALE) - expandingAnimationParams_.topItemInvisiblePart;
        ensureVisible(expandingAnimationParams_.top, (expandingAnimationParams_.height + expandingAnimationParams_.topItemInvisiblePart) * progress
                                                      + expandingAnimationParams_.top + topItemVisibleHeight);
    }
}

void LocationsView::adjustPosBetweenMinAndMax(int &pos)
{
    if (pos < 0)
        pos = 0;
    if (pos > scrollBar_->maximum())
        pos = scrollBar_->maximum();
}

} // namespace gui_locations

