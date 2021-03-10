#include "widgetlocations.h"

#include <QDateTime>
#include <QGestureEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScroller>
#include <QApplication>
#include <qmath.h>
#include "graphicresources/fontmanager.h"
#include "widgetlocationssizes.h"
#include "languagecontroller.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "tooltips/tooltipcontroller.h"
#include "utils/logger.h"
#include "utils/utils.h"


// #include <QDebug>

namespace GuiLocations {


WidgetLocations::WidgetLocations(QWidget *parent, const QString name) : QScrollArea(parent)
  , name_(name)
  , filterString_("")
  , countOfAvailableItemSlots_(7)
  , bIsFreeSession_(false)
  , bShowLatencyInMs_(false)
  , bTapGestureStarted_(false)
  , locationsModel_(NULL)
  , currentScale_(G_SCALE)
  , lastScrollPos_(0)
  , animationScollTarget_(0)
  , heightChanging_(false)
{
    setFrameStyle(QFrame::NoFrame);
    setMouseTracking(true);
    setStyleSheet("background-color: rgba(0,0,0,0)");
    setFocusPolicy(Qt::NoFocus);

    #ifdef Q_OS_WIN
        viewport()->grabGesture(Qt::TapGesture);
        viewport()->grabGesture(Qt::PanGesture);
        viewport()->installEventFilter(this);

        QScroller::grabGesture(viewport(), QScroller::TouchGesture);
        QScroller *scroller = QScroller::scroller(viewport());
        QScrollerProperties properties = scroller->scrollerProperties();
        QVariant overshootPolicy = QVariant::fromValue<QScrollerProperties::OvershootPolicy>(QScrollerProperties::OvershootAlwaysOff);
        properties.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, overshootPolicy);
        properties.setScrollMetric(QScrollerProperties::DecelerationFactor, 0.2);
        properties.setScrollMetric(QScrollerProperties::MaximumVelocity, 0.135);
        scroller->setScrollerProperties(properties);
    #endif


    // scrollbar
    scrollBar_ = new ScrollBar(this);
    setVerticalScrollBar(scrollBar_);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollBar_->setSingleStep(qCeil(LOCATION_ITEM_HEIGHT * G_SCALE)); // scroll by this many px at a time
    scrollBar_->setGeometry(WINDOW_WIDTH * G_SCALE - getScrollBarWidth(), 0, getScrollBarWidth(), 170 * G_SCALE);
    connect(scrollBar_, SIGNAL(handleDragged(int)), SLOT(onScrollBarHandleDragged(int)));
    connect(scrollBar_, SIGNAL(stopScroll(bool)), SLOT(onScrollBarStopScroll(bool)));

    // central widget
    widgetLocationsList_ = new WidgetLocationsList(this, this);
    setWidget(widgetLocationsList_);
    connect(widgetLocationsList_, SIGNAL(heightChanged(int)), SLOT(onLocationItemListWidgetHeightChanged(int)));
    connect(widgetLocationsList_, SIGNAL(favoriteClicked(ItemWidgetCity*,bool)), SLOT(onLocationItemListWidgetFavoriteClicked(ItemWidgetCity *, bool)));
    connect(widgetLocationsList_, SIGNAL(locationIdSelected(LocationID)), SLOT(onLocationItemListWidgetLocationIdSelected(LocationID)));
    connect(widgetLocationsList_, SIGNAL(regionExpanding(ItemWidgetRegion*)), SLOT(onLocationItemListWidgetRegionExpanding(ItemWidgetRegion*)));
    widgetLocationsList_->setGeometry(0,0, WINDOW_WIDTH*G_SCALE - getScrollBarWidth(), 0);
    widgetLocationsList_->show();

    preventMouseSelectionTimer_.start();
    gestureScrollingElapsedTimer_.start();

    connect(&scrollAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onScrollAnimationValueChanged(QVariant)));
    connect(&scrollAnimation_, SIGNAL(finished()), SLOT(onScrollAnimationFinished()));
    connect(&scrollAnimationForKeyPress_, SIGNAL(valueChanged(QVariant)), SLOT(onScrollAnimationForKeyPressValueChanged(QVariant)));
    connect(&scrollAnimationForKeyPress_, SIGNAL(finished()), SLOT(onScrollAnimationForKeyPressFinished()));
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
        // qCDebug(LOG_LOCATION_LIST) << "Filter collapsing items: " << name_;
        widgetLocationsList_->collapseAllLocationsWithoutAnimation();
    }
    else
    {
        // qCDebug(LOG_LOCATION_LIST) << "Filter expanding items: " << name_;
        widgetLocationsList_->expandAllLocationsWithoutAnimation();
    }
    // qCDebug(LOG_LOCATION_LIST) << "Resetting accent item and list pos: " << name_;
    widgetLocationsList_->accentFirstSelectableItemWithoutAnimation();
    scrollToIndex(0);
}

