#include "locationitemregionwidget.h"

#include <QPainter>
#include "dpiscalemanager.h"
#include "commongraphics/commongraphics.h"
#include "widgetlocationssizes.h"

namespace GuiLocations {

LocationItemRegionWidget::LocationItemRegionWidget(LocationItem *locationItem, QWidget *parent) : QWidget(parent)
  , expanded_(false)
  , locationItem_(locationItem)
{
    height_ = REGION_HEIGHT;
    textLabel_.setText(locationItem->getName());
}

LocationItemRegionWidget::LocationItemRegionWidget(QWidget *parent) : QWidget(parent)
  , expanded_(false)
{
    height_ = REGION_HEIGHT;

}

void LocationItemRegionWidget::setExpanded(bool expand)
{
    // TODO: add animation
    if (expand)
    {
        foreach (auto city, cities_)
        {
            city->show();
        }
    }
    else
    {
        foreach (auto city, cities_)
        {
            city->hide();
        }
    }
    expanded_ = expand;
    recalcItemPos();

}

void LocationItemRegionWidget::setShowLatencyMs(bool showLatencyMs)
{
    foreach (auto city, cities_)
    {
        city->setShowLatencyMs(showLatencyMs);
    }
}

void LocationItemRegionWidget::setRegion(LocationItem *locationItem)
{
    locationItem_ = locationItem;
    textLabel_.setText(locationItem->getName());
}

void LocationItemRegionWidget::setCities(QList<QSharedPointer<CityNode>> cities)
{
    // cities_ =  cities;
    foreach (auto city, cities)
    {
        auto cityWidget = new LocationItemCityWidget(city, this);
        cities_.append(QSharedPointer<LocationItemCityWidget>(cityWidget));
    }
    recalcItemPos();
}

void LocationItemRegionWidget::updateScaling()
{
    textLabel_.move(10 * G_SCALE, 10 * G_SCALE);
    foreach (auto city, cities_)
    {
        city->updateScaling();
    }
    update();
}

void LocationItemRegionWidget::recalcItemPos()
{
    int height = REGION_HEIGHT;

    if (expanded_)
    {
        foreach (auto city, cities_)
        {
            city->move(0, height);
            height += city->geometry().height();
        }
    }

    if (height != height_)
    {
        height_ = height;
        emit heightChanged(height);
    }
    update();
}

void LocationItemRegionWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(QRect(0, 0, WINDOW_WIDTH * G_SCALE, height_ * G_SCALE),
                      WidgetLocationsSizes::instance().getBackgroundColor());
}

} // namespace
