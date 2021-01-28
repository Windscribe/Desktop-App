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
  , currentScale_(G_SCALE)
  , lastScrollPos_(0)
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
    locationItemListWidget_->accentFirstSelectableItem();
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
    QPoint cursorPos = QCursor::pos();
    SelectableLocationItemWidget *lastSelWidget = locationItemListWidget_->lastAccentedItemWidget();

    if (lastSelWidget)
    {
        // qDebug() << "Last selected: " << lastSelWidget->name();
        QCursor::setPos(QPoint(cursorPos.x(), static_cast<int>(lastSelWidget->globalGeometry().top() - LOCATION_ITEM_HEIGHT*G_SCALE/2)));
    }
}

void SearchWidgetLocations::centerCursorOnItem(LocationID locId)
{
    QPoint cursorPos = QCursor::pos();
    if (locId.isValid())
    {
        //qDebug() << "Moving cursor to: " << locId->name();
        if (locationIdInViewport(locId))
        {
            SelectableLocationItemWidget *widget = locationItemListWidget_->selectableWidget(locId);
            if (widget)
            {
                QCursor::setPos(QPoint(cursorPos.x(), static_cast<int>(widget->globalGeometry().top() + LOCATION_ITEM_HEIGHT*G_SCALE/2)));
            }
        }
    }
}

LocationID SearchWidgetLocations::selectedItemLocationId()
{
    return locationItemListWidget_->lastAccentedItemWidget()->getId();
}

const QString SearchWidgetLocations::scrollbarStyleSheet()
{
    // TODO: don't use stylesheet to draw
    QString css = QString( "QScrollBar:vertical { margin: %1px %2 %3px %4px; ")
                    .arg(qCeil(0))   // top margin
                    .arg(qCeil(0))   // right
                    .arg(qCeil(0))   //  bottom margin
                    .arg(qCeil(0));  // left
    css += QString("border: none; background: rgba(0,0,0,255); width: %1px; padding: %2 %3 %4 %5; }")
                    .arg(getScrollBarWidth())  // width
                    .arg(0)           // padding top
                    .arg(0 * G_SCALE) // padding left
                    .arg(0)           // padding bottom
                    .arg(0);          // padding right
    css += QString( "QScrollBar::handle:vertical { background: rgb(106, 119, 144); color:  rgb(106, 119, 144);"
                    "border-width: %1px; border-style: solid; border-radius: %2px;}")
                        .arg(qCeil(2))  // handle border-width
                        .arg(qCeil(2)); // handle border-radius
    css += QString( "QScrollBar::add-line:vertical { border: none; background: none; }"
                     "QScrollBar::sub-line:vertical { border: none; background: none; }");


    return css;
}

void SearchWidgetLocations::scrollToIndex(int index)
{
    // qDebug() <<
    int pos = static_cast<int>(LocationItemListWidget::ITEM_HEIGHT * G_SCALE * index);
    pos = closestPositionIncrement(pos);
    scrollBar_->forceSetValue(pos);
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

const LocationID SearchWidgetLocations::topViewportSelectableLocationId()
{
    int index = qRound(qAbs(locationItemListWidget_->geometry().y())/(LOCATION_ITEM_HEIGHT * G_SCALE));

    auto widgets = locationItemListWidget_->selectableWidgets();
    if (index < 0 || index > widgets.count() - 1)
    {
        qDebug(LOG_BASIC) << "Err: Can't index selectable items with: " << index;
        return LocationID();
    }

    return widgets[index]->getId();
}

int SearchWidgetLocations::viewportOffsetIndex()
{
    int index = qRound(qAbs(locationItemListWidget_->geometry().y())/(LOCATION_ITEM_HEIGHT * G_SCALE));

    // qDebug() << locationItemListWidget_->geometry().y() << " / " << (LOCATION_ITEM_HEIGHT * G_SCALE) <<  " -> " << index;
    return index;
}

int SearchWidgetLocations::accentItemViewportIndex()
{
    if (!locationItemListWidget_->hasAccentItem())
    {
        return -1;
    }
    return viewportIndex(locationItemListWidget_->lastAccentedItemWidget()->getId());
}

int SearchWidgetLocations::viewportIndex(LocationID locationId)
{
    int topItemSelIndex = locationItemListWidget_->selectableIndex(topViewportSelectableLocationId());
    int desiredLocSelIndex = locationItemListWidget_->selectableIndex(locationId);
    int desiredLocationViewportIndex = desiredLocSelIndex - topItemSelIndex;
    return desiredLocationViewportIndex;
}

bool SearchWidgetLocations::locationIdInViewport(LocationID location)
{
    int vi = viewportIndex(location);
    return vi >= 0 && vi < countOfAvailableItemSlots_;
}

int SearchWidgetLocations::closestPositionIncrement(int value)
{
    int current = 0;
    int last = 0;
    while (current < value)
    {
        last = current;
        current += LOCATION_ITEM_HEIGHT * G_SCALE;
    }

    if (qAbs(current - value) < qAbs(last - value))
    {
        return current;
    }
    return last;
}

void SearchWidgetLocations::updateWidgetList(QVector<LocationModelItem *> items)
{
    // storing previous location widget state
    QVector<LocationID> expandedLocationIds = locationItemListWidget_->expandedOrExpandingLocationIds();
    LocationID topSelectableLocationIdInViewport = topViewportSelectableLocationId();
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
    locationItemListWidget_->accentFirstSelectableItem();
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
    }
}

