#include "searchwidgetlocations.h"

#include <QDateTime>
#include <QGestureEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScroller>
#include <qmath.h>
#include "graphicresources/fontmanager.h"
#include "widgetlocationssizes.h"
#include "languagecontroller.h"
#include "dpiscalemanager.h"
#include "commongraphics/commongraphics.h"
#include "tooltips/tooltipcontroller.h"
#include "utils/logger.h"

#include <QDebug>

namespace GuiLocations {


SearchWidgetLocations::SearchWidgetLocations(QWidget *parent) : QScrollArea(parent)
  , filterString_("")
  , countOfAvailableItemSlots_(7)
  , bIsFreeSession_(false)
  , bShowLatencyInMs_(false)
  , bTapGestureStarted_(false)
  , locationsModel_(NULL)
  , kickPreventMouseSelectionTimer_(false)
  , animationScollTarget_(0)
{
    setFrameStyle(QFrame::NoFrame);
    setMouseTracking(true);
    setStyleSheet("background-color: rgba(0,0,0,0)");
    setFocusPolicy(Qt::NoFocus);

    // scrollbar
    scrollBar_ = new ScrollBar(this);
    scrollBar_->setStyleSheet(scrollbarStyleSheet());
    setVerticalScrollBar(scrollBar_);
    locationItemListWidget_ = new LocationItemListWidget(this, this);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollBar_->setSingleStep(LocationItemListWidget::ITEM_HEIGHT * G_SCALE); // scroll by this many px at a time
    scrollBar_->setGeometry(WINDOW_WIDTH * G_SCALE - getScrollBarWidth(), 0, getScrollBarWidth(), 170 * G_SCALE);

    // central widget
    setWidget(locationItemListWidget_);
    connect(locationItemListWidget_, SIGNAL(heightChanged(int)), SLOT(onLocationItemListWidgetHeightChanged(int)));
    connect(locationItemListWidget_, SIGNAL(favoriteClicked(LocationItemCityWidget*,bool)), SLOT(onLocationItemListWidgetFavoriteClicked(LocationItemCityWidget *, bool)));
    connect(locationItemListWidget_, SIGNAL(locationIdSelected(LocationID)), SLOT(onLocationItemListWidgetLocationIdSelected(LocationID)));
    connect(locationItemListWidget_, SIGNAL(regionExpanding(LocationItemRegionWidget*, LocationItemListWidget::ExpandReason)), SLOT(onLocationItemListWidgetRegionExpanding(LocationItemRegionWidget*, LocationItemListWidget::ExpandReason)));
    locationItemListWidget_->setGeometry(0,0, WINDOW_WIDTH*G_SCALE, 0);
    locationItemListWidget_->show();

    preventMouseSelectionTimer_.start();

    connect(&scrollAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onScrollAnimationValueChanged(QVariant)));
    connect(&LanguageController::instance(), SIGNAL(languageChanged()), SLOT(onLanguageChanged()));
}

SearchWidgetLocations::~SearchWidgetLocations()
{
    locationItemListWidget_->disconnect();
    locationItemListWidget_->deleteLater();

    scrollBar_->disconnect();
    scrollBar_->deleteLater();
}

void SearchWidgetLocations::setFilterString(QString text)
{
    filterString_ = text;
    updateWidgetList(locationsModel_->items());

    if (filterString_.isEmpty())
    {
        qDebug() << "Filter collapsing all";
        locationItemListWidget_->collapseAllLocations();
    }
    else
    {
        qDebug() << "Filter expanding all";
        locationItemListWidget_->expandAllLocations();
    }
    locationItemListWidget_->accentFirstItem();
    scrollToIndex(0);
}

bool SearchWidgetLocations::cursorInViewport()
{
    QPoint cursorPos = QCursor::pos();
    QRect rect = globalLocationsListViewportRect();
    return rect.contains(cursorPos);
}

bool SearchWidgetLocations::hasSelection()
{
    return locationItemListWidget_->hasAccentItem();
}


void SearchWidgetLocations::centerCursorOnSelectedItem()
{
    // TODO:
}

