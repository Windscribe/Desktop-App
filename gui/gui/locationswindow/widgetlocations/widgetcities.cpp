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
#include "languagecontroller.h"
#include "commongraphics/commongraphics.h"
#include "cursorupdatehelper.h"
#include "graphicresources/imageresourcessvg.h"
#include "dpiscalemanager.h"
#include "tooltips/tooltipcontroller.h"
#include "utils/logger.h"

#include <QDebug>

namespace GuiLocations {

WidgetCities::WidgetCities(QWidget *parent, int visible_item_slots) : QScrollArea(parent)
  , citiesModel_(NULL)
  , countOfAvailableItemSlots_(visible_item_slots)
  , bIsFreeSession_(false)
  , bShowLatencyInMs_(false)
  , bTapGestureStarted_(false)
  , currentScale_(G_SCALE)
  , lastScrollPos_(0)
  , kickPreventMouseSelectionTimer_(false)
  , animationScollTarget_(0)
  , emptyListDisplayIcon_("")
  , emptyListDisplayText_("")
  , emptyListDisplayTextWidth_(0)
  , emptyListDisplayTextHeight_(0)
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
    widgetCitiesList_ = new WidgetCitiesList(this, this);
    setWidget(widgetCitiesList_);
    connect(widgetCitiesList_, SIGNAL(heightChanged(int)), SLOT(onLocationItemListWidgetHeightChanged(int)));
    connect(widgetCitiesList_, SIGNAL(favoriteClicked(ItemWidgetCity*,bool)), SLOT(onLocationItemListWidgetFavoriteClicked(ItemWidgetCity *, bool)));
    connect(widgetCitiesList_, SIGNAL(locationIdSelected(LocationID)), SLOT(onLocationItemListWidgetLocationIdSelected(LocationID)));
    widgetCitiesList_->setGeometry(0,0, WINDOW_WIDTH*G_SCALE, 0);
    widgetCitiesList_->show();

    emptyListButton_ = new CommonWidgets::TextButtonWidget(QString(), this);
    emptyListButton_->setFont(FontDescr(12, false));
    emptyListButton_->hide();
    connect(emptyListButton_, SIGNAL(clicked()), SIGNAL(emptyListButtonClicked()));

    preventMouseSelectionTimer_.start();
    connect(&scrollAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onScrollAnimationValueChanged(QVariant)));
    connect(&LanguageController::instance(), SIGNAL(languageChanged()), SLOT(onLanguageChanged()));
}