int SearchWidgetLocations::getItemHeight() const
{
    return WidgetLocationsSizes::instance().getItemHeight();
}

bool SearchWidgetLocations::eventFilter(QObject *object, QEvent *event)
{
    // TODO: this entire function (for gestures)
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
    // TODO: scrolling a bit laggy when scrolling through open regions on windows

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
    lastScrollPos_ = locationItemListWidget_->geometry().y();
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
    // qDebug() << "List widget height: " << listWidgetHeight;
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
    int topItemSelIndex = locationItemListWidget_->selectableIndex(topViewportSelectableLocationId());
    int regionSelIndex = locationItemListWidget_->selectableIndex(region->getId());
    int regionViewportIndex = regionSelIndex - topItemSelIndex;
    int bottomCityViewportIndex  = regionViewportIndex + region->cityWidgets().count();

    int diff = bottomCityViewportIndex - countOfAvailableItemSlots_;
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

void SearchWidgetLocations::handleKeyEvent(QKeyEvent *event)
{
    // qDebug() << "SearchWidgetLocations::handleKeyEvent";

    if (event->key() == Qt::Key_Up)
    {
        if (locationItemListWidget_->hasAccentItem())
        {
            if (locationItemListWidget_->accentItemSelectableIndex() > 0)
            {
                if (accentItemViewportIndex() <= 0)
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

                        QPoint pt(cursorPos.x(), static_cast<int>(cursorPos.y() - LOCATION_ITEM_HEIGHT*G_SCALE));
                        if (!lastSelWidget->containsCursor()) // selected item does not match with cursor
                        {
                            pt = QPoint(cursorPos.x(), static_cast<int>(lastSelWidget->globalGeometry().top() - LOCATION_ITEM_HEIGHT*G_SCALE/2));
                        }

                        if (isGlobalPointInViewport(pt)) // prevent jump outside of locations region
                        {
                            // Note: this kind of cursor control requires Accessibility Permissions on MacOS
                            QCursor::setPos(pt);
                        }
                    }
                }
                locationItemListWidget_->moveAccentUp();
            }
        }
        else
        {
            locationItemListWidget_->accentFirstSelectableItem();
        }
    }
    else if (event->key() == Qt::Key_Down)
    {
        if (locationItemListWidget_->hasAccentItem())
        {
            if (locationItemListWidget_->accentItemSelectableIndex() < locationItemListWidget_->selectableWidgets().count() - 1)
            {
                if (accentItemViewportIndex() >= countOfAvailableItemSlots_ - 1)
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

                        QPoint pt(cursorPos.x(), static_cast<int>(cursorPos.y() + LOCATION_ITEM_HEIGHT*G_SCALE));
                        if (!lastSelWidget->containsCursor()) // selected item does not match with cursor
                        {
                            pt = QPoint(cursorPos.x(), static_cast<int>(lastSelWidget->globalGeometry().bottom() + LOCATION_ITEM_HEIGHT*G_SCALE/2));
                        }

                        if (isGlobalPointInViewport(pt)) // prevent jump outside of locations region
                        {
                            // Note: this kind of cursor control requires Accessibility Permissions on MacOS
                            QCursor::setPos(pt);
                        }
                    }
                }
                locationItemListWidget_->moveAccentDown();
            }
        }
        else
        {
            qDebug() << "Accenting first";
            locationItemListWidget_->accentFirstSelectableItem();
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
    Q_UNUSED(cursorPos)
//    if (!isScrollAnimationNow_)
//    {
//        detectSelectedItem(cursorPos);
//        viewport()->update();
//        setCursorForSelected();

//        if (!detectItemClickOnArrow())
//        {
//            emitSelectedIfNeed();
//        }
//    }
}

int SearchWidgetLocations::countVisibleItems()
{
    return countOfAvailableItemSlots_; // currently this is more reliable than recomputing based on list and viewport geometries (due to rounding while scaled)
}

void SearchWidgetLocations::updateScaling()
{
    // TODO: bug when changing scales when displaying lower in the list
    const auto scale_adjustment = G_SCALE / currentScale_;
    if (scale_adjustment != 1.0)
    {
        qDebug() << "Scale change!";
        currentScale_ = G_SCALE;
        int pos = -lastScrollPos_  * scale_adjustment; // lastScrollPos_ is negative (from geometry)
        lastScrollPos_ = -closestPositionIncrement(pos);
        //qDebug() << "forcing scroll value: " << lastScrollPos_;
        scrollBar_->forceSetValue(-lastScrollPos_); // use + for scrollbar
    }

    locationItemListWidget_->updateScaling();
    scrollBar_->setStyleSheet(scrollbarStyleSheet());

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

