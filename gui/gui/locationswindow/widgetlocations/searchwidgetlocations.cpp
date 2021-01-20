#include "searchwidgetlocations.h"

#include <QDateTime>
#include <QGestureEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScroller>
#include <qmath.h>
#include "graphicresources/fontmanager.h"
#include "cursorupdatehelper.h"
#include "widgetlocationssizes.h"
#include "languagecontroller.h"
#include "dpiscalemanager.h"
#include "commongraphics/commongraphics.h"
#include "tooltips/tooltipcontroller.h"

#include <QDebug>

namespace GuiLocations {

SearchWidgetLocations::SearchWidgetLocations(QWidget *parent) : QScrollArea(parent)
  , filterString_("")
  , topInd_(0)
  , topOffs_(0)
  , indSelected_(-1)
  , indParentPressed_(-1)
  , indChildPressed_(-1)
  , indChildFavourite_(-1)
  , countOfAvailableItemSlots_(7)
  , bIsFreeSession_(false)
  , bestLocation_(nullptr)
  , bestLocationName_("")
  , currentScale_(G_SCALE)
  , bMouseInViewport_(false)
  , bShowLatencyInMs_(false)
  , bTapGestureStarted_(false)
  , locationsModel_(NULL)
{
    setFrameStyle(QFrame::NoFrame);
    setMouseTracking(true);
    setStyleSheet("background-color: rgba(0,0,0,0)");

    scrollBar_ = new ScrollBar(this);
    scrollBar_->setStyleSheet(scrollbarStyleSheet());
    setVerticalScrollBar(scrollBar_);
    locationItemListWidget_ = new LocationItemListWidget(this, this);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    verticalScrollBar()->setSingleStep(LocationItemListWidget::ITEM_HEIGHT * G_SCALE); // scroll by this many px at a time
    scrollBar_->setGeometry(WINDOW_WIDTH * G_SCALE - getScrollBarWidth(), 0, getScrollBarWidth(), 170 * G_SCALE);


    setWidget(locationItemListWidget_);
    connect(locationItemListWidget_, SIGNAL(heightChanged(int)), SLOT(onLocationItemListWidgetHeightChanged(int)));
    connect(locationItemListWidget_, SIGNAL(favoriteClicked(LocationItemCityWidget*,bool)), SLOT(onLocationItemListWidgetFavoriteClicked(LocationItemCityWidget *, bool)));
    connect(locationItemListWidget_, SIGNAL(locationIdSelected(LocationID)), SLOT(onLocationItemListWidgetLocationIdSelected(LocationID)));
    locationItemListWidget_->setGeometry(0,0, WINDOW_WIDTH*G_SCALE, 0);
    locationItemListWidget_->show();

    setFocusPolicy(Qt::NoFocus);
    cursorUpdateHelper_.reset(new CursorUpdateHelper(viewport()));

    connect(&LanguageController::instance(), SIGNAL(languageChanged()), SLOT(onLanguageChanged()));
}

SearchWidgetLocations::~SearchWidgetLocations()
{

}

void SearchWidgetLocations::setFilterString(QString text)
{
    filterString_ = text;
}

bool SearchWidgetLocations::cursorInViewport()
{
    QPoint cursorPos = QCursor::pos();
    QRect rect = globalLocationsListViewportRect();
    return rect.contains(cursorPos);
}

bool SearchWidgetLocations::hasSelection()
{
    return locationItemListWidget_->lastSelectedLocationId() != LocationID();
}


void SearchWidgetLocations::centerCursorOnSelectedItem()
{
    // TODO
}

void SearchWidgetLocations::updateSelectionCursorAndToolTipByCursorPos()
{

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

void SearchWidgetLocations::scrollDown(int itemCount)
{
    // Scrollbar values use positive (whilte itemListWidget uses negative geometry)
    int newY = locationItemListWidget_->geometry().y() + LocationItemListWidget::ITEM_HEIGHT * G_SCALE * itemCount;

    // qDebug() << "SearchWidgetLocations is setting scrollbar value: " << newY;
    verticalScrollBar()->setValue(newY);
}

void SearchWidgetLocations::updateWidgetList(QVector<LocationModelItem *> items)
{
    // storing previous location widget state
    QVector<LocationID> expandedLocationIds = locationItemListWidget_->expandedorExpandingLocationIds();
    LocationID topSelectableLocationIdInViewport = locationItemListWidget_->topSelectableLocationIdInViewport();
    LocationID lastSelectedLocationId = locationItemListWidget_->lastSelectedLocationId();

    qDebug() << "Updating search locations widget list";
    locationItemListWidget_->clearWidgets();
    foreach (LocationModelItem *item, items)
    {
        if (item->title.contains(filterString_, Qt::CaseInsensitive))
        {
            // qDebug() << "About to add region and cities";

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
                // qDebug() << "About to add city to region";

                if (cityItem.city.contains(filterString_, Qt::CaseInsensitive) ||
                    cityItem.city.contains(filterString_, Qt::CaseInsensitive))
                {
                    locationItemListWidget_->addCityToRegion(cityItem, item);
                }
            }
        }
    }

    // restoring previous widget state
    locationItemListWidget_->expandLocationIds(expandedLocationIds);
    int indexInNewList = locationItemListWidget_->selectableIndex(topSelectableLocationIdInViewport);
    // qDebug() << "Moving viewport to index: " << indexInNewList;
    scrollDown(indexInNewList);
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

void SearchWidgetLocations::setCurrentSelected(LocationID id)
{

}

void SearchWidgetLocations::setFirstSelected()
{

}

void SearchWidgetLocations::startAnimationWithPixmap(const QPixmap &pixmap)
{
    backgroundPixmapAnimation_.startWith(pixmap, 150);
}

void SearchWidgetLocations::setShowLatencyInMs(bool showLatencyInMs)
{
    bShowLatencyInMs_ = showLatencyInMs;
    foreach (QSharedPointer<LocationItemCityWidget> w, locationItemListWidget_->cityWidgets())
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
        se->setContentPosRange(QRectF(0, 0, 1, verticalScrollBar()->maximum() * getItemHeight()));
        se->setContentPos(QPointF(0, verticalScrollBar()->value() * getItemHeight()));
        se->accept();
        return true;
    }
    else if (object == viewport() && event->type() == QEvent::Scroll)
    {
        QScrollEvent *se = static_cast<QScrollEvent *>(event);
        verticalScrollBar()->setValue(se->contentPos().y() / getItemHeight());
        return true;
    }
    return QAbstractScrollArea::eventFilter(object, event);
}

void SearchWidgetLocations::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

//    QPainter painter(this);
//    QRect bkgd(0,0,geometry().width(), geometry().height());
//    painter.fillRect(bkgd, WidgetLocationsSizes::instance().getBackgroundColor());
}