void WidgetLocations::updateScaling()
{
    scrollBar_->setSingleStep(qCeil(LOCATION_ITEM_HEIGHT * G_SCALE)); // scroll by this many px at a time
    scrollBar_->updateCustomStyleSheet();
    widgetLocationsList_->updateScaling();

    const auto scale_adjustment = G_SCALE / currentScale_;
    if (scale_adjustment != 1.0)
    {
        currentScale_ = G_SCALE;
        int pos = (double) -lastScrollPos_  * scale_adjustment; // lastScrollPos_ is negative (from geometry)
        lastScrollPos_ = -closestPositionIncrement(pos);

        // correct listWidgets pos during scale change // delay necessary for reducing scale factor when low in list
        QTimer::singleShot(0, [this](){
            scrollBar_->forceSetValue(-lastScrollPos_); // use + for scrollbar
        });
    }
}

bool WidgetLocations::hasAccentItem()
{
    return widgetLocationsList_->hasAccentItem();
}

LocationID WidgetLocations::accentedItemLocationId()
{
    if (widgetLocationsList_->lastAccentedItemWidget())
    {
        return widgetLocationsList_->lastAccentedItemWidget()->getId();
    }
    return LocationID();
}

void WidgetLocations::accentFirstItem()
{
    widgetLocationsList_->accentFirstSelectableItem();
}

void WidgetLocations::setMuteAccentChanges(bool mute)
{
    widgetLocationsList_->setMuteAccentChanges(mute);
}

bool WidgetLocations::cursorInViewport()
{
    QPoint cursorPos = QCursor::pos();
    QRect rect = globalLocationsListViewportRect();
    return rect.contains(cursorPos);
}

