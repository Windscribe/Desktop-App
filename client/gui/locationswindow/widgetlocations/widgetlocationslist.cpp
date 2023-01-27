#include "widgetlocationslist.h"

#include <QPainter>
#include <QtMath>
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "utils/logger.h"


namespace GuiLocations {


WidgetLocationsList::WidgetLocationsList(IWidgetLocationsInfo * widgetLocationsInfo, QWidget *parent) : QWidget(parent)
  , height_(0)
  , lastAccentedItemWidget_(nullptr)
  , widgetLocationsInfo_(widgetLocationsInfo)
{
    setFocusPolicy(Qt::NoFocus);

    cursorUpdateHelper_.reset(new CursorUpdateHelper(this));
}

WidgetLocationsList::~WidgetLocationsList()
{
    // qDebug() << "Deleting list object";
    clearWidgets();
}

void WidgetLocationsList::clearWidgets()
{
    lastAccentedItemWidget_ = nullptr;
    recentlyAccentedWidgets_.clear();
    for (auto *regionWidget : qAsConst(itemWidgets_))
    {
        regionWidget->disconnect();
        regionWidget->deleteLater();
    }
    itemWidgets_.clear();
}

void WidgetLocationsList::addRegionWidget(LocationModelItem *item)
{
    auto regionWidget = new ItemWidgetRegion(widgetLocationsInfo_, item, this);
    connect(regionWidget, SIGNAL(heightChanged(int)), SLOT(onRegionWidgetHeightChanged(int)));
    connect(regionWidget, SIGNAL(clicked(ItemWidgetCity *)), SLOT(onLocationItemCityClicked(ItemWidgetCity *)));
    connect(regionWidget, SIGNAL(clicked(ItemWidgetRegion *)), SLOT(onLocationItemRegionClicked(ItemWidgetRegion *)));
    connect(regionWidget, SIGNAL(accented(IItemWidget *)), SLOT(onSelectableLocationItemAccented(IItemWidget *)));
    connect(regionWidget, SIGNAL(favoriteClicked(ItemWidgetCity*, bool)), SIGNAL(favoriteClicked(ItemWidgetCity*,bool)));
    itemWidgets_.append(regionWidget);
    regionWidget->setGeometry(0, 0, static_cast<int>(WINDOW_WIDTH *G_SCALE), static_cast<int>(qCeil(LOCATION_ITEM_HEIGHT * G_SCALE)));
    regionWidget->show();
    recalcItemPositions();
}

void WidgetLocationsList::addCityToRegion(const CityModelItem &city, LocationModelItem *region)
{
    auto matchingId = [&](ItemWidgetRegion  *regionWidget){
        return regionWidget->getId() == region->id;
    };

    if (!(std::find_if(itemWidgets_.begin(), itemWidgets_.end(), matchingId) != itemWidgets_.end()))
    {
        addRegionWidget(region);
    }
    itemWidgets_.last()->addCity(city);

    recalcItemPositions();
}

void WidgetLocationsList::updateScaling()
{
    // update scaling for each widget instead of recalcItemPositions
    // relying on resizeEvent after setGeometry on region items blocks expanding animation for some reason
    // this will trigger recalcItemPositions() anyway
    for (auto *itemWidget : qAsConst(itemWidgets_))
    {
        itemWidget->updateScaling();
    }
}

void WidgetLocationsList::accentWidgetContainingCursor()
{
    const auto widgets = selectableWidgets();
    for (auto *selectableWidget : widgets)
    {
        if (selectableWidget->containsCursor())
        {
            // qDebug() << "Selecting by containing cursor";
            selectableWidget->setAccented(true);
            updateCursorWithSelectableWidget(selectableWidget);
            break;
        }
    }
}

void WidgetLocationsList::selectWidgetContainingGlobalPt(const QPoint &pt)
{
    const auto widgets = selectableWidgets();
    for (auto *selectableWidget : widgets)
    {
        if (selectableWidget->containsGlobalPoint(pt))
        {
            //qDebug() << "Selecting: " << selectableWidget->name();
            selectableWidget->setAccented(true);
            safeEmitLocationIdSelected(selectableWidget);
            break;
        }
    }
}

void WidgetLocationsList::expand(LocationID locId)
{
    for (auto *regionWidget : qAsConst(itemWidgets_))
    {
        if (regionWidget->getId() == locId)
        {
            regionWidget->expand();
        }
    }
}

void WidgetLocationsList::collapse(LocationID locId)
{
    for (auto *regionWidget : qAsConst(itemWidgets_))
    {
        if (regionWidget->getId() == locId)
        {
            regionWidget->collapse();
        }
    }
}

void WidgetLocationsList::expandAllLocationsWithoutAnimation()
{
    for (auto *regionWidget : qAsConst(itemWidgets_))
    {
        if (regionWidget->expandable())
        {
            regionWidget->setExpandedWithoutAnimation(true);
        }
    }
}

void WidgetLocationsList::collapseAllLocationsWithoutAnimation()
{
    for (auto *regionWidget : qAsConst(itemWidgets_))
    {
        regionWidget->setExpandedWithoutAnimation(false);
    }
}

void WidgetLocationsList::expandLocationIds(QVector<LocationID> locIds)
{
    for (auto *regionWidget : qAsConst(itemWidgets_))
    {
        for (const LocationID &locId : qAsConst(locIds))
        {
            if (regionWidget->getId() == locId)
            {
                regionWidget->setExpandedWithoutAnimation(true);
            }
        }
    }
}

QVector<LocationID> WidgetLocationsList::expandedOrExpandingLocationIds()
{
    QVector<LocationID> expanded;
    for (auto *regionWidget : qAsConst(itemWidgets_))
    {
        if (regionWidget->expandedOrExpanding())
        {
            expanded.append(regionWidget->getId());
        }
    }
    return expanded;
}

QVector<ItemWidgetRegion *> WidgetLocationsList::itemWidgets()
{
    return itemWidgets_;
}

int WidgetLocationsList::selectableIndex(LocationID locationId)
{
    int i = 0;
    const auto widgets = selectableWidgets();
    for (auto *widget : widgets)
    {
        if (widget->getId() == locationId)
        {
            return i;
        }
        i++;
    }
    return -1;
}

void WidgetLocationsList::accentFirstSelectableItem()
{
    const auto widgets = selectableWidgets();
    if (widgets.count() > 0)
    {
        widgets[0]->setAccented(true);
    }
}

void WidgetLocationsList::accentFirstSelectableItemWithoutAnimation()
{
    QVector<IItemWidget *> widgets = selectableWidgets();
    if (widgets.count() > 0)
    {
        widgets[0]->setAccentedWithoutAnimation(true);
    }
}

bool WidgetLocationsList::hasAccentItem()
{
    return lastAccentedItemWidget_;
}

void WidgetLocationsList::moveAccentUp()
{
    IItemWidget *lastWidget = nullptr;
    IItemWidget *currentWidget =  nullptr;
    const auto widgets = selectableWidgets();
    for (auto *widget : widgets)
    {
        lastWidget = currentWidget;
        currentWidget = widget;

        if (widget->isAccented())
        {
            break;
        }
    }

    if (lastWidget != nullptr)
    {
        // qDebug() << "Selection by moveAccentUp";
        lastWidget->setAccented(true);
    }
}

void WidgetLocationsList::moveAccentDown()
{
    auto list = selectableWidgets();
    QVectorIterator<IItemWidget *> it(list);
    while (it.hasNext())
    {
        if (it.next()->isAccented())
        {
            break;
        }
    }

    if (it.hasNext())
    {
        auto widget = it.next();
        // qDebug() << "Selection by moveAccentDown";

        widget->setAccented(true);
    }
}

int WidgetLocationsList::accentItemSelectableIndex()
{
    if (!lastAccentedItemWidget_)
    {
        // qDebug() << "LocationItemListWidget::accentItemSelectableIndex - no accent item";
        return -1;
    }
    return selectableIndex(lastAccentedItemWidget_->getId());
}

IItemWidget *WidgetLocationsList::lastAccentedItemWidget()
{
    return lastAccentedItemWidget_;
}

const LocationID WidgetLocationsList::lastAccentedLocationId() const
{
    if (!lastAccentedItemWidget_)
    {
        return LocationID();
    }
    return lastAccentedItemWidget_->getId();
}

void WidgetLocationsList::accentItem(LocationID locationId)
{
    const auto widgets = selectableWidgets();
    for (auto *widget : widgets)
    {
        if (widget->getId() == locationId)
        {
            widget->setAccented(true);
            break;
        }
    }
}

void WidgetLocationsList::accentItemWithoutAnimation(LocationID locationId)
{
    const auto widgets = selectableWidgets();
    for (auto *widget : widgets)
    {
        if (widget->getId() == locationId)
        {
            widget->setAccentedWithoutAnimation(true);
            break;
        }
    }
}

void WidgetLocationsList::setMuteAccentChanges(bool mute)
{
    for (auto *region : qAsConst(itemWidgets_))
    {
        region->setMuteAccentChanges(mute);
    }
}

IItemWidget *WidgetLocationsList::selectableWidget(LocationID locationId)
{
    const auto widgets = selectableWidgets();
    for (auto *widget : widgets)
    {
        if (widget->getId() == locationId)
        {
            return widget;
        }
    }
    return nullptr;
}

ItemWidgetRegion *WidgetLocationsList::regionWidget(LocationID locationId)
{
    for (ItemWidgetRegion *regionWidget : qAsConst(itemWidgets_))
    {
        if (regionWidget->getId() == locationId)
        {
            return regionWidget;
        }
    }
    return nullptr;
}

IItemWidget *WidgetLocationsList::itemWidget(LocationID locationId)
{
    const auto widgets = cityWidgets();
    for (ItemWidgetCity *w : widgets)
    {
        if (w->getId() == locationId)
        {
            return w;
        }
    }
    return nullptr;
}

QVector<ItemWidgetCity *> WidgetLocationsList::cityWidgets()
{
    QVector<ItemWidgetCity *> cityWidgets;
    for (auto *regionWidget : qAsConst(itemWidgets_))
    {
        cityWidgets.append(regionWidget->cityWidgets());
    }
    return cityWidgets;
}

QVector<IItemWidget *> WidgetLocationsList::selectableWidgets()
{
    QVector<IItemWidget *> selectableItemWidgets;
    for (auto *regionWidget : qAsConst(itemWidgets_))
    {
        selectableItemWidgets.append(regionWidget->selectableWidgets());
    }
    return selectableItemWidgets;
}

void WidgetLocationsList::updateCursorWithSelectableWidget(IItemWidget *widget)
{
    if (widget->isForbidden() ||
        widget->isDisabled()  ||
        widget->isBrokenConfig())
    {
        cursorUpdateHelper_->setForbiddenCursor();
    }
    else
    {
        cursorUpdateHelper_->setPointingHandCursor();
    }
}

void WidgetLocationsList::safeEmitLocationIdSelected(IItemWidget *widget)
{
    if (!widget->isForbidden() &&
        !widget->isDisabled()  &&
        !widget->isBrokenConfig())
    {
        emit locationIdSelected(widget->getId());
    }
}


void WidgetLocationsList::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    // qDebug() << "List repainting";
}

