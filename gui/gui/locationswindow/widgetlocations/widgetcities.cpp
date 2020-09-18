#include "widgetcities.h"
#include <QDateTime>
#include <QGestureEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScroller>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <qmath.h>
#include "graphicresources/fontmanager.h"
#include "widgetlocationssizes.h"
#include "commongraphics/commongraphics.h"
#include "cityitem.h"
#include "staticipdeviceitem.h"
#include "configfooteritem.h"
#include "cursorupdatehelper.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"

namespace GuiLocations {

WidgetCities::WidgetCities(QWidget *parent) : QAbstractScrollArea(parent),
    width_(0),
    height_(0),
    topInd_(0),
    topOffs_(0),
    indSelected_(-1),
    indPressed_(-1),
    indFavouritePressed_(false),
    countOfAvailableItemSlots_(7),
    bIsFreeSession_(false),
    isScrollAnimationNow_(false),
    currentScale_(G_SCALE),
    // toolTipRender_(NULL),
    bMouseInViewport_(false),
    bExpanded_(false),
    bShowLatencyInMs_(false),
    bTapGestureStarted_(false),
    citiesModel_(NULL),
    ribbonType_(RIBBON_TYPE_NONE),
    deviceName_(""),
    emptyListDisplayIcon_(""),
    emptyListDisplayText_("")
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

    setupScrollBar();
}

WidgetCities::~WidgetCities()
{
    clearItems();
    /*if (toolTipRender_)
    {
        delete toolTipRender_;
    }*/
}

bool WidgetCities::cursorInViewport()
{
    QPoint cursorPos = QCursor::pos();
    QRect rect = globalLocationsListViewportRect();
    return rect.contains(cursorPos);
}

bool WidgetCities::hasSelection()
{
    return indSelected_ != -1;
}

void WidgetCities::centerCursorOnSelectedItem()
{
    if (indSelected_ != -1)
    {
        // Updating cursor
        int visibleIndexOfCurrentSelection = currentVisibleItems_.indexOf(items_[indSelected_]->getLocationId());
        if (visibleIndexOfCurrentSelection != -1)
        {
            int posY = globalLocationsListViewportRect().y() + viewportPosYOfIndex(visibleIndexOfCurrentSelection, true);
            QPoint pt(QCursor::pos().x(), posY);
            QCursor::setPos(pt);
        }
    }
    else
    {
        updateSelectionCursorAndToolTipByCursorPos();
    }
}

void WidgetCities::updateSelectionCursorAndToolTipByCursorPos()
{
    detectSelectedItem(QCursor::pos());
    setCursorForSelected();
    handleMouseMoveForTooltip();
}

void WidgetCities::updateListDisplay(QVector<CityModelItem*> items)
{
    bool bSaved = false;
    LocationID savedTopLocationId;
    LocationID savedSelectedLocationId;

    /*if (items_.count() > 0)
    {
        // save location id of top item
        savedTopLocationId = detectLocationForTopInd(topInd_);

        if (indSelected_ != -1)
        {
            savedSelectedLocationId = items_[indSelected_]->getLocationId();

            int itemSelectedInd = items_[indSelected_]->getSelected();
            if (itemSelectedInd > 0)
            {
                savedSelectedLocationId.setCity(items_[indSelected_]->getCity(itemSelectedInd - 1));
            }
        }

        bSaved = true;
    }*/

    clearItems();

    int curInd = 0;
    Q_UNUSED(curInd);
    int newTopInd = -1;  // cppcheck-suppress variableScope
    int allIndCounter = 0;  // cppcheck-suppress variableScope

    indSelected_ = -1;

    int indInVector = 0;
    Q_FOREACH(const CityModelItem *cmi, items)
    {
        CityItem *item = new CityItem(this, cmi->id, cmi->title1, cmi->title2, cmi->countryCode,
                                              cmi->pingTimeMs, cmi->bShowPremiumStarOnly, isShowLatencyInMs(), cmi->staticIp,
                                              cmi->staticIpType, cmi->isFavorite, cmi->isDisabled);
        items_ << item;
        indInVector++;
    }

    if (ribbonType_ == RIBBON_TYPE_STATIC_IP)
    {
        StaticIpDeviceItem * sidi = new StaticIpDeviceItem(this);
        sidi->setDeviceName(deviceName_);
        items_ << sidi;
    }
    else if (ribbonType_ == RIBBON_TYPE_CONFIG)
    {
        items_ << new ConfigFooterItem(this);
    }

    /*Q_FOREACH(LocationItem *item, items_)
    {
        if (bSaved && newTopInd == -1)
        {
            if (item->getId() == savedTopLocationId.getId())
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

        if (bSaved && savedSelectedLocationId.getId() == item->getId())
        {
            indSelected_ = curInd;
            item->setSelectedByCity(savedSelectedLocationId);
        }

        curInd++;
        allIndCounter += item->countVisibleItems();
    }*/

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
}

