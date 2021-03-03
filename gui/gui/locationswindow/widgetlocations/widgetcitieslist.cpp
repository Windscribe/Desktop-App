#include "widgetcitieslist.h"

#include <QtMath>
#include "dpiscalemanager.h"
#include "commongraphics/commongraphics.h"
#include "utils/logger.h"


namespace GuiLocations {

WidgetCitiesList::WidgetCitiesList(IWidgetLocationsInfo *widgetLocationsInfo, QWidget *parent) : QWidget(parent)
  , height_(0)
  , lastAccentedItemWidget_(nullptr)
  , widgetLocationsInfo_(widgetLocationsInfo)
  , muteAccentChanges_(false)
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
    recentlyAccentedWidgets_.clear();
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
    connect(cityWidget, SIGNAL(accented()), SLOT(onCityItemAccented()));
    connect(cityWidget, SIGNAL(hoverEnter()), SLOT(onCityItemHoverEnter()));
    connect(cityWidget, SIGNAL(favoriteClicked(ItemWidgetCity *, bool)), SIGNAL(favoriteClicked(ItemWidgetCity*, bool)));
    cityWidget->setSelectable(true);
    cityWidget->show();
    itemWidgets_.push_back(cityWidget);

    // qDebug() << "Added: " << cityWidget->name();

    recalcItemPositions();
}

void WidgetCitiesList::updateScaling()
{
    foreach (ItemWidgetCity *city, itemWidgets_)
    {
        city->updateScaling();
    }

    recalcItemPositions();

}

void WidgetCitiesList::accentWidgetContainingCursor()
{
    foreach (IItemWidget *selectableWidget , itemWidgets_)
    {
        if (selectableWidget->containsCursor())
        {
            // qDebug() << "Selecting by containing cursor";
            selectableWidget->setAccented(true);
            updateCursorWithWidget(selectableWidget);
            break;
        }
    }
}

void WidgetCitiesList::selectWidgetContainingGlobalPt(const QPoint &pt)
{
    foreach (IItemWidget *selectableWidget , itemWidgets())
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
            widget->setAccented(true);
            break;
        }
    }
}

void WidgetCitiesList::accentFirstItem()
{
    if (itemWidgets_.count() > 0)
    {
        itemWidgets_[0]->setAccented(true);
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

void WidgetCitiesList::moveAccentDown()
{
    QVectorIterator<ItemWidgetCity *> it(itemWidgets_);
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
        widget->setAccented(true);
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

void WidgetCitiesList::setMuteAccentChanges(bool mute)
{
    muteAccentChanges_ = mute;
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
    // only iterate through cache of previously accented widgets
    for (IItemWidget *widget : recentlyAccentedWidgets_)
    {
        if (widget != itemWidget)
        {
            widget->setAccentedWithoutAnimation(false);
        }
    }
    recentlyAccentedWidgets_.clear();

    updateCursorWithWidget(itemWidget);
    recentlyAccentedWidgets_.append(itemWidget);
    lastAccentedItemWidget_ = itemWidget;
}

void WidgetCitiesList::onCityItemClicked()
{
    // block false-clicks that come after gesture scroll
    if (widgetLocationsInfo_->gestureScrollingElapsedTime() > 100)
    {
        auto cityWidget = static_cast<ItemWidgetCity*>(sender());
        safeEmitLocationIdSelected(cityWidget);
    }
}

void WidgetCitiesList::onCityItemHoverEnter()
{
    if (!muteAccentChanges_)
    {
        auto cityWidget = static_cast<ItemWidgetCity*>(sender());
        cityWidget->setAccented(true);
    }
}

void WidgetCitiesList::recalcItemPositions()
{
    // qDebug() << "City List recalc";

    int height = 0;

    foreach (ItemWidgetCity *city, itemWidgets_)
    {
        city->setGeometry(0, height, WINDOW_WIDTH * G_SCALE, qCeil(LOCATION_ITEM_HEIGHT * G_SCALE));
        height += city->geometry().height();
    }

    if (height != height_)
    {
        height_ = height;
        emit heightChanged(height);
    }
    update();
}

void WidgetCitiesList::updateCursorWithWidget(IItemWidget *widget)
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

void WidgetCitiesList::safeEmitLocationIdSelected(IItemWidget *widget)
{
    if (!widget->isDisabled() &&
        !widget->isForbidden() &&
        !widget->isBrokenConfig())
    {
        emit locationIdSelected(widget->getId());
    }
}

} // namespace