void WidgetLocationsList::onRegionWidgetHeightChanged(int height)
{
    auto regionWidget = static_cast<ItemWidgetRegion*>(sender());
    regionWidget->setGeometry(0, 0, static_cast<int>(WINDOW_WIDTH * G_SCALE), static_cast<int>(height));
    recalcItemPositions();
}

void WidgetLocationsList::onLocationItemCityClicked(ItemWidgetCity *cityWidget)
{
    // block false-clicks that come after gesture scroll
    if (widgetLocationsInfo_->gestureScrollingElapsedTime() > 100)
    {
        safeEmitLocationIdSelected(cityWidget);
    }
}


void WidgetLocationsList::onLocationItemRegionClicked(ItemWidgetRegion *regionWidget)
{
    // block false-clicks that come after gesture scroll
    if (widgetLocationsInfo_->gestureScrollingElapsedTime() > 100)
    {
        if (regionWidget->getId().isBestLocation())
        {
            // shouldn't need to verify location is valid since it is best location
            emit locationIdSelected(regionWidget->getId());
        }
        else
        {
            if (regionWidget->expandedOrExpanding())
            {
                regionWidget->collapse();
            }
            else
            {
                if (regionWidget->expandable())
                {
                    regionWidget->expand();
                    emit regionExpanding(regionWidget);
                }
            }
        }
    }
}

void WidgetLocationsList::onSelectableLocationItemAccented(IItemWidget *itemWidget)
{
    // iterating through all selectable widgets is too slow for scrolling animation
    // only iterate through cache of previously accented widgets
    for (IItemWidget *widget : recentlyAccentedWidgets_)
    {
        if (widget != itemWidget)
        {
            widget->setAccentedWithoutAnimation(false);
        }
    }
    recentlyAccentedWidgets_.clear();

    updateCursorWithSelectableWidget(itemWidget);
    recentlyAccentedWidgets_.append(itemWidget);
    lastAccentedItemWidget_ = itemWidget;
}

void WidgetLocationsList::recalcItemPositions()
{
    // qDebug() << "List repositioning items";
    int heightSoFar = 0;
    for (auto *itemWidget : qAsConst(itemWidgets_))
    {
        itemWidget->setGeometry(0, heightSoFar, static_cast<int>(WINDOW_WIDTH * G_SCALE), itemWidget->geometry().height());
        heightSoFar += itemWidget->geometry().height();
    }

    if (heightSoFar != height_)
    {
        height_ = heightSoFar;
        emit heightChanged(heightSoFar);
    }
}

} // namespace