WidgetCities::~WidgetCities()
{
    widgetCitiesList_->disconnect();
    widgetCitiesList_->deleteLater();

    scrollBar_->disconnect();
    scrollBar_->deleteLater();
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

void WidgetCities::updateScaling()
{
    const auto scale_adjustment = G_SCALE / currentScale_;
    if (scale_adjustment != 1.0)
    {
        // qDebug() << "Scale change!";
        currentScale_ = G_SCALE;
        int pos = -lastScrollPos_  * scale_adjustment; // lastScrollPos_ is negative (from geometry)
        lastScrollPos_ = -closestPositionIncrement(pos);
        scrollBar_->forceSetValue(-lastScrollPos_); // use + for scrollbar
    }

    widgetCitiesList_->updateScaling();
    scrollBar_->updateCustomStyleSheet();
    updateEmptyListButton();
}

bool WidgetCities::hasSelection()
{
    return widgetCitiesList_->hasAccentItem();
}

LocationID WidgetCities::selectedItemLocationId()
{
    return widgetCitiesList_->lastAccentedItemWidget()->getId();
}

void WidgetCities::setFirstSelected()
{
    widgetCitiesList_->accentFirstItem();
}


bool WidgetCities::cursorInViewport()
{
    QPoint cursorPos = QCursor::pos();
    QRect rect = globalLocationsListViewportRect();
    return rect.contains(cursorPos);
}

void WidgetCities::centerCursorOnSelectedItem()
{
    QPoint cursorPos = QCursor::pos();
    IItemWidget *lastSelWidget = widgetCitiesList_->lastAccentedItemWidget();

    if (lastSelWidget)
    {
        QCursor::setPos(QPoint(cursorPos.x(), static_cast<int>(lastSelWidget->globalGeometry().top() - LOCATION_ITEM_HEIGHT*G_SCALE/2)));
    }
}

void WidgetCities::centerCursorOnItem(LocationID locId)
{
    QPoint cursorPos = QCursor::pos();
    if (locId.isValid())
    {
        if (locationIdInViewport(locId))
        {
            IItemWidget *widget = widgetCitiesList_->selectableWidget(locId);
            if (widget)
            {
                QCursor::setPos(QPoint(cursorPos.x(), static_cast<int>(widget->globalGeometry().top() + LOCATION_ITEM_HEIGHT*G_SCALE/2)));
            }
        }
    }
}

int WidgetCities::countViewportItems()
{
    return countOfAvailableItemSlots_;
}

void WidgetCities::setCountViewportItems(int cnt)
{
    if (countOfAvailableItemSlots_ != cnt)
    {
        countOfAvailableItemSlots_ = cnt;
        updateEmptyListButton();
    }
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

void WidgetCities::startAnimationWithPixmap(const QPixmap &pixmap)
{
    backgroundPixmapAnimation_.startWith(pixmap, 150);
}

bool WidgetCities::eventFilter(QObject *object, QEvent *event)
{
    // TODO: thie entire function (for gestures)
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
    return QScrollArea::eventFilter(object, event);
}

void WidgetCities::setEmptyListDisplayIcon(QString emptyListDisplayIcon)
{
    emptyListDisplayIcon_ = emptyListDisplayIcon;
    update();
}

void WidgetCities::setEmptyListDisplayText(QString emptyListDisplayText, int width)
{
    emptyListDisplayText_ = emptyListDisplayText;
    emptyListDisplayTextWidth_ = width;
    update();
}

void WidgetCities::setEmptyListButtonText(QString text)
{
    emptyListButton_->setText(text);
    update();
}

void WidgetCities::handleKeyEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Up)
    {
        if (widgetCitiesList_->hasAccentItem())
        {
            if (widgetCitiesList_->accentItemIndex() > 0)
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

                        IItemWidget *lastSelWidget = widgetCitiesList_->lastAccentedItemWidget();

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
                widgetCitiesList_->moveAccentUp();
            }
        }
        else
        {
            widgetCitiesList_->accentFirstItem();
        }
    }
    else if (event->key() == Qt::Key_Down)
    {
        if (widgetCitiesList_->hasAccentItem())
        {
            if (widgetCitiesList_->accentItemIndex() < widgetCitiesList_->itemWidgets().count() - 1)
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

                        IItemWidget *lastSelWidget = widgetCitiesList_->lastAccentedItemWidget();

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
                widgetCitiesList_->moveAccentDown();
            }
        }
        else
        {
            // qDebug() << "Accenting first";
            widgetCitiesList_->accentFirstItem();
        }
    }
    else if (event->key() == Qt::Key_Return)
    {
        // qDebug() << "Selection by key press";
        IItemWidget *lastSelWidget = widgetCitiesList_->lastAccentedItemWidget();

        if (lastSelWidget)
        {
            if(!lastSelWidget->isForbidden() && !lastSelWidget->isDisabled())
            {
                emit selected(widgetCitiesList_->lastAccentedLocationId());
            }
        }
    }
}

void WidgetCities::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    // qDebug() << "WidgetCities::paintEvent - geo: " << geometry();

    // draw background for when list is < size of viewport
    QPainter painter(viewport());
    QRect bkgd(0,0,geometry().width(), geometry().height());
    painter.fillRect(bkgd, WidgetLocationsSizes::instance().getBackgroundColor());

    // empty list drawing items
    if (widgetCitiesList_->itemWidgets().isEmpty())
    {
        const int kVerticalOffset = emptyListButton_->text().isEmpty() ? 0 : 16;
        if (!emptyListDisplayIcon_.isEmpty())
        {
            // qDebug() << "Drawing Broken Heart";
            IndependentPixmap *brokenHeartPixmap =
                ImageResourcesSvg::instance().getIndependentPixmap(emptyListDisplayIcon_);
            brokenHeartPixmap->draw(width() / 2 - 16 * G_SCALE,
                height() / 2 - (kVerticalOffset + 32) * G_SCALE, &painter);
        }

        if (!emptyListDisplayText_.isEmpty())
        {
            painter.save();
            QFont font = *FontManager::instance().getFont(12, false);
            painter.setFont(font);
            painter.setPen(Qt::white);
            painter.setOpacity(0.5);
            QRect rc, rcb;
            const int wide = emptyListDisplayTextWidth_ ? (emptyListDisplayTextWidth_ * G_SCALE) :
                CommonGraphics::textWidth(emptyListDisplayText_, font);
            rc.setRect((width() - wide) / 2, height() / 2 - (kVerticalOffset - 16) * G_SCALE,
                       wide, height() );
            painter.drawText(rc, Qt::AlignHCenter | Qt::TextWordWrap, emptyListDisplayText_, &rcb);
            painter.restore();
            if (emptyListDisplayTextHeight_ != rcb.height()) {
                emptyListDisplayTextHeight_ = rcb.height();
                updateEmptyListButton();
            }
        }
    }
}

