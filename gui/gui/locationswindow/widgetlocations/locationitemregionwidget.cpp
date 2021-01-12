#include "locationitemregionwidget.h"

#include <QPainter>
#include "dpiscalemanager.h"
#include "commongraphics/commongraphics.h"
#include "widgetlocationssizes.h"
#include "graphicresources/fontmanager.h"

#include <QDebug>

namespace GuiLocations {

LocationItemRegionWidget::LocationItemRegionWidget(LocationModelItem *locationModelItem, QWidget *parent) : QAbstractButton(parent)
  , expanded_(false)
  , locationID_(locationModelItem->id)
{
    height_ = REGION_HEIGHT;
    textLabel_ = QSharedPointer<QLabel>(new QLabel(this));
    textLabel_->setFont(*FontManager::instance().getFont(16, true));
    textLabel_->setStyleSheet("QLabel { color : white; }");
    textLabel_->setText(locationModelItem->title);
    textLabel_->show();
    updateScaling();
}

LocationItemRegionWidget::~LocationItemRegionWidget()
{
    qDebug() << "Deleting region widget: " << textLabel_->text();

}

LocationID LocationItemRegionWidget::getId()
{
    return locationID_;
}

const QString LocationItemRegionWidget::name() const
{
    return textLabel_->text();
}

bool LocationItemRegionWidget::expandable() const
{
    return cities_.count() > 0;
}

bool LocationItemRegionWidget::expanded() const
{
    return expanded_;
}

void LocationItemRegionWidget::setExpanded(bool expand)
{
    if (!expandable())
    {
        qDebug() << "Cannot expand/collapse region without city widgets";
        return;
    }

    // TODO: add animation
    if (expand != expanded_)
    {
        if (expand)
        {
            qDebug() << "Expanding: " << textLabel_->text();
            foreach (auto city, cities_)
            {
                city->show();
            }
        }
        else
        {
            qDebug() << "Collapsing: " << textLabel_->text();
            foreach (auto city, cities_)
            {
                city->hide();
            }
        }
        expanded_ = expand;
        recalcItemPos();
    }
}

void LocationItemRegionWidget::setShowLatencyMs(bool showLatencyMs)
{
    foreach (auto city, cities_)
    {
        city->setShowLatencyMs(showLatencyMs);
    }
}

void LocationItemRegionWidget::addCity(CityModelItem city)
{
    auto cityWidget = QSharedPointer<LocationItemCityWidget>(new LocationItemCityWidget(city, this));
    cities_.append(cityWidget);
    recalcItemPos();
}

void LocationItemRegionWidget::updateScaling()
{
    textLabel_->setFont(*FontManager::instance().getFont(16, true));
    textLabel_->move(10 * G_SCALE, 10 * G_SCALE);

    foreach (auto city, cities_)
    {
        city->updateScaling();
    }
    recalcItemPos();
}

void LocationItemRegionWidget::recalcItemPos()
{
    int height = REGION_HEIGHT * G_SCALE;

    if (expanded_)
    {
        foreach (auto city, cities_)
        {
            city->setGeometry(0, height, WINDOW_WIDTH * G_SCALE, LocationItemCityWidget::HEIGHT * G_SCALE);
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
    // qDebug() << "Region painting";
    QPainter painter(this);
    painter.fillRect(QRect(0, 0, WINDOW_WIDTH * G_SCALE, height_ * G_SCALE),
                     WidgetLocationsSizes::instance().getBackgroundColor());
}

} // namespace
