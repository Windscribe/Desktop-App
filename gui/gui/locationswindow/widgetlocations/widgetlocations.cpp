#include "widgetlocations.h"

#include <QDateTime>
#include <QGestureEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScroller>
#include <qmath.h>
#include "graphicresources/fontmanager.h"
#include "widgetlocationssizes.h"
#include "languagecontroller.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "tooltips/tooltipcontroller.h"
#include "utils/logger.h"

#include <QDebug>

namespace GuiLocations {


WidgetLocations::WidgetLocations(QWidget *parent) : QScrollArea(parent)
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

    //#ifdef Q_OS_WIN
    //    viewport()->grabGesture(Qt::TapGesture);
    //    viewport()->grabGesture(Qt::PanGesture);
    //    viewport()->installEventFilter(this);

    //    QScroller::grabGesture(viewport(), QScroller::TouchGesture);
    //    QScroller *scroller = QScroller::scroller(viewport());
    //    QScrollerProperties properties = scroller->scrollerProperties();
    //    QVariant overshootPolicy = QVariant::fromValue<QScrollerProperties::OvershootPolicy>(QScrollerProperties::OvershootAlwaysOff);
    //    properties.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, overshootPolicy);
    //    properties.setScrollMetric(QScrollerProperties::DecelerationFactor, 0.2);
    //    properties.setScrollMetric(QScrollerProperties::MaximumVelocity, 0.135);
    //    scroller->setScrollerProperties(properties);
    //#endif


    // scrollbar
    scrollBar_ = new ScrollBar(this);
    setVerticalScrollBar(scrollBar_);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollBar_->setSingleStep(LOCATION_ITEM_HEIGHT * G_SCALE); // scroll by this many px at a time
    scrollBar_->setGeometry(WINDOW_WIDTH * G_SCALE - getScrollBarWidth(), 0, getScrollBarWidth(), 170 * G_SCALE);
    connect(scrollBar_, SIGNAL(handleDragged(int)), SLOT(onScrollBarHandleDragged(int)));


    // central widget
    widgetLocationsList_ = new WidgetLocationsList(this, this);
    setWidget(widgetLocationsList_);
    connect(widgetLocationsList_, SIGNAL(heightChanged(int)), SLOT(onLocationItemListWidgetHeightChanged(int)));
    connect(widgetLocationsList_, SIGNAL(favoriteClicked(ItemWidgetCity*,bool)), SLOT(onLocationItemListWidgetFavoriteClicked(ItemWidgetCity *, bool)));
    connect(widgetLocationsList_, SIGNAL(locationIdSelected(LocationID)), SLOT(onLocationItemListWidgetLocationIdSelected(LocationID)));
    connect(widgetLocationsList_, SIGNAL(regionExpanding(ItemWidgetRegion*, WidgetLocationsList::ExpandReason)), SLOT(onLocationItemListWidgetRegionExpanding(ItemWidgetRegion*, WidgetLocationsList::ExpandReason)));
    widgetLocationsList_->setGeometry(0,0, WINDOW_WIDTH*G_SCALE, 0);
    widgetLocationsList_->show();

    preventMouseSelectionTimer_.start();

    connect(&scrollAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onScrollAnimationValueChanged(QVariant)));
    connect(&LanguageController::instance(), SIGNAL(languageChanged()), SLOT(onLanguageChanged()));
}

WidgetLocations::~WidgetLocations()
{
    widgetLocationsList_->disconnect();
    widgetLocationsList_->deleteLater();

    scrollBar_->disconnect();
    scrollBar_->deleteLater();
}

void WidgetLocations::setModel(BasicLocationsModel *locationsModel)
{
    Q_ASSERT(locationsModel_ == NULL);
    locationsModel_ = locationsModel;
    connect(locationsModel_, SIGNAL(itemsUpdated(QVector<LocationModelItem*>)),
                           SLOT(onItemsUpdated(QVector<LocationModelItem*>)), Qt::DirectConnection);
    connect(locationsModel_, SIGNAL(connectionSpeedChanged(LocationID, PingTime)), SLOT(onConnectionSpeedChanged(LocationID, PingTime)), Qt::DirectConnection);
    connect(locationsModel_, SIGNAL(isFavoriteChanged(LocationID, bool)), SLOT(onIsFavoriteChanged(LocationID, bool)), Qt::DirectConnection);
    connect(locationsModel, SIGNAL(freeSessionStatusChanged(bool)), SLOT(onFreeSessionStatusChanged(bool)));
}

