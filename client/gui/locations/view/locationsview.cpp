#include "locationsview.h"

#include <QPainter>
#include <QEvent>
#include <QScroller>
#include <QWheelEvent>

#include "dpiscalemanager.h"
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"

namespace gui_locations {

LocationsView::LocationsView(QWidget *parent, QAbstractItemModel *model) : QScrollArea(parent)
{
    setFrameStyle(QFrame::NoFrame);

    // scrollbar
    scrollBar_ = new ScrollBar(this);
    setVerticalScrollBar(scrollBar_);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(scrollBar_, &ScrollBar::actionTriggered, this, &LocationsView::onScrollBarActionTriggered);
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
    //connect(widget_, &ExpandableItemsWidget::expandingAnimationProgress, this, &LocationsView::onExpandingAnimationProgress);
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
    scrollBar_->setValueWithoutAnimation(0);
}

bool LocationsView::eventFilter(QObject *object, QEvent *event)
{
    if (object == scrollBar_ && event->type() == QEvent::Wheel)
    {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent *>(event);
        // We process only real events from the mouse.
        // The event can also be from the touchpad - in this case, let's let it be processed in the ScrollBar.
        if (wheelEvent->source() == Qt::MouseEventNotSynthesized) {
            scrollBar_->scrollDx(-wheelEvent->angleDelta().y()/2);
            return true;
        }
    }

    return QScrollArea::eventFilter(object, event);
}

void LocationsView::paintEvent(QPaintEvent */*event*/)
{
    QPainter painter(viewport());
    QRect bkgd(0,0,geometry().width(), geometry().height());
    painter.fillRect(bkgd, FontManager::instance().getMidnightColor());
}

void LocationsView::resizeEvent(QResizeEvent *event)
{
    QScrollArea::resizeEvent(event);
    widget_->setFixedWidth(size().width() - kScrollBarWidth * G_SCALE);
    scrollBar_->setFixedWidth(kScrollBarWidth * G_SCALE);
}


void LocationsView::ensureVisible(int top, int bottom)
{
    if (bottom > (scrollBar_->value() + viewport()->height())) {
        int newPos = bottom - viewport()->height();
        newPos = qMin(newPos, top);
        scrollBar_->setValueWithAnimation(newPos);
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
            scrollBar_->setValueWithAnimation(scrollBar_->sliderPosition());
            break;
        case QAbstractSlider::SliderMove:
            scrollBar_->setValueWithoutAnimation(scrollBar_->sliderPosition());
            break;
    }
}

void LocationsView::onExpandingAnimationStarted(int top, int height)
{
    ensureVisible(top, height + top + qCeil(LOCATION_ITEM_HEIGHT * G_SCALE));
}

} // namespace gui_locations

