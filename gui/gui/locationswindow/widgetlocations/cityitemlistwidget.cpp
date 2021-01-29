#include "cityitemlistwidget.h"

#include "dpiscalemanager.h"
#include "commongraphics/commongraphics.h"
#include "utils/logger.h"


namespace GuiLocations {

CityItemListWidget::CityItemListWidget(IWidgetLocationsInfo *widgetLocationsInfo, QWidget *parent) : QWidget(parent)
  , height_(0)
  , lastAccentedItemWidget_(nullptr)
  , widgetLocationsInfo_(widgetLocationsInfo)
{
    setFocusPolicy(Qt::NoFocus);
    cursorUpdateHelper_.reset(new CursorUpdateHelper(this));

    recalcItemPositions();
}

CityItemListWidget::~CityItemListWidget()
{
    clearWidgets();

}

void CityItemListWidget::clearWidgets()
{
    lastAccentedItemWidget_ = nullptr;
    recentlySelectedWidgets_.clear();
    foreach (LocationItemCityWidget *regionWidget, itemWidgets_)
    {
        regionWidget->disconnect();
        regionWidget->deleteLater();
    }
    itemWidgets_.clear();
}

// TODO: can speed things up with by removing copies?
void CityItemListWidget::addCity(CityModelItem city)
{
    auto cityWidget = new LocationItemCityWidget(widgetLocationsInfo_, city, this);
    connect(cityWidget, SIGNAL(clicked()), SLOT(onCityItemClicked()));
    connect(cityWidget, SIGNAL(selected()), SLOT(onCityItemAccented()));
    connect(cityWidget, SIGNAL(favoriteClicked(LocationItemCityWidget *, bool)), SIGNAL(favoriteClicked(LocationItemCityWidget*, bool)));
    cityWidget->setSelectable(true);
    cityWidget->show();
    itemWidgets_.push_back(cityWidget);

    qDebug() << "Added: " << cityWidget->name();

    recalcItemPositions();
}

void CityItemListWidget::updateScaling()
{
    recalcItemPositions();

}

void CityItemListWidget::selectWidgetContainingCursor()
{
    foreach (SelectableLocationItemWidget *selectableWidget , itemWidgets_)
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

QVector<LocationItemCityWidget *> CityItemListWidget::itemWidgets()
{
    return itemWidgets_;
}

int CityItemListWidget::selectableIndex(LocationID locationId)
{
    int i = 0;
    foreach (SelectableLocationItemWidget *widget, itemWidgets_)
    {
        if (widget->getId() == locationId)
        {
            return i;
        }
        i++;
    }
    return -1;
}

const LocationID CityItemListWidget::lastAccentedLocationId() const
{
    if (!lastAccentedItemWidget_)
    {
        return LocationID();
    }
    return lastAccentedItemWidget_->getId();
}

void CityItemListWidget::accentItem(LocationID locationId)
{
    foreach (SelectableLocationItemWidget *widget, itemWidgets_)
    {
        if (widget->getId() == locationId)
        {
            widget->setSelected(true);
            break;
        }
    }
}

void CityItemListWidget::accentFirstItem()
{
    if (itemWidgets_.count() > 0)
    {
        itemWidgets_[0]->setSelected(true);
    }
}

bool CityItemListWidget::hasAccentItem()
{
    return lastAccentedItemWidget_;
}

void CityItemListWidget::moveAccentUp()
{
    SelectableLocationItemWidget *lastWidget = nullptr;
    SelectableLocationItemWidget *currentWidget =  nullptr;
    foreach (SelectableLocationItemWidget *widget, itemWidgets_)
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

void CityItemListWidget::moveAccentDown()
{
    QVectorIterator<LocationItemCityWidget *> it(itemWidgets_);
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

int CityItemListWidget::accentItemIndex()
{
    if (!lastAccentedItemWidget_)
    {
        qDebug() << "LocationItemListWidget::accentItemSelectableIndex - no accent item";
        return -1;
    }
    return selectableIndex(lastAccentedItemWidget_->getId());
}

SelectableLocationItemWidget *CityItemListWidget::lastAccentedItemWidget()
{
    return lastAccentedItemWidget_;
}

SelectableLocationItemWidget *CityItemListWidget::selectableWidget(LocationID locationId)
{
    foreach (SelectableLocationItemWidget *widget, itemWidgets_)
    {
        if (widget->getId() == locationId)
        {
            return widget;
        }
    }
    return nullptr;
}

void CityItemListWidget::onCityItemAccented()
{
    SelectableLocationItemWidget * itemWidget = static_cast<SelectableLocationItemWidget*>(sender());

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

void CityItemListWidget::onCityItemClicked()
{
    auto cityWidget = static_cast<LocationItemCityWidget*>(sender());
    emit locationIdSelected(cityWidget->getId());
}

void CityItemListWidget::recalcItemPositions()
{
    qDebug() << "City List recalc";

    int height = 0;

    foreach (LocationItemCityWidget *city, itemWidgets_)
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

void CityItemListWidget::updateCursorWithSelectableWidget(SelectableLocationItemWidget *widget)
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
