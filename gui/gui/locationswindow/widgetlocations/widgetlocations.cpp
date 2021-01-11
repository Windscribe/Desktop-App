#include "widgetlocations.h"
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
#include <QDebug>

namespace GuiLocations {

WidgetLocations::WidgetLocations(QWidget *parent) : QAbstractScrollArea(parent),
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
    bExpanded_(false),
    bShowLatencyInMs_(false),
    bTapGestureStarted_(false),
    locationsModel_(NULL)
{
    setFrameStyle(QFrame::NoFrame);
    setMouseTracking(true);
    setStyleSheet("background-color: rgba(0,0,0,0)");

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

    scrollBarHidden_ = new CustomScrollBar(NULL, true);
    scrollBarOnTop_ = new CustomScrollBar(this, false);
    scrollBarOnTop_->setGeometry(50, 0, getScrollBarWidth(), 170 * G_SCALE);
    connect(scrollBarOnTop_, SIGNAL(valueChanged(int)), SLOT(onTopScrollBarValueChanged(int)));

    scrollBarHidden_->setStyleSheet(QString( "QScrollBar:vertical { margin: %1px 0px %2px 0px; border: none; background: rgba(0, 0, 0, 255); width: %3px; }"
                                             "QScrollBar::handle:vertical { background: rgb(106, 119, 144); color:  rgb(106, 119, 144);"
                                             "border-width: %4px; border-style: solid; border-radius: %5px;}"
                                             "QScrollBar::add-line:vertical { border: none; background: none; }"
                                             "QScrollBar::sub-line:vertical { border: none; background: none; }")
                                              .arg(qCeil(0))
                                              .arg(qCeil(0))
                                              .arg(qCeil(0))
                                              .arg(qCeil(0)).arg(qCeil(0)));

    scrollBarOnTop_->setStyleSheet(QString( "QScrollBar:vertical { margin: %1px 0px %2px 0px; border: none; background: rgba(0, 0, 0, 255); width: %3px; }"
                                             "QScrollBar::handle:vertical { background: rgb(106, 119, 144); color:  rgb(106, 119, 144);"
                                             "border-width: %4px; border-style: solid; border-radius: %5px;}"
                                             "QScrollBar::add-line:vertical { border: none; background: none; }"
                                             "QScrollBar::sub-line:vertical { border: none; background: none; }")
                                              .arg(qCeil(CustomScrollBar::SCROLL_BAR_MARGIN))
                                              .arg(qCeil(CustomScrollBar::SCROLL_BAR_MARGIN))
                                              .arg(getScrollBarWidth())
                                              .arg(qCeil(4)).arg(qCeil(3)));

    setVerticalScrollBar(scrollBarHidden_);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    setFocusPolicy(Qt::NoFocus);
    easingCurve_.setType(QEasingCurve::Linear);
    cursorUpdateHelper_.reset(new CursorUpdateHelper(viewport()));

    connect(&LanguageController::instance(), SIGNAL(languageChanged()), SLOT(onLanguageChanged()));
    setupScrollBar();
}

WidgetLocations::~WidgetLocations()
{
    clearItems();
}

bool WidgetLocations::cursorInViewport()
{
    QPoint cursorPos = QCursor::pos();
    QRect rect = globalLocationsListViewportRect();
    return rect.contains(cursorPos);
}

bool WidgetLocations::hasSelection()
{
    return indSelected_ != -1;
}


void WidgetLocations::centerCursorOnSelectedItem()
{
    if (indSelected_ != -1)
    {
        int visibleIndexOfLocation = viewportIndexOfLocationItemSelection(items_[indSelected_]);

        if (visibleIndexOfLocation != -1)
        {
            int posY = globalLocationsListViewportRect().y() + viewportPosYOfIndex(visibleIndexOfLocation, true);
            QCursor::setPos(QPoint(QCursor::pos().x(), posY));
        }
    }
    else
    {
        updateSelectionCursorAndToolTipByCursorPos();
    }
}

void WidgetLocations::updateSelectionCursorAndToolTipByCursorPos()
{
    detectSelectedItem(QCursor::pos());
    setCursorForSelected();
    handleMouseMoveForTooltip();
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

void WidgetLocations::setCurrentSelected(LocationID id)
{
    isScrollAnimationNow_ = false;
    int curAllInd = 0;
    for (int i = 0; i < items_.count(); ++i)
    {
        if (items_[i]->getId() == id)
        {
            if (id.isTopLevelLocation())
            {
                if (indSelected_ != -1)
                {
                    items_[indSelected_]->setSelected(-1);
                }
                items_[i]->setSelected(0);
                indSelected_ = i;

                verticalScrollBar()->blockSignals(true);
                verticalScrollBar()->setValue(curAllInd);
                verticalScrollBar()->blockSignals(false);

                scrollBarOnTop_->blockSignals(true);
                scrollBarOnTop_->setValue(curAllInd);
                scrollBarOnTop_->blockSignals(false);

                topInd_ = -verticalScrollBar()->value();
                topOffs_ = getItemHeight() * topInd_;

                viewport()->update();
            }
            else
            {
                items_[i]->setExpandedWithoutAnimation();
                setupScrollBarMaxValue();

                int cityInd = items_[i]->findCityInd(id);
                if (cityInd != -1)
                {
                    if (indSelected_ != -1)
                    {
                        items_[indSelected_]->setSelected(-1);
                    }
                    items_[i]->setSelected(cityInd + 1);
                    indSelected_ = i;

                    verticalScrollBar()->blockSignals(true);
                    if (cityInd < (countOfAvailableItemSlots_ - 1))
                    {
                        verticalScrollBar()->setValue(curAllInd);
                    }
                    else
                    {
                        verticalScrollBar()->setValue(curAllInd + cityInd + 1 - (countOfAvailableItemSlots_ - 1));
                    }
                    verticalScrollBar()->blockSignals(false);

                    scrollBarOnTop_->blockSignals(true);
                    if (cityInd < (countOfAvailableItemSlots_ - 1))
                    {
                        scrollBarOnTop_->setValue(curAllInd);
                    }
                    else
                    {
                        scrollBarOnTop_->setValue(curAllInd + cityInd + 1 - (countOfAvailableItemSlots_ - 1));
                    }
                    scrollBarOnTop_->blockSignals(false);


                    topInd_ = -verticalScrollBar()->value();
                    topOffs_ = getItemHeight() * topInd_;

                    viewport()->update();
                }
            }
            break;
        }
        curAllInd += items_[i]->countVisibleItems();
    }
}

void WidgetLocations::setFirstSelected()
{
    if (items_.count() > 0)
    {
        setCurrentSelected(items_[0]->getId());
    }
}

void WidgetLocations::setExpanded(bool bExpanded)
{
    bExpanded_ = bExpanded;
}

void WidgetLocations::startAnimationWithPixmap(const QPixmap &pixmap)
{
    backgroundPixmapAnimation_.startWith(pixmap, 150);
}

void WidgetLocations::setShowLatencyInMs(bool showLatencyInMs)
{
    bShowLatencyInMs_ = showLatencyInMs;
    viewport()->update();
}

bool WidgetLocations::isShowLatencyInMs()
{
    return bShowLatencyInMs_;
}

bool WidgetLocations::isFreeSessionStatus()
{
    return bIsFreeSession_;
}

int WidgetLocations::getWidth()
{
    return viewport()->width();
}

int WidgetLocations::getScrollBarWidth()
{
    return WidgetLocationsSizes::instance().getScrollBarWidth();
}

void WidgetLocations::setCountAvailableItemSlots(int cnt)
{
    if (countOfAvailableItemSlots_ != cnt)
    {
        countOfAvailableItemSlots_ = cnt;
        setupScrollBar();
        setupScrollBarMaxValue();
        viewport()->update();
    }
}

int WidgetLocations::getCountAvailableItemSlots()
{
    return countOfAvailableItemSlots_;
}

int WidgetLocations::getItemHeight() const
{
    return WidgetLocationsSizes::instance().getItemHeight();
}

int WidgetLocations::getTopOffset() const
{
    return WidgetLocationsSizes::instance().getTopOffset();
}

QSize WidgetLocations::sizeHint() const
{
    return QSize(355, getItemHeight() * countOfAvailableItemSlots_ + (getTopOffset() - 1));
}

void WidgetLocations::onKeyPressEvent(QKeyEvent *event)
{
    handleKeyEvent((QKeyEvent *)event);
}

bool WidgetLocations::eventFilter(QObject *object, QEvent *event)
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

void WidgetLocations::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    if (isScrollAnimationNow_)
    {
        bool bCursorInViewport = isGlobalPointInViewport(QCursor::pos());

        calcScrollPosition();
        if (bCursorInViewport)
        {
            detectSelectedItem(QCursor::pos());
            setCursorForSelected();
        }

        if (!isScrollAnimationNow_)
        {
            prevCursorPos_ = QPoint();
            if (bCursorInViewport)
            {
                handleMouseMoveForTooltip();
            }
        }

        viewport()->update();
    }
    else if (isExpandAnimationNow())
    {
        viewport()->update();
    }
    else if (backgroundPixmapAnimation_.isAnimationActive())
    {
        viewport()->update();
    }

    QPainter painter(viewport());

    QRect geometryRect = viewport()->geometry();

    // painter.fillRect(QRect(0,0, width(), height()), Qt::blue);

    painter.fillRect(QRect(0, 0, geometryRect.width(), geometryRect.height()), QBrush(WidgetLocationsSizes::instance().getBackgroundColor()));
    // painter.fillRect(QRect(0, 0, geometryRect.width(), geometryRect.height()), Qt::green);

    QRect locationsGeometry = QRect(0, getTopOffset(), geometryRect.width(), geometryRect.height() - getTopOffset());
    //painter.fillRect(locationsGeometry, Qt::green);

    // draw locations items and expanded cities items
    int itemInd = 0;
    int curTop = 0;
    QVector<int> linesForScrollbar;
    linesForScrollbar.reserve(10);
    int selectedY1 = -1, selectedY2 = -1;

    currentVisibleItems_.clear();
    Q_FOREACH(LocationItem *item, items_)
    {
        QRect rc(0, curTop + getTopOffset() + topOffs_, geometryRect.width(), getItemHeight()*item->countVisibleItems());
        QRect rcLocationCaption(0, curTop + getTopOffset() + topOffs_, geometryRect.width(), getItemHeight());
        curTop += getItemHeight();

        int curItemAnimationHeight = item->currentAnimationHeight();

        if (locationsGeometry.intersects(rc))
        {
            linesForScrollbar << rcLocationCaption.bottom();

            if (item->getSelected() == 0)
            {
                selectedY1 = rcLocationCaption.top();
                selectedY2 = rcLocationCaption.bottom();
            }

            item->drawLocationCaption(&painter, rcLocationCaption);
            currentVisibleItems_.append(item);

            if (item->citySubMenuState() != LocationItem::COLLAPSED)
            {
                QRect rcCities(0, curTop + getTopOffset() + topOffs_, geometryRect.width(), item->currentAnimationHeight());
                item->drawCities(&painter, rcCities, linesForScrollbar, selectedY1, selectedY2);
            }
        }
        curTop += curItemAnimationHeight;

        itemInd++;
    }

    if (backgroundPixmapAnimation_.isAnimationActive())
    {
        painter.setOpacity(backgroundPixmapAnimation_.curOpacity());
        painter.drawPixmap(0, 0, backgroundPixmapAnimation_.curPixmap());
    }
}

void WidgetLocations::scrollContentsBy(int dx, int dy)
{
    Q_UNUSED(dx);
    Q_UNUSED(dy);
    //clearBottomFlags();

    if (verticalScrollBar()->value() != scrollBarOnTop_->value())
    {
        scrollBarOnTop_->setValue(verticalScrollBar()->value());
    }

    emit hideTooltip(TOOLTIP_ID_LOCATIONS_P2P);
    emit hideTooltip(TOOLTIP_ID_LOCATIONS_PING_TIME);
    emit hideTooltip(TOOLTIP_ID_LOCATIONS_ERROR_MESSAGE);

    startAnimationTop_ = topOffs_;
    endAnimationTop_ = -getItemHeight() * verticalScrollBar()->value();

    int scrollAreaHeight;
    if (startAnimationTop_ < endAnimationTop_)
    {
        scrollAreaHeight = endAnimationTop_ - startAnimationTop_;
    }
    else if (startAnimationTop_ > endAnimationTop_)
    {
        scrollAreaHeight = startAnimationTop_ - endAnimationTop_;
    }
    else
    {
        topInd_ = -verticalScrollBar()->value();
        isScrollAnimationNow_ = false;
        cursorUpdateHelper_->setPointingHandCursor();
        viewport()->update();
        return;
    }

    double countScrollItems = scrollAreaHeight / ((double)getItemHeight());
    double curSpeed = calcScrollingSpeed(countScrollItems);

    double d = (double)(scrollAreaHeight) / (double)curSpeed;
    scrollAnimationDuration_ = d;
    if (scrollAnimationDuration_ <= 0.01)
    {
        scrollAnimationDuration_ = 1;
    }

    topInd_ = -verticalScrollBar()->value();
    scrollAnimationElapsedTimer_.start();
    isScrollAnimationNow_ = true;
    cursorUpdateHelper_->setPointingHandCursor();
    viewport()->update();
}

void WidgetLocations::mouseMoveEvent(QMouseEvent *event)
{
#ifdef Q_OS_WIN
    if (event->source() == Qt::MouseEventSynthesizedBySystem)
    {
        return;
    }
#endif

    if (!isScrollAnimationNow_)
    {
        if (detectSelectedItem(QCursor::pos()))
        {
            // if something changed, then update viewport
            viewport()->update();
        }
        setCursorForSelected();
        handleMouseMoveForTooltip();
    }
    QAbstractScrollArea::mouseMoveEvent(event);
}

void WidgetLocations::mousePressEvent(QMouseEvent *event)
{
#ifdef Q_OS_WIN
    if (event->source() == Qt::MouseEventSynthesizedBySystem)
    {
        return;
    }
#endif

    if (event->button() == Qt::LeftButton && indSelected_ != -1)
    {
        indParentPressed_ = indSelected_;
        indChildPressed_ = items_[indSelected_]->getSelected();
        indChildFavourite_ = detectItemClickOnFavoriteLocationIcon();
    }
}

void WidgetLocations::mouseReleaseEvent(QMouseEvent *event)
{
#ifdef Q_OS_WIN
    if (event->source() == Qt::MouseEventSynthesizedBySystem)
    {
        return;
    }
#endif

    if (event->button() == Qt::LeftButton)
    {
        if (indSelected_ == indParentPressed_ && isGlobalPointInViewport(QCursor::pos()))
        {
            int cityInd = detectItemClickOnFavoriteLocationIcon();
            if (cityInd != -1 && cityInd == indChildFavourite_) // favorite press and release
            {
                bool isFavorite = items_[indSelected_]->toggleFavorite(cityInd);
                LocationID lid = items_[indSelected_]->getLocationIdForCity(cityInd);
                if (isFavorite)
                {
                    emit switchFavorite(lid, true);
                }
                else
                {
                    emit switchFavorite(lid, false);
                }

                viewport()->update();
            }
            else if (!detectItemClickOnArrow())
            {
                if (indSelected_ != -1 && (cityInd == -1 && indChildFavourite_ == -1)) // valid and non-favourite
                {
                    if(items_[indSelected_]->getSelected() == indChildPressed_) // buttonPressed == buttonReleased
                    {
                        if (items_[indSelected_]->isForceExpand() && indChildPressed_ == 0) // Location
                        {
                            if (items_[indSelected_]->getId().isBestLocation())
                            {
                                emitSelectedIfNeed();
                            }
                            else
                            {
                                expandOrCollapseSelectedItem();
                            }
                        }
                        else // City
                        {
                            emitSelectedIfNeed();
                        }
                    }
                }
            }
        }
    }
}

void WidgetLocations::mouseDoubleClickEvent(QMouseEvent *event)
{
#ifdef Q_OS_WIN
    if (event->source() == Qt::MouseEventSynthesizedBySystem)
    {
        return;
    }
#endif

    mousePressEvent(event);
}

void WidgetLocations::leaveEvent(QEvent *event)
{
    bMouseInViewport_ = false;
    handleLeaveForTooltip();

    Q_FOREACH(LocationItem *item, items_)
    {
        item->mouseLeaveEvent();
    }

    viewport()->update();
    QWidget::leaveEvent(event);
}

void WidgetLocations::enterEvent(QEvent *event)
{
    bMouseInViewport_ = true;
    QWidget::enterEvent(event);
}

void WidgetLocations::resizeEvent(QResizeEvent *event)
{
    QAbstractScrollArea::resizeEvent(event);
    scrollBarOnTop_->setGeometry(width_*G_SCALE - getScrollBarWidth(), 0, getScrollBarWidth(), height_ * G_SCALE);
}

void WidgetLocations::onItemsUpdated(QVector<LocationModelItem *> items)
{
    bool bSaved = false;
    LocationID savedTopLocationId;
    QSet<LocationID> expandedLocationsIds;
    LocationID savedSelectedLocationId;

    if (items_.count() > 0)
    {
        // save location id of top item
        savedTopLocationId = detectLocationForTopInd(topInd_);

        // save expanded locations id
        for (int i = 0; i < items_.count(); ++i)
        {
            if (items_[i]->isExpandedOrExpanding())
            {
                expandedLocationsIds << items_[i]->getId();
            }
        }

        if (indSelected_ != -1)
        {
            int itemSelectedInd = items_[indSelected_]->getSelected();
            if (itemSelectedInd > 0)
            {
                savedSelectedLocationId = items_[indSelected_]->getLocationIdForCity(itemSelectedInd - 1);
            }
            else
            {
                savedSelectedLocationId = items_[indSelected_]->getId();
            }
        }

        bSaved = true;
    }

    clearItems();

    int curInd = 0;
    int newTopInd = -1;
    int allIndCounter = 0;

    indSelected_ = -1;

    Q_FOREACH(const LocationModelItem *lmi, items)
    {
        QVector<CityNode> cities;
        for (int i = 0; i < lmi->cities.count(); ++i)
        {
            cities << CityNode(lmi->cities[i].id, lmi->cities[i].city, lmi->cities[i].nick,
                               lmi->cities[i].countryCode, lmi->cities[i].pingTimeMs,
                               lmi->cities[i].bShowPremiumStarOnly, isShowLatencyInMs(),
                               lmi->cities[i].staticIp, lmi->cities[i].staticIpType,
                               lmi->cities[i].isFavorite, lmi->cities[i].isDisabled,
                               lmi->cities[i].isCustomConfigCorrect,
                               lmi->cities[i].customConfigType,
                               lmi->cities[i].customConfigErrorMessage);
        }
        LocationItem *item = new LocationItem(this, lmi->id, lmi->countryCode, lmi->title,
                                              lmi->isShowP2P, PingTime(), cities, true, lmi->isPremiumOnly);
        items_ << item;

        if (lmi->id.isBestLocation())
        {
            bestLocation_ = item;
            bestLocationName_ = lmi->title;
        }
    }

    Q_FOREACH(LocationItem *item, items_)
    {
        if (bSaved && newTopInd == -1)
        {
            if (item->getId() == savedTopLocationId)
            {
                int cityInd = item->findCityInd(savedTopLocationId);
                if (cityInd != -1)
                {
                    newTopInd = allIndCounter + cityInd + 1;
                }
                else
                {
                    newTopInd = allIndCounter;
                }
            }
        }

        if (bSaved && expandedLocationsIds.contains(item->getId()))
        {
            item->setExpandedWithoutAnimation();
        }

        if (bSaved && savedSelectedLocationId == item->getId())
        {
            indSelected_ = curInd;
            item->setSelectedByCity(savedSelectedLocationId);
        }

        curInd++;
        allIndCounter += item->countVisibleItems();
    }

    if (bSaved)
    {
        if (newTopInd != -1 && allIndCounter > countOfAvailableItemSlots_)
        {
            if (allIndCounter - newTopInd <= countOfAvailableItemSlots_)
            {
                topInd_ = -newTopInd + (countOfAvailableItemSlots_ - (allIndCounter - newTopInd));
            }
            else
            {
                topInd_ = -newTopInd;
            }
        }
        else
        {
            topInd_ = 0;
        }

        topOffs_ = getItemHeight() * topInd_;

        isScrollAnimationNow_ = false;

        verticalScrollBar()->setValue(-topInd_);
        setupScrollBarMaxValue();
        viewport()->update();
    }
    else
    {
        topInd_ = 0;
        topOffs_ = 0;
        isScrollAnimationNow_ = false;

        verticalScrollBar()->blockSignals(true);
        verticalScrollBar()->setValue(0);
        verticalScrollBar()->blockSignals(false);

        scrollBarOnTop_->blockSignals(true);
        scrollBarOnTop_->setValue(0);
        scrollBarOnTop_->blockSignals(false);

        setupScrollBarMaxValue();

        viewport()->update();
    }

    updateScaling();
}

bool WidgetLocations::isIdVisible(LocationID id)
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

void WidgetLocations::onConnectionSpeedChanged(LocationID id, PingTime timeMs)
{
    bool bChanged = false;
    for (int i = 0; i < items_.count(); ++i)
    {
        if (items_[i]->changeSpeedConnection(id, timeMs))
        {
            bChanged = items_[i]->isExpandedOrExpanding();
            break;
        }
    }

    if (bChanged && isIdVisible(id))
    {
        viewport()->update();
    }
}

void WidgetLocations::onIsFavoriteChanged(LocationID id, bool isFavorite)
{
    bool bChanged = false;
    for (int i = 0; i < items_.count(); ++i)
    {
        if (items_[i]->getId().toTopLevelLocation() == id.toTopLevelLocation())
        {
            bChanged = items_[i]->changeIsFavorite(id, isFavorite) && items_[i]->isExpandedOrExpanding();
            break;
        }
    }

    if (bChanged && isIdVisible(id))
    {
        viewport()->update();
    }
}

void WidgetLocations::onFreeSessionStatusChanged(bool isFreeSessionStatus)
{
    if (bIsFreeSession_ != isFreeSessionStatus)
    {
        bIsFreeSession_ = isFreeSessionStatus;

        /*if (bIsFreeSession_)
        {
            // collapse all premium locations
            for (int i = 0; i < items_.count(); ++i)
            {
                items_[i]->collapseForbidden();
            }
            setupScrollBarMaxValue();
        }*/

        handleLeaveForTooltip();

        viewport()->update();
    }

}

void WidgetLocations::onTopScrollBarValueChanged(int value)
{
    verticalScrollBar()->setValue(value);
}

void WidgetLocations::onLanguageChanged()
{
    if (bestLocation_ != nullptr)
    {
        bestLocation_->updateLangauge(tr(bestLocationName_.toStdString().c_str()));
    }
}

void WidgetLocations::calcScrollPosition()
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

void WidgetLocations::setupScrollBar()
{
    verticalScrollBar()->setMinimum(0);
    verticalScrollBar()->setSingleStep(1);
    verticalScrollBar()->setPageStep(countOfAvailableItemSlots_);

    scrollBarOnTop_->setMinimum(0);
    scrollBarOnTop_->setSingleStep(1);
    scrollBarOnTop_->setPageStep(countOfAvailableItemSlots_);
}

void WidgetLocations::setupScrollBarMaxValue()
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

bool WidgetLocations::detectSelectedItem(const QPoint &cursorPos)
{
    int curTop = 0;
    for (int i = 0; i < items_.size(); ++i)
    {
        QPoint pt(0, curTop + getTopOffset() + topOffs_);
        pt = mapToGlobal(pt);

        for (int intersectInd = 0; intersectInd < items_[i]->countVisibleItems(); ++intersectInd)
        {
            QPoint localBottomPt = viewport()->mapFromGlobal(QPoint(pt.x(), pt.y() + intersectInd * getItemHeight()));
            if (localBottomPt.y() >= getTopOffset() &&
                QRect(pt.x(), pt.y() + intersectInd * getItemHeight(), viewport()->geometry().width(), getItemHeight()).contains(cursorPos))
            {
                if (indSelected_ == i)
                {
                    bool bChanged = false;
                    if (items_[indSelected_]->getSelected() != intersectInd)
                    {
                        items_[indSelected_]->setSelected(intersectInd);
                        bChanged = true;
                    }

                    QPoint ptLocal(QPoint(cursorPos.x() - pt.x(), cursorPos.y() - pt.y()));
                    return items_[indSelected_]->mouseMoveEvent(ptLocal) || bChanged;
                }
                else
                {
                    if (indSelected_ != -1)
                    {
                        items_[indSelected_]->setSelected(-1);
                    }

                    indSelected_ = i;
                    topOffsSelected_ = curTop;

                    items_[indSelected_]->setSelected(intersectInd);
                    QPoint ptLocal(QPoint(cursorPos.x() - pt.x(), cursorPos.y() - pt.y()));
                    items_[indSelected_]->mouseMoveEvent(ptLocal);

                    return true;
                }
            }
        }
        curTop += getItemHeight() + items_[i]->currentAnimationHeight();
    }
    return false;
}

bool WidgetLocations::detectItemClickOnArrow()
{
    if (indSelected_ != -1)
    {
        if (items_[indSelected_]->isCursorOverArrow())
        {
            expandOrCollapseSelectedItem();
            return true;
        }
    }
    return false;
}

int WidgetLocations::detectItemClickOnFavoriteLocationIcon()
{
    if (indSelected_ != -1)
    {
        return items_[indSelected_]->detectOverFavoriteIconCity();
    }
    return -1;
}

bool WidgetLocations::isExpandAnimationNow()
{
    Q_FOREACH(LocationItem *item, items_)
    {
        if (!item->isAnimationFinished())
        {
            return true;
        }
    }
    return false;
}

void WidgetLocations::setCursorForSelected()
{
    if (indSelected_ != -1)
    {
        if (items_[indSelected_]->isSelectedDisabled())
        {
            if (items_[indSelected_]->isCursorOverFavouriteIcon())
            {
                cursorUpdateHelper_->setPointingHandCursor();
            }
            else
            {
                cursorUpdateHelper_->setForbiddenCursor();
            }
        }
        else if (items_[indSelected_]->isForbidden(items_[indSelected_]->getSelected()))
        {
            if (items_[indSelected_]->isCursorOverArrow())
            {
                cursorUpdateHelper_->setPointingHandCursor();
            }
            else
            {
                cursorUpdateHelper_->setForbiddenCursor();
            }
        }
        else
        {
            if (!items_[indSelected_]->isCursorOverArrow() && items_[indSelected_]->isNoConnection(items_[indSelected_]->getSelected()))
            {
                cursorUpdateHelper_->setForbiddenCursor();
            }
            else
            {
                cursorUpdateHelper_->setPointingHandCursor();
            }
        }
    }
}

void WidgetLocations::setVisibleExpandedItem(int ind)
{
    // detect position in visible list for item with ind
    int allTop = 0;
    for (int i = 0; i < items_.count(); ++i)
    {
        if (i == ind)
        {
            break;
        }
        allTop += items_[i]->countVisibleItems();
    }

    int visibleInd = allTop + topInd_;
    //Q_ASSERT(visibleInd >= 0 && visibleInd <= (countOfVisibleItems_ - 1));

    if ((visibleInd + items_[ind]->countVisibleItems() - 1) > (countOfAvailableItemSlots_ - 1))
    {
        int moveCount = (items_[ind]->countVisibleItems() - 1) - ((countOfAvailableItemSlots_ - 1) - visibleInd);
        if (moveCount > visibleInd)
        {
            moveCount = visibleInd;
        }
        verticalScrollBar()->setValue(verticalScrollBar()->value() + moveCount);
    }
}

LocationID WidgetLocations::detectLocationForTopInd(int topInd)
{
    LocationID locationId;

    int allItems = 0;

    for (int i = 0; i < items_.count(); ++i)
    {
        for (int k = 0; k < items_[i]->countVisibleItems(); ++k)
        {
            if (allItems == (-topInd))
            {
                if (k > 0)
                {
                    locationId = items_[i]->getLocationIdForCity(k - 1);
                }
                else
                {
                    locationId = items_[i]->getId();
                }
                goto endDetect;
            }
            allItems++;
        }
    }

endDetect:
    return locationId;
}

int WidgetLocations::detectVisibleIndForCursorPos(const QPoint &pt)
{
    QPoint localPt = viewport()->mapFromGlobal(pt);
    return (localPt.y() - getTopOffset()) / getItemHeight();
}

void WidgetLocations::handleMouseMoveForTooltip()
{
    if (bMouseInViewport_)
    {
        QPoint cursorPos = mapFromGlobal(QCursor::pos());
        if (cursorPos != prevCursorPos_)
        {
            if (indSelected_ != -1)
            {
                if (items_[indSelected_]->isCursorOverP2P())
                {
                    int visibleIndexOfLocation = viewportIndexOfLocationItemSelection(items_[indSelected_]);

                    int posY = 0;
                    if (visibleIndexOfLocation != -1)
                    {
                        posY = viewportPosYOfIndex(visibleIndexOfLocation, true);
                    }

                    QPoint pt = mapToGlobal(QPoint((width_ - 58) * G_SCALE, posY - 13 * G_SCALE));

                    TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_LOCATIONS_P2P);
                    ti.x = pt.x();
                    ti.y = pt.y();
                    ti.title = tr("File Sharing Frowned Upon");
                    ti.tailtype = TOOLTIP_TAIL_BOTTOM;
                    ti.tailPosPercent = 0.5;
                    emit showTooltip(ti);
                }
                else if (!bShowLatencyInMs_ && items_[indSelected_]->isCursorOverConnectionMeter())
                {
                    int visibleIndexOfLocation = viewportIndexOfLocationItemSelection(items_[indSelected_]);
                    int posY = 0;
                    if (visibleIndexOfLocation != -1)
                    {
                        posY = viewportPosYOfIndex(visibleIndexOfLocation, true);
                    }
                    QPoint pt = mapToGlobal(QPoint((width_ - 24) * G_SCALE, posY - 13 * G_SCALE));

                    int cityIndexSelected = items_[indSelected_]->getSelected() - 1;
                    QList<CityNode> cities = items_[indSelected_]->cityNodes();

                    if (cityIndexSelected >= 0 && cityIndexSelected < cities.count())
                    {
                        TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_LOCATIONS_PING_TIME);
                        ti.x = pt.x();
                        ti.y = pt.y();
                        ti.title = QString("%1 Ms").arg(cities[cityIndexSelected].timeMs().toInt());
                        ti.tailtype = TOOLTIP_TAIL_BOTTOM;
                        ti.tailPosPercent = 0.5;
                        emit showTooltip(ti);
                    }
                }
    //          else if (items_[indSelected_]->isForbidden(items_[indSelected_]->getSelected()) && !items_[indSelected_]->isCursorOverArrow())
                else
                {
                    emit hideTooltip(TOOLTIP_ID_LOCATIONS_P2P);
                    emit hideTooltip(TOOLTIP_ID_LOCATIONS_PING_TIME);
                    emit hideTooltip(TOOLTIP_ID_LOCATIONS_ERROR_MESSAGE);
                }
            }
        }