// called by change in the vertical scrollbar
void WidgetCities::scrollContentsBy(int dx, int dy)
{
    TooltipController::instance().hideAllTooltips();

    if (!scrollBar_->dragging())
    {
        // cursor should not interfere when animation is running
        // prevent floating cursor from overriding a keypress animation
        // this can only occur when accessibility is disabled (since cursor will not be moved)
        if (cursorInViewport() && preventMouseSelectionTimer_.elapsed() > 100)
        {
            widgetCitiesList_->selectWidgetContainingCursor();
        }

        QScrollArea::scrollContentsBy(dx,dy);
        lastScrollPos_ = widgetCitiesList_->geometry().y();
    }
}

void WidgetCities::mouseMoveEvent(QMouseEvent *event)
{
#ifdef Q_OS_WIN
    if (event->source() == Qt::MouseEventSynthesizedBySystem)
    {
        return;
    }
#endif

    QScrollArea::mouseMoveEvent(event);
}

void WidgetCities::mousePressEvent(QMouseEvent *event)
{
#ifdef Q_OS_WIN
    if (event->source() == Qt::MouseEventSynthesizedBySystem)
    {
        return;
    }
#endif

}

void WidgetCities::mouseReleaseEvent(QMouseEvent *event)
{
#ifdef Q_OS_WIN
    if (event->source() == Qt::MouseEventSynthesizedBySystem)
    {
        return;
    }
#endif

    QScrollArea::mouseReleaseEvent(event);

}

void WidgetCities::mouseDoubleClickEvent(QMouseEvent *event)
{
#ifdef Q_OS_WIN
    if (event->source() == Qt::MouseEventSynthesizedBySystem)
    {
        return;
    }
#endif

    QScrollArea::mousePressEvent(event);
}

void WidgetCities::leaveEvent(QEvent *event)
{
    QScrollArea::leaveEvent(event);
}

void WidgetCities::enterEvent(QEvent *event)
{
    QScrollArea::enterEvent(event);
}

void WidgetCities::resizeEvent(QResizeEvent *event)
{
    // qDebug() << "WidgetCities::resizeEvent";
    QScrollArea::resizeEvent(event);
}

void WidgetCities::onItemsUpdated(QVector<CityModelItem *> items)
{
    updateWidgetList(items);
}

void WidgetCities::onConnectionSpeedChanged(LocationID id, PingTime timeMs)
{
    foreach (ItemWidgetCity *w, widgetCitiesList_->itemWidgets())
    {
        if (w->getId() == id)
        {
            w->setLatencyMs(timeMs);
            break;
        }
    }
}

void WidgetCities::onIsFavoriteChanged(LocationID id, bool isFavorite)
{
    foreach (ItemWidgetCity *city, widgetCitiesList_->itemWidgets())
    {
        if (city->getId() == id)
        {
            city->setFavourited(isFavorite);
            break;
        }
    }
}

void WidgetCities::onFreeSessionStatusChanged(bool isFreeSessionStatus)
{
    if (bIsFreeSession_ != isFreeSessionStatus)
    {
        bIsFreeSession_ = isFreeSessionStatus;
    }
}

void WidgetCities::onLanguageChanged()
{
    // TODO: language change
}

