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
    cityLabel_ = QSharedPointer<QLabel>(new QLabel(this));
    cityLabel_->setFont(*FontManager::instance().getFont(13, true));
    cityLabel_->setStyleSheet("QLabel { color : white; }");
    cityLabel_->setText(cityModelItem.city);

    nickLabel_ = QSharedPointer<QLabel>(new QLabel(this));
    nickLabel_->setFont(*FontManager::instance().getFont(13, false));
    nickLabel_->setStyleSheet("QLabel { color : white; }");
    nickLabel_->setText(cityModelItem.nick);

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
    cityLabel_->setFont(*FontManager::instance().getFont(16, true));
    cityLabel_->move(10 * G_SCALE, 10 * G_SCALE);

    nickLabel_->setFont(*FontManager::instance().getFont(13, false));
    nickLabel_->move(150 * G_SCALE, 10 * G_SCALE);

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