        prevCursorPos_ = cursorPos;
    }
}

void WidgetLocations::handleLeaveForTooltip()
{
    if (!bMouseInViewport_)
    {
        emit hideTooltip(TOOLTIP_ID_LOCATIONS_P2P);
        emit hideTooltip(TOOLTIP_ID_LOCATIONS_PING_TIME);
        emit hideTooltip(TOOLTIP_ID_LOCATIONS_ERROR_MESSAGE);
    }
}

QPoint WidgetLocations::adjustToolTipPosition(const QPoint &globalPoint, const QSize &toolTipSize, bool isP2PToolTip)
{
    QPoint localPoint = viewport()->mapFromGlobal(globalPoint);
    QRect viewportRc = viewport()->geometry();

    // adjust y position
    int y_offs = 15;
    if ((localPoint.y() - toolTipSize.height() - y_offs) > getItemHeight() / 2)
    {
        localPoint = QPoint(localPoint.x(), localPoint.y() - toolTipSize.height() - y_offs);
    }
    else
    {
        int additionalOffs = 0;
        if (isP2PToolTip)
        {
            additionalOffs = 15;
        }
        localPoint = QPoint(localPoint.x(), localPoint.y() + y_offs + additionalOffs);
    }

    // adjust x position
    localPoint.setX(localPoint.x() - toolTipSize.width() / 2);

    if (localPoint.x() < 0)
    {
        localPoint.setX(0);
    }
    int viewportWidthWithoutScrollBar = viewportRc.width() - getScrollBarWidth();
    if ((localPoint.x() + toolTipSize.width()) > viewportWidthWithoutScrollBar)
    {
        localPoint.setX(viewportWidthWithoutScrollBar - toolTipSize.width());
    }

    return localPoint;
}