void WidgetCities::onLocationItemListWidgetHeightChanged(int listWidgetHeight)
{

    widgetCitiesList_->setGeometry(0,widgetCitiesList_->geometry().y(), WINDOW_WIDTH*G_SCALE, listWidgetHeight);
    scrollBar_->setRange(0, listWidgetHeight - scrollBar_->pageStep()); // update scroll bar
    scrollBar_->setSingleStep(LOCATION_ITEM_HEIGHT * G_SCALE); // scroll by this many px at a time

    update();
}

void WidgetCities::onLocationItemListWidgetFavoriteClicked(ItemWidgetCity *cityWidget, bool favorited)
{
    emit switchFavorite(cityWidget->getId(), favorited);

}

void WidgetCities::onLocationItemListWidgetLocationIdSelected(LocationID id)
{
    emit selected(id);
}

void WidgetCities::onScrollAnimationValueChanged(const QVariant &value)
{
    if (kickPreventMouseSelectionTimer_) preventMouseSelectionTimer_.restart();

    widgetCitiesList_->move(0, value.toInt());
    lastScrollPos_ = widgetCitiesList_->geometry().y();

    // update scroll bar for keypress navigation
    if (!scrollBar_->dragging())
    {
        scrollBar_->forceSetValue(-animationScollTarget_);
    }

    viewport()->update();
}

void WidgetCities::onScrollBarHandleDragged(int valuePos)
{
    animationScollTarget_ = -valuePos;

    // qDebug() << "Dragged: " << locationItemListWidget_->geometry().y() << " -> " << animationScollTarget_;
    scrollAnimation_.stop();
    scrollAnimation_.setDuration(PROGRAMMATIC_SCROLL_ANIMATION_DURATION);
    scrollAnimation_.setStartValue(widgetCitiesList_->geometry().y());
    scrollAnimation_.setEndValue(animationScollTarget_);
    scrollAnimation_.setDirection(QAbstractAnimation::Forward);
    scrollAnimation_.start();
}

void WidgetCities::updateEmptyListButton()
{
    if (!widgetCitiesList_->itemWidgets().isEmpty() || emptyListButton_->text().isEmpty()) {
        emptyListButton_->hide();
        return;
    }
    const auto sizeHint = emptyListButton_->sizeHint();
    const int xpos = (width() - sizeHint.width()) / 2;
    const int ypos = height() / 2 + emptyListDisplayTextHeight_ + 16 * G_SCALE;
    emptyListButton_->setGeometry(xpos, ypos, sizeHint.width(), sizeHint.height());
    emptyListButton_->show();
}

void WidgetCities::updateWidgetList(QVector<CityModelItem *> items)
{
    LocationID topSelectableLocationIdInViewport = topViewportSelectableLocationId();
    LocationID lastSelectedLocationId = widgetCitiesList_->lastAccentedLocationId();

    qCDebug(LOG_BASIC) << "Updating locations-city widget list";
    widgetCitiesList_->clearWidgets();
    foreach (CityModelItem *item, items)
    {
        const CityModelItem &itemRef = *item;
        widgetCitiesList_->addCity(itemRef);
    }

    qCDebug(LOG_BASIC) << "Restoring locations-city widgets state";

    // restoring previous widget state
    int indexInNewList = widgetCitiesList_->selectableIndex(topSelectableLocationIdInViewport);
    if (indexInNewList >= 0)
    {
        scrollDown(indexInNewList);
    }
    widgetCitiesList_->accentItem(lastSelectedLocationId);
    qCDebug(LOG_BASIC) << "Done updating locations-city widget list";

}

void WidgetCities::scrollToIndex(int index)
{
    int pos = static_cast<int>(LOCATION_ITEM_HEIGHT * G_SCALE * index);
    pos = closestPositionIncrement(pos);
    scrollBar_->forceSetValue(pos);
}

void WidgetCities::scrollDown(int itemCount)
{
    // Scrollbar values use positive (whilte itemListWidget uses negative geometry)
    int newY = static_cast<int>(widgetCitiesList_->geometry().y() + LOCATION_ITEM_HEIGHT * G_SCALE * itemCount);
    scrollBar_->setValue(newY);
}