void WidgetLocations::centerCursorOnAccentedItem()
{
    QPoint cursorPos = QCursor::pos();
    IItemWidget *lastSelWidget = widgetLocationsList_->lastAccentedItemWidget();

    if (lastSelWidget)
    {
        // qDebug() << "Last accented: " << lastSelWidget->name();
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
    countOfAvailableItemSlots_ = cnt;
}

bool WidgetLocations::isShowLatencyInMs()
{
    return bShowLatencyInMs_;
}

void WidgetLocations::setShowLatencyInMs(bool showLatencyInMs)
{
    bShowLatencyInMs_ = showLatencyInMs;
    const auto widgetList = widgetLocationsList_->cityWidgets();
    for (auto *w : widgetList)
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
    // order of events:
    //      scroll: prepare -> start -> scroll -> canceled
    //      tap   : prepate -> start -> finished
    if (object == viewport() && event->type() == QEvent::Gesture)
    {
        QGestureEvent *ge = static_cast<QGestureEvent *>(event);
        QTapGesture *g = static_cast<QTapGesture *>(ge->gesture(Qt::TapGesture));
        if (g)
        {
            if (g->state() == Qt::GestureStarted)
            {
                qCDebug(LOG_USER) << "Tap Gesture started";
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
                    qCDebug(LOG_USER) << "Tap Gesture finished, selecting by pt: " << pt;
                    widgetLocationsList_->selectWidgetContainingGlobalPt(pt);
                }
            }
            else if (g->state() == Qt::GestureCanceled)
            {
                // this will run if scrolling gestures start coming through
                qCDebug(LOG_USER) << "Tap Gesture canceled - scrolling";
                bTapGestureStarted_ = false;
            }
        }
        QPanGesture *gp = static_cast<QPanGesture *>(ge->gesture(Qt::PanGesture));
        if (gp)
        {
            qCDebug(LOG_USER) << "Pan gesture";
            if (gp->state() == Qt::GestureStarted)
            {
                qCDebug(LOG_USER) << "Pan gesture started";
                bTapGestureStarted_ = false;
            }
        }
        return true;
    }
    else if (object == viewport() && event->type() == QEvent::ScrollPrepare)
    {
        // runs before tap and hold/scroll
        QScrollPrepareEvent *se = static_cast<QScrollPrepareEvent *>(event);
        se->setViewportSize(QSizeF(viewport()->size()));
        se->setContentPosRange(QRectF(0, 0, 1, scrollBar_->maximum()));
        se->setContentPos(QPointF(0, scrollBar_->value()));
        se->accept();
        return true;
    }
    else if (object == viewport() && event->type() == QEvent::Scroll)
    {
        // runs while scrolling
        QScrollEvent *se = static_cast<QScrollEvent *>(event);
        int scrollGesturePos = se->contentPos().y();
        gestureScrollingElapsedTimer_.restart();
        int notchedGesturePos = closestPositionIncrement(scrollGesturePos);
        gestureScrollAnimation(notchedGesturePos);
        widgetLocationsList_->accentWidgetContainingCursor();
        return true;
    }

    return QScrollArea::eventFilter(object, event);
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
                    animatedScrollUpByKeyPress(1);
                }
                else
                {
                    QPoint cursorPos = QCursor::pos();
                    if (isGlobalPointInViewport(cursorPos))
                    {
                        TooltipController::instance().hideAllTooltips();

                        IItemWidget *lastSelWidget = widgetLocationsList_->lastAccentedItemWidget();

                        QPoint pt(cursorPos.x(), static_cast<int>(cursorPos.y() - LOCATION_ITEM_HEIGHT*G_SCALE));
                        if (!lastSelWidget->containsCursor()) // accent item does not match with cursor
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
                    animatedScrollDownByKeyPress(1);
                }
                else
                {
                    QPoint cursorPos = QCursor::pos();
                    if (isGlobalPointInViewport(cursorPos))
                    {
                        TooltipController::instance().hideAllTooltips();

                        IItemWidget *lastSelWidget = widgetLocationsList_->lastAccentedItemWidget();

                        QPoint pt(cursorPos.x(), static_cast<int>(cursorPos.y() + LOCATION_ITEM_HEIGHT*G_SCALE));
                        if (!lastSelWidget->containsCursor()) // accent item does not match with cursor
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
            qCDebug(LOG_LOCATION_LIST) << "Accenting first";
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
            emit selected(widgetLocationsList_->lastAccentedLocationId());
        }
        else if (lastSelWidget->getId().isTopLevelLocation())
        {
            if (lastSelWidget->isExpanded())
            {
                widgetLocationsList_->collapse(widgetLocationsList_->lastAccentedLocationId());
            }
            else
            {
                LocationID locId = widgetLocationsList_->lastAccentedLocationId();
                widgetLocationsList_->expand(locId);
                ItemWidgetRegion *widget = widgetLocationsList_->regionWidget(locId);
                regionExpandingAnimation(widget);
            }
        }
        else // city
        {
            if (lastSelWidget)
            {
                if(!lastSelWidget->isForbidden() &&
                   !lastSelWidget->isDisabled()  &&
                   !lastSelWidget->isBrokenConfig())
                {
                    emit selected(widgetLocationsList_->lastAccentedLocationId());
                }
            }
        }
    }
}

int WidgetLocations::gestureScrollingElapsedTime()
{
    return gestureScrollingElapsedTimer_.elapsed();
}

void WidgetLocations::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    // draw background for when list is < size of viewport
    QPainter painter(viewport());
    QRect bkgd(0,0,geometry().width(), geometry().height());
    painter.fillRect(bkgd, FontManager::instance().getMidnightColor());
}

// called by change in the vertical scrollbar
void WidgetLocations::scrollContentsBy(int dx, int dy)
{
    // qDebug() << "Scrolling contents by: " << dy;
    TooltipController::instance().hideAllTooltips();

    if (!scrollBar_->dragging())
    {
        // cursor should not interfere when animation is running
        // prevent floating cursor from overriding a keypress animation
        // this can only occur when accessibility is disabled (since cursor will not be moved)
        if (cursorInViewport() && preventMouseSelectionTimer_.elapsed() > 100)
        {
            widgetLocationsList_->accentWidgetContainingCursor();
        }

        if (!heightChanging_) // prevents false-positive scrolling when changing scale -- setting widgetLocationsList_ geometry in the heightChanged slot races with the manual forceSetValue
        {
            QScrollArea::scrollContentsBy(dx,dy);
            lastScrollPos_ = widgetLocationsList_->geometry().y();
            // qDebug() << "New last after scrolling: " << lastScrollPos_;
        }
    }
}

void WidgetLocations::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    // are we blocking mousePressEvent from bubbling up here intentionally?
    // need note
}