// called by change in the vertical scrollbar
void SearchWidgetLocations::scrollContentsBy(int dx, int dy)
{
    // qDebug() << "Scrolling contents by: " << dy;

    TooltipController::instance().hideAllTooltips();

    locationItemListWidget_->selectWidgetContainingCursor();

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

bool SearchWidgetLocations::isIdVisible(LocationID id)
{
    bool idVisible = false;
    for (int i = 0; i < currentVisibleItems_.size(); i++)
    {
        auto item = currentVisibleItems_[i];
        if (item->getId() == id)
        {
            idVisible = true;
            break;
        }
    }

    return idVisible;
}

void SearchWidgetLocations::onConnectionSpeedChanged(LocationID id, PingTime timeMs)
{
    // qDebug() << "Search widget speed change";
    foreach (QSharedPointer<LocationItemCityWidget> w, locationItemListWidget_->cityWidgets())
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
    qDebug() << "SearchWidget setting";
    foreach (QSharedPointer<LocationItemRegionWidget> region, locationItemListWidget_->itemWidgets())
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

void SearchWidgetLocations::onTopScrollBarValueChanged(int value)
{
    // verticalScrollBar()->setValue(value);
}

void SearchWidgetLocations::onLanguageChanged()
{
}

void SearchWidgetLocations::onLocationItemListWidgetHeightChanged(int listWidgetHeight)
{
    //qDebug() << "SearchWidgetLocations::onLocationItemListWidgetHeightChanged: " << listWidgetHeight;
    //qDebug() << "SearchLocations height: "<< geometry().height();

    locationItemListWidget_->setGeometry(0,locationItemListWidget_->geometry().y(), WINDOW_WIDTH*G_SCALE, listWidgetHeight);
    verticalScrollBar()->setRange(0, listWidgetHeight - verticalScrollBar()->pageStep()); // update scroll bar
    verticalScrollBar()->setSingleStep(LocationItemListWidget::ITEM_HEIGHT * G_SCALE); // scroll by this many px at a time
}

void SearchWidgetLocations::onLocationItemListWidgetFavoriteClicked(LocationItemCityWidget *cityWidget, bool favorited)
{
    qDebug() << "SearchWidget favorite clicked: " << cityWidget->name();
    emit switchFavorite(cityWidget->getId(), favorited);
}

void SearchWidgetLocations::onLocationItemListWidgetLocationIdSelected(LocationID id)
{
    emit selected(id);
}

bool SearchWidgetLocations::detectSelectedItem(const QPoint &cursorPos)
{

    return false;
}

bool SearchWidgetLocations::detectItemClickOnArrow()
{

    return false;
}

int SearchWidgetLocations::detectItemClickOnFavoriteLocationIcon()
{

    return -1;
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

void SearchWidgetLocations::handleMouseMoveForTooltip()
{

}

void SearchWidgetLocations::clearItems()
{
    Q_FOREACH(LocationItem *item, items_)
    {
        delete item;
    }
    items_.clear();
    currentVisibleItems_.clear();
}

double SearchWidgetLocations::calcScrollingSpeed(double scrollItemsCount)
{
    if (scrollItemsCount < 3)
    {
        scrollItemsCount = 3;
    }
    return WidgetLocationsSizes::instance().getScrollingSpeedKef() * scrollItemsCount;
}

void SearchWidgetLocations::handleKeyEvent(QKeyEvent *event)
{
}

bool SearchWidgetLocations::isGlobalPointInViewport(const QPoint &pt)
{
    QPoint pt1 = viewport()->mapToGlobal(QPoint(0,0));
    QRect globalRectOfViewport(pt1.x(), pt1.y(), viewport()->geometry().width(), viewport()->geometry().height());
    return globalRectOfViewport.contains(pt);
}

void SearchWidgetLocations::scrollDownToSelected()
{

}

void SearchWidgetLocations::scrollUpToSelected()
{

}

void SearchWidgetLocations::handleTapClick(const QPoint &cursorPos)
{

}

void SearchWidgetLocations::emitSelectedIfNeed()
{

}

void SearchWidgetLocations::expandOrCollapseSelectedItem()
{

}

int SearchWidgetLocations::countVisibleItems()
{
    int cntItems = 0;
    Q_FOREACH(LocationItem *item, items_)
    {
        cntItems += item->countVisibleItems();
    }
    return cntItems;
}

void SearchWidgetLocations::updateScaling()
{
    // Update scale-dependent class members.
    Q_ASSERT(currentScale_ > 0);
    const auto scale_adjustment = G_SCALE / currentScale_;
    if (scale_adjustment != 1.0) {
        currentScale_ = G_SCALE;
        topOffs_ *= scale_adjustment;
    }

    for (int i = 0; i < items_.count(); ++i)
    {
        if (items_[i] != nullptr)
        {
            items_[i]->updateScaling();
        }
    }

    locationItemListWidget_->updateScaling();

    // scrollBar_->setGeometry(WINDOW_WIDTH*G_SCALE - getScrollBarWidth(), 0, getScrollBarWidth(), height_ * G_SCALE);
}

int SearchWidgetLocations::countVisibleItemsInViewport(LocationItem *locationItem)
{
    int visibleInViewport = 0;

    if (itemHeaderInViewport(locationItem)) visibleInViewport++;

    QList<int> visibleCityIndexes = cityIndexesInViewport(locationItem);
    for (int i = 0; i < locationItem->cityNodes().length(); i++)
    {
        if (visibleCityIndexes.contains(i))
        {
            visibleInViewport++;
        }
    }

    return visibleInViewport;
}

int SearchWidgetLocations::viewportPosYOfIndex(int index, bool centerY)
{
    int viewportTopLeft = viewport()->geometry().y();
    int result = viewportTopLeft + index * getItemHeight();
    if (centerY) result += getItemHeight()/2;
    return result;
}

int SearchWidgetLocations::viewportIndexOfLocationItemSelection(LocationItem *locationItem)
{
    int visibleIndex = 0;

    bool found = false;
    for (int i = 0; i < currentVisibleItems_.length(); i++)
    {
        LocationItem * item = currentVisibleItems_[i];

        if (item->getId() == locationItem->getId()) // is location header
        {
            if (item->getSelected() == 0) //  header selected
            {
                visibleIndex = viewportIndexOfLocationHeader(item);
            }
            else
            {
                int index = 0;
                if (itemHeaderInViewport(item)) index++;

                QList<int> cityIndicesInViewport = cityIndexesInViewport(item);
                int selectedIndex = cityIndicesInViewport.indexOf(item->getSelected());
                index += cityIndicesInViewport.mid(0, selectedIndex).length()-1;

                visibleIndex += index;
            }
            found = true;
            break;
        }
        else // not in current location
        {
            visibleIndex += countVisibleItemsInViewport(item);
        }
    }

    if (!found) return -1;
    return visibleIndex;
}

QRect SearchWidgetLocations::globalLocationsListViewportRect()
{
    int offset = getItemHeight();
    QPoint locationItemListTopLeft(0, geometry().y() - offset); // TODO: issue?
    locationItemListTopLeft = mapToGlobal(locationItemListTopLeft);

    return QRect(locationItemListTopLeft.x(), locationItemListTopLeft.y(),
                 viewport()->geometry().width(), viewport()->geometry().height());
}

QRect SearchWidgetLocations::locationItemRect(LocationItem *locationItem, bool includeCities)
{
    QRect result;
    QRect geometryRect = viewport()->geometry();
    int curTop = 0;

    Q_FOREACH(LocationItem *item, items_)
    {
        int height = getItemHeight();
        if (includeCities) height = getItemHeight() * item->countVisibleItems();

        QRect rc(0, curTop + getTopOffset() + topOffs_, geometryRect.width(), height);

        if (locationItem->getId() == item->getId())
        {
            result = rc;
            break;
        }

        curTop += getItemHeight();
        int curItemAnimationHeight = item->currentAnimationHeight();

        curTop += curItemAnimationHeight;
    }

    return result;
}

QRect SearchWidgetLocations::cityRect(LocationItem *locationItem, int cityIndex)
{
    QRect result;
    QRect geometryRect = viewport()->geometry();

    Q_FOREACH(LocationItem *item, items_)
    {
        if (locationItem->getId() == item->getId())
        {
            QRect headerRect = locationItemRect(item, false);

            for (int i = 0; i < item->cityNodes().length(); i++)
            {
                if (cityIndex == i)
                {
                    QRect rcCities(0, headerRect.bottom() + i * getItemHeight(), geometryRect.width(), getItemHeight());
                    result = rcCities;
                    break;
                }
            }
        }
    }

    return result;
}

bool SearchWidgetLocations::itemHeaderInViewport(LocationItem *locationItem)
{
    bool result = false;

    QRect vp = viewport()->geometry();

    foreach (LocationItem *item, currentVisibleItems_)
    {
        if (locationItem->getId() == item->getId())
        {
            QRect rect = locationItemRect(item, false);

            if (vp.intersects(rect))
            {
                result = true;
                break;
            }
        }
    }

    return result;
}

QList<int> SearchWidgetLocations::cityIndexesInViewport(LocationItem *locationItem)
{
    QList<int> result;

    QRect vp = viewport()->geometry();

    if (locationItem->isExpandedOrExpanding())
    {
        foreach (LocationItem *item, currentVisibleItems_)
        {
            if (locationItem->getId() == item->getId())
            {
                for (int i = 0; i < item->cityNodes().length(); i++)
                {
                    QRect cr = cityRect(item, i);

                    if (vp.intersects(cr))
                    {
                        result.append(i);
                    }
                }
            }
        }
    }

    return result;
}

int SearchWidgetLocations::viewportIndexOfLocationHeader(LocationItem *locationItem)
{
    int result = -1;

    QRect vp = viewport()->geometry();

    for (int i = 0; i < countOfAvailableItemSlots_; i++)
    {
        QRect slice(0, vp.top() + i *getItemHeight(),vp.width(), getItemHeight());

        if (slice.intersects(locationItemRect(locationItem, false)))
        {
            result = i;
            break;
        }
    }

    return result;
}

} // namespace GuiLocations