QPoint WidgetLocations::getSelectItemConnectionMeterCenter()
{
    QPoint ptLocalCenter = items_[indSelected_]->getConnectionMeterCenter();
    return QPoint(ptLocalCenter.x(), topOffsSelected_ + ptLocalCenter.y() + topOffs_);
}

void WidgetLocations::clearItems()
{
    Q_FOREACH(LocationItem *item, items_)
    {
        delete item;
    }
    items_.clear();
    currentVisibleItems_.clear();
}

double WidgetLocations::calcScrollingSpeed(double scrollItemsCount)
{
    if (scrollItemsCount < 3)
    {
        scrollItemsCount = 3;
    }
    return WidgetLocationsSizes::instance().getScrollingSpeedKef() * scrollItemsCount;
}

void WidgetLocations::handleKeyEvent(QKeyEvent *event)
{
    QPoint cursorPos = QCursor::pos();

    if (event->key() == Qt::Key_Up)
    {
        if (isGlobalPointInViewport(cursorPos))
        {
            handleLeaveForTooltip();

            int ind = detectVisibleIndForCursorPos(cursorPos);
            if (ind > 0 && ind <= (countOfAvailableItemSlots_ - 1))
            {
                cursorPos.setY(cursorPos.y() - getItemHeight());
                QCursor::setPos(cursorPos);
            }
            else if (ind == 0)
            {
                verticalScrollBar()->setValue(verticalScrollBar()->value() - 1);
            }
            else
            {
                Q_ASSERT(false);
            }
            updateSelectionCursorAndToolTipByCursorPos();
        }
        else
        {
            if (indSelected_ != -1)
            {
                if (items_[indSelected_]->getSelected() == 0)
                {
                    if (indSelected_ > 0)
                    {
                        items_[indSelected_]->setSelected(-1);
                        indSelected_--;
                        items_[indSelected_]->setSelected(items_[indSelected_]->countVisibleItems() - 1);
                    }
                }
                else if (items_[indSelected_]->getSelected() > 0)
                {
                    items_[indSelected_]->setSelected(items_[indSelected_]->getSelected() - 1);
                }
                scrollUpToSelected();
            }
        }
        update();
    }
    else if (event->key() == Qt::Key_Down)
    {
        if (isGlobalPointInViewport(cursorPos))
        {
            handleLeaveForTooltip();

            int ind = detectVisibleIndForCursorPos(cursorPos);
            if (ind >= 0 && ind < (countOfAvailableItemSlots_ - 1))
            {
                if (ind < countVisibleItems() - 1)
                {
                    cursorPos.setY(cursorPos.y() + getItemHeight());
                    QCursor::setPos(cursorPos);
                }
            }
            else if (ind == (countOfAvailableItemSlots_ - 1))
            {
                verticalScrollBar()->setValue(verticalScrollBar()->value() + 1);
            }
            else
            {
                Q_ASSERT(false);
            }
            updateSelectionCursorAndToolTipByCursorPos();
        }
        else
        {
            if (indSelected_ != -1)
            {
                int curSelItemInLocation = items_[indSelected_]->getSelected();
                if (curSelItemInLocation < (items_[indSelected_]->countVisibleItems() - 1))
                {
                    items_[indSelected_]->setSelected(curSelItemInLocation + 1);
                }
                else
                {
                    if (indSelected_ < (items_.count() - 1))
                    {
                        items_[indSelected_]->setSelected(-1);
                        indSelected_++;
                        items_[indSelected_]->setSelected(0);
                    }
                }
                scrollDownToSelected();
            }
        }
        update();
    }
    else if (event->key() == Qt::Key_PageUp)
    {
    }
    else if (event->key() == Qt::Key_PageDown)
    {
    }
    else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        if (items_[indSelected_]->isForceExpand() && items_[indSelected_]->getSelected() == 0) // Location
        {
            expandOrCollapseSelectedItem();
        }
        else // City
        {
            emitSelectedIfNeed();
        }
    }
}

