#include "locationitemcitywidget.h"

#include <QPainter>
#include "dpiscalemanager.h"
#include "commongraphics/commongraphics.h"
#include "widgetlocationssizes.h"

namespace GuiLocations {

LocationItemCityWidget::LocationItemCityWidget(QSharedPointer<CityNode> cityNode, QWidget *parent) : QWidget(parent)
  , cityNode_(cityNode)
{
    textLabel_.setText(cityNode->caption1FullText());
    updateScaling();
}

void LocationItemCityWidget::setCity(QSharedPointer<CityNode> city)
{
    cityNode_ = city;
    textLabel_.setText(city->caption1FullText());
    updateScaling();
}

void LocationItemCityWidget::setShowLatencyMs(bool showLatencyMs)
{

}

void LocationItemCityWidget::updateScaling()
{
    textLabel_.move(10 * G_SCALE, 10 * G_SCALE);
    update();
}

void LocationItemCityWidget::paintEvent(QPaintEvent *event)
{
    // background
    QPainter painter(this);
    painter.fillRect(QRect(0, 0, WINDOW_WIDTH * G_SCALE, HEIGHT * G_SCALE),
                      WidgetLocationsSizes::instance().getBackgroundColor());

}

}