void WidgetCities::setModel(BasicCitiesModel *citiesModel)
{
    Q_ASSERT(citiesModel_ == NULL);
    citiesModel_ = citiesModel;
    connect(citiesModel_, SIGNAL(itemsUpdated(QVector<CityModelItem*>)),
                           SLOT(onItemsUpdated(QVector<CityModelItem*>)), Qt::DirectConnection);
    connect(citiesModel_, SIGNAL(connectionSpeedChanged(LocationID, PingTime)), SLOT(onConnectionSpeedChanged(LocationID, PingTime)), Qt::DirectConnection);
    connect(citiesModel_, SIGNAL(isFavoriteChanged(LocationID, bool)), SLOT(onIsFavoriteChanged(LocationID, bool)), Qt::DirectConnection);
}

void WidgetCities::setCurrentSelected(LocationID id)
{
    isScrollAnimationNow_ = false;

    int curAllInd = 0;
    for (int i = 0; i < items_.count(); ++i)
    {
        if (items_[i]->getLocationId().getId() == id.getId())
        {
            if (id.getCity().isEmpty())
            {
                // update selected item
                if (indSelected_ != -1)
                {
                    items_[indSelected_]->setSelected(false);
                }
                items_[i]->setSelected(true);
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
                setupScrollBarMaxValue();

                int cityInd = items_[i]->findCityInd(id);
                if (cityInd != -1)
                {
                    if (indSelected_ != -1)
                    {
                        items_[indSelected_]->setSelected(false);
                    }
                    items_[i]->setSelected(true);
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

void WidgetCities::setFirstSelected()
{
    if (items_.count() > 0)
    {
        LocationID locationId;
        locationId.setId(items_[0]->getLocationId().getId());
        setCurrentSelected(locationId);
    }
}

void WidgetCities::setExpanded(bool bExpanded)
{
    bExpanded_ = bExpanded;
}

void WidgetCities::startAnimationWithPixmap(const QPixmap &pixmap)
{
    backgroundPixmapAnimation_.startWith(pixmap, 150);
}

void WidgetCities::setShowLatencyInMs(bool showLatencyInMs)
{
    bShowLatencyInMs_ = showLatencyInMs;
    viewport()->update();
}

bool WidgetCities::isShowLatencyInMs()
{
    return bShowLatencyInMs_;
}

bool WidgetCities::isFreeSessionStatus()
{
    return bIsFreeSession_;
}

int WidgetCities::getWidth()
{
    return viewport()->width();
}

int WidgetCities::getScrollBarWidth()
{
    return WidgetLocationsSizes::instance().getScrollBarWidth();
}

void WidgetCities::setCountAvailableItemSlots(int cnt)
{
    if (countOfAvailableItemSlots_ != cnt)
    {
        QSize oldSize = sizeHint();
        countOfAvailableItemSlots_ = cnt;
        QSize newSize = sizeHint();
        emit heightChanged(oldSize.height(), newSize.height());
        setupScrollBar();
        setupScrollBarMaxValue();
        viewport()->update();
    }
}

int WidgetCities::getCountAvailableItemSlots()
{
    return countOfAvailableItemSlots_;
}

int WidgetCities::getItemHeight() const
{
    return WidgetLocationsSizes::instance().getItemHeight();
}

int WidgetCities::getTopOffset() const
{
    return WidgetLocationsSizes::instance().getTopOffset();
}

QSize WidgetCities::sizeHint() const
{
    return QSize(355, getItemHeight() * countOfAvailableItemSlots_ + (getTopOffset() - 1));
}

void WidgetCities::onKeyPressEvent(QKeyEvent *event)
{
    handleKeyEvent((QKeyEvent *)event);
}

bool WidgetCities::eventFilter(QObject *object, QEvent *event)
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

void WidgetCities::paintEvent(QPaintEvent *event)
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
    else if (backgroundPixmapAnimation_.isAnimationActive())
    {
        viewport()->update();
    }

    // background
    QPainter painter(viewport());
    QRect geometryRect = viewport()->geometry();
    painter.fillRect(QRect(0, 0, geometryRect.width(), geometryRect.height()), QBrush(WidgetLocationsSizes::instance().getBackgroundColor()));


    // empty list drawing items
    if (countVisibleItems() == 0)
    {
        if (emptyListDisplayIcon_ != "")
        {
            // qDebug() << "Drawing Broken Heart";
            IndependentPixmap *brokenHeartPixmap = ImageResourcesSvg::instance().getIndependentPixmap(emptyListDisplayIcon_);
             brokenHeartPixmap->draw(width()/2 - 16 *G_SCALE,
                               height()/2 - 32*G_SCALE, &painter);
        }

        if (emptyListDisplayText_ != "")
        {
            painter.save();
            QFont font = *FontManager::instance().getFont(12, false);
            painter.setFont(font);
            painter.setPen(Qt::white);
            painter.drawText(width()/2 - CommonGraphics::textWidth(emptyListDisplayText_,font)/2,
                             height()/2 + 24*G_SCALE, emptyListDisplayText_);
            painter.restore();
        }
    }

    QRect locationsGeometry = QRect(0, getTopOffset(), geometryRect.width(), geometryRect.height() - getTopOffset());

    // draw items
    int itemInd = 0;
    int curTop = 0;
    QVector<int> linesForScrollbar;
    linesForScrollbar.reserve(10);

    currentVisibleItems_.clear();
    Q_FOREACH(ICityItem *item, items_)
    {
        QRect rc(0, curTop + getTopOffset() + topOffs_, geometryRect.width(), getItemHeight()*item->countVisibleItems());
        QRect rcLocationCaption(0, curTop + getTopOffset() + topOffs_, geometryRect.width(), getItemHeight());
        curTop += getItemHeight();

        if (locationsGeometry.intersects(rc))
        {
            linesForScrollbar << rcLocationCaption.bottom();

            item->drawLocationCaption(&painter, rcLocationCaption);
            currentVisibleItems_.append(item->getLocationId());
        }

        itemInd++;
    }

    if (backgroundPixmapAnimation_.isAnimationActive())
    {
        painter.setOpacity(backgroundPixmapAnimation_.curOpacity());
        painter.drawPixmap(0, 0, backgroundPixmapAnimation_.curPixmap());
    }
}

void WidgetCities::scrollContentsBy(int dx, int dy)
{
    Q_UNUSED(dx);
    Q_UNUSED(dy);
    //clearBottomFlags();

    if (verticalScrollBar()->value() != scrollBarOnTop_->value())
    {
        scrollBarOnTop_->setValue(verticalScrollBar()->value());
    }

    emit hideTooltip(TOOLTIP_ID_LOCATIONS_ITEM_CAPTION);
    emit hideTooltip(TOOLTIP_ID_LOCATIONS_PING_TIME);

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

void WidgetCities::mouseMoveEvent(QMouseEvent *event)
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

void WidgetCities::mousePressEvent(QMouseEvent *event)
{
#ifdef Q_OS_WIN
    if (event->source() == Qt::MouseEventSynthesizedBySystem)
    {
        return;
    }
#endif

    if (event->button() == Qt::LeftButton)
    {
        indPressed_ = indSelected_;
        indFavouritePressed_ = detectItemClickOnFavoriteLocationIcon();
    }
}

void WidgetCities::mouseReleaseEvent(QMouseEvent *event)
{
#ifdef Q_OS_WIN
    if (event->source() == Qt::MouseEventSynthesizedBySystem)
    {
        return;
    }
#endif

    if (event->button() == Qt::LeftButton)
    {
        if (indPressed_ == indSelected_ && isGlobalPointInViewport(QCursor::pos()))
        {
            int indFav = detectItemClickOnFavoriteLocationIcon();
            if ( indFav && indFavouritePressed_)
            {
                bool isFavorite = items_[indSelected_]->toggleFavorite();
                if (isFavorite)
                {
                    emit switchFavorite(items_[indSelected_]->getLocationId(), true);
                }
                else
                {
                    emit switchFavorite(items_[indSelected_]->getLocationId(), false);
                }

                viewport()->update();
            }
            else if (indSelected_ != -1) // valid
            {
                if (!indFav && !indFavouritePressed_) // neither favourite press or release
                {
                    emitSelectedIfNeed();
                }
            }
        }
    }
}

void WidgetCities::mouseDoubleClickEvent(QMouseEvent *event)
{
#ifdef Q_OS_WIN
    if (event->source() == Qt::MouseEventSynthesizedBySystem)
    {
        return;
    }
#endif

    mousePressEvent(event);
}

void WidgetCities::leaveEvent(QEvent *event)
{
    bMouseInViewport_ = false;
    handleLeaveForTooltip();

    Q_FOREACH(ICityItem *item, items_)
    {
        item->mouseLeaveEvent();
    }

    viewport()->update();
    QWidget::leaveEvent(event);
}

void WidgetCities::enterEvent(QEvent *event)
{
    bMouseInViewport_ = true;
    QWidget::enterEvent(event);
}

void WidgetCities::resizeEvent(QResizeEvent *event)
{
    QAbstractScrollArea::resizeEvent(event);
    scrollBarOnTop_->setGeometry(width_*G_SCALE - getScrollBarWidth(), 0, getScrollBarWidth(), height_ * G_SCALE);
}

void WidgetCities::onItemsUpdated(QVector<CityModelItem *> items)
{
   updateListDisplay(items);
   updateScaling();
}

void WidgetCities::onConnectionSpeedChanged(LocationID id, PingTime timeMs)
{
    bool bChanged = false;
    for (int i = 0; i < items_.count(); ++i)
    {
        if (items_[i]->getLocationId() == id)
        {
            if (items_[i]->getPingTimeMs() != timeMs.toInt())
            {
                items_[i]->changeSpeedConnection(id, timeMs);
                bChanged = true;
                break;
            }
        }
    }

    if (bChanged && currentVisibleItems_.contains(id))
    {
        viewport()->update();
    }
}

void WidgetCities::onIsFavoriteChanged(LocationID id, bool isFavorite)
{
    bool bChanged = false;
    for (int i = 0; i < items_.count(); ++i)
    {
        if (items_[i]->getLocationId() == id)
        {
            bChanged = items_[i]->changeIsFavorite(id, isFavorite);
            break;
        }
    }

    if (bChanged && currentVisibleItems_.contains(id))
    {
        viewport()->update();
    }
}

void WidgetCities::onFreeSessionStatusChanged(bool isFreeSessionStatus)
{
    if (bIsFreeSession_ != isFreeSessionStatus)
    {
        bIsFreeSession_ = isFreeSessionStatus;
        handleLeaveForTooltip();
        viewport()->update();
    }
}

void WidgetCities::onTopScrollBarValueChanged(int value)
{
    verticalScrollBar()->setValue(value);
}

void WidgetCities::calcScrollPosition()
{
    double d = static_cast<double>(scrollAnimationElapsedTimer_.elapsed()) / static_cast<double>(scrollAnimationDuration_);
    topOffs_ = static_cast<int>(startAnimationTop_ + (endAnimationTop_ - startAnimationTop_) * d);

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

void WidgetCities::setupScrollBar()
{
    verticalScrollBar()->setMinimum(0);
    verticalScrollBar()->setSingleStep(1);
    verticalScrollBar()->setPageStep(countOfAvailableItemSlots_);

    scrollBarOnTop_->setMinimum(0);
    scrollBarOnTop_->setSingleStep(1);
    scrollBarOnTop_->setPageStep(countOfAvailableItemSlots_);
}

void WidgetCities::setupScrollBarMaxValue()
{
    int cntItems = 0;
    for (int i = 0; i < items_.length(); i++)
    {
        ICityItem *item = items_[i];
        cntItems += item->countVisibleItems();
    }
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

bool WidgetCities::detectSelectedItem(const QPoint &cursorPos)
{
    int curTop = 0;
    for (int i = 0; i < items_.size(); ++i)
    {
        QPoint pt(0, curTop + getTopOffset() + topOffs_);
        pt = mapToGlobal(pt);

        QPoint localBottomPt = viewport()->mapFromGlobal(QPoint(pt.x(), pt.y()));
        if (localBottomPt.y() >= getTopOffset() && QRect(pt.x(), pt.y(), viewport()->geometry().width(), getItemHeight()).contains(cursorPos))
        {
            if (indSelected_ == i)
            {
                bool bChanged = false;
                if (!items_[indSelected_]->isSelected())
                {
                    items_[indSelected_]->setSelected(true);
                    bChanged = true;
                }

                QPoint ptLocal(QPoint(cursorPos.x() - pt.x(), cursorPos.y() - pt.y()));
                return items_[indSelected_]->mouseMoveEvent(ptLocal) || bChanged;
            }
            else
            {
                if (indSelected_ != -1)
                {
                    items_[indSelected_]->setSelected(false);
                }

                indSelected_ = i;
                topOffsSelected_ = curTop;

                items_[indSelected_]->setSelected(true);
                QPoint ptLocal(QPoint(cursorPos.x() - pt.x(), cursorPos.y() - pt.y()));
                items_[indSelected_]->mouseMoveEvent(ptLocal);

                return true;
            }
        }
        curTop += getItemHeight();
    }
    if (indSelected_ != -1)
    {
        items_[indSelected_]->setSelected(false);
        indSelected_ = -1;
        return true;
    }
    return false;
}


bool WidgetCities::detectItemClickOnFavoriteLocationIcon()
{
    if (indSelected_ != -1)
    {
        return items_[indSelected_]->detectOverFavoriteIconCity();
    }
    return false;
}

void WidgetCities::setCursorForSelected()
{
    if (indSelected_ != -1)
    {
        if (items_[indSelected_]->isDisabled())
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
        else if (items_[indSelected_]->isForbidden())
        {
            cursorUpdateHelper_->setForbiddenCursor();
        }
        else
        {
            if (items_[indSelected_]->isNoConnection())
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

LocationID WidgetCities::detectLocationForTopInd(int topInd)
{
    Q_UNUSED(topInd);

    Q_ASSERT(false);
    LocationID locationId;

    /*int allItems = 0;

    for (int i = 0; i < items_.count(); ++i)
    {
        for (int k = 0; k < items_[i]->countVisibleItems(); ++k)
        {
            if (allItems == (-topInd))
            {
                locationId.setId(items_[i]->getId());
                if (k > 0)
                {
                    locationId.setCity(items_[i]->getCity(k - 1));
                }
                goto endDetect;
            }
            allItems++;
        }
    }

endDetect:*/
    return locationId;
}

int WidgetCities::detectVisibleIndForCursorPos(const QPoint &pt)
{
    QPoint localPt = viewport()->mapFromGlobal(pt);
    return (localPt.y() - getTopOffset()) / getItemHeight();
}

void WidgetCities::handleMouseMoveForTooltip()
{
    if (bMouseInViewport_)
    {
        QPoint cursorPos = mapFromGlobal(QCursor::pos());
        if (cursorPos != prevCursorPos_)
        {
            if (indSelected_ != -1)
            {
                if (items_[indSelected_]->isCursorOverCaption1Text())
                {
                    QString fullText = items_[indSelected_]->getCaption1FullText();
                    QString truncatedText = items_[indSelected_]->getCaption1TruncatedText();
                    if (fullText != truncatedText)
                    {
                        QString text = items_[indSelected_]->getCaption1FullText();
                        QPoint pt = mapToGlobal(QPoint(75 * G_SCALE, getSelectItemCaption1TextCenter().y()));

                        TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_LOCATIONS_ITEM_CAPTION);
                        ti.x = pt.x();
                        ti.y = pt.y();
                        ti.title = text;
                        ti.tailtype = TOOLTIP_TAIL_BOTTOM;
                        ti.tailPosPercent = 0.2;
                        emit showTooltip(ti);
                    }
                }
                else if (!bShowLatencyInMs_ && items_[indSelected_]->isCursorOverConnectionMeter())
                {
                    int ind = detectVisibleIndForCursorPos(QCursor::pos());
                    int posY = viewportPosYOfIndex(ind, true);
                    QPoint pt = mapToGlobal(QPoint((width_ - 24) * G_SCALE, posY - 13*G_SCALE));

                    if (indSelected_ >= 0 && indSelected_ < items_.count())
                    {
                        TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_LOCATIONS_PING_TIME);
                        ti.x = pt.x();
                        ti.y = pt.y();
                        ti.title = QString("%1 Ms").arg(items_[indSelected_]->getPingTimeMs());
                        ti.tailtype = TOOLTIP_TAIL_BOTTOM;
                        ti.tailPosPercent = 0.5;
                        emit showTooltip(ti);
                    }
                }
//              else if (items_[indSelected_]->isCursorOverP2P())
//              else if (items_[indSelected_]->isForbidden(items_[indSelected_]->getSelected()) && !items_[indSelected_]->isCursorOverArrow())
                else
                {
                    emit hideTooltip(TOOLTIP_ID_LOCATIONS_ITEM_CAPTION);
                    emit hideTooltip(TOOLTIP_ID_LOCATIONS_PING_TIME);
                }
            }
        }
        prevCursorPos_ = cursorPos;
    }
}

void WidgetCities::handleLeaveForTooltip()
{
    if (!bMouseInViewport_)
    {
        emit hideTooltip(TOOLTIP_ID_LOCATIONS_ITEM_CAPTION);
        emit hideTooltip(TOOLTIP_ID_LOCATIONS_PING_TIME);
    }
}

QPoint WidgetCities::adjustToolTipPosition(const QPoint &globalPoint, const QSize &toolTipSize, bool isP2PToolTip)
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

QPoint WidgetCities::getSelectItemCaption1TextCenter()
{
    QPoint ptLocalCenter = items_[indSelected_]->getCaption1TextCenter();
    return QPoint(ptLocalCenter.x(), topOffsSelected_ + ptLocalCenter.y() + topOffs_);
}

void WidgetCities::clearItems()
{
    Q_FOREACH(ICityItem *item, items_)
    {
        delete item;
    }
    items_.clear();
}

double WidgetCities::calcScrollingSpeed(double scrollItemsCount)
{
    if (scrollItemsCount < 3)
    {
        scrollItemsCount = 3;
    }
    return WidgetLocationsSizes::instance().getScrollingSpeedKef() * scrollItemsCount;
}

void WidgetCities::handleKeyEvent(QKeyEvent *event)
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
                if (items_[indSelected_]->isSelected())
                {
                    if (indSelected_ > 0)
                    {
                        items_[indSelected_]->setSelected(false);
                        indSelected_--;
                        items_[indSelected_]->setSelected(true);
                    }
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
                if (ind < items_.count() - 1)
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
                if (indSelected_ < (items_.count() - 1))
                {
                    items_[indSelected_]->setSelected(false);
                    indSelected_++;
                    items_[indSelected_]->setSelected(true);
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
    else if (event->key() == Qt::Key_Return)
    {
        emitSelectedIfNeed();
    }
}

bool WidgetCities::isGlobalPointInViewport(const QPoint &pt)
{
    QPoint pt1 = viewport()->mapToGlobal(QPoint(0,0));
    QRect globalRectOfViewport(pt1.x(), pt1.y(), viewport()->geometry().width(), viewport()->geometry().height());
    return globalRectOfViewport.contains(pt);
}

void WidgetCities::scrollDownToSelected()
{
    // detect position in visible list for item with ind
    int allTop = 0;
    for (int i = 0; i < items_.count(); ++i)
    {
        if (i == indSelected_)
        {
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

void WidgetCities::scrollUpToSelected()
{
    // detect position in visible list for item with ind
    int allTop = 0;
    for (int i = 0; i < items_.count(); ++i)
    {
        if (i == indSelected_)
        {
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

void WidgetCities::handleTapClick(const QPoint &cursorPos)
{
    if (!isScrollAnimationNow_)
    {
        detectSelectedItem(cursorPos);
        viewport()->update();
        setCursorForSelected();

        emitSelectedIfNeed();
    }
}

void WidgetCities::emitSelectedIfNeed()
{
    if (indSelected_ != -1)
    {
        LocationID locationId = items_[indSelected_]->getLocationId();

        if (!items_[indSelected_]->isForbidden() && !items_[indSelected_]->isNoConnection() && !items_[indSelected_]->isDisabled())
        {
            emit selected(locationId);
        }
    }
}

QRect WidgetCities::globalLocationsListViewportRect()
{
    int offset = getItemHeight();
    QPoint locationItemListTopLeft(0, geometry().y() - offset);
    locationItemListTopLeft = mapToGlobal(locationItemListTopLeft);
    return QRect(locationItemListTopLeft.x(), locationItemListTopLeft.y(), viewport()->geometry().width(), viewport()->geometry().height());
}

int WidgetCities::countVisibleItems()
{
    int count = 0;
    for (int i = 0; i < items_.count(); ++i)
    {
        count += items_[i]->countVisibleItems();
    }
    return count;
}

void WidgetCities::setRibbonInList(RibbonType ribbonType)
{
    ribbonType_ = ribbonType;
    updateListDisplay(citiesModel_->getCities());
}

void WidgetCities::setSize(int width, int height)
{
    width_ = width;
    height_ = height;
    update();
}

void WidgetCities::updateScaling()
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

void WidgetCities::setDeviceName(const QString &deviceName)
{
    deviceName_ = deviceName;
    update();
}

void WidgetCities::setEmptyListDisplayIcon(QString emptyListDisplayIcon)
{
    emptyListDisplayIcon_ = emptyListDisplayIcon;
    update();
}

void WidgetCities::setEmptyListDisplayText(QString emptyListDisplayText)
{
    emptyListDisplayText_ = emptyListDisplayText;
    update();
}

int WidgetCities::viewportPosYOfIndex(int index, bool centerY)
{
    int viewportTopLeft = viewport()->geometry().y();
    int result = viewportTopLeft + index * getItemHeight();
    if (centerY) result += getItemHeight()/2;
    return result;
}

} // namespace GuiLocations
