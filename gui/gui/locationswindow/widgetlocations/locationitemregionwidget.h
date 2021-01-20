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


    // TODO: add view change when expanding large (relative to viewport) city set
    CITY_SUBMENU_STATE citySubMenuState() { return citySubMenuState_; }
    bool expandable() const;
    bool expandedOrExpanding();
    void setExpandedWithoutAnimation(bool expand);
    void expand();
    void collapse();

    void addCity(CityModelItem city);
    QVector<QSharedPointer<SelectableLocationItemWidget>> selectableWidgets();
    QVector<QSharedPointer<LocationItemCityWidget>> selectableCityWidgets();
    QVector<QSharedPointer<LocationItemCityWidget>> cityWidgets();

    void setFavorited(LocationID id, bool isFavorite);

    void recalcItemPos();
    void recalcHeight();

signals:
    void heightChanged(int height);
    void selected(SelectableLocationItemWidget *itemWidget); // TODO: re-word to "accent"
    void clicked(LocationItemCityWidget *cityWidget);
    void clicked(LocationItemRegionWidget *regionWidget);
    void favoriteClicked(LocationItemCityWidget *cityWidget, bool favorited);

private slots:
    void onRegionHeaderSelected(SelectableLocationItemWidget *regionWidget);
    void onRegionHeaderClicked();
    void onCityItemSelected(SelectableLocationItemWidget *cityWidget);
    void onCityItemClicked();
    void onExpandingHeightAnimationValueChanged(const QVariant &value);
private:
    QSharedPointer<LocationItemRegionHeaderWidget> regionHeaderWidget_;
    QVector<QSharedPointer<LocationItemCityWidget>> cities_;
    IWidgetLocationsInfo *widgetLocationsInfo_;

    CITY_SUBMENU_STATE citySubMenuState_;
    QVariantAnimation expandingHeightAnimation_;
    int height_;
    int expandedHeight();
};

}

#endif // LOCATIONITEMREGIONWIDGET_H