void WidgetLocations::setFilterString(QString text)
{
    filterString_ = text;
    updateWidgetList(locationsModel_->items());

    if (filterString_.isEmpty())
    {
        qDebug() << "Filter collapsing all";
        widgetLocationsList_->collapseAllLocations();
    }
    else
    {
        qDebug() << "Filter expanding all";
        widgetLocationsList_->expandAllLocations();
    }
    widgetLocationsList_->accentFirstSelectableItem();
    scrollToIndex(0);
}

void WidgetLocations::updateScaling()
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

    widgetLocationsList_->updateScaling();
    scrollBar_->updateCustomStyleSheet();
}

bool WidgetLocations::hasSelection()
{
    return widgetLocationsList_->hasAccentItem();
}

LocationID WidgetLocations::selectedItemLocationId()
{
    return widgetLocationsList_->lastAccentedItemWidget()->getId();
}

void WidgetLocations::setFirstSelected()
{
    widgetLocationsList_->accentFirstSelectableItem();
}

bool WidgetLocations::cursorInViewport()
{
    QPoint cursorPos = QCursor::pos();
    QRect rect = globalLocationsListViewportRect();
    return rect.contains(cursorPos);
}

void WidgetLocations::centerCursorOnSelectedItem()
{
    QPoint cursorPos = QCursor::pos();
    IItemWidget *lastSelWidget = widgetLocationsList_->lastAccentedItemWidget();

    if (lastSelWidget)
    {
        // qDebug() << "Last selected: " << lastSelWidget->name();
        QCursor::setPos(QPoint(cursorPos.x(), static_cast<int>(lastSelWidget->globalGeometry().top() - LOCATION_ITEM_HEIGHT*G_SCALE/2)));
    }
}

void WidgetLocations::centerCursorOnItem(LocationID locId)
{
    QPoint cursorPos = QCursor::pos();
    if (locId.isValid())
    {
        //qDebug() << "Moving cursor to: " << locId->name();
        if (locationIdInViewport(locId))
        {
            IItemWidget *widget = widgetLocationsList_->selectableWidget(locId);
            if (widget)
            {
                QCursor::setPos(QPoint(cursorPos.x(), static_cast<int>(widget->globalGeometry().top() + LOCATION_ITEM_HEIGHT*G_SCALE/2)));
            }
        }
    }
}

int WidgetLocations::countViewportItems()
{
    return countOfAvailableItemSlots_; // currently this is more reliable than recomputing based on list and viewport geometries (due to rounding while scaled)
}

void WidgetLocations::setCountViewportItems(int cnt)
{
    if (countOfAvailableItemSlots_ != cnt)
    {
        countOfAvailableItemSlots_ = cnt;
    }
}

bool WidgetLocations::isShowLatencyInMs()
{
    return bShowLatencyInMs_;
}

void WidgetLocations::setShowLatencyInMs(bool showLatencyInMs)
{
    bShowLatencyInMs_ = showLatencyInMs;
    foreach (ItemWidgetCity *w, widgetLocationsList_->cityWidgets())
    {
        w->setShowLatencyMs(showLatencyInMs);
    }
}

bool WidgetLocations::isFreeSessionStatus()
{
    return bIsFreeSession_;
}



void WidgetLocations::startAnimationWithPixmap(const QPixmap &pixmap)
{
    backgroundPixmapAnimation_.startWith(pixmap, 150);
}

bool WidgetLocations::eventFilter(QObject *object, QEvent *event)
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

