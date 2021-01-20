#include "locationitemlistwidget.h"

#include <QPainter>
#include <QDebug>
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "cursorupdatehelper.h"


namespace GuiLocations {

LocationItemListWidget::LocationItemListWidget(IWidgetLocationsInfo * widgetLocationsInfo, QWidget *parent) : QWidget(parent)
  , height_(0)
  , widgetLocationsInfo_(widgetLocationsInfo)
{
    // qDebug() << "Constructed Location Item List Widget";
    cursorUpdateHelper_.reset(new CursorUpdateHelper(this));

}

LocationItemListWidget::~LocationItemListWidget()
{

}

int LocationItemListWidget::countRegions() const
{
    return itemWidgets_.count();
}

void LocationItemListWidget::clearWidgets()
{
    recentlySelectedWidgets_.clear();
    foreach (QSharedPointer<LocationItemRegionWidget> regionWidget, itemWidgets_)
    {
        disconnect(regionWidget.get());
    }
    itemWidgets_.clear();
}

void LocationItemListWidget::addRegionWidget(LocationModelItem *item)
{
    auto regionWidget = QSharedPointer<LocationItemRegionWidget>(new LocationItemRegionWidget(widgetLocationsInfo_, item, this));
    connect(regionWidget.get(), SIGNAL(heightChanged(int)), SLOT(onRegionWidgetHeightChanged(int)));
    connect(regionWidget.get(), SIGNAL(clicked(LocationItemCityWidget *)), SLOT(onLocationItemCityClicked(LocationItemCityWidget *)));
    connect(regionWidget.get(), SIGNAL(clicked(LocationItemRegionWidget *)), SLOT(onLocationItemRegionClicked(LocationItemRegionWidget *)));
    connect(regionWidget.get(), SIGNAL(selected(SelectableLocationItemWidget *)), SLOT(onSelectableLocationItemSelected(SelectableLocationItemWidget *)));
    connect(regionWidget.get(), SIGNAL(favoriteClicked(LocationItemCityWidget*, bool)), SIGNAL(favoriteClicked(LocationItemCityWidget*,bool)));
    itemWidgets_.append(regionWidget);
    regionWidget->setGeometry(0, 0, WINDOW_WIDTH *G_SCALE, LOCATION_ITEM_HEIGHT * G_SCALE);
    regionWidget->show();
    // qDebug() << "Added region: " << item->title;
    recalcItemPositions();
}

void LocationItemListWidget::addCityToRegion(CityModelItem city, LocationModelItem *region)
{
    auto matchingId = [&](const QSharedPointer<LocationItemRegionWidget> regionWidget){
        return regionWidget->getId() == region->id;
    };

    if (!(std::find_if(itemWidgets_.begin(), itemWidgets_.end(), matchingId) != itemWidgets_.end()))
    {
        addRegionWidget(region);
        // qDebug() << "Added region (JIT): " << region->title;
    }

    itemWidgets_.last()->addCity(city);
    // qDebug() << "Added City: " << city.city << " " << city.nick;

    recalcItemPositions();
}

void LocationItemListWidget::updateScaling()
{
    recalcItemPositions();
}

void LocationItemListWidget::selectWidgetContainingCursor()
{
    //qDebug() << "checking for new selection";
    foreach (QSharedPointer<SelectableLocationItemWidget> selectableWidget , selectableWidgets())
    {
        if (selectableWidget->containsCursor())
        {
            //qDebug() << "Selecting: " << selectableWidget->name();
            selectableWidget->setSelected(true);
            updateCursorWithSelectableWidget(selectableWidget.get());

            break;
        }
    }
}

void LocationItemListWidget::expandLocationIds(QVector<LocationID> locIds)
{
    foreach (QSharedPointer<LocationItemRegionWidget> regionWidget, itemWidgets_)
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
    foreach (QSharedPointer<LocationItemRegionWidget> regionWidget, itemWidgets_)
    {
        if (regionWidget->expandedOrExpanding())
        {
            expanded.append(regionWidget->getId());
        }
    }
    return expanded;
}

const QVector<QSharedPointer<LocationItemRegionWidget>> &LocationItemListWidget::itemWidgets()
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
        qDebug() << "Err: There isn't enough items in selectable list to index (locationID)";
        return LocationID();
    }

    return widgets[index]->getId();
}

int LocationItemListWidget::selectableIndex(LocationID locationId)
{
    int i = 0;
    foreach (QSharedPointer<SelectableLocationItemWidget> widget, selectableWidgets())
    {
        if (widget->getId() == locationId)
        {
            return i;
        }
        i++;
    }
    return -1;
}

const LocationID LocationItemListWidget::lastSelectedLocationId() const
{
    return lastSelectedLocationId_;
}

void LocationItemListWidget::selectItem(LocationID locationId)
{
    foreach (QSharedPointer<SelectableLocationItemWidget> widget, selectableWidgets())
    {
        if (widget->getId() == locationId)
        {
            // qDebug() << "Selecting: "<< widget->name();
            widget->setSelected(true);
            break;
        }
    }
}

const QVector<QSharedPointer<LocationItemCityWidget> > LocationItemListWidget::selectableCityWidgets()
{
    QVector<QSharedPointer<LocationItemCityWidget>> selectableItemWidgets;
    foreach (QSharedPointer<LocationItemRegionWidget> regionWidget, itemWidgets_)
    {
        selectableItemWidgets.append(regionWidget->selectableCityWidgets());
    }
    return selectableItemWidgets;
}

QVector<QSharedPointer<LocationItemCityWidget> > LocationItemListWidget::cityWidgets()
{
    QVector<QSharedPointer<LocationItemCityWidget>> cityWidgets;
    foreach (QSharedPointer<LocationItemRegionWidget> regionWidget, itemWidgets_)
    {
        cityWidgets.append(regionWidget->cityWidgets());
    }
    return cityWidgets;
}


QVector<QSharedPointer<SelectableLocationItemWidget>> LocationItemListWidget::selectableWidgets()
{
    QVector<QSharedPointer<SelectableLocationItemWidget>> selectableItemWidgets;
    foreach (QSharedPointer<LocationItemRegionWidget> regionWidget, itemWidgets_)
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
    // qDebug() << "Painting LocationItemListWidget: " << geometry();
    Q_UNUSED(event)
    QPainter painter(this);
    QRect background(0,0, geometry().width(), geometry().height());
    painter.fillRect(background, Qt::gray);

}

void LocationItemListWidget::onRegionWidgetHeightChanged(int height)
{
    auto regionWidget = static_cast<LocationItemRegionWidget*>(sender());
    // qDebug() << "Region changed height: " << regionWidget->name();
    regionWidget->setGeometry(0, 0, WINDOW_WIDTH * G_SCALE, height * G_SCALE);
    recalcItemPositions();
}

void LocationItemListWidget::onLocationItemCityClicked(LocationItemCityWidget *cityWidget)
{
    // qDebug() << "List sees click: " << cityWidget->name();
    emit locationIdSelected(cityWidget->getId());
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
    lastSelectedLocationId_ = itemWidget->getId();
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
                emit regionExpanding(regionWidget);
            }
        }
    }
}

void LocationItemListWidget::recalcItemPositions()
{
    // qDebug() << "Recalc list height";
    int heightSoFar = 0;
    foreach (QSharedPointer<LocationItemRegionWidget> itemWidget, itemWidgets_)
    {
        itemWidget->setGeometry(0, heightSoFar, WINDOW_WIDTH * G_SCALE, itemWidget->geometry().height());
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