const QString SearchWidgetLocations::scrollbarStyleSheet()
{
    // TODO: don't use stylesheet to draw
    return QString( "QScrollBar:vertical { margin: %1px %2 %3px %4px; border: none; background: rgba(0, 0, 0, 255); width: %5px; }"
                 "QScrollBar::handle:vertical { background: rgb(106, 119, 144); color:  rgb(106, 119, 144);"
                                               "border-width: %6px; border-style: solid; border-radius: %7px;}"
                 "QScrollBar::add-line:vertical { border: none; background: black; }"
                 "QScrollBar::sub-line:vertical { border: none; background: black; }")
                  .arg(qCeil(0)) // top margin
                  .arg(qCeil(0))  // left margin
                  .arg(qCeil(0)) //  bottom margin
                  .arg(qCeil(0))  // left
                  .arg(3*G_SCALE) // width
                  .arg(qCeil(4))  // handle border-width
            .arg(qCeil(3)); // handle border-radius
}

void SearchWidgetLocations::scrollToIndex(int index)
{
    scrollBar_->forceSetValue(static_cast<int>(LocationItemListWidget::ITEM_HEIGHT * G_SCALE * index));
}

void SearchWidgetLocations::scrollDown(int itemCount)
{
    // Scrollbar values use positive (whilte itemListWidget uses negative geometry)
    int newY = static_cast<int>(locationItemListWidget_->geometry().y() + LocationItemListWidget::ITEM_HEIGHT * G_SCALE * itemCount);

    // qDebug() << "SearchWidgetLocations is setting scrollbar value: " << newY;
    scrollBar_->setValue(newY);
}

void SearchWidgetLocations::animatedScrollDown(int itemCount)
{
    int scrollBy = static_cast<int>(LocationItemListWidget::ITEM_HEIGHT * G_SCALE * itemCount);

    if (scrollAnimation_.state() == QAbstractAnimation::Running)
    {
        animationScollTarget_ += scrollBy;
    }
    else
    {
        animationScollTarget_ = scrollBar_->value() + scrollBy;
    }

    scrollAnimation_.stop();
    scrollAnimation_.setDuration(PROGRAMMATIC_SCROLL_ANIMATION_DURATION);
    scrollAnimation_.setStartValue(scrollBar_->value());
    scrollAnimation_.setEndValue(animationScollTarget_);
    scrollAnimation_.setDirection(QAbstractAnimation::Forward);
    scrollAnimation_.start();
}

void SearchWidgetLocations::animatedScrollUp(int itemCount)
{
    int scrollBy = static_cast<int>(LocationItemListWidget::ITEM_HEIGHT * G_SCALE * itemCount);

    if (scrollAnimation_.state() == QAbstractAnimation::Running)
    {
        animationScollTarget_ -= scrollBy;
    }
    else
    {
        animationScollTarget_ = scrollBar_->value() - scrollBy;
    }

    scrollAnimation_.stop();
    scrollAnimation_.setDuration(PROGRAMMATIC_SCROLL_ANIMATION_DURATION);
    scrollAnimation_.setStartValue(scrollBar_->value());
    scrollAnimation_.setEndValue(animationScollTarget_);
    scrollAnimation_.setDirection(QAbstractAnimation::Forward);
    scrollAnimation_.start();
}

void SearchWidgetLocations::updateWidgetList(QVector<LocationModelItem *> items)
{
    // storing previous location widget state
    QVector<LocationID> expandedLocationIds = locationItemListWidget_->expandedOrExpandingLocationIds();
    LocationID topSelectableLocationIdInViewport = locationItemListWidget_->topSelectableLocationIdInViewport();
    LocationID lastSelectedLocationId = locationItemListWidget_->lastSelectedLocationId();

    qCDebug(LOG_BASIC) << "Updating search locations widget list";
    locationItemListWidget_->clearWidgets();
    foreach (LocationModelItem *item, items)
    {
        if (item->title.contains(filterString_, Qt::CaseInsensitive))
        {
            // add item and all children to list
            locationItemListWidget_->addRegionWidget(item);

            foreach (CityModelItem cityItem, item->cities)
            {
                locationItemListWidget_->addCityToRegion(cityItem, item);
            }
        }
        else
        {
            foreach (CityModelItem cityItem, item->cities)
            {
                if (cityItem.city.contains(filterString_, Qt::CaseInsensitive) ||
                    cityItem.city.contains(filterString_, Qt::CaseInsensitive))
                {
                    locationItemListWidget_->addCityToRegion(cityItem, item);
                }
            }
        }
    }
    qDebug() << "Restoring widget state";

    // restoring previous widget state
    locationItemListWidget_->expandLocationIds(expandedLocationIds);
    int indexInNewList = locationItemListWidget_->selectableIndex(topSelectableLocationIdInViewport);
    if (indexInNewList >= 0)
    {
        scrollDown(indexInNewList);
    }
    locationItemListWidget_->selectItem(lastSelectedLocationId);
}


