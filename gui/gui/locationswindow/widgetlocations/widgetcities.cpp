#include "widgetcities.h"
#include <QDateTime>
#include <QGestureEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScroller>
#include <QApplication>
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
#include "utils/utils.h"

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

    gestureScrollingElapsedTimer_.start();

    // scrollbar
    scrollBar_ = new ScrollBar(this);
    setVerticalScrollBar(scrollBar_);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollBar_->setSingleStep(LOCATION_ITEM_HEIGHT * G_SCALE); // scroll by this many px at a time
    scrollBar_->setGeometry(WINDOW_WIDTH * G_SCALE - getScrollBarWidth(), 0, getScrollBarWidth(), 170 * G_SCALE);
    connect(scrollBar_, SIGNAL(handleDragged(int)), SLOT(onScrollBarHandleDragged(int)));
    connect(scrollBar_, SIGNAL(stopScroll(bool)), SLOT(onScrollBarStopScroll(bool)));

    // central widget
    widgetCitiesList_ = new WidgetCitiesList(this, this);
    setWidget(widgetCitiesList_);
    connect(widgetCitiesList_, SIGNAL(heightChanged(int)), SLOT(onLocationItemListWidgetHeightChanged(int)));
    connect(widgetCitiesList_, SIGNAL(favoriteClicked(ItemWidgetCity*,bool)), SLOT(onLocationItemListWidgetFavoriteClicked(ItemWidgetCity *, bool)));
    connect(widgetCitiesList_, SIGNAL(locationIdSelected(LocationID)), SLOT(onLocationItemListWidgetLocationIdSelected(LocationID)));
    widgetCitiesList_->setGeometry(0,0, WINDOW_WIDTH*G_SCALE - getScrollBarWidth(), 0);
    widgetCitiesList_->show();

    emptyListButton_ = new CommonWidgets::TextButtonWidget(QString(), this);
    emptyListButton_->setFont(FontDescr(12, false));
    emptyListButton_->hide();
    connect(emptyListButton_, SIGNAL(clicked()), SIGNAL(emptyListButtonClicked()));

    preventMouseSelectionTimer_.start();
    connect(&scrollAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onScrollAnimationValueChanged(QVariant)));
    connect(&scrollAnimation_, SIGNAL(finished()), SLOT(onScrollAnimationFinished()));
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
    scrollBar_->setSingleStep(LOCATION_ITEM_HEIGHT * G_SCALE); // scroll by this many px at a time
    scrollBar_->updateCustomStyleSheet();
    widgetCitiesList_->updateScaling();
    updateEmptyListButton();

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

bool WidgetCities::hasAccentItem()
{
    return widgetCitiesList_->hasAccentItem();
}

LocationID WidgetCities::accentedItemLocationId()
{
    if (widgetCitiesList_->lastAccentedItemWidget())
    {
        return widgetCitiesList_->lastAccentedItemWidget()->getId();
    }
    return LocationID();
}

void WidgetCities::accentFirstItem()
{
    widgetCitiesList_->accentFirstItem();
}


bool WidgetCities::cursorInViewport()
{
    QPoint cursorPos = QCursor::pos();
    QRect rect = globalLocationsListViewportRect();
    return rect.contains(cursorPos);
}

void WidgetCities::centerCursorOnAccentedItem()
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
    const auto widgetCities = widgetCitiesList_->itemWidgets();
    for (auto *w : widgetCities)
        w->setShowLatencyMs(showLatencyInMs);
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
    // order of events:
    //      scroll: prepare -> start -> scroll -> canceled
    //      tap   : prepate -> start -> finished
    if (object == viewport() && event->type() == QEvent::Gesture)
    {
        QGestureEvent *ge = static_cast<QGestureEvent *>(event);
        qDebug() << "Event filter - Some Gesture: " << ge->gestures();
        QTapGesture *g = static_cast<QTapGesture *>(ge->gesture(Qt::TapGesture));
        if (g)
        {
            if (g->state() == Qt::GestureStarted)
            {
                qDebug() << "Tap Gesture started";
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
                    qDebug() << "Tap Gesture finished, selecting by pt: " << pt;
                    widgetCitiesList_->selectWidgetContainingGlobalPt(pt);
                }
            }
            else if (g->state() == Qt::GestureCanceled)
            {
                qDebug() << "Tap Gesture canceled - scrolling";
                bTapGestureStarted_ = false;
            }
        }
        QPanGesture *gp = static_cast<QPanGesture *>(ge->gesture(Qt::PanGesture));
        if (gp)
        {
            qDebug() << "Pan gesture";
            if (gp->state() == Qt::GestureStarted)
            {
                qDebug() << "Pan gesture started";
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
        widgetCitiesList_->accentWidgetContainingCursor();
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
            if(!lastSelWidget->isForbidden() &&
               !lastSelWidget->isDisabled()  &&
               !lastSelWidget->isBrokenConfig())
            {
                emit selected(widgetCitiesList_->lastAccentedLocationId());
            }
        }
    }
}

int WidgetCities::gestureScrollingElapsedTime()
{
    return gestureScrollingElapsedTimer_.elapsed();
}

void WidgetCities::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    // qDebug() << "WidgetCities::paintEvent - geo: " << geometry();

    // draw background for when list is < size of viewport
    QPainter painter(viewport());
    QRect bkgd(0,0,geometry().width(), geometry().height());
    painter.fillRect(bkgd, FontManager::instance().getMidnightColor());

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
            widgetCitiesList_->accentWidgetContainingCursor();
        }

        if (!heightChanging_) // prevents false-positive scrolling when changing scale -- setting widgetLocationsList_ geometry in the heightChanged slot races with the manual forceSetValue
        {
            QScrollArea::scrollContentsBy(dx,dy);
            lastScrollPos_ = widgetCitiesList_->geometry().y();
        }
    }
}

