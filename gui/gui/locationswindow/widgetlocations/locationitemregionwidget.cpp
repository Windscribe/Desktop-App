#include "locationitemregionwidget.h"

#include <QPainter>
#include "dpiscalemanager.h"
#include "commongraphics/commongraphics.h"

#include <QDebug>

namespace GuiLocations {

LocationItemRegionWidget::LocationItemRegionWidget(IWidgetLocationsInfo * widgetLocationsInfo, LocationModelItem *locationModelItem, QWidget *parent) : QWidget(parent)
  , citySubMenuState_(COLLAPSED)
  , widgetLocationsInfo_(widgetLocationsInfo)
{
    height_ = LOCATION_ITEM_HEIGHT * G_SCALE;

    regionHeaderWidget_ = QSharedPointer<LocationItemRegionHeaderWidget>(new LocationItemRegionHeaderWidget(widgetLocationsInfo, locationModelItem, this));
    connect(regionHeaderWidget_.get(), SIGNAL(clicked()), SLOT(onRegionItemClicked()));
    connect(regionHeaderWidget_.get(), SIGNAL(selected(SelectableLocationItemWidget *)), SLOT(onRegionItemSelected(SelectableLocationItemWidget *)));

    expandingHeightAnimation_.setDirection(QAbstractAnimation::Forward);
    expandingHeightAnimation_.setDuration(200);
    connect(&expandingHeightAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onExpandingHeightAnimationValueChanged(QVariant)));

    recalcItemPos();
}

LocationItemRegionWidget::~LocationItemRegionWidget()
{
    // qDebug() << "Deleting region widget: " << textLabel_->text();
}

const LocationID LocationItemRegionWidget::getId() const
{
    return regionHeaderWidget_->getId();
}

bool LocationItemRegionWidget::expandable() const
{
    return cities_.count() > 0;
}

void LocationItemRegionWidget::setShowLatencyMs(bool showLatencyMs)
{
    foreach (auto city, cities_)
    {
        city->setShowLatencyMs(showLatencyMs);
    }
}

bool LocationItemRegionWidget::expandedOrExpanding()
{
    return citySubMenuState_ == EXPANDED || citySubMenuState_ == EXPANDING;
}

void LocationItemRegionWidget::expand()
{
    qDebug() << "Expanding: " << regionHeaderWidget_->name();
    foreach (auto city, cities_)
    {
        city->setSelectable(true);
    }

    regionHeaderWidget_->setExpanded(true);
    citySubMenuState_ = EXPANDING;
    expandingHeightAnimation_.stop();
    expandingHeightAnimation_.setStartValue(height_);
    expandingHeightAnimation_.setEndValue(expandedHeight());
    expandingHeightAnimation_.start();
}

void LocationItemRegionWidget::collapse()
{
    qDebug() << "Collapsing: " << regionHeaderWidget_->name();

    foreach (auto city, cities_)
    {
        city->setSelectable(false);
    }

    regionHeaderWidget_->setExpanded(false);
    citySubMenuState_ = COLLAPSING;
    expandingHeightAnimation_.stop();
    expandingHeightAnimation_.setStartValue(height_);
    expandingHeightAnimation_.setEndValue((int)(LOCATION_ITEM_HEIGHT*G_SCALE));
    expandingHeightAnimation_.start();
}

void LocationItemRegionWidget::addCity(CityModelItem city)
{
    auto cityWidget = QSharedPointer<LocationItemCityWidget>(new LocationItemCityWidget(widgetLocationsInfo_, city, this));
    connect(cityWidget.get(), SIGNAL(clicked(LocationItemCityWidget *)), SLOT(onCityItemClicked(LocationItemCityWidget *)));
    connect(cityWidget.get(), SIGNAL(selected(SelectableLocationItemWidget *)), SLOT(onCityItemSelected(SelectableLocationItemWidget *)));
    cities_.append(cityWidget);
    recalcItemPos();
}

QVector<QSharedPointer<SelectableLocationItemWidget>> LocationItemRegionWidget::selectableWidgets()
{
    QVector<QSharedPointer<SelectableLocationItemWidget>> widgets;
    widgets.append(regionHeaderWidget_);
    if (expandedOrExpanding())
    {
        foreach (auto city, cities_)
        {
            widgets.append(city);
        }
    }
    return widgets;
}

QVector<QSharedPointer<LocationItemCityWidget> > LocationItemRegionWidget::selectableCityWidgets()
{
    QVector<QSharedPointer<LocationItemCityWidget>> widgets;
    if (expandedOrExpanding())
    {
        foreach (auto city, cities_)
        {
            widgets.append(city);
        }
    }
    return widgets;
}

void LocationItemRegionWidget::recalcItemPos()
{
    // qDebug() << "Recalc region height";

    regionHeaderWidget_->setGeometry(0,0, WINDOW_WIDTH * G_SCALE, LOCATION_ITEM_HEIGHT);

    int height = LOCATION_ITEM_HEIGHT * G_SCALE;

    foreach (auto city, cities_)
    {
        city->setGeometry(0, height, WINDOW_WIDTH * G_SCALE, LOCATION_ITEM_HEIGHT * G_SCALE);
        height += city->geometry().height();
    }

    // only update the height if cities are visible
    if (citySubMenuState_ == EXPANDED)
    {
        if (height != height_)
        {
            height_ = height;
            emit heightChanged(height);
        }
    }

    update();
}

void LocationItemRegionWidget::onRegionItemSelected(SelectableLocationItemWidget *regionWidget)
{
    emit selected(regionWidget);
}

void LocationItemRegionWidget::onRegionItemClicked()
{
    emit clicked(this);
}

void LocationItemRegionWidget::onCityItemClicked(LocationItemCityWidget *cityWidget)
{
    emit clicked(cityWidget);
}

void LocationItemRegionWidget::onExpandingHeightAnimationValueChanged(const QVariant &value)
{
    int height = value.toInt();
    // qDebug() << "new height: " << height;

    if (height == LOCATION_ITEM_HEIGHT*G_SCALE)
    {
        citySubMenuState_ = COLLAPSED;
        qDebug() << "Collapsed";
    }
    else if (height == expandedHeight())
    {
        citySubMenuState_ = EXPANDED;
        qDebug() << "Expanded";
    }
    height_ = height;
    emit heightChanged(height);
}

int LocationItemRegionWidget::expandedHeight()
{
    int height = LOCATION_ITEM_HEIGHT * G_SCALE;
    foreach (auto city, cities_)
    {
        // city->setGeometry(0, height, WINDOW_WIDTH * G_SCALE, LOCATION_ITEM_HEIGHT * G_SCALE);
        height += city->geometry().height();
    }

    return height;
}

void LocationItemRegionWidget::onCityItemSelected(SelectableLocationItemWidget *cityWidget)
{
    emit selected(cityWidget);
}


} // namespace

