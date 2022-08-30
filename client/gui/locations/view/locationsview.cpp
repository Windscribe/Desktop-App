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

    scrollBar_->setSingleStep(qCeil(LOCATION_ITEM_HEIGHT * G_SCALE));

    // widget
    widget_ = new ExpandableItemsWidget(nullptr, model);
    setWidget(widget_);
    countryItemDelegate_ = new CountryItemDelegate();
    cityItemDelegate_ = new CityItemDelegate();
    widget_->setItemDelegate(countryItemDelegate_, cityItemDelegate_);
    widget_->setItemHeight(qCeil(LOCATION_ITEM_HEIGHT * G_SCALE));
    connect(widget_, &ExpandableItemsWidget::ensureVisible, this, &LocationsView::onEnsureVisible);
    connect(widget_, &ExpandableItemsWidget::expandingAnimationStarted, [this]() {
        isExpandingAnimationForbidden_ = false;
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
    scrollBar_->setFixedWidth(SCROLL_BAR_WIDTH * G_SCALE);
}


void LocationsView::onEnsureVisible(int top, int bottom)
{
    if (!isExpandingAnimationForbidden_)
    {
        if (bottom > (lastScrollTargetPos_ + viewport()->height())) {
            lastScrollTargetPos_ = qMin(bottom - viewport()->height(), scrollBar_->maximum());
            lastScrollTargetPos_ = qMin(lastScrollTargetPos_, top);
            scrollBar_->setValue(lastScrollTargetPos_, true);
        }
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

void LocationsView::adjustPosBetweenMinAndMax(int &pos)
{
    if (pos < 0)
        pos = 0;
    if (pos > scrollBar_->maximum())
        pos = scrollBar_->maximum();
}


} // namespace gui_locations

