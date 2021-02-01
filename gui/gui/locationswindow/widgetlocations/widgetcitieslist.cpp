#include "widgetcitieslist.h"

#include "dpiscalemanager.h"
#include "commongraphics/commongraphics.h"
#include "utils/logger.h"


namespace GuiLocations {

WidgetCitiesList::WidgetCitiesList(IWidgetLocationsInfo *widgetLocationsInfo, QWidget *parent) : QWidget(parent)
  , height_(0)
  , lastAccentedItemWidget_(nullptr)
  , widgetLocationsInfo_(widgetLocationsInfo)
{
    setFocusPolicy(Qt::NoFocus);
    cursorUpdateHelper_.reset(new CursorUpdateHelper(this));

    recalcItemPositions();
}

WidgetCitiesList::~WidgetCitiesList()
{
    clearWidgets();

}

void WidgetCitiesList::clearWidgets()
{
    lastAccentedItemWidget_ = nullptr;
    recentlySelectedWidgets_.clear();
    foreach (ItemWidgetCity *regionWidget, itemWidgets_)
    {
        regionWidget->disconnect();
        regionWidget->deleteLater();
    }
    itemWidgets_.clear();
}

void WidgetCitiesList::addCity(const CityModelItem &city)
{
    auto cityWidget = new ItemWidgetCity(widgetLocationsInfo_, city, this);
    connect(cityWidget, SIGNAL(clicked()), SLOT(onCityItemClicked()));
    connect(cityWidget, SIGNAL(selected()), SLOT(onCityItemAccented()));
    connect(cityWidget, SIGNAL(favoriteClicked(ItemWidgetCity *, bool)), SIGNAL(favoriteClicked(ItemWidgetCity*, bool)));
    cityWidget->setSelectable(true);
    cityWidget->show();
    itemWidgets_.push_back(cityWidget);

    // qDebug() << "Added: " << cityWidget->name();

    recalcItemPositions();
}

void WidgetCitiesList::updateScaling()
{
    recalcItemPositions();

}

void WidgetCitiesList::selectWidgetContainingCursor()
{
    foreach (IItemWidget *selectableWidget , itemWidgets_)
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

QVector<ItemWidgetCity *> WidgetCitiesList::itemWidgets()
{
    return itemWidgets_;
}

int WidgetCitiesList::selectableIndex(LocationID locationId)
{
    int i = 0;
    foreach (IItemWidget *widget, itemWidgets_)
    {
        if (widget->getId() == locationId)
        {
            return i;
        }
        i++;
    }
    return -1;
}

const LocationID WidgetCitiesList::lastAccentedLocationId() const
{
    if (!lastAccentedItemWidget_)
    {
        return LocationID();
    }
    return lastAccentedItemWidget_->getId();
}

void WidgetCitiesList::accentItem(LocationID locationId)
{
    foreach (IItemWidget *widget, itemWidgets_)
    {
        if (widget->getId() == locationId)
        {
            widget->setSelected(true);
            break;
        }
    }
}

void WidgetCitiesList::accentFirstItem()
{
    if (itemWidgets_.count() > 0)
    {
        itemWidgets_[0]->setSelected(true);
    }
}

bool WidgetCitiesList::hasAccentItem()
{
    return lastAccentedItemWidget_;
}

void WidgetCitiesList::moveAccentUp()
{
    IItemWidget *lastWidget = nullptr;
    IItemWidget *currentWidget =  nullptr;
    foreach (IItemWidget *widget, itemWidgets_)
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

void WidgetCitiesList::moveAccentDown()
{
    QVectorIterator<ItemWidgetCity *> it(itemWidgets_);
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
        widget->setSelected(true);
    }
}

int WidgetCitiesList::accentItemIndex()
{
    if (!lastAccentedItemWidget_)
    {
        // qDebug() << "LocationItemListWidget::accentItemSelectableIndex - no accent item";
        return -1;
    }
    return selectableIndex(lastAccentedItemWidget_->getId());
}

IItemWidget *WidgetCitiesList::lastAccentedItemWidget()
{
    return lastAccentedItemWidget_;
}

IItemWidget *WidgetCitiesList::selectableWidget(LocationID locationId)
{
    foreach (IItemWidget *widget, itemWidgets_)
    {
        if (widget->getId() == locationId)
        {
            return widget;
        }
    }
    return nullptr;
}

void WidgetCitiesList::onCityItemAccented()
{
    IItemWidget * itemWidget = static_cast<IItemWidget*>(sender());

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

void WidgetCitiesList::onCityItemClicked()
{
    auto cityWidget = static_cast<ItemWidgetCity*>(sender());
    emit locationIdSelected(cityWidget->getId());
}

void WidgetCitiesList::recalcItemPositions()
{
    // qDebug() << "City List recalc";

    int height = 0;

    foreach (ItemWidgetCity *city, itemWidgets_)
    {
        city->setGeometry(0, height, WINDOW_WIDTH * G_SCALE, LOCATION_ITEM_HEIGHT * G_SCALE);
        height += city->geometry().height();
    }

    if (height != height_)
    {
        height_ = height;
        emit heightChanged(height);
    }
    update();
}

void WidgetCitiesList::updateCursorWithSelectableWidget(IItemWidget *widget)
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

} // namespace