void SearchWidgetLocations::setModel(BasicLocationsModel *locationsModel)
{
    Q_ASSERT(locationsModel_ == NULL);
    locationsModel_ = locationsModel;
    connect(locationsModel_, SIGNAL(itemsUpdated(QVector<LocationModelItem*>)),
                           SLOT(onItemsUpdated(QVector<LocationModelItem*>)), Qt::DirectConnection);
    connect(locationsModel_, SIGNAL(connectionSpeedChanged(LocationID, PingTime)), SLOT(onConnectionSpeedChanged(LocationID, PingTime)), Qt::DirectConnection);
    connect(locationsModel_, SIGNAL(isFavoriteChanged(LocationID, bool)), SLOT(onIsFavoriteChanged(LocationID, bool)), Qt::DirectConnection);
    connect(locationsModel, SIGNAL(freeSessionStatusChanged(bool)), SLOT(onFreeSessionStatusChanged(bool)));
}

void SearchWidgetLocations::setFirstSelected()
{
    qDebug() << "SearchWidgetLocations::setFirstSelected";
    locationItemListWidget_->accentFirstItem();
}

void SearchWidgetLocations::startAnimationWithPixmap(const QPixmap &pixmap)
{
    backgroundPixmapAnimation_.startWith(pixmap, 150);
}

void SearchWidgetLocations::setShowLatencyInMs(bool showLatencyInMs)
{
    bShowLatencyInMs_ = showLatencyInMs;
    foreach (LocationItemCityWidget *w, locationItemListWidget_->cityWidgets())
    {
        w->setShowLatencyMs(showLatencyInMs);
    }
}

bool SearchWidgetLocations::isShowLatencyInMs()
{
    return bShowLatencyInMs_;
}

bool SearchWidgetLocations::isFreeSessionStatus()
{
    return bIsFreeSession_;
}

int SearchWidgetLocations::getWidth()
{
    return viewport()->width();
}

int SearchWidgetLocations::getScrollBarWidth()
{
    return WidgetLocationsSizes::instance().getScrollBarWidth();
}

void SearchWidgetLocations::setCountAvailableItemSlots(int cnt)
{
    if (countOfAvailableItemSlots_ != cnt)
    {
        countOfAvailableItemSlots_ = cnt;
        // setupScrollBar();
        // setupScrollBarMaxValue();
        // viewport()->update();
    }
}

//int SearchWidgetLocations::getCountAvailableItemSlots()
//{
//    return countOfAvailableItemSlots_;
//}

int SearchWidgetLocations::getItemHeight() const
{
    return WidgetLocationsSizes::instance().getItemHeight();
}

int SearchWidgetLocations::getTopOffset() const
{
    return WidgetLocationsSizes::instance().getTopOffset();
}

//void SearchWidgetLocations::onKeyPressEvent(QKeyEvent *event)
//{
//    handleKeyEvent((QKeyEvent *)event);
//}

bool SearchWidgetLocations::eventFilter(QObject *object, QEvent *event)
{
    if (object == viewport() && event->type() == QEvent::Gesture)
    {
        QGestureEvent *ge = static_cast<QGestureEvent *>(event);
        QTapGesture *g = static_cast<QTapGesture *>(ge->gesture(Qt::TapGesture));
        if (g)
        {
            if (g->state() == Qt::GestureStarted)
            {
                bTapGestureStarted_ = true;
            }
            else if (g->state() == Qt::GestureFinished)
            {
                if (bTapGestureStarted_)
                {
                    bTapGestureStarted_ = false;
                    QPointF ptf = g->position();
                    QPoint pt(ptf.x(), ptf.y());
                    handleTapClick(viewport()->mapToGlobal(pt));
                }
            }
            else if (g->state() == Qt::GestureCanceled)
            {
                bTapGestureStarted_ = false;
            }
        }
        QPanGesture *gp = static_cast<QPanGesture *>(ge->gesture(Qt::PanGesture));
        if (gp)
        {
            if (gp->state() == Qt::GestureStarted)
            {

                bTapGestureStarted_ = false;
            }
        }
        return true;
    }
    else if (object == viewport() && event->type() == QEvent::ScrollPrepare)
    {
        QScrollPrepareEvent *se = static_cast<QScrollPrepareEvent *>(event);
        se->setViewportSize(QSizeF(viewport()->size()));
        se->setContentPosRange(QRectF(0, 0, 1, scrollBar_->maximum() * getItemHeight()));
        se->setContentPos(QPointF(0, scrollBar_->value() * getItemHeight()));
        se->accept();
        return true;
    }
    else if (object == viewport() && event->type() == QEvent::Scroll)
    {
        QScrollEvent *se = static_cast<QScrollEvent *>(event);
        scrollBar_->setValue(se->contentPos().y() / getItemHeight());
        return true;
    }
    return QAbstractScrollArea::eventFilter(object, event);
}

