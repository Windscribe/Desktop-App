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


#include <QDebug>

namespace GuiLocations {

SearchWidgetLocations::SearchWidgetLocations(QWidget *parent) : QScrollArea(parent),
    width_(0),
    height_(0),
    topInd_(0),
    topOffs_(0),
    indSelected_(-1),
    indParentPressed_(-1),
    indChildPressed_(-1),
    indChildFavourite_(-1),
    countOfAvailableItemSlots_(7),
    bIsFreeSession_(false),
    bestLocation_(nullptr),
    bestLocationName_(""),
    isScrollAnimationNow_(false),
    currentScale_(G_SCALE),
    bMouseInViewport_(false),
    bShowLatencyInMs_(false),
    bTapGestureStarted_(false),
    locationsModel_(NULL)
  , filterString_("")
{
    setFrameStyle(QFrame::NoFrame);
    setMouseTracking(true);
    setStyleSheet("background-color: rgba(0,0,0,0)");


//    scrollBarHidden_ = new CustomScrollBar(NULL, true);
//    scrollBarOnTop_ = new CustomScrollBar(this, false);
//    scrollBarOnTop_->setGeometry(50, 0, getScrollBarWidth(), 170 * G_SCALE);
//    connect(scrollBarOnTop_, SIGNAL(valueChanged(int)), SLOT(onTopScrollBarValueChanged(int)));

//    scrollBarHidden_->setStyleSheet(QString( "QScrollBar:vertical { margin: %1px 0px %2px 0px; border: none; background: rgba(0, 0, 0, 255); width: %3px; }"
//                                             "QScrollBar::handle:vertical { background: rgb(106, 119, 144); color:  rgb(106, 119, 144);"
//                                             "border-width: %4px; border-style: solid; border-radius: %5px;}"
//                                             "QScrollBar::add-line:vertical { border: none; background: none; }"
//                                             "QScrollBar::sub-line:vertical { border: none; background: none; }")
//                                              .arg(qCeil(0))
//                                              .arg(qCeil(0))
//                                              .arg(qCeil(0))
//                                              .arg(qCeil(0)).arg(qCeil(0)));

//    scrollBarOnTop_->setStyleSheet(QString( "QScrollBar:vertical { margin: %1px 0px %2px 0px; border: none; background: rgba(0, 0, 0, 255); width: %3px; }"
//                                             "QScrollBar::handle:vertical { background: rgb(106, 119, 144); color:  rgb(106, 119, 144);"
//                                             "border-width: %4px; border-style: solid; border-radius: %5px;}"
//                                             "QScrollBar::add-line:vertical { border: none; background: none; }"
//                                             "QScrollBar::sub-line:vertical { border: none; background: none; }")
//                                              .arg(qCeil(CustomScrollBar::SCROLL_BAR_MARGIN))
//                                              .arg(qCeil(CustomScrollBar::SCROLL_BAR_MARGIN))
//                                              .arg(getScrollBarWidth())
//                                              .arg(qCeil(4)).arg(qCeil(3)));

    scrollBar_ = new ScrollBar(this);
    setVerticalScrollBar(scrollBar_);
    locationItemListWidget_ = new LocationItemListWidget(this);
    setWidget(locationItemListWidget_);
    connect(locationItemListWidget_, SIGNAL(heightChanged(int)), SLOT(onLocationItemListWidgetHeightChanged(int)));
    locationItemListWidget_->setGeometry(0,0, WINDOW_WIDTH*G_SCALE, 0);
    locationItemListWidget_->show();


    verticalScrollBar()->setSingleStep(50); // scroll by this many px at a time
    // verticalScrollBar()->setSizeIncrement(QSize(1,1));
    //setVerticalScrollBar(scrollBarHidden_);
    //this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    setFocusPolicy(Qt::NoFocus);
    cursorUpdateHelper_.reset(new CursorUpdateHelper(viewport()));

    connect(&LanguageController::instance(), SIGNAL(languageChanged()), SLOT(onLanguageChanged()));
    // setupScrollBar();
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
    return indSelected_ != -1;
}


void SearchWidgetLocations::centerCursorOnSelectedItem()
{

}

void SearchWidgetLocations::updateSelectionCursorAndToolTipByCursorPos()
{

}

void SearchWidgetLocations::updateWidgetList(QVector<LocationModelItem *> items)
{
    qDebug() << "Updating location widgets";

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

    qDebug() << "Done updating location widgets";
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

//QSize SearchWidgetLocations::sizeHint() const
//{
//    return QSize(355, getItemHeight() * countOfAvailableItemSlots_ + (getTopOffset() - 1));
//}

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

    //QPainter painter(viewport());
    //QRect bkgd(0,0,geometry().width(), geometry().height());
    // painter.fillRect(bkgd, Qt::black);
}

// called by change in the vertical scrollbar
void SearchWidgetLocations::scrollContentsBy(int dx, int dy)
{
    // qDebug() << "Scrolling contents by: " << dy;

    // verticalScrollBar()->value();


    QScrollArea::scrollContentsBy(dx,dy);
    //update();

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
    // scrollBarOnTop_->setGeometry(width_*G_SCALE - getScrollBarWidth(), 0, getScrollBarWidth(), height_ * G_SCALE);
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

}

void SearchWidgetLocations::onIsFavoriteChanged(LocationID id, bool isFavorite)
{

}

void SearchWidgetLocations::onFreeSessionStatusChanged(bool isFreeSessionStatus)
{


}

void SearchWidgetLocations::onTopScrollBarValueChanged(int value)
{
    // verticalScrollBar()->setValue(value);
}

void SearchWidgetLocations::onLanguageChanged()
{
}

void SearchWidgetLocations::onLocationItemListWidgetHeightChanged(int height)
{
    locationItemListWidget_->setGeometry(0,0, WINDOW_WIDTH*G_SCALE, height);
    // TODO: update scrollbar size

}

void SearchWidgetLocations::calcScrollPosition()
{
    double d = (double)scrollAnimationElapsedTimer_.elapsed() / (double)scrollAnimationDuration_;
    topOffs_ = startAnimationTop_ + (endAnimationTop_ - startAnimationTop_) * d;

    if ((endAnimationTop_ > startAnimationTop_ && topOffs_ >= endAnimationTop_) ||
      (endAnimationTop_ < startAnimationTop_ && topOffs_ <= endAnimationTop_))
    {
        topOffs_ = endAnimationTop_;
        isScrollAnimationNow_ = false;
        //setBottomFlag(topInd_);
    }

    if (topOffs_ > 0)
    {
        topOffs_ = endAnimationTop_;
        isScrollAnimationNow_ = false;
    }
}

void SearchWidgetLocations::setupScrollBar()
{
    verticalScrollBar()->setMinimum(0);
    verticalScrollBar()->setSingleStep(1);
    verticalScrollBar()->setPageStep(countOfAvailableItemSlots_);

    scrollBarOnTop_->setMinimum(0);
    scrollBarOnTop_->setSingleStep(1);
    scrollBarOnTop_->setPageStep(countOfAvailableItemSlots_);
}

void SearchWidgetLocations::setupScrollBarMaxValue()
{
    int cntItems = countVisibleItems();

    if ((cntItems - countOfAvailableItemSlots_) > 0)
    {
        scrollBarOnTop_->setMaximum(cntItems - countOfAvailableItemSlots_);
        verticalScrollBar()->setMaximum(cntItems - countOfAvailableItemSlots_);
        if (!scrollBarOnTop_->isVisible())
        {
            scrollBarOnTop_->show();
        }
    }
    else
    {
        scrollBarOnTop_->setMaximum(0);
        verticalScrollBar()->setMaximum(0);
        scrollBarOnTop_->hide();
    }
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

void SearchWidgetLocations::handleLeaveForTooltip()
{
//    if (!bMouseInViewport_)
//    {
//        emit hideTooltip(TOOLTIP_ID_LOCATIONS_P2P);
//        emit hideTooltip(TOOLTIP_ID_LOCATIONS_PING_TIME);
//        emit hideTooltip(TOOLTIP_ID_LOCATIONS_ERROR_MESSAGE);
//    }
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

void SearchWidgetLocations::setSize(int width, int height)
{
    width_ = width;
    height_ = height;
    update();
}

void SearchWidgetLocations::updateScaling()
{
    // Update scale-dependent class members.
    Q_ASSERT(currentScale_ > 0);
    const auto scale_adjustment = G_SCALE / currentScale_;
    if (scale_adjustment != 1.0) {
        currentScale_ = G_SCALE;
        topOffs_ *= scale_adjustment;
        startAnimationTop_ *= scale_adjustment;
        endAnimationTop_ *= scale_adjustment;
    }

    for (int i = 0; i < items_.count(); ++i)
    {
        if (items_[i] != nullptr)
        {
            items_[i]->updateScaling();
        }
    }

    locationItemListWidget_->updateScaling();

    // scrollBarOnTop_->setGeometry(width_*G_SCALE - getScrollBarWidth(), 0, getScrollBarWidth(), height_ * G_SCALE);
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
