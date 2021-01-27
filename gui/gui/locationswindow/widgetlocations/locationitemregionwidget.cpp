#include "locationitemregionwidget.h"

#include <QPainter>
#include "dpiscalemanager.h"
#include "commongraphics/commongraphics.h"
#include "utils/logger.h"

// #include <QDebug>

namespace GuiLocations {

LocationItemRegionWidget::LocationItemRegionWidget(IWidgetLocationsInfo * widgetLocationsInfo, LocationModelItem *locationModelItem, QWidget *parent) : QWidget(parent)
  , widgetLocationsInfo_(widgetLocationsInfo)
  , citySubMenuState_(COLLAPSED)
{
    setFocusPolicy(Qt::NoFocus);

    height_ = LOCATION_ITEM_HEIGHT * G_SCALE;

    regionHeaderWidget_ = new LocationItemRegionHeaderWidget(widgetLocationsInfo, locationModelItem, this);
    connect(regionHeaderWidget_, SIGNAL(clicked()), SLOT(onRegionHeaderClicked()));
    connect(regionHeaderWidget_, SIGNAL(selected()), SLOT(onRegionHeaderSelected()));

    // qDebug() << "Creating region: " << regionHeaderWidget_->name();

    expandingHeightAnimation_.setDirection(QAbstractAnimation::Forward);
    expandingHeightAnimation_.setDuration(200);
    connect(&expandingHeightAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onExpandingHeightAnimationValueChanged(QVariant)));

    recalcItemPositions();
}

LocationItemRegionWidget::~LocationItemRegionWidget()
{
    // qDebug() << "Deleting region: " << regionHeaderWidget_->name();

    regionHeaderWidget_->disconnect();
    regionHeaderWidget_->deleteLater();

    foreach (LocationItemCityWidget *city, cities_)
    {
        city->disconnect();
        city->deleteLater();
    }
    cities_.clear();
}

const LocationID LocationItemRegionWidget::getId() const
{
    return regionHeaderWidget_->getId();
}

bool LocationItemRegionWidget::expandable() const
{
    return cities_.count() > 0;
}

bool LocationItemRegionWidget::expandedOrExpanding()
{
    return citySubMenuState_ == EXPANDED || citySubMenuState_ == EXPANDING;
}

void LocationItemRegionWidget::setExpandedWithoutAnimation(bool expand)
{
    if (expand)
    {
        citySubMenuState_ = EXPANDED;
    }
    else
    {
        citySubMenuState_ = COLLAPSED;
    }

    regionHeaderWidget_->setExpandedWithoutAnimation(expand);
    foreach (LocationItemCityWidget *city, cities_)
    {
        city->setSelectable(expand);
    }
    recalcItemPositions();
}

void LocationItemRegionWidget::expand()
{
    qCDebug(LOG_BASIC) << "Expanding: " << regionHeaderWidget_->name();
    foreach (LocationItemCityWidget * city, cities_)
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
    qCDebug(LOG_BASIC) << "Collapsing: " << regionHeaderWidget_->name();

    foreach (LocationItemCityWidget *city, cities_)
    {
        city->setSelectable(false);
    }

    regionHeaderWidget_->setExpanded(false);
    citySubMenuState_ = COLLAPSING;
    expandingHeightAnimation_.stop();
    expandingHeightAnimation_.setStartValue(height_);
    expandingHeightAnimation_.setEndValue(static_cast<int>(LOCATION_ITEM_HEIGHT*G_SCALE));
    expandingHeightAnimation_.start();
}

void LocationItemRegionWidget::addCity(CityModelItem city)
{
    auto cityWidget = new LocationItemCityWidget(widgetLocationsInfo_, city, this);
    connect(cityWidget, SIGNAL(clicked()), SLOT(onCityItemClicked()));
    connect(cityWidget, SIGNAL(selected()), SLOT(onCityItemSelected()));
    connect(cityWidget, SIGNAL(favoriteClicked(LocationItemCityWidget *, bool)), SIGNAL(favoriteClicked(LocationItemCityWidget*, bool)));
    cities_.append(cityWidget);
    cityWidget->show();
    recalcItemPositions();
}

QVector<SelectableLocationItemWidget*> LocationItemRegionWidget::selectableWidgets()
{
    QVector<SelectableLocationItemWidget *> widgets;
    widgets.append(regionHeaderWidget_);
    if (expandedOrExpanding())
    {
        foreach (LocationItemCityWidget *city, cities_)
        {
            widgets.append(city);
        }
    }
    return widgets;
}

QVector<LocationItemCityWidget *> LocationItemRegionWidget::cityWidgets()
{
    return cities_;
}

void LocationItemRegionWidget::setFavorited(LocationID id, bool isFavorite)
{
    foreach (LocationItemCityWidget *city, cities_)
    {
        if (city->getId() == id)
        {
            city->setFavourited(isFavorite);
            break;
        }
    }
}

void LocationItemRegionWidget::recalcItemPositions()
{
    // qDebug() << "Region recalc item positions";
    regionHeaderWidget_->setGeometry(0,0, WINDOW_WIDTH * G_SCALE, LOCATION_ITEM_HEIGHT * G_SCALE);

    int height = LOCATION_ITEM_HEIGHT * G_SCALE;

    foreach (LocationItemCityWidget *city, cities_)
    {
        city->setGeometry(0, height, WINDOW_WIDTH * G_SCALE, LOCATION_ITEM_HEIGHT * G_SCALE);
        height += city->geometry().height();
    }
    recalcHeight();
    update();
}

void LocationItemRegionWidget::recalcHeight()
{
    if  (citySubMenuState_ == EXPANDED)
    {
        height_ = expandedHeight();
        emit heightChanged(height_);
    }
    else if (citySubMenuState_ == COLLAPSED)
    {
        height_ = LOCATION_ITEM_HEIGHT*G_SCALE;
        emit heightChanged(height_);
    }
}

void LocationItemRegionWidget::updateScaling()
{
    recalcItemPositions();
}

void LocationItemRegionWidget::onRegionHeaderSelected()
{
    emit selected(regionHeaderWidget_);
}

void LocationItemRegionWidget::onRegionHeaderClicked()
{
    emit clicked(this);
}

void LocationItemRegionWidget::onCityItemClicked()
{
    LocationItemCityWidget *cityWidget = static_cast<LocationItemCityWidget*>(sender());

    if (cityWidget->isForbidden() || cityWidget->isDisabled())
    {
        return;
    }

    emit clicked(cityWidget);
}

void LocationItemRegionWidget::onExpandingHeightAnimationValueChanged(const QVariant &value)
{
    int height = value.toInt();

    // qDebug() << "height animation: " << height;
    if (height == static_cast<int>(LOCATION_ITEM_HEIGHT*G_SCALE))
    {
        citySubMenuState_ = COLLAPSED;
    }
    else if (height == expandedHeight())
    {
        citySubMenuState_ = EXPANDED;
    }

    height_ = height;
    emit heightChanged(height);
}

int LocationItemRegionWidget::expandedHeight()
{
    int height = LOCATION_ITEM_HEIGHT * G_SCALE;
    foreach (auto city, cities_)
    {
        height += city->geometry().height();
    }

    return height;
}

void LocationItemRegionWidget::onCityItemSelected()
{
    emit selected(static_cast<SelectableLocationItemWidget*>(sender()));
}


} // namespace