void SearchWidgetLocations::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    // draw background for when list is < size of viewport
    QPainter painter(viewport());
    QRect bkgd(0,0,geometry().width(), geometry().height());
    painter.fillRect(bkgd, WidgetLocationsSizes::instance().getBackgroundColor());
}

// called by change in the vertical scrollbar
void SearchWidgetLocations::scrollContentsBy(int dx, int dy)
{
    // qDebug() << "Scrolling contents by: " << dy;

    TooltipController::instance().hideAllTooltips();

    // cursor should not interfere when animation is running
    // prevent floating cursor from overriding a keypress animation
    // this can only occur when accessibility is disabled (since cursor will not be moved)
    if (cursorInViewport() && preventMouseSelectionTimer_.elapsed() > 100)
    {
        locationItemListWidget_->selectWidgetContainingCursor();
    }

    QScrollArea::scrollContentsBy(dx,dy);

    // qDebug() << "New top index: " << locationItemListWidget_->topSelectableIndex();
}

void SearchWidgetLocations::mouseMoveEvent(QMouseEvent *event)
{

    QAbstractScrollArea::mouseMoveEvent(event);
}

void SearchWidgetLocations::mousePressEvent(QMouseEvent *event)
{
//#ifdef Q_OS_WIN
//    if (event->source() == Qt::MouseEventSynthesizedBySystem)
//    {
//        return;
//    }
//#endif

//    if (event->button() == Qt::LeftButton && indSelected_ != -1)
//    {
//        indParentPressed_ = indSelected_;
//        indChildPressed_ = items_[indSelected_]->getSelected();
//        indChildFavourite_ = detectItemClickOnFavoriteLocationIcon();
//    }
}

void SearchWidgetLocations::mouseReleaseEvent(QMouseEvent *event)
{
    QScrollArea::mouseReleaseEvent(event);
}

void SearchWidgetLocations::mouseDoubleClickEvent(QMouseEvent *event)
{
    QScrollArea::mouseDoubleClickEvent(event);
}

void SearchWidgetLocations::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
}

void SearchWidgetLocations::enterEvent(QEvent *event)
{
    QScrollArea::enterEvent(event);
}

void SearchWidgetLocations::resizeEvent(QResizeEvent *event)
{
    QScrollArea::resizeEvent(event);
    // scrollBar_->setGeometry(WINDOW_WIDTH*G_SCALE - getScrollBarWidth(), 0, getScrollBarWidth(), geometry().height());
}

void SearchWidgetLocations::onItemsUpdated(QVector<LocationModelItem *> items)
{
    Q_UNUSED(items);

    updateWidgetList(items);
}

void SearchWidgetLocations::onConnectionSpeedChanged(LocationID id, PingTime timeMs)
{
    // qDebug() << "Search widget speed change";
    foreach (LocationItemCityWidget *w, locationItemListWidget_->cityWidgets())
    {
        if (w->getId() == id)
        {
            w->setLatencyMs(timeMs);
            break;
        }
    }
}

void SearchWidgetLocations::onIsFavoriteChanged(LocationID id, bool isFavorite)
{
    // qDebug() << "SearchWidget setting";
    foreach (LocationItemRegionWidget *region, locationItemListWidget_->itemWidgets())
    {
        if (region->getId().toTopLevelLocation() == id.toTopLevelLocation())
        {
            region->setFavorited(id, isFavorite);
            break;
        }
    }
}

void SearchWidgetLocations::onFreeSessionStatusChanged(bool isFreeSessionStatus)
{
    if (bIsFreeSession_ != isFreeSessionStatus)
    {
        bIsFreeSession_ = isFreeSessionStatus;
    }
}

void SearchWidgetLocations::onLanguageChanged()
{
    // TODO: language change
}

