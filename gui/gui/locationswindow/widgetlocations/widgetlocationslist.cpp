#include "widgetlocationslist.h"

#include <QPainter>
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "cursorupdatehelper.h"
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
    recentlySelectedWidgets_.clear();
    foreach (ItemWidgetRegion *regionWidget, itemWidgets_)
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
    connect(regionWidget, SIGNAL(selected(IItemWidget *)), SLOT(onSelectableLocationItemSelected(IItemWidget *)));
    connect(regionWidget, SIGNAL(favoriteClicked(ItemWidgetCity*, bool)), SIGNAL(favoriteClicked(ItemWidgetCity*,bool)));
    itemWidgets_.append(regionWidget);
    regionWidget->setGeometry(0, 0, static_cast<int>(WINDOW_WIDTH *G_SCALE), static_cast<int>(LOCATION_ITEM_HEIGHT * G_SCALE));
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
    foreach (ItemWidgetRegion  *itemWidget, itemWidgets_)
    {
        itemWidget->updateScaling();
    }
}

void WidgetLocationsList::selectWidgetContainingCursor()
{
    foreach (IItemWidget *selectableWidget , selectableWidgets())
    {
        if (selectableWidget->containsCursor())
        {
            // qDebug() << "Selecting by containing cursor";
            selectableWidget->setSelected(true);
            updateCursorWithSelectableWidget(selectableWidget);

            break;
        }
    }
}

void WidgetLocationsList::expand(LocationID locId)
{
    foreach (ItemWidgetRegion *regionWidget, itemWidgets_)
    {
        if (regionWidget->getId() == locId)
        {
            regionWidget->expand();
            emit regionExpanding(regionWidget, EXPAND_REASON_AUTO);
        }
    }
}

void WidgetLocationsList::collapse(LocationID locId)
{
    foreach (ItemWidgetRegion *regionWidget, itemWidgets_)
    {
        if (regionWidget->getId() == locId)
        {
            regionWidget->collapse();
        }
    }
}

void WidgetLocationsList::expandAllLocations()
{
    foreach (ItemWidgetRegion *regionWidget, itemWidgets_)
    {
        if (regionWidget->expandable())
        {
            regionWidget->expand();
        }
    }
}

void WidgetLocationsList::collapseAllLocations()
{
    foreach (ItemWidgetRegion *regionWidget, itemWidgets_)
    {
        regionWidget->collapse();
    }
}

void WidgetLocationsList::expandLocationIds(QVector<LocationID> locIds)
{
    foreach (ItemWidgetRegion *regionWidget, itemWidgets_)
    {
        foreach (LocationID locId, locIds)
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
    foreach (ItemWidgetRegion *regionWidget, itemWidgets_)
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
    foreach (IItemWidget *widget, selectableWidgets())
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
    QVector<IItemWidget *> widgets = selectableWidgets();
    if (widgets.count() > 0)
    {
        widgets[0]->setSelected(true);
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
    foreach (IItemWidget *widget, selectableWidgets())
    {
        lastWidget = currentWidget;
        currentWidget = widget;

        if (widget->isSelected())
        {
            break;
        }
    }

    if (lastWidget != nullptr)
    {
        // qDebug() << "Selection by moveAccentUp";
        lastWidget->setSelected(true);
    }
}

void WidgetLocationsList::moveAccentDown()
{
    auto list = selectableWidgets();
    QVectorIterator<IItemWidget *> it(list);
    while (it.hasNext())
    {
        if (it.next()->isSelected())
        {
            break;
        }
    }

    if (it.hasNext())
    {
        auto widget = it.next();
        // qDebug() << "Selection by moveAccentDown";

        widget->setSelected(true);
    }
}

int WidgetLocationsList::accentItemSelectableIndex()
{
    if (!lastAccentedItemWidget_)
    {
        qDebug() << "LocationItemListWidget::accentItemSelectableIndex - no accent item";
        return -1;
    }
    return selectableIndex(lastAccentedItemWidget_->getId());
}

IItemWidget *WidgetLocationsList::lastAccentedItemWidget()
{
    return lastAccentedItemWidget_;
}

const LocationID WidgetLocationsList::lastSelectedLocationId() const
{
    if (!lastAccentedItemWidget_)
    {
        return LocationID();
    }
    return lastAccentedItemWidget_->getId();
}

void WidgetLocationsList::selectItem(LocationID locationId)
{
    foreach (IItemWidget *widget, selectableWidgets())
    {
        if (widget->getId() == locationId)
        {
            widget->setSelected(true);
            break;
        }
    }
}

IItemWidget *WidgetLocationsList::selectableWidget(LocationID locationId)
{
    foreach (IItemWidget *widget, selectableWidgets())
    {
        if (widget->getId() == locationId)
        {
            return widget;
        }
    }
    return nullptr;
}

QVector<ItemWidgetCity *> WidgetLocationsList::cityWidgets()
{
    QVector<ItemWidgetCity *> cityWidgets;
    foreach (ItemWidgetRegion *regionWidget, itemWidgets_)
    {
        cityWidgets.append(regionWidget->cityWidgets());
    }
    return cityWidgets;
}

QVector<IItemWidget *> WidgetLocationsList::selectableWidgets()
{
    QVector<IItemWidget *> selectableItemWidgets;
    foreach (ItemWidgetRegion *regionWidget, itemWidgets_)
    {
        selectableItemWidgets.append(regionWidget->selectableWidgets());
    }
    return selectableItemWidgets;
}

void WidgetLocationsList::updateCursorWithSelectableWidget(IItemWidget *widget)
{
    if (widget->isForbidden() || widget->isDisabled())
    {
        cursorUpdateHelper_->setForbiddenCursor();
    }
    else
    {
        cursorUpdateHelper_->setPointingHandCursor();
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
    emit locationIdSelected(cityWidget->getId());
}


void WidgetLocationsList::onLocationItemRegionClicked(ItemWidgetRegion *regionWidget)
{
    if (regionWidget->getId().isBestLocation())
    {
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
                emit regionExpanding(regionWidget, EXPAND_REASON_USER);
            }
        }
    }
}

void WidgetLocationsList::onSelectableLocationItemSelected(IItemWidget *itemWidget)
{
    // iterating through all selectable widgets is too slow for scrolling animation
    // only iterate through cache of previously selected widgets
    for (IItemWidget *widget : recentlySelectedWidgets_)
    {
        if (widget != itemWidget)
        {
            widget->setSelected(false);
        }
    }
    recentlySelectedWidgets_.clear();

    updateCursorWithSelectableWidget(itemWidget);
    recentlySelectedWidgets_.append(itemWidget);
    lastAccentedItemWidget_ = itemWidget;
}

void WidgetLocationsList::recalcItemPositions()
{
    // qDebug() << "List repositioning items";
    int heightSoFar = 0;
    foreach (ItemWidgetRegion  *itemWidget, itemWidgets_)
    {
        itemWidget->setGeometry(0, heightSoFar, static_cast<int>(WINDOW_WIDTH * G_SCALE), itemWidget->geometry().height());
        heightSoFar += itemWidget->geometry().height();
    }

    if (heightSoFar != height_)
    {
        height_ = heightSoFar;
        emit heightChanged(heightSoFar);
    }
    update();
}

} // namespace
