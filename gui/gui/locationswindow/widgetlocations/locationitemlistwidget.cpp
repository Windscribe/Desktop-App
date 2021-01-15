#include "locationitemlistwidget.h"

#include <QPainter>
#include <QDebug>
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"


namespace GuiLocations {

LocationItemListWidget::LocationItemListWidget(QWidget *parent) : QWidget(parent)
  , height_(0)
{
    // qDebug() << "Constructed Location Item List Widget";
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
    foreach (auto regionWidget, itemWidgets_)
    {
        disconnect(regionWidget.get());
    }
    itemWidgets_.clear();
}

void LocationItemListWidget::addRegionWidget(LocationModelItem *item)
{
    auto regionWidget = QSharedPointer<LocationItemRegionWidget>(new LocationItemRegionWidget(item, this));
    connect(regionWidget.get(), SIGNAL(heightChanged(int)), SLOT(onRegionWidgetHeightChanged(int)));
    connect(regionWidget.get(), SIGNAL(clicked(LocationItemCityWidget *)), SLOT(onLocationItemCityClicked(LocationItemCityWidget *)));
    connect(regionWidget.get(), SIGNAL(clicked(LocationItemRegionWidget *)), SLOT(onLocationItemRegionClicked(LocationItemRegionWidget *)));
    connect(regionWidget.get(), SIGNAL(selected(SelectableLocationItemWidget *)), SLOT(onSelectableLocationItemSelected(SelectableLocationItemWidget *)));
    itemWidgets_.append(regionWidget);
    regionWidget->setGeometry(0, 0, WINDOW_WIDTH *G_SCALE, LocationItemRegionHeaderWidget::REGION_HEADER_HEIGHT * G_SCALE);
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
    // qDebug() << "checking for new selection";
    foreach (auto widget , selectableWidgets())
    {
        if (widget->containsCursor())
        {
            // qDebug() << "Selecting: " << widget->name();
            widget->setSelected(true);
            break;
        }
    }
}

void LocationItemListWidget::expandLocationIds(QVector<LocationID> locIds)
{
    foreach (auto regionWidget, itemWidgets_)
    {
        foreach (auto locId, locIds)
        {
            if (regionWidget->getId() == locId)
            {
                regionWidget->setExpanded(true);
            }
        }
    }
}

QVector<LocationID> LocationItemListWidget::expandedLocationIds()
{
    QVector<LocationID> expanded;
    foreach (auto regionWidget, itemWidgets_)
    {
        if (regionWidget->expanded())
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
    foreach (auto widget, selectableWidgets())
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
    foreach (auto widget, selectableWidgets())
    {
        if (widget->getId() == locationId)
        {
            qDebug() << "Selecting: "<< widget->name();
            widget->setSelected(true);
            break;
        }
    }
}

const QVector<QSharedPointer<SelectableLocationItemWidget> > &LocationItemListWidget::visibleItemWidgets()
{
    QVector<QSharedPointer<SelectableLocationItemWidget>> visible;
    foreach (auto widget, selectableWidgets())
    {
        //qDebug() << "List geometry: " << geometry() << " - widget geometry: " << widget->geometry();
        if (geometry().contains(QPoint(widget->geometry().x(), widget->geometry().y())))
        {
            //qDebug() << " -> contained " << widget->name();
            visible.append(widget);
        }
    }
    return visible;
}


QVector<QSharedPointer<SelectableLocationItemWidget>> LocationItemListWidget::selectableWidgets()
{
    QVector<QSharedPointer<SelectableLocationItemWidget>> selectableItemWidgets;
    foreach (auto regionWidget, itemWidgets_)
    {
        selectableItemWidgets.append(regionWidget->selectableWidgets());
    }
    return selectableItemWidgets;
}


void LocationItemListWidget::paintEvent(QPaintEvent *event)
{
    // qDebug() << "Painting LocationItemListWidget: " << geometry();
    Q_UNUSED(event);
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
    // probably emit click
}

void LocationItemListWidget::onSelectableLocationItemSelected(SelectableLocationItemWidget *itemWidget)
{
    foreach (auto widget, selectableWidgets())
    {
        if (widget.get() != itemWidget)
        {
            widget->setSelected(false);
        }
    }

    lastSelectedLocationId_ = itemWidget->getId();
}

void LocationItemListWidget::onLocationItemRegionClicked(LocationItemRegionWidget *regionWidget)
{
    regionWidget->setExpanded(!regionWidget->expanded());
}

void LocationItemListWidget::recalcItemPositions()
{
    // qDebug() << "Recalc list height";
    int heightSoFar = 0;
    foreach (auto itemWidget, itemWidgets_)
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
