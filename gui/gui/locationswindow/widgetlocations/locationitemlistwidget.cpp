#include "locationitemlistwidget.h"

#include <QPainter>
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "cursorupdatehelper.h"
#include "utils/logger.h"


namespace GuiLocations {


LocationItemListWidget::LocationItemListWidget(IWidgetLocationsInfo * widgetLocationsInfo, QWidget *parent) : QWidget(parent)
  , height_(0)
  , lastAccentedItemWidget_(nullptr)
  , widgetLocationsInfo_(widgetLocationsInfo)
{
    setFocusPolicy(Qt::NoFocus);

    cursorUpdateHelper_.reset(new CursorUpdateHelper(this));
}

LocationItemListWidget::~LocationItemListWidget()
{
    qDebug() << "Deleting list object";
    clearWidgets();
}

int LocationItemListWidget::countRegions() const
{
    return itemWidgets_.count();
}

void LocationItemListWidget::clearWidgets()
{
    lastAccentedItemWidget_ = nullptr;
    recentlySelectedWidgets_.clear();
    foreach (LocationItemRegionWidget *regionWidget, itemWidgets_)
    {
        regionWidget->disconnect();
        regionWidget->deleteLater();
    }
    itemWidgets_.clear();
}

void LocationItemListWidget::addRegionWidget(LocationModelItem *item)
{
    auto regionWidget = new LocationItemRegionWidget(widgetLocationsInfo_, item, this);
    connect(regionWidget, SIGNAL(heightChanged(int)), SLOT(onRegionWidgetHeightChanged(int)));
    connect(regionWidget, SIGNAL(clicked(LocationItemCityWidget *)), SLOT(onLocationItemCityClicked(LocationItemCityWidget *)));
    connect(regionWidget, SIGNAL(clicked(LocationItemRegionWidget *)), SLOT(onLocationItemRegionClicked(LocationItemRegionWidget *)));
    connect(regionWidget, SIGNAL(selected(SelectableLocationItemWidget *)), SLOT(onSelectableLocationItemSelected(SelectableLocationItemWidget *)));
    connect(regionWidget, SIGNAL(favoriteClicked(LocationItemCityWidget*, bool)), SIGNAL(favoriteClicked(LocationItemCityWidget*,bool)));
    itemWidgets_.append(regionWidget);
    regionWidget->setGeometry(0, 0, static_cast<int>(WINDOW_WIDTH *G_SCALE), static_cast<int>(LOCATION_ITEM_HEIGHT * G_SCALE));
    regionWidget->show();
    recalcItemPositions();
}

void LocationItemListWidget::addCityToRegion(CityModelItem city, LocationModelItem *region)
{
    auto matchingId = [&](LocationItemRegionWidget  *regionWidget){
        return regionWidget->getId() == region->id;
    };

    if (!(std::find_if(itemWidgets_.begin(), itemWidgets_.end(), matchingId) != itemWidgets_.end()))
    {
        addRegionWidget(region);
    }
    itemWidgets_.last()->addCity(city);

    recalcItemPositions();
}

void LocationItemListWidget::updateScaling()
{
    recalcItemPositions();
}

void LocationItemListWidget::selectWidgetContainingCursor()
{
    foreach (SelectableLocationItemWidget *selectableWidget , selectableWidgets())
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

void LocationItemListWidget::expand(LocationID locId)
{
    foreach (LocationItemRegionWidget *regionWidget, itemWidgets_)
    {
        if (regionWidget->getId() == locId)
        {
            regionWidget->expand();
            emit regionExpanding(regionWidget, EXPAND_REASON_AUTO);
        }
    }
}

void LocationItemListWidget::collapse(LocationID locId)
{
    foreach (LocationItemRegionWidget *regionWidget, itemWidgets_)
    {
        if (regionWidget->getId() == locId)
        {
            regionWidget->collapse();
        }
    }
}

void LocationItemListWidget::expandAllLocations()
{
    foreach (LocationItemRegionWidget *regionWidget, itemWidgets_)
    {
        if (regionWidget->expandable())
        {
            regionWidget->expand();
        }
    }
}

void LocationItemListWidget::collapseAllLocations()
{
    foreach (LocationItemRegionWidget *regionWidget, itemWidgets_)
    {
        regionWidget->collapse();
    }
}

void LocationItemListWidget::expandLocationIds(QVector<LocationID> locIds)
{
    foreach (LocationItemRegionWidget *regionWidget, itemWidgets_)
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

QVector<LocationID> LocationItemListWidget::expandedOrExpandingLocationIds()
{
    QVector<LocationID> expanded;
    foreach (LocationItemRegionWidget *regionWidget, itemWidgets_)
    {
        if (regionWidget->expandedOrExpanding())
        {
            expanded.append(regionWidget->getId());
        }
    }
    return expanded;
}

QVector<LocationItemRegionWidget *> LocationItemListWidget::itemWidgets()
{
    return itemWidgets_;
}

const LocationID LocationItemListWidget::topSelectableLocationIdInViewport()
{
    int index = qAbs(geometry().y())/ITEM_HEIGHT;
    if (index < 0) return LocationID();

    auto widgets = selectableWidgets();
    if (index > widgets.count() - 1)
    {
        // this shouldn't happen
        qDebug(LOG_BASIC) << "Err: There isn't enough items in selectable list to index (locationID)";
        return LocationID();
    }

    return widgets[index]->getId();
}

int LocationItemListWidget::selectableIndex(LocationID locationId)
{
    int i = 0;
    foreach (SelectableLocationItemWidget *widget, selectableWidgets())
    {
        if (widget->getId() == locationId)
        {
            return i;
        }
        i++;
    }
    return -1;
}

int LocationItemListWidget::viewportIndex(LocationID locationId)
{
    int topItemSelIndex = selectableIndex(topSelectableLocationIdInViewport());
    int desiredLocSelIndex = selectableIndex(locationId);
    int desiredLocationViewportIndex = desiredLocSelIndex - topItemSelIndex;
    return desiredLocationViewportIndex;
}

void LocationItemListWidget::accentFirstItem()
{
    QVector<SelectableLocationItemWidget *> widgets = selectableWidgets();
    if (widgets.count() > 0)
    {
        widgets[0]->setSelected(true);
    }
}

bool LocationItemListWidget::hasAccentItem()
{
    return lastAccentedItemWidget_;
}

void LocationItemListWidget::moveAccentUp()
{
    SelectableLocationItemWidget *lastWidget = nullptr;
    SelectableLocationItemWidget *currentWidget =  nullptr;
    foreach (SelectableLocationItemWidget *widget, selectableWidgets())
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

void LocationItemListWidget::moveAccentDown()
{
    auto list = selectableWidgets();
    QVectorIterator<SelectableLocationItemWidget *> it(list);
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

int LocationItemListWidget::accentItemSelectableIndex()
{
    if (!lastAccentedItemWidget_)
    {
        qDebug() << "LocationItemListWidget::accentItemSelectableIndex - no accent item";
        return -1;
    }
    return selectableIndex(lastAccentedItemWidget_->getId());
}

int LocationItemListWidget::accentItemViewportIndex()
{
    if (!lastAccentedItemWidget_)
    {
        qDebug() << "LocationItemListWidget::accentItemViewportIndex - no accent item";
        return -1;
    }
    return viewportIndex(lastAccentedItemWidget_->getId());
}

SelectableLocationItemWidget *LocationItemListWidget::lastAccentedItemWidget()
{
    return lastAccentedItemWidget_;
}

const LocationID LocationItemListWidget::lastSelectedLocationId() const
{
    if (!lastAccentedItemWidget_)
    {
        return LocationID();
    }
    return lastAccentedItemWidget_->getId();
}

void LocationItemListWidget::selectItem(LocationID locationId)
{
    foreach (SelectableLocationItemWidget *widget, selectableWidgets())
    {
        if (widget->getId() == locationId)
        {
            widget->setSelected(true);
            break;
        }
    }
}

QVector<LocationItemCityWidget *> LocationItemListWidget::cityWidgets()
{
    QVector<LocationItemCityWidget *> cityWidgets;
    foreach (LocationItemRegionWidget *regionWidget, itemWidgets_)
    {
        cityWidgets.append(regionWidget->cityWidgets());
    }
    return cityWidgets;
}

QVector<SelectableLocationItemWidget *> LocationItemListWidget::selectableWidgets()
{
    QVector<SelectableLocationItemWidget *> selectableItemWidgets;
    foreach (LocationItemRegionWidget *regionWidget, itemWidgets_)
    {
        selectableItemWidgets.append(regionWidget->selectableWidgets());
    }
    return selectableItemWidgets;
}

void LocationItemListWidget::updateCursorWithSelectableWidget(SelectableLocationItemWidget *widget)
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


void LocationItemListWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    QRect background(0,0, geometry().width(), geometry().height());
    painter.fillRect(background, Qt::gray);

}

void LocationItemListWidget::onRegionWidgetHeightChanged(int height)
{
    auto regionWidget = static_cast<LocationItemRegionWidget*>(sender());
    regionWidget->setGeometry(0, 0, static_cast<int>(WINDOW_WIDTH * G_SCALE), static_cast<int>(height * G_SCALE));
    recalcItemPositions();
}

void LocationItemListWidget::onLocationItemCityClicked(LocationItemCityWidget *cityWidget)
{
    emit locationIdSelected(cityWidget->getId());
}


void LocationItemListWidget::onLocationItemRegionClicked(LocationItemRegionWidget *regionWidget)
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

void LocationItemListWidget::onSelectableLocationItemSelected(SelectableLocationItemWidget *itemWidget)
{
    // iterating through all selectable widgets is too slow for scrolling animation
    // only iterate through cache of previously selected widgets
    for (SelectableLocationItemWidget *widget : recentlySelectedWidgets_)
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

void LocationItemListWidget::recalcItemPositions()
{
    int heightSoFar = 0;
    foreach (LocationItemRegionWidget  *itemWidget, itemWidgets_)
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
