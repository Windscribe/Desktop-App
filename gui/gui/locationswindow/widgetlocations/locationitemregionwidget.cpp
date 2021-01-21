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

    regionHeaderWidget_ = QSharedPointer<LocationItemRegionHeaderWidget>(new LocationItemRegionHeaderWidget(widgetLocationsInfo, locationModelItem, this));
    connect(regionHeaderWidget_.get(), SIGNAL(clicked()), SLOT(onRegionHeaderClicked()));
    connect(regionHeaderWidget_.get(), SIGNAL(selected(SelectableLocationItemWidget *)), SLOT(onRegionHeaderSelected(SelectableLocationItemWidget *)));

    expandingHeightAnimation_.setDirection(QAbstractAnimation::Forward);
    expandingHeightAnimation_.setDuration(200);
    connect(&expandingHeightAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onExpandingHeightAnimationValueChanged(QVariant)));

    recalcItemPos();
}

LocationItemRegionWidget::~LocationItemRegionWidget()
{

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
    foreach (auto city, cities_)
    {
        city->setSelectable(expand);
    }
    recalcItemPos();
}

void LocationItemRegionWidget::expand()
{
    qCDebug(LOG_BASIC) << "Expanding: " << regionHeaderWidget_->name();
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
    qCDebug(LOG_BASIC) << "Collapsing: " << regionHeaderWidget_->name();

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
    connect(cityWidget.get(), SIGNAL(clicked()), SLOT(onCityItemClicked()));
    connect(cityWidget.get(), SIGNAL(selected(SelectableLocationItemWidget *)), SLOT(onCityItemSelected(SelectableLocationItemWidget *)));
    connect(cityWidget.get(), SIGNAL(favoriteClicked(LocationItemCityWidget *, bool)), SIGNAL(favoriteClicked(LocationItemCityWidget*, bool)));
    cities_.append(cityWidget);
    cityWidget->show();
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

QVector<QSharedPointer<LocationItemCityWidget> > LocationItemRegionWidget::cityWidgets()
{
    return cities_;
}

void LocationItemRegionWidget::setFavorited(LocationID id, bool isFavorite)
{
    foreach (QSharedPointer<LocationItemCityWidget> city, cities_)
    {
        if (city->getId() == id)
        {
            city->setFavourited(isFavorite);
            break;
        }
    }
}

void LocationItemRegionWidget::recalcItemPos()
{

    regionHeaderWidget_->setGeometry(0,0, WINDOW_WIDTH * G_SCALE, LOCATION_ITEM_HEIGHT);

    int height = LOCATION_ITEM_HEIGHT * G_SCALE;

    foreach (auto city, cities_)
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

void LocationItemRegionWidget::onRegionHeaderSelected(SelectableLocationItemWidget *regionWidget)
{
    emit selected(regionWidget);
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

    if (height == LOCATION_ITEM_HEIGHT*G_SCALE)
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

void LocationItemRegionWidget::onCityItemSelected(SelectableLocationItemWidget *cityWidget)
{
    emit selected(cityWidget);
}


} // namespace

