#ifndef LOCATIONITEMREGIONWIDGET_H
#define LOCATIONITEMREGIONWIDGET_H

#include <QVector>
#include "../backend/locationsmodel/basiclocationsmodel.h"
#include "locationitemcitywidget.h"
#include "locationitemregionheaderwidget.h"

namespace GuiLocations {

class LocationItemRegionWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LocationItemRegionWidget(IWidgetLocationsInfo *widgetLocationInfo, LocationModelItem *locationItem, QWidget *parent = nullptr);
    ~LocationItemRegionWidget();

    enum CITY_SUBMENU_STATE { EXPANDED, COLLAPSED, EXPANDING, COLLAPSING };

    const LocationID getId() const;

    CITY_SUBMENU_STATE citySubMenuState() { return citySubMenuState_; }
    bool expandable() const;
    bool expandedOrExpanding();
    void setExpandedWithoutAnimation(bool expand);
    void expand();
    void collapse();

    void addCity(CityModelItem city);
    QVector<SelectableLocationItemWidget *> selectableWidgets();
    QVector<LocationItemCityWidget *> cityWidgets();

    void setFavorited(LocationID id, bool isFavorite);

    void recalcItemPositions();
    void recalcHeight();

    void updateScaling();

signals:
    void heightChanged(int height);
    void selected(SelectableLocationItemWidget *itemWidget);
    void clicked(LocationItemCityWidget *cityWidget);
    void clicked(LocationItemRegionWidget *regionWidget);
    void favoriteClicked(LocationItemCityWidget *cityWidget, bool favorited);

private slots:
    void onRegionHeaderSelected();
    void onRegionHeaderClicked();
    void onCityItemSelected();
    void onCityItemClicked();
    void onExpandingHeightAnimationValueChanged(const QVariant &value);
private:
    LocationItemRegionHeaderWidget *regionHeaderWidget_;
    QVector<LocationItemCityWidget *> cities_;
    IWidgetLocationsInfo *widgetLocationsInfo_;

    CITY_SUBMENU_STATE citySubMenuState_;
    QVariantAnimation expandingHeightAnimation_;
    int height_;
    int expandedHeight();
};

}

#endif // LOCATIONITEMREGIONWIDGET_H