void WidgetLocations::handleKeyEvent(QKeyEvent *event)
{

    if (event->key() == Qt::Key_Up)
    {
        if (widgetLocationsList_->hasAccentItem())
        {
            if (widgetLocationsList_->accentItemSelectableIndex() > 0)
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

                        IItemWidget *lastSelWidget = widgetLocationsList_->lastAccentedItemWidget();

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
                widgetLocationsList_->moveAccentUp();
            }
        }
        else
        {
            widgetLocationsList_->accentFirstSelectableItem();
        }
    }
    else if (event->key() == Qt::Key_Down)
    {
        if (widgetLocationsList_->hasAccentItem())
        {
            if (widgetLocationsList_->accentItemSelectableIndex() < widgetLocationsList_->selectableWidgets().count() - 1)
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

                        IItemWidget *lastSelWidget = widgetLocationsList_->lastAccentedItemWidget();

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
                widgetLocationsList_->moveAccentDown();
            }
        }
        else
        {
            qDebug() << "Accenting first";
            widgetLocationsList_->accentFirstSelectableItem();
        }
    }
    else if (event->key() == Qt::Key_Return)
    {
        // qDebug() << "Selection by key press";

        IItemWidget *lastSelWidget = widgetLocationsList_->lastAccentedItemWidget();

        if (!lastSelWidget)
        {
            Q_ASSERT(false);
            return;
        }

        if (lastSelWidget->getId().isBestLocation())
        {
            emit selected(widgetLocationsList_->lastSelectedLocationId());
        }
        else if (lastSelWidget->getId().isTopLevelLocation())
        {
            if (lastSelWidget->isExpanded())
            {
                widgetLocationsList_->collapse(widgetLocationsList_->lastSelectedLocationId());
            }
            else
            {
                widgetLocationsList_->expand(widgetLocationsList_->lastSelectedLocationId());
            }
        }
        else // city
        {
            if (lastSelWidget)
            {
                if(!lastSelWidget->isForbidden() && !lastSelWidget->isDisabled())
                {
                    emit selected(widgetLocationsList_->lastSelectedLocationId());
                }
            }
        }
    }
}



void WidgetLocations::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    // draw background for when list is < size of viewport
    QPainter painter(viewport());
    QRect bkgd(0,0,geometry().width(), geometry().height());
    painter.fillRect(bkgd, WidgetLocationsSizes::instance().getBackgroundColor());
}

// called by change in the vertical scrollbar
void WidgetLocations::scrollContentsBy(int dx, int dy)
{
    TooltipController::instance().hideAllTooltips();

    if (!scrollBar_->dragging())
    {
        // qDebug() << "Scrolling contents by: " << dy;
        // cursor should not interfere when animation is running
        // prevent floating cursor from overriding a keypress animation
        // this can only occur when accessibility is disabled (since cursor will not be moved)
        if (cursorInViewport() && preventMouseSelectionTimer_.elapsed() > 100)
        {
            widgetLocationsList_->selectWidgetContainingCursor();
        }

        QScrollArea::scrollContentsBy(dx,dy);
        lastScrollPos_ = widgetLocationsList_->geometry().y();
    }
}

void WidgetLocations::mouseMoveEvent(QMouseEvent *event)
{
    //#ifdef Q_OS_WIN
    //    if (event->source() == Qt::MouseEventSynthesizedBySystem)
    //    {
    //        return;
    //    }
    QAbstractScrollArea::mouseMoveEvent(event);
}

void WidgetLocations::mousePressEvent(QMouseEvent *event)
{

//#ifdef Q_OS_WIN
//    if (event->source() == Qt::MouseEventSynthesizedBySystem)
//    {
//        return;
//    }
//#endif
}

void WidgetLocations::mouseReleaseEvent(QMouseEvent *event)
{
    //#ifdef Q_OS_WIN
    //    if (event->source() == Qt::MouseEventSynthesizedBySystem)
    //    {
    //        return;
    //    }
    //#endif
    QScrollArea::mouseReleaseEvent(event);
}