void SearchWidgetLocations::onLocationItemListWidgetHeightChanged(int listWidgetHeight)
{
    //qDebug() << "SearchWidgetLocations::onLocationItemListWidgetHeightChanged: " << listWidgetHeight;
    //qDebug() << "SearchLocations height: "<< geometry().height();

    locationItemListWidget_->setGeometry(0,locationItemListWidget_->geometry().y(), WINDOW_WIDTH*G_SCALE, listWidgetHeight);
    scrollBar_->setRange(0, listWidgetHeight - scrollBar_->pageStep()); // update scroll bar
    scrollBar_->setSingleStep(LocationItemListWidget::ITEM_HEIGHT * G_SCALE); // scroll by this many px at a time
}

void SearchWidgetLocations::onLocationItemListWidgetFavoriteClicked(LocationItemCityWidget *cityWidget, bool favorited)
{
    // qDebug() << "SearchWidget favorite clicked: " << cityWidget->name();
    emit switchFavorite(cityWidget->getId(), favorited);
}

void SearchWidgetLocations::onLocationItemListWidgetLocationIdSelected(LocationID id)
{
    emit selected(id);
}

void SearchWidgetLocations::onLocationItemListWidgetRegionExpanding(LocationItemRegionWidget *region, LocationItemListWidget::ExpandReason reason)
{
    // qDebug() << "Checking for need to scroll due to expand";
    int topItemSelIndex = locationItemListWidget_->selectableIndex(locationItemListWidget_->topSelectableLocationIdInViewport());
    int regionSelIndex = locationItemListWidget_->selectableIndex(region->getId());
    int regionViewportIndex = regionSelIndex - topItemSelIndex;
    int bottomCityViewportIndex  = regionViewportIndex + region->cityWidgets().count();

    int diff = bottomCityViewportIndex - countVisibleItems();
    if (diff >= 0)
    {
        int change = diff + 1;
        if (change > regionViewportIndex) change = regionViewportIndex;

        // qDebug() << "Expanding auto-scroll by items: " << change;
        if (reason == LocationItemListWidget::EXPAND_REASON_AUTO) kickPreventMouseSelectionTimer_ = true;
        else kickPreventMouseSelectionTimer_ = false;
        animatedScrollDown(change);
    }
}

void SearchWidgetLocations::onScrollAnimationValueChanged(const QVariant &value)
{
    if (kickPreventMouseSelectionTimer_) preventMouseSelectionTimer_.restart();
    scrollBar_->forceSetValue(value.toInt());
    viewport()->update();
}

bool SearchWidgetLocations::isExpandAnimationNow()
{

    return false;
}

void SearchWidgetLocations::setCursorForSelected()
{

}

void SearchWidgetLocations::setVisibleExpandedItem(int ind)
{

}

LocationID SearchWidgetLocations::detectLocationForTopInd(int topInd)
{

    return LocationID();
}

int SearchWidgetLocations::detectVisibleIndForCursorPos(const QPoint &pt)
{
    QPoint localPt = viewport()->mapFromGlobal(pt);
    return (localPt.y() - getTopOffset()) / getItemHeight();
}

