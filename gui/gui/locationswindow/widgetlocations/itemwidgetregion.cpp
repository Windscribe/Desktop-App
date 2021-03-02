#include "itemwidgetregion.h"

#include <QPainter>
#include <QtMath>
#include "dpiscalemanager.h"
#include "commongraphics/commongraphics.h"
#include "utils/logger.h"

// #include <QDebug>

namespace GuiLocations {

ItemWidgetRegion::ItemWidgetRegion(IWidgetLocationsInfo * widgetLocationsInfo, LocationModelItem *locationModelItem, QWidget *parent) : QWidget(parent)
  , widgetLocationsInfo_(widgetLocationsInfo)
  , muteAccentChanges_(false)
  , citySubMenuState_(COLLAPSED)
{
    setFocusPolicy(Qt::NoFocus);

    height_ = qCeil(LOCATION_ITEM_HEIGHT * G_SCALE);

    regionHeaderWidget_ = new ItemWidgetHeader(widgetLocationsInfo, locationModelItem, this);
    connect(regionHeaderWidget_, SIGNAL(clicked()), SLOT(onRegionHeaderClicked()));
    connect(regionHeaderWidget_, SIGNAL(accented()), SLOT(onRegionHeaderAccented()));
    connect(regionHeaderWidget_, SIGNAL(hoverEnter()), SLOT(onRegionHeaderHoverEnter()));

    // qDebug() << "Creating region: " << regionHeaderWidget_->name();

    expandingHeightAnimation_.setDirection(QAbstractAnimation::Forward);
    expandingHeightAnimation_.setDuration(200);
    connect(&expandingHeightAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onExpandingHeightAnimationValueChanged(QVariant)));

    recalcItemPositions();
}

ItemWidgetRegion::~ItemWidgetRegion()
{
    // qDebug() << "Deleting region: " << regionHeaderWidget_->name();

    regionHeaderWidget_->disconnect();
    regionHeaderWidget_->deleteLater();

    foreach (ItemWidgetCity *city, cities_)
    {
        city->disconnect();
        city->deleteLater();
    }
    cities_.clear();
}

const LocationID ItemWidgetRegion::getId() const
{
    return regionHeaderWidget_->getId();
}

bool ItemWidgetRegion::expandable() const
{
    return cities_.count() > 0;
}

bool ItemWidgetRegion::expandedOrExpanding()
{
    return citySubMenuState_ == EXPANDED || citySubMenuState_ == EXPANDING;
}

void ItemWidgetRegion::setExpandedWithoutAnimation(bool expand)
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
    foreach (ItemWidgetCity *city, cities_)
    {
        city->setSelectable(expand);
    }
    recalcItemPositions();
}

void ItemWidgetRegion::setMuteAccentChanges(bool mute)
{
    muteAccentChanges_ = mute;
}