void WidgetCities::animatedScrollDown(int itemCount)
{
    int scrollBy = static_cast<int>(LOCATION_ITEM_HEIGHT * G_SCALE * itemCount);

    if (scrollAnimation_.state() == QAbstractAnimation::Running)
    {
        animationScollTarget_ -= scrollBy;
    }
    else
    {
        animationScollTarget_ = widgetCitiesList_->geometry().y() - scrollBy;
    }

    scrollAnimation_.stop();
    scrollAnimation_.setDuration(PROGRAMMATIC_SCROLL_ANIMATION_DURATION);
    scrollAnimation_.setStartValue(widgetCitiesList_->geometry().y());
    scrollAnimation_.setEndValue(animationScollTarget_);
    scrollAnimation_.setDirection(QAbstractAnimation::Forward);
    scrollAnimation_.start();
}

void WidgetCities::animatedScrollUp(int itemCount)
{
    int scrollBy = static_cast<int>(LOCATION_ITEM_HEIGHT * G_SCALE * itemCount);

    if (scrollAnimation_.state() == QAbstractAnimation::Running)
    {
        animationScollTarget_ += scrollBy;
    }
    else
    {
        animationScollTarget_ = widgetCitiesList_->geometry().y() + scrollBy;
    }

    scrollAnimation_.stop();
    scrollAnimation_.setDuration(PROGRAMMATIC_SCROLL_ANIMATION_DURATION);
    scrollAnimation_.setStartValue(widgetCitiesList_->geometry().y());
    scrollAnimation_.setEndValue(animationScollTarget_);
    scrollAnimation_.setDirection(QAbstractAnimation::Forward);
    scrollAnimation_.start();
}

const LocationID WidgetCities::topViewportSelectableLocationId()
{
    int index = qRound(qAbs(widgetCitiesList_->geometry().y())/(LOCATION_ITEM_HEIGHT * G_SCALE));

    auto widgets = widgetCitiesList_->itemWidgets();
    if (index < 0 || index > widgets.count() - 1)
    {
        qDebug(LOG_BASIC) << "Err: Can't index selectable items with: " << index;
        return LocationID();
    }

    return widgets[index]->getId();
}

int WidgetCities::viewportOffsetIndex()
{
    int index = qRound(qAbs(widgetCitiesList_->geometry().y())/(LOCATION_ITEM_HEIGHT * G_SCALE));
    return index;
}

int WidgetCities::accentItemViewportIndex()
{
    if (!widgetCitiesList_->hasAccentItem())
    {
        return -1;
    }
    return viewportIndex(widgetCitiesList_->lastAccentedItemWidget()->getId());
}

int WidgetCities::viewportIndex(LocationID locationId)
{
    int topItemSelIndex = widgetCitiesList_->selectableIndex(topViewportSelectableLocationId());
    int desiredLocSelIndex = widgetCitiesList_->selectableIndex(locationId);
    int desiredLocationViewportIndex = desiredLocSelIndex - topItemSelIndex;
    return desiredLocationViewportIndex;
}

bool WidgetCities::locationIdInViewport(LocationID location)
{
    int vi = viewportIndex(location);
    return vi >= 0 && vi < countOfAvailableItemSlots_;
}

bool WidgetCities::isGlobalPointInViewport(const QPoint &pt)
{
    QPoint pt1 = viewport()->mapToGlobal(QPoint(0,0));
    QRect globalRectOfViewport(pt1.x(), pt1.y(), viewport()->geometry().width(), viewport()->geometry().height());
    return globalRectOfViewport.contains(pt);
}

QRect WidgetCities::globalLocationsListViewportRect()
{
    int offset = getItemHeight();
    QPoint locationItemListTopLeft(0, geometry().y() - offset);
    locationItemListTopLeft = mapToGlobal(locationItemListTopLeft);
    return QRect(locationItemListTopLeft.x(), locationItemListTopLeft.y(), viewport()->geometry().width(), viewport()->geometry().height());
}

int WidgetCities::getScrollBarWidth()
{
    return WidgetLocationsSizes::instance().getScrollBarWidth();
}

int WidgetCities::getItemHeight() const
{
    return WidgetLocationsSizes::instance().getItemHeight();
}

void WidgetCities::handleTapClick(const QPoint &cursorPos)
{
    Q_UNUSED(cursorPos)
//    if (!isScrollAnimationNow_)
//    {
//        detectSelectedItem(cursorPos);
//        viewport()->update();
//        setCursorForSelected();

//        emitSelectedIfNeed();
//    }
}

int WidgetCities::closestPositionIncrement(int value)
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
