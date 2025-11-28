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
    setFocusPolicy(Qt::NoFocus);
    viewport()->setFixedWidth(size().width());

    // scrollbar
    scrollBar_ = new CommonWidgets::ScrollBar(this);
    setVerticalScrollBar(scrollBar_);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(scrollBar_, &CommonWidgets::ScrollBar::actionTriggered, this, &LocationsView::onScrollBarActionTriggered);
    connect(scrollBar_, &CommonWidgets::ScrollBar::valueChanged, [this]() {
        if (isCursorInList()) {
            widget_->updateSelectedItemByCursorPos();
        }
    });
    connect(scrollBar_, &CommonWidgets::ScrollBar::rangeChanged, [this]() {
        if (isCursorInList()) {
            widget_->updateSelectedItemByCursorPos();
        }
    });

    scrollBar_->setSingleStep(qCeil(LOCATION_ITEM_HEIGHT * G_SCALE));

    // widget
    widget_ = new ExpandableItemsWidget(nullptr, model);
    setWidget(widget_);
    widget_->setFixedWidth(size().width());
    countryItemDelegate_ = new CountryItemDelegate();
    cityItemDelegate_ = new CityItemDelegate();
    widget_->setItemDelegate(countryItemDelegate_, cityItemDelegate_);
    widget_->setItemHeight(qCeil(LOCATION_ITEM_HEIGHT * G_SCALE));
    connect(widget_, &ExpandableItemsWidget::expandingAnimationStarted, this, &LocationsView::onExpandingAnimationStarted);
    //connect(widget_, &ExpandableItemsWidget::expandingAnimationProgress, this, &LocationsView::onExpandingAnimationProgress);
    connect(widget_, &ExpandableItemsWidget::emptyListStateChanged, [this](bool isEmptyList) {
        emit emptyListStateChanged(isEmptyList);
    });
    connect(widget_, &ExpandableItemsWidget::selected, [this](const LocationID &lid) {
        emit selected(lid);
    });
    connect(widget_, &ExpandableItemsWidget::clickedOnPremiumStarCity, [this]() {
        emit clickedOnPremiumStarCity();
    });
}

LocationsView::~LocationsView()
{
    delete countryItemDelegate_;
    delete cityItemDelegate_;
}

void LocationsView::setShowLocationLoad(bool isShowLocationLoad)
{
    widget_->setShowLocationLoad(isShowLocationLoad);
}

void LocationsView::updateScaling()
{
    scrollBar_->updateCustomStyleSheet();
    widget_->updateScaling();
}

void LocationsView::scrollToTop()
{
    scrollBar_->setValueWithoutAnimation(0);
}

void LocationsView::updateSelected()
{
    if (isCursorInList())
        widget_->updateSelectedItemByCursorPos();
}

bool LocationsView::handleKeyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        widget_->doActionOnSelectedItem();
        return true;
    }

    int scrollOffs = 0;
    if (event->key() == Qt::Key_Up) {
        scrollOffs = -1;
    } else if (event->key() == Qt::Key_Down) {
        scrollOffs = 1;
    }
    else if (event->key() == Qt::Key_PageUp) {
        scrollOffs = -viewport()->height() / qCeil(LOCATION_ITEM_HEIGHT * G_SCALE);
    }
    else if (event->key() == Qt::Key_PageDown) {
        scrollOffs = viewport()->height() / qCeil(LOCATION_ITEM_HEIGHT * G_SCALE);
    }

    if (scrollOffs != 0) {
        int pos = widget_->selectItemByOffs(scrollOffs);
        QScrollArea::ensureVisible(0, pos, 0, 0);
        QScrollArea::ensureVisible(0, pos + qCeil(LOCATION_ITEM_HEIGHT * G_SCALE), 0, 0);
        if (isCursorInList()) {
            moveCursorToItem(pos);
        }
        return true;
    }

    return false;
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
    widget_->setFixedWidth(size().width());
    scrollBar_->setFixedWidth(kScrollBarWidth * G_SCALE);
    viewport()->setFixedWidth(size().width());
}

void LocationsView::ensureVisible(int top, int bottom)
{
    if (bottom > (scrollBar_->value() + viewport()->height())) {
        int newPos = bottom - viewport()->height();
        newPos = qMin(newPos, top);
        scrollBar_->setValueWithAnimation(newPos);
    }
}

bool LocationsView::isCursorInList()
{
    const QPoint widgetPos = geometry().topLeft();
    const QPoint widgetPosGlobal = this->mapToGlobal(widgetPos);
    const QRect widgetRectGlobal(widgetPosGlobal.x(), widgetPosGlobal.y(), widget_->geometry().width(), geometry().height());
    return widgetRectGlobal.contains(QCursor::pos());
}

void LocationsView::moveCursorToItem(int top)
{
    const QPoint globalItemPos = mapToGlobal(QPoint(0, top - scrollBar_->value()));
    //const QRect globalItemRect(globalItemPos.x(), globalItemPos.y(), widget_->geometry().width(), qCeil(LOCATION_ITEM_HEIGHT * G_SCALE));
    QCursor::setPos(QCursor::pos().x(), globalItemPos.y() + qCeil(LOCATION_ITEM_HEIGHT * G_SCALE) / 2);
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

int LocationsView::count() const
{
    return widget_->count();
}

void LocationsView::setShowCountryFlagForCity(bool show)
{
    widget_->setShowCountryFlagForCity(show);
}

} // namespace gui_locations