bool WidgetLocations::isGlobalPointInViewport(const QPoint &pt)
{
    QPoint pt1 = viewport()->mapToGlobal(QPoint(0,0));
    QRect globalRectOfViewport(pt1.x(), pt1.y(), viewport()->geometry().width(), viewport()->geometry().height());
    return globalRectOfViewport.contains(pt);
}

void WidgetLocations::scrollDownToSelected()
{
    // detect position in visible list for item with ind
    int allTop = 0;
    for (int i = 0; i < items_.count(); ++i)
    {
        if (i == indSelected_)
        {
            allTop += items_[i]->getSelected();
            break;
        }
        allTop += items_[i]->countVisibleItems();
    }
    int visibleInd = allTop + topInd_;

    // if selected item visible then nothing todo
    if (visibleInd >= 0 && visibleInd <= (countOfAvailableItemSlots_ - 1))
    {
    }
    else
    {
        verticalScrollBar()->setValue(verticalScrollBar()->value() + 1);
    }

    viewport()->update();
}

void WidgetLocations::scrollUpToSelected()
{
    // detect position in visible list for item with ind
    int allTop = 0;
    for (int i = 0; i < items_.count(); ++i)
    {
        if (i == indSelected_)
        {
            allTop += items_[i]->getSelected();
            break;
        }
        allTop += items_[i]->countVisibleItems();
    }
    int visibleInd = allTop + topInd_;

    // if selected item visible then nothing todo
    if (visibleInd >= 0 && visibleInd <= (countOfAvailableItemSlots_ - 1))
    {
    }
    else
    {
        verticalScrollBar()->setValue(verticalScrollBar()->value() - 1);
    }

    viewport()->update();
}