void WidgetLocations::mouseDoubleClickEvent(QMouseEvent *event)
{
    qDebug() << "WidgetLocations::mouseDoubleClickEvent";
    // mousePressEvent instead of double click? need note here
    QScrollArea::mousePressEvent(event);
}

void WidgetLocations::onItemsUpdated(QVector<LocationModelItem *> items)
{
    qCDebug(LOG_LOCATION_LIST) << "Items updated: " << name_;
    updateWidgetList(items);
}

void WidgetLocations::onConnectionSpeedChanged(LocationID id, PingTime timeMs)
{
    // qCDebug(LOG_LOCATION_LIST) << "Search widget speed change";
    const auto widgetList = widgetLocationsList_->cityWidgets();
    for (auto *w : widgetList)
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
    const auto widgetList = widgetLocationsList_->itemWidgets();
    for (auto *region : widgetList)
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
    bIsFreeSession_ = isFreeSessionStatus;
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
    // qDebug() << "List widget height: " << listWidgetHeight << ", geoY: " << widgetLocationsList_->geometry().y();
    heightChanging_ = true;
    widgetLocationsList_->setGeometry(0,widgetLocationsList_->geometry().y(), WINDOW_WIDTH*G_SCALE - getScrollBarWidth(), listWidgetHeight);
    heightChanging_ = false;
}

void WidgetLocations::onLocationItemListWidgetFavoriteClicked(ItemWidgetCity *cityWidget, bool favorited)
{
    // qDebug() << "SearchWidget favorite clicked: " << cityWidget->name();
    emit switchFavorite(cityWidget->getId(), favorited);
}

void WidgetLocations::onLocationItemListWidgetLocationIdSelected(LocationID id)
{
    // gesture scroll will trigger release event on the item -- block this
    if (gestureScrollingElapsedTimer_.elapsed() > 100)
    {
        // qDebug() << "Not blocking gesture-click";
        emit selected(id);
    }
}

void WidgetLocations::onLocationItemListWidgetRegionExpanding(ItemWidgetRegion *region)
{
    int regionVI = regionViewportIndex(region);
    int diff = regionOutOfViewBy(region);
    if (diff >= 0)
    {
        int change = diff + 1;
        if (change > regionVI) change = regionVI;

        // qDebug() << "Expanding auto-scroll by items: " << change;
        animatedScrollDown(change);
    }
}

void WidgetLocations::onScrollAnimationValueChanged(const QVariant &value)
{
    // qDebug() << "ScrollAnimation: " << value.toInt();

    widgetLocationsList_->move(0, value.toInt());
    lastScrollPos_ = widgetLocationsList_->geometry().y();
    viewport()->update();
}

void WidgetLocations::onScrollAnimationFinished()
{
    updateScrollBarWithView();
}

void WidgetLocations::onScrollAnimationForKeyPressValueChanged(const QVariant &value)
{
    // qDebug() << "ScrollAnimationForKeyPress: " << value.toInt() << ", Kicking mouse prevention timer (keyPress)";
    if (!Utils::accessibilityPermissions())
    {
        preventMouseSelectionTimer_.restart();
    }

    widgetLocationsList_->move(0, value.toInt());
    lastScrollPos_ = widgetLocationsList_->geometry().y();
    viewport()->update();

}

void WidgetLocations::onScrollAnimationForKeyPressFinished()
{
    updateScrollBarWithView();
}

void WidgetLocations::onScrollBarHandleDragged(int valuePos)
{
    animationScollTarget_ = -valuePos;

    // qDebug() << "Dragged: " << locationItemListWidget_->geometry().y() << " -> " << animationScollTarget_;
    startAnimationScrollByPosition(animationScollTarget_, scrollAnimation_);
}

void WidgetLocations::onScrollBarStopScroll(bool lastScrollDirectionUp)
{
    if (lastScrollDirectionUp)
    {
        int nextPos = previousPositionIncrement(-widgetLocationsList_->geometry().y());
        animationScollTarget_ = -nextPos;
        startAnimationScrollByPosition(animationScollTarget_, scrollAnimation_);
    }
    else
    {
        int nextPos = nextPositionIncrement(-widgetLocationsList_->geometry().y());
        animationScollTarget_ = -nextPos;
        startAnimationScrollByPosition(animationScollTarget_, scrollAnimation_);
    }
}