void ItemWidgetRegion::expand()
{
    qCDebug(LOG_LOCATION_LIST) << "Expanding: " << regionHeaderWidget_->name();
    foreach (ItemWidgetCity * city, cities_)
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

void ItemWidgetRegion::collapse()
{
    qCDebug(LOG_LOCATION_LIST) << "Collapsing: " << regionHeaderWidget_->name();

    foreach (ItemWidgetCity *city, cities_)
    {
        city->setSelectable(false);
    }

    regionHeaderWidget_->setExpanded(false);
    citySubMenuState_ = COLLAPSING;
    expandingHeightAnimation_.stop();
    expandingHeightAnimation_.setStartValue(height_);
    expandingHeightAnimation_.setEndValue(static_cast<int>(qCeil(LOCATION_ITEM_HEIGHT*G_SCALE)));
    expandingHeightAnimation_.start();
}

void ItemWidgetRegion::addCity(const CityModelItem &city)
{
    auto cityWidget = new ItemWidgetCity(widgetLocationsInfo_, city, this);
    connect(cityWidget, SIGNAL(clicked()), SLOT(onCityItemClicked()));
    connect(cityWidget, SIGNAL(accented()), SLOT(onCityItemAccented()));
    connect(cityWidget, SIGNAL(hoverEnter()), SLOT(onCityItemHoverEnter()));
    connect(cityWidget, SIGNAL(favoriteClicked(ItemWidgetCity *, bool)), SIGNAL(favoriteClicked(ItemWidgetCity*, bool)));
    cities_.append(cityWidget);
    cityWidget->show();
    recalcItemPositions();
}

QVector<IItemWidget*> ItemWidgetRegion::selectableWidgets()
{
    QVector<IItemWidget *> widgets;
    widgets.append(regionHeaderWidget_);
    if (expandedOrExpanding())
    {
        foreach (ItemWidgetCity *city, cities_)
        {
            widgets.append(city);
        }
    }
    return widgets;
}

QVector<ItemWidgetCity *> ItemWidgetRegion::cityWidgets()
{
    return cities_;
}

void ItemWidgetRegion::setFavorited(LocationID id, bool isFavorite)
{
    foreach (ItemWidgetCity *city, cities_)
    {
        if (city->getId() == id)
        {
            city->setFavourited(isFavorite);
            break;
        }
    }
}

void ItemWidgetRegion::recalcItemPositions()
{
    // qDebug() << "Region recalc item positions";
    regionHeaderWidget_->setGeometry(0,0, WINDOW_WIDTH * G_SCALE, qCeil(LOCATION_ITEM_HEIGHT * G_SCALE));

    int height = qCeil(LOCATION_ITEM_HEIGHT * G_SCALE);

    foreach (ItemWidgetCity *city, cities_)
    {
        city->setGeometry(0, height, WINDOW_WIDTH * G_SCALE, qCeil(LOCATION_ITEM_HEIGHT * G_SCALE));
        height += city->geometry().height();
    }
    recalcHeight();
}

void ItemWidgetRegion::recalcHeight()
{
    int height = qCeil(LOCATION_ITEM_HEIGHT*G_SCALE);
    if  (citySubMenuState_ == EXPANDED) height = expandedHeight();

    if (height != height_)
    {
        // qDebug() << "Region height changed";
        height_ = height;
        emit heightChanged(height_);
    }
}

void ItemWidgetRegion::updateScaling()
{
    regionHeaderWidget_->updateScaling();
    foreach (ItemWidgetCity *city , cities_)
    {
        city->updateScaling();
    }
    recalcItemPositions();
}

void ItemWidgetRegion::onRegionHeaderAccented()
{
    emit accented(regionHeaderWidget_);
}

void ItemWidgetRegion::onRegionHeaderClicked()
{
    emit clicked(this);
}

void ItemWidgetRegion::onRegionHeaderHoverEnter()
{
    if (!muteAccentChanges_)
    {
        regionHeaderWidget_->setAccented(true);
    }
}

void ItemWidgetRegion::onCityItemClicked()
{
    ItemWidgetCity *cityWidget = static_cast<ItemWidgetCity*>(sender());

    if (cityWidget->isForbidden() || cityWidget->isDisabled() || cityWidget->isBrokenConfig())
    {
        return;
    }

    emit clicked(cityWidget);
}

void ItemWidgetRegion::onCityItemHoverEnter()
{
    if (!muteAccentChanges_)
    {
        ItemWidgetCity *cityWidget = static_cast<ItemWidgetCity*>(sender());
        cityWidget->setAccented(true);
    }
}

void ItemWidgetRegion::onExpandingHeightAnimationValueChanged(const QVariant &value)
{
    int height = value.toInt();

    // qDebug() << "height animation: " << height;
    if (height == static_cast<int>(qCeil(LOCATION_ITEM_HEIGHT*G_SCALE)))
    {
        citySubMenuState_ = COLLAPSED;
    }
    else if (height == expandedHeight())
    {
        citySubMenuState_ = EXPANDED;
    }

    if (height != height_)
    {
        height_ = height;
        emit heightChanged(height);
    }
}

int ItemWidgetRegion::expandedHeight()
{
    int height = qCeil(LOCATION_ITEM_HEIGHT * G_SCALE);
    foreach (auto city, cities_)
    {
        height += city->geometry().height();
    }

    return height;
}

void ItemWidgetRegion::onCityItemAccented()
{
    emit accented(static_cast<IItemWidget*>(sender()));
}


} // namespace