void WidgetLocations::mouseDoubleClickEvent(QMouseEvent *event)
{
    //#ifdef Q_OS_WIN
    //    if (event->source() == Qt::MouseEventSynthesizedBySystem)
    //    {
    //        return;
    //    }

    // mousePressEvent instead of double click?
    // QScrollArea::mousePressEvent(event);
    QScrollArea::mouseDoubleClickEvent(event);
}

void WidgetLocations::leaveEvent(QEvent *event)
{
    QScrollArea::leaveEvent(event);
}

void WidgetLocations::enterEvent(QEvent *event)
{
    QScrollArea::enterEvent(event);
}

void WidgetLocations::resizeEvent(QResizeEvent *event)
{
    QScrollArea::resizeEvent(event);
}

void WidgetLocations::onItemsUpdated(QVector<LocationModelItem *> items)
{
    updateWidgetList(items);
}

void WidgetLocations::onConnectionSpeedChanged(LocationID id, PingTime timeMs)
{
    // qDebug() << "Search widget speed change";
    foreach (ItemWidgetCity *w, widgetLocationsList_->cityWidgets())
    {
        if (w->getId() == id)
        {
            w->setLatencyMs(timeMs);
            break;
        }
    }
}

void WidgetLocations::onIsFavoriteChanged(LocationID id, bool isFavorite)
{
    // qDebug() << "SearchWidget setting";
    foreach (ItemWidgetRegion *region, widgetLocationsList_->itemWidgets())
    {
        if (region->getId().toTopLevelLocation() == id.toTopLevelLocation())
        {
            region->setFavorited(id, isFavorite);
            break;
        }
    }
}

void WidgetLocations::onFreeSessionStatusChanged(bool isFreeSessionStatus)
{
    if (bIsFreeSession_ != isFreeSessionStatus)
    {
        bIsFreeSession_ = isFreeSessionStatus;
    }
}

void WidgetLocations::onLanguageChanged()
{
    // TODO: language change
    //    if (bestLocation_ != nullptr)
    //    {
    //        bestLocation_->updateLangauge(tr(bestLocationName_.toStdString().c_str()));
    //    }
}

void WidgetLocations::onLocationItemListWidgetHeightChanged(int listWidgetHeight)
{
    // qDebug() << "List widget height: " << listWidgetHeight;
    widgetLocationsList_->setGeometry(0,widgetLocationsList_->geometry().y(), WINDOW_WIDTH*G_SCALE, listWidgetHeight);
	scrollBar_->setRange(0, listWidgetHeight - scrollBar_->pageStep()); // update scroll bar
    scrollBar_->setSingleStep(LOCATION_ITEM_HEIGHT * G_SCALE); // scroll by this many px at a time
}

void WidgetLocations::onLocationItemListWidgetFavoriteClicked(ItemWidgetCity *cityWidget, bool favorited)
{
    // qDebug() << "SearchWidget favorite clicked: " << cityWidget->name();
    emit switchFavorite(cityWidget->getId(), favorited);
}

void WidgetLocations::onLocationItemListWidgetLocationIdSelected(LocationID id)
{
    emit selected(id);
}

void WidgetLocations::onLocationItemListWidgetRegionExpanding(ItemWidgetRegion *region, WidgetLocationsList::ExpandReason reason)
{
    // qDebug() << "Checking for need to scroll due to expand";
    int topItemSelIndex = widgetLocationsList_->selectableIndex(topViewportSelectableLocationId());
    int regionSelIndex = widgetLocationsList_->selectableIndex(region->getId());
    int regionViewportIndex = regionSelIndex - topItemSelIndex;
    int bottomCityViewportIndex  = regionViewportIndex + region->cityWidgets().count();

    int diff = bottomCityViewportIndex - countOfAvailableItemSlots_;
    if (diff >= 0)
    {
        int change = diff + 1;
        if (change > regionViewportIndex) change = regionViewportIndex;

        // qDebug() << "Expanding auto-scroll by items: " << change;
        if (reason == WidgetLocationsList::EXPAND_REASON_AUTO) kickPreventMouseSelectionTimer_ = true;
        else kickPreventMouseSelectionTimer_ = false;
        animatedScrollDown(change);
    }
}