void WidgetLocations::updateWidgetList(QVector<LocationModelItem *> items)
{
    qCDebug(LOG_LOCATION_LIST) << name_ << " updating locations widget list";

    // storing previous location widget state
    QVector<LocationID> expandedLocationIds = widgetLocationsList_->expandedOrExpandingLocationIds();
    LocationID topSelectableLocationIdInViewport = topViewportSelectableLocationId();
    LocationID lastAccentedLocationId = widgetLocationsList_->lastAccentedLocationId();

    //qCDebug(LOG_LOCATION_LIST) << name_ << " caching previous display state ";
    widgetLocationsList_->clearWidgets();
    for (LocationModelItem *item: qAsConst(items))
    {
        if (item->title.contains(filterString_, Qt::CaseInsensitive))
        {
            // add item and all children to list
            widgetLocationsList_->addRegionWidget(item);

            for (const CityModelItem &cityItem: qAsConst(item->cities))
            {
                widgetLocationsList_->addCityToRegion(cityItem, item);
            }
        }
        else
        {
            for (const CityModelItem &cityItem: qAsConst(item->cities))
            {
                if (cityItem.city.contains(filterString_, Qt::CaseInsensitive) ||
                    cityItem.nick.contains(filterString_, Qt::CaseInsensitive))
                {
                    widgetLocationsList_->addCityToRegion(cityItem, item);
                }
            }
        }
    }
    qCDebug(LOG_LOCATION_LIST) << name_ << " restoring display state";

    // restoring previous widget state
    widgetLocationsList_->expandLocationIds(expandedLocationIds);
    int indexInNewList = widgetLocationsList_->selectableIndex(topSelectableLocationIdInViewport);
    if (indexInNewList >= 0)
    {
        scrollDown(indexInNewList);
    }
    widgetLocationsList_->accentItemWithoutAnimation(lastAccentedLocationId);
    // qCDebug(LOG_LOCATION_LIST) << name_ << " done updating locations widget list";
}

void WidgetLocations::scrollToIndex(int index)
{
    int pos = static_cast<int>(qCeil(LOCATION_ITEM_HEIGHT * G_SCALE) * index);
    pos = closestPositionIncrement(pos);
    // qDebug() << "ScrollToIndex position: " << pos;
    scrollBar_->forceSetValue(pos);
}

void WidgetLocations::scrollDown(int itemCount)
{
    // Scrollbar values use positive (whilte itemListWidget uses negative geometry)
    int newY = static_cast<int>(widgetLocationsList_->geometry().y() + qCeil(LOCATION_ITEM_HEIGHT * G_SCALE) * itemCount);

    scrollBar_->setValue(newY);
}

void WidgetLocations::animatedScrollDown(int itemCount)
{
    int scrollBy = static_cast<int>(qCeil(LOCATION_ITEM_HEIGHT * G_SCALE) * itemCount);

    if (scrollAnimation_.state() == QAbstractAnimation::Running)
    {
        animationScollTarget_ -= scrollBy;
    }
    else
    {
        animationScollTarget_ = widgetLocationsList_->geometry().y() - scrollBy;
    }
    startAnimationScrollByPosition(animationScollTarget_, scrollAnimation_);
}

void WidgetLocations::animatedScrollUp(int itemCount)
{
    int scrollBy = static_cast<int>(qCeil(LOCATION_ITEM_HEIGHT * G_SCALE) * itemCount);

    if (scrollAnimation_.state() == QAbstractAnimation::Running)
    {
        animationScollTarget_ += scrollBy;
    }
    else
    {
        animationScollTarget_ = widgetLocationsList_->geometry().y() + scrollBy;
    }
    startAnimationScrollByPosition(animationScollTarget_, scrollAnimation_);
}

void WidgetLocations::animatedScrollDownByKeyPress(int itemCount)
{
    int scrollBy = static_cast<int>(qCeil(LOCATION_ITEM_HEIGHT * G_SCALE) * itemCount);

    if (scrollAnimationForKeyPress_.state() == QAbstractAnimation::Running)
    {
        updateScrollBarWithView();
        animationScollTarget_ -= scrollBy;
        if (animationScollTarget_ < -scrollBar_->maximum()) animationScollTarget_ = -scrollBar_->maximum(); // prevent scrolling too far during key press animation
    }
    else
    {
        animationScollTarget_ = widgetLocationsList_->geometry().y() - scrollBy;
    }

    startAnimationScrollByPosition(animationScollTarget_, scrollAnimationForKeyPress_);
}