void SearchWidgetLocations::handleKeyEvent(QKeyEvent *event)
{
    // qDebug() << "SearchWidgetLocations::handleKeyEvent";

    // TODO: quickly repeating UP can cause cursor to jump out of locations region
    if (event->key() == Qt::Key_Up)
    {
        if (locationItemListWidget_->hasAccentItem())
        {
            if (locationItemListWidget_->accentItemSelectableIndex() > 0)
            {
                if (locationItemListWidget_->accentItemViewportIndex() <= 0)
                {
                    kickPreventMouseSelectionTimer_ = true;
                    animatedScrollUp(1);
                }
                else
                {
                    QPoint cursorPos = QCursor::pos();
                    if (isGlobalPointInViewport(cursorPos))
                    {
                        TooltipController::instance().hideAllTooltips();

                        SelectableLocationItemWidget *lastSelWidget = locationItemListWidget_->lastAccentedItemWidget();
                        if (lastSelWidget->containsCursor())
                        {
                            // Note: this kind of cursor control requires Accessibility Permissions on MacOS
                            QCursor::setPos(QPoint(cursorPos.x(), static_cast<int>(cursorPos.y() - LOCATION_ITEM_HEIGHT*G_SCALE)));
                        }
                        else
                        {
                            QCursor::setPos(QPoint(cursorPos.x(),
                                                   static_cast<int>(lastSelWidget->globalGeometry().top() - LOCATION_ITEM_HEIGHT*G_SCALE/2)));
                        }
                    }
                }
                locationItemListWidget_->moveAccentUp();
            }
        }
        else
        {
            locationItemListWidget_->accentFirstItem();
        }
    }
    else if (event->key() == Qt::Key_Down)
    {
        if (locationItemListWidget_->hasAccentItem())
        {
            if (locationItemListWidget_->accentItemSelectableIndex() < locationItemListWidget_->selectableWidgets().count() - 1)
            {
                if (locationItemListWidget_->accentItemViewportIndex() >= countVisibleItems() - 1)
                {
                    kickPreventMouseSelectionTimer_ = true;
                    animatedScrollDown(1);
                }
                else
                {
                    QPoint cursorPos = QCursor::pos();
                    if (isGlobalPointInViewport(cursorPos))
                    {
                        TooltipController::instance().hideAllTooltips();

                        SelectableLocationItemWidget *lastSelWidget = locationItemListWidget_->lastAccentedItemWidget();
                        if (lastSelWidget->containsCursor())
                        {
                            // Note: this kind of cursor control requires Accessibility Permissions on MacOS
                            QCursor::setPos(QPoint(cursorPos.x(), static_cast<int>(cursorPos.y() + LOCATION_ITEM_HEIGHT*G_SCALE)));
                        }
                        else
                        {
                            QCursor::setPos(QPoint(cursorPos.x(), static_cast<int>(lastSelWidget->globalGeometry().bottom() + LOCATION_ITEM_HEIGHT*G_SCALE/2)));
                        }
                    }
                }
                locationItemListWidget_->moveAccentDown();
            }
        }
        else
        {
            qDebug() << "Accenting first";
            locationItemListWidget_->accentFirstItem();
        }
    }
    else if (event->key() == Qt::Key_Return)
    {
        // qDebug() << "Selection by key press";

        SelectableLocationItemWidget *lastSelWidget = locationItemListWidget_->lastAccentedItemWidget();

        if (!lastSelWidget)
        {
            qDebug() << "SearchWidgetLocations::ReturnPress - no item accented";
            Q_ASSERT(false);
            return;
        }

        if (lastSelWidget->getId().isBestLocation())
        {
            emit selected(locationItemListWidget_->lastSelectedLocationId());
        }
        else if (lastSelWidget->getId().isTopLevelLocation())
        {
            if (lastSelWidget->isExpanded())
            {
                locationItemListWidget_->collapse(locationItemListWidget_->lastSelectedLocationId());
            }
            else
            {
                locationItemListWidget_->expand(locationItemListWidget_->lastSelectedLocationId());
            }
        }
        else // city
        {
            if (lastSelWidget)
            {
                if(!lastSelWidget->isForbidden() && !lastSelWidget->isDisabled())
                {
                    emit selected(locationItemListWidget_->lastSelectedLocationId());
                }
            }
        }
    }
}

bool SearchWidgetLocations::isGlobalPointInViewport(const QPoint &pt)
{
    QPoint pt1 = viewport()->mapToGlobal(QPoint(0,0));
    QRect globalRectOfViewport(pt1.x(), pt1.y(), viewport()->geometry().width(), viewport()->geometry().height());
    return globalRectOfViewport.contains(pt);
}

void SearchWidgetLocations::handleTapClick(const QPoint &cursorPos)
{

}

int SearchWidgetLocations::countVisibleItems()
{
    int geoHeight = geometry().height();
    int count = static_cast<int>( geoHeight / LocationItemListWidget::ITEM_HEIGHT * G_SCALE);
    return count;
}

void SearchWidgetLocations::updateScaling()
{
    locationItemListWidget_->updateScaling();
    // scrollBar_->setGeometry(WINDOW_WIDTH*G_SCALE - getScrollBarWidth(), 0, getScrollBarWidth(), height_ * G_SCALE);
}

QRect SearchWidgetLocations::globalLocationsListViewportRect()
{
    int offset = getItemHeight();
    QPoint locationItemListTopLeft(0, geometry().y() - offset);
    locationItemListTopLeft = mapToGlobal(locationItemListTopLeft);

    return QRect(locationItemListTopLeft.x(), locationItemListTopLeft.y(),
                 viewport()->geometry().width(), viewport()->geometry().height());
}


} // namespace GuiLocations

