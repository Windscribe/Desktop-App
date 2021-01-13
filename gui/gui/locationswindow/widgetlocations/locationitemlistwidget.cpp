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
    connect(regionWidget.get(), SIGNAL(clicked()), SLOT(onRegionWidgetClicked()));
    itemWidgets_.append(regionWidget);
    regionWidget->setGeometry(0, 0, WINDOW_WIDTH *G_SCALE, LocationItemRegionWidget::REGION_HEIGHT * G_SCALE);
    regionWidget->show();
    // qDebug() << "Added region: " << item->title;
    recalcItemPos();
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

    recalcItemPos();
}

void LocationItemListWidget::updateScaling()
{
    foreach (auto itemWidget, itemWidgets_)
    {
        itemWidget->updateScaling();
    }
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
    qDebug() << "Region changed height: " << regionWidget->name();
    regionWidget->setGeometry(0, 0, WINDOW_WIDTH * G_SCALE, height * G_SCALE);
    recalcItemPos();
}

void LocationItemListWidget::onRegionWidgetClicked()
{
    auto regionWidget = static_cast<LocationItemRegionWidget*>(sender());
    qDebug() << "Region clicked: " << regionWidget->name();
    regionWidget->setExpanded(!regionWidget->expanded());
}

void LocationItemListWidget::recalcItemPos()
{
    qDebug() << "Recalc list height";
    int height = 0;
    foreach (auto itemWidget, itemWidgets_)
    {
        itemWidget->move(0, height);
        height += itemWidget->geometry().height();
    }

    if (height != height_)
    {
        height_ = height;
        emit heightChanged(height);
    }
    update();
}

} // namespace