void WidgetLocations::onScrollAnimationValueChanged(const QVariant &value)
{
    // qDebug() << "ScrollAnimation: " << value.toInt();
    if (kickPreventMouseSelectionTimer_) preventMouseSelectionTimer_.restart();

    widgetLocationsList_->move(0, value.toInt());
    lastScrollPos_ = widgetLocationsList_->geometry().y();

    // update scroll bar for keypress navigation
    if (!scrollBar_->dragging())
    {
        scrollBar_->forceSetValue(-animationScollTarget_);
    }
    viewport()->update();
}

void WidgetLocations::onScrollBarHandleDragged(int valuePos)
{
    animationScollTarget_ = -valuePos;

    // qDebug() << "Dragged: " << locationItemListWidget_->geometry().y() << " -> " << animationScollTarget_;
    scrollAnimation_.stop();
    scrollAnimation_.setDuration(PROGRAMMATIC_SCROLL_ANIMATION_DURATION);
    scrollAnimation_.setStartValue(widgetLocationsList_->geometry().y());
    scrollAnimation_.setEndValue(animationScollTarget_);
    scrollAnimation_.setDirection(QAbstractAnimation::Forward);
    scrollAnimation_.start();
}

void WidgetLocations::updateWidgetList(QVector<LocationModelItem *> items)
{
    // storing previous location widget state
    QVector<LocationID> expandedLocationIds = widgetLocationsList_->expandedOrExpandingLocationIds();
    LocationID topSelectableLocationIdInViewport = topViewportSelectableLocationId();
    LocationID lastSelectedLocationId = widgetLocationsList_->lastSelectedLocationId();

    qCDebug(LOG_BASIC) << "Updating locations widget list";
    widgetLocationsList_->clearWidgets();
    foreach (LocationModelItem *item, items)
    {
        if (item->title.contains(filterString_, Qt::CaseInsensitive))
        {
            // add item and all children to list
            widgetLocationsList_->addRegionWidget(item);

            foreach (CityModelItem cityItem, item->cities)
            {
                widgetLocationsList_->addCityToRegion(cityItem, item);
            }
        }
        else
        {
            foreach (CityModelItem cityItem, item->cities)
            {
                if (cityItem.city.contains(filterString_, Qt::CaseInsensitive) ||
                    cityItem.city.contains(filterString_, Qt::CaseInsensitive))
                {
                    widgetLocationsList_->addCityToRegion(cityItem, item);
                }
            }
        }
    }
    qCDebug(LOG_BASIC) << "Restoring location widgets state";

    // restoring previous widget state
    widgetLocationsList_->expandLocationIds(expandedLocationIds);
    int indexInNewList = widgetLocationsList_->selectableIndex(topSelectableLocationIdInViewport);
    if (indexInNewList >= 0)
    {
        scrollDown(indexInNewList);
    }
    widgetLocationsList_->selectItem(lastSelectedLocationId);
    qCDebug(LOG_BASIC) << "Done updating location widgets";
}

void WidgetLocations::scrollToIndex(int index)
{
    // qDebug() <<
    int pos = static_cast<int>(LOCATION_ITEM_HEIGHT * G_SCALE * index);
    pos = closestPositionIncrement(pos);
    scrollBar_->forceSetValue(pos);
}

void WidgetLocations::scrollDown(int itemCount)
{
    // Scrollbar values use positive (whilte itemListWidget uses negative geometry)
    int newY = static_cast<int>(widgetLocationsList_->geometry().y() + LOCATION_ITEM_HEIGHT * G_SCALE * itemCount);

    scrollBar_->setValue(newY);
}

void WidgetLocations::animatedScrollDown(int itemCount)
{
    int scrollBy = static_cast<int>(LOCATION_ITEM_HEIGHT * G_SCALE * itemCount);

    if (scrollAnimation_.state() == QAbstractAnimation::Running)
    {
        animationScollTarget_ -= scrollBy;
    }
    else
    {
        animationScollTarget_ = widgetLocationsList_->geometry().y() - scrollBy;
    }

    scrollAnimation_.stop();
    scrollAnimation_.setDuration(PROGRAMMATIC_SCROLL_ANIMATION_DURATION);
    scrollAnimation_.setStartValue(widgetLocationsList_->geometry().y());
    scrollAnimation_.setEndValue(animationScollTarget_);
    scrollAnimation_.setDirection(QAbstractAnimation::Forward);
    scrollAnimation_.start();
}