void WidgetLocations::handleTapClick(const QPoint &cursorPos)
{
    if (!isScrollAnimationNow_)
    {
        detectSelectedItem(cursorPos);
        viewport()->update();
        setCursorForSelected();

        if (!detectItemClickOnArrow())
        {
            emitSelectedIfNeed();
        }
    }
}

void WidgetLocations::emitSelectedIfNeed()
{
    if (indSelected_ != -1)
    {
        LocationID locationId;

        int itemSelectedInd = items_[indSelected_]->getSelected();
        if (itemSelectedInd > 0)
        {
            locationId = items_[indSelected_]->getLocationIdForCity(itemSelectedInd - 1);
        }
        else
        {
            locationId = items_[indSelected_]->getId();
        }

        // click on location
        if (itemSelectedInd == 0)
        {
            if (!items_[indSelected_]->isForbidden(0) && !items_[indSelected_]->isNoConnection(itemSelectedInd))
            {
                emit selected(locationId);
            }
        }
        // click on city
        else
        {
            if (!items_[indSelected_]->isForbidden(itemSelectedInd) && !items_[indSelected_]->isNoConnection(itemSelectedInd) && !items_[indSelected_]->isSelectedDisabled())
            {
                emit selected(locationId);
            }
        }
    }
}

void WidgetLocations::expandOrCollapseSelectedItem()
{
    if (indSelected_ != -1)
    {
        if (items_[indSelected_]->citySubMenuState() == LocationItem::EXPANDED || items_[indSelected_]->citySubMenuState() == LocationItem::EXPANDING)
        {
            items_[indSelected_]->collapse();
        }
        else
        {
            items_[indSelected_]->expand();
        }
        setupScrollBarMaxValue();
        setVisibleExpandedItem(indSelected_);
        viewport()->update();
    }
}