void WidgetCities::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    // are we blocking mousePressEvent from bubbling up here intentionally?
    // need note
}

void WidgetCities::mouseDoubleClickEvent(QMouseEvent *event)
{
    // not mouseDoubleClickEvent? was this intentional? need note here
    QScrollArea::mousePressEvent(event);
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
    bIsFreeSession_ = isFreeSessionStatus;
}

void WidgetCities::onLanguageChanged()
{
    // TODO: language change
}

void WidgetCities::onLocationItemListWidgetHeightChanged(int listWidgetHeight)
{
    heightChanging_ = true;
    widgetCitiesList_->setGeometry(0,widgetCitiesList_->geometry().y(), WINDOW_WIDTH*G_SCALE - getScrollBarWidth(), listWidgetHeight);
    heightChanging_ = false;
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
    if (kickPreventMouseSelectionTimer_ && !Utils::accessibilityPermissions())
    {
        preventMouseSelectionTimer_.restart();
    }

    widgetCitiesList_->move(0, value.toInt());
    lastScrollPos_ = widgetCitiesList_->geometry().y();
    viewport()->update();
}

void WidgetCities::onScrollAnimationFinished()
{
    updateScrollBarWithView();
}

void WidgetCities::onScrollBarHandleDragged(int valuePos)
{
    animationScollTarget_ = -valuePos;
    // qDebug() << "Dragged: " << locationItemListWidget_->geometry().y() << " -> " << animationScollTarget_;
    startAnimationScrollByPosition(animationScollTarget_, scrollAnimation_);
}

void WidgetCities::onScrollBarStopScroll(bool lastScrollDirectionUp)
{
    if (lastScrollDirectionUp)
    {
        int nextPos = previousPositionIncrement(-widgetCitiesList_->geometry().y());
        animationScollTarget_ = -nextPos;
        startAnimationScrollByPosition(animationScollTarget_, scrollAnimation_);
    }
    else
    {
        int nextPos = nextPositionIncrement(-widgetCitiesList_->geometry().y());
        animationScollTarget_ = -nextPos;
        startAnimationScrollByPosition(animationScollTarget_, scrollAnimation_);
    }
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
    LocationID lastAccentedLocationId = widgetCitiesList_->lastAccentedLocationId();

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
    widgetCitiesList_->accentItem(lastAccentedLocationId);
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
        updateScrollBarWithView();
        animationScollTarget_ -= scrollBy;
        if (animationScollTarget_ < -scrollBar_->maximum()) animationScollTarget_ = -scrollBar_->maximum(); // prevent scrolling too far during key press animation
    }
    else
    {
        animationScollTarget_ = widgetCitiesList_->geometry().y() - scrollBy;
    }
    startAnimationScrollByPosition(animationScollTarget_, scrollAnimation_);
}

void WidgetCities::animatedScrollUp(int itemCount)
{
    int scrollBy = static_cast<int>(LOCATION_ITEM_HEIGHT * G_SCALE * itemCount);

    if (scrollAnimation_.state() == QAbstractAnimation::Running)
    {
        updateScrollBarWithView();
        animationScollTarget_ += scrollBy;
        if (animationScollTarget_ > 0) animationScollTarget_ = 0; // prevent scrolling too far during key press animation
    }
    else
    {
        animationScollTarget_ = widgetCitiesList_->geometry().y() + scrollBy;
    }
    startAnimationScrollByPosition(animationScollTarget_, scrollAnimation_);
}

void WidgetCities::startAnimationScrollByPosition(int positionValue, QVariantAnimation &animation)
{
    animation.stop();
    animation.setDuration(PROGRAMMATIC_SCROLL_ANIMATION_DURATION);
    animation.setStartValue(widgetCitiesList_->geometry().y());
    animation.setEndValue(positionValue);
    animation.setDirection(QAbstractAnimation::Forward);
    animation.start();
}

void WidgetCities::updateScrollBarWithView()
{
    // update scroll bar for keypress navigation
    if (!scrollBar_->dragging())
    {
        scrollBar_->forceSetValue(-widgetCitiesList_->geometry().y());
    }
}

const LocationID WidgetCities::topViewportSelectableLocationId()
{
    int index = qRound(qAbs(widgetCitiesList_->geometry().y())/(LOCATION_ITEM_HEIGHT * G_SCALE));

    auto widgets = widgetCitiesList_->itemWidgets();
    if (index < 0 || index > widgets.count() - 1)
    {
        // qDebug(LOG_BASIC) << "Err: Can't index selectable items with: " << index;
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

int WidgetCities::nextPositionIncrement(int value)
{
    int current = 0;
    while (current < value)
    {
        current += LOCATION_ITEM_HEIGHT * G_SCALE;
    }

    return current;
}

int WidgetCities::previousPositionIncrement(int value)
{
    int current = 0;
    int last = 0;
    while (current < value)
    {
        last = current;
        current += LOCATION_ITEM_HEIGHT * G_SCALE;
    }

    return last;
}

void WidgetCities::gestureScrollAnimation(int value)
{
    animationScollTarget_ = -value;

    scrollAnimation_.stop();
    scrollAnimation_.setDuration(GESTURE_SCROLL_ANIMATION_DURATION);
    scrollAnimation_.setStartValue(widgetCitiesList_->geometry().y());
    scrollAnimation_.setEndValue(animationScollTarget_);
    scrollAnimation_.setDirection(QAbstractAnimation::Forward);
    scrollAnimation_.start();
    qApp->processEvents(); // animate scrolling at same time as gesture is moving
}

} // namespace GuiLocations