void WidgetLocations::animatedScrollUpByKeyPress(int itemCount)
{
    int scrollBy = static_cast<int>(qCeil(LOCATION_ITEM_HEIGHT * G_SCALE) * itemCount);

    if (scrollAnimationForKeyPress_.state() == QAbstractAnimation::Running)
    {
        updateScrollBarWithView();
        animationScollTarget_ += scrollBy;
        if (animationScollTarget_ > 0) animationScollTarget_ = 0; // prevent scrolling too far during key press animation
    }
    else
    {
        animationScollTarget_ = widgetLocationsList_->geometry().y() + scrollBy;
    }

    startAnimationScrollByPosition(animationScollTarget_, scrollAnimationForKeyPress_);
}

void WidgetLocations::startAnimationScrollByPosition(int positionValue, QVariantAnimation &animation)
{
    animation.stop();
    animation.setDuration(PROGRAMMATIC_SCROLL_ANIMATION_DURATION);
    animation.setStartValue(widgetLocationsList_->geometry().y());
    animation.setEndValue(positionValue);
    animation.setDirection(QAbstractAnimation::Forward);
    animation.start();
}

void WidgetLocations::gestureScrollAnimation(int value)
{
    animationScollTarget_ = -value;

    scrollAnimation_.stop();
    scrollAnimation_.setDuration(GESTURE_SCROLL_ANIMATION_DURATION);
    scrollAnimation_.setStartValue(widgetLocationsList_->geometry().y());
    scrollAnimation_.setEndValue(animationScollTarget_);
    scrollAnimation_.setDirection(QAbstractAnimation::Forward);
    scrollAnimation_.start();
    qApp->processEvents(); // animate scrolling at same time as gesture is moving
}

void WidgetLocations::updateScrollBarWithView()
{
    if (!scrollBar_->dragging())
    {
        scrollBar_->forceSetValue(-widgetLocationsList_->geometry().y());
    }
}

const LocationID WidgetLocations::topViewportSelectableLocationId()
{
    int index = viewportOffsetIndex();

    auto widgets = widgetLocationsList_->selectableWidgets();
    if (index < 0 || index > widgets.count() - 1)
    {
        // qDebug(LOG_BASIC) << "Err: Can't index selectable items with: " << index;
        return LocationID();
    }

    return widgets[index]->getId();
}

int WidgetLocations::viewportOffsetIndex()
{
    int index = qRound(static_cast<double>(qAbs(widgetLocationsList_->geometry().y())/(qCeil(LOCATION_ITEM_HEIGHT * G_SCALE))));

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

int WidgetLocations::regionViewportIndex(ItemWidgetRegion *region)
{
    int topItemSelIndex = widgetLocationsList_->selectableIndex(topViewportSelectableLocationId());
    int regionSelIndex = widgetLocationsList_->selectableIndex(region->getId());
    return regionSelIndex - topItemSelIndex;
}

int WidgetLocations::regionOutOfViewBy(ItemWidgetRegion *region)
{
    // qDebug() << "Checking for need to scroll due to expand";
    int rvi = regionViewportIndex(region);
    int bottomCityViewportIndex  = rvi + region->cityWidgets().count();
    return bottomCityViewportIndex - countOfAvailableItemSlots_;
}

int WidgetLocations::getScrollBarWidth()
{
    return WidgetLocationsSizes::instance().getScrollBarWidth();
}

int WidgetLocations::getItemHeight() const
{
    return WidgetLocationsSizes::instance().getItemHeight();
}

int WidgetLocations::closestPositionIncrement(int value)
{
    int current = 0;
    int last = 0;
    while (current < value)
    {
        last = current;
        current += qCeil(LOCATION_ITEM_HEIGHT * G_SCALE);
    }

    if (qAbs(current - value) < qAbs(last - value))
    {
        return current;
    }
    return last;
}

int WidgetLocations::nextPositionIncrement(int value)
{
    int current = 0;
    while (current < value)
    {
        current += qCeil(LOCATION_ITEM_HEIGHT * G_SCALE);
    }

    return current;
}

int WidgetLocations::previousPositionIncrement(int value)
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

void WidgetLocations::regionExpandingAnimation(ItemWidgetRegion *region)
{
    int regionVI = regionViewportIndex(region);
    int diff = regionOutOfViewBy(region);
    if (diff >= 0)
    {
        int change = diff + 1;
        if (change > regionVI) change = regionVI;

        // qDebug() << "Expanding auto-scroll by items: " << change;
        animatedScrollDownByKeyPress(change);
    }
}


} // namespace GuiLocations