void WidgetLocations::animatedScrollUp(int itemCount)
{
    int scrollBy = static_cast<int>(LOCATION_ITEM_HEIGHT * G_SCALE * itemCount);

    if (scrollAnimation_.state() == QAbstractAnimation::Running)
    {
        animationScollTarget_ += scrollBy;
    }
    else
    {
        animationScollTarget_ = widgetLocationsList_->geometry().y() + scrollBy;
    }

    scrollAnimation_.stop();
    scrollAnimation_.setDuration(PROGRAMMATIC_SCROLL_ANIMATION_DURATION);
    scrollAnimation_.setStartValue(widgetLocationsList_->geometry().y());
    scrollAnimation_.setEndValue(animationScollTarget_);
    scrollAnimation_.setDirection(QAbstractAnimation::Forward);
    scrollAnimation_.start();
}

const LocationID WidgetLocations::topViewportSelectableLocationId()
{
    int index = qRound(qAbs(widgetLocationsList_->geometry().y())/(LOCATION_ITEM_HEIGHT * G_SCALE));

    auto widgets = widgetLocationsList_->selectableWidgets();
    if (index < 0 || index > widgets.count() - 1)
    {
        qDebug(LOG_BASIC) << "Err: Can't index selectable items with: " << index;
        return LocationID();
    }

    return widgets[index]->getId();
}

int WidgetLocations::viewportOffsetIndex()
{
    int index = qRound(qAbs(widgetLocationsList_->geometry().y())/(LOCATION_ITEM_HEIGHT * G_SCALE));

    // qDebug() << locationItemListWidget_->geometry().y() << " / " << (LOCATION_ITEM_HEIGHT * G_SCALE) <<  " -> " << index;
    return index;
}

int WidgetLocations::accentItemViewportIndex()
{
    if (!widgetLocationsList_->hasAccentItem())
    {
        return -1;
    }
    return viewportIndex(widgetLocationsList_->lastAccentedItemWidget()->getId());
}

int WidgetLocations::viewportIndex(LocationID locationId)
{
    int topItemSelIndex = widgetLocationsList_->selectableIndex(topViewportSelectableLocationId());
    int desiredLocSelIndex = widgetLocationsList_->selectableIndex(locationId);
    int desiredLocationViewportIndex = desiredLocSelIndex - topItemSelIndex;
    return desiredLocationViewportIndex;
}

bool WidgetLocations::locationIdInViewport(LocationID location)
{
    int vi = viewportIndex(location);
    return vi >= 0 && vi < countOfAvailableItemSlots_;
}

bool WidgetLocations::isGlobalPointInViewport(const QPoint &pt)
{
    QPoint pt1 = viewport()->mapToGlobal(QPoint(0,0));
    QRect globalRectOfViewport(pt1.x(), pt1.y(), viewport()->geometry().width(), viewport()->geometry().height());
    return globalRectOfViewport.contains(pt);
}

QRect WidgetLocations::globalLocationsListViewportRect()
{
    int offset = getItemHeight();
    QPoint locationItemListTopLeft(0, geometry().y() - offset);
    locationItemListTopLeft = mapToGlobal(locationItemListTopLeft);

    return QRect(locationItemListTopLeft.x(), locationItemListTopLeft.y(),
                 viewport()->geometry().width(), viewport()->geometry().height());
}

int WidgetLocations::getScrollBarWidth()
{
    return WidgetLocationsSizes::instance().getScrollBarWidth();
}

int WidgetLocations::getItemHeight() const
{
    return WidgetLocationsSizes::instance().getItemHeight();
}

void WidgetLocations::handleTapClick(const QPoint &cursorPos)
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

int WidgetLocations::closestPositionIncrement(int value)
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


} // namespace GuiLocations

