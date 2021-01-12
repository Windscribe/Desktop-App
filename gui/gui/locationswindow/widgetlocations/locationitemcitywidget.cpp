#include "locationitemcitywidget.h"

#include <QPainter>
#include "dpiscalemanager.h"
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "widgetlocationssizes.h"


#include <QDebug>

namespace GuiLocations {

LocationItemCityWidget::LocationItemCityWidget(CityModelItem cityModelItem, QWidget *parent) : QAbstractButton(parent)
  , cityModelItem_(cityModelItem)
{
    textLabel_ = QSharedPointer<QLabel>(new QLabel(this));
    textLabel_->setFont(*FontManager::instance().getFont(16, true));
    textLabel_->setStyleSheet("QLabel { color : white; }");
    textLabel_->setText(cityModelItem.city);
    updateScaling();
}

LocationItemCityWidget::~LocationItemCityWidget()
{
    // qDebug() << "Deleting city widget: " << cityModelItem_.city << " " << cityModelItem_.nick;
}

void LocationItemCityWidget::setShowLatencyMs(bool showLatencyMs)
{

}

void LocationItemCityWidget::updateScaling()
{
    textLabel_->setFont(*FontManager::instance().getFont(16, true));
    textLabel_->move(10 * G_SCALE, 10 * G_SCALE);
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