int WidgetLocations::countVisibleItems()
{
    int cntItems = 0;
    Q_FOREACH(LocationItem *item, items_)
    {
        cntItems += item->countVisibleItems();
    }
    return cntItems;
}

void WidgetLocations::setSize(int width, int height)
{
    width_ = width;
    height_ = height;
    update();
}

void WidgetLocations::updateScaling()
{
    // Update scale-dependent class members.
    Q_ASSERT(currentScale_ > 0);
    const auto scale_adjustment = G_SCALE / currentScale_;
    if (scale_adjustment != 1.0) {
        currentScale_ = G_SCALE;
        topOffs_ *= scale_adjustment;
        topOffsSelected_ *= scale_adjustment;
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

    scrollBarOnTop_->setGeometry(width_*G_SCALE - getScrollBarWidth(), 0, getScrollBarWidth(), height_ * G_SCALE);
}

int WidgetLocations::countVisibleItemsInViewport(LocationItem *locationItem)
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

int WidgetLocations::viewportPosYOfIndex(int index, bool centerY)
{
    int viewportTopLeft = viewport()->geometry().y();
    int result = viewportTopLeft + index * getItemHeight();
    if (centerY) result += getItemHeight()/2;
    return result;
}

int WidgetLocations::viewportIndexOfLocationItemSelection(LocationItem *locationItem)
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

QRect WidgetLocations::globalLocationsListViewportRect()
{
    int offset = getItemHeight();
    QPoint locationItemListTopLeft(0, geometry().y() - offset); // TODO: issue?
    locationItemListTopLeft = mapToGlobal(locationItemListTopLeft);

    return QRect(locationItemListTopLeft.x(), locationItemListTopLeft.y(),
                 viewport()->geometry().width(), viewport()->geometry().height());
}

QRect WidgetLocations::locationItemRect(LocationItem *locationItem, bool includeCities)
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

QRect WidgetLocations::cityRect(LocationItem *locationItem, int cityIndex)
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

bool WidgetLocations::itemHeaderInViewport(LocationItem *locationItem)
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

QList<int> WidgetLocations::cityIndexesInViewport(LocationItem *locationItem)
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

int WidgetLocations::viewportIndexOfLocationHeader(LocationItem *locationItem)
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
