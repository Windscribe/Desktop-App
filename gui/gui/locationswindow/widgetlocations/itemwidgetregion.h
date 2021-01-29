#ifndef LOCATIONITEMREGIONWIDGET_H
#define LOCATIONITEMREGIONWIDGET_H

#include <QVector>
#include "../backend/locationsmodel/basiclocationsmodel.h"
#include "itemwidgetcity.h"
#include "itemwidgetheader.h"

namespace GuiLocations {

class ItemWidgetRegion : public QWidget
{
    Q_OBJECT
public:
    explicit ItemWidgetRegion(IWidgetLocationsInfo *widgetLocationInfo, LocationModelItem *locationItem, QWidget *parent = nullptr);
    ~ItemWidgetRegion();

    enum CITY_SUBMENU_STATE { EXPANDED, COLLAPSED, EXPANDING, COLLAPSING };

    const LocationID getId() const;

    CITY_SUBMENU_STATE citySubMenuState() { return citySubMenuState_; }
    bool expandable() const;
    bool expandedOrExpanding();
    void setExpandedWithoutAnimation(bool expand);
    void expand();
    void collapse();

    // TODO: can speed things up with by removing copies?
    void addCity(CityModelItem city);
    QVector<IItemWidget *> selectableWidgets();
    QVector<ItemWidgetCity *> cityWidgets();

    void setFavorited(LocationID id, bool isFavorite);

    void recalcItemPositions();
    void recalcHeight();

    void updateScaling();

signals:
    void heightChanged(int height);
    void selected(IItemWidget *itemWidget);
    void clicked(ItemWidgetCity *cityWidget);
    void clicked(ItemWidgetRegion *regionWidget);
    void favoriteClicked(ItemWidgetCity *cityWidget, bool favorited);

private slots:
    void onRegionHeaderSelected();
    void onRegionHeaderClicked();
    void onCityItemSelected();
    void onCityItemClicked();
    void onExpandingHeightAnimationValueChanged(const QVariant &value);
private:
    ItemWidgetHeader *regionHeaderWidget_;
    QVector<ItemWidgetCity *> cities_;
    IWidgetLocationsInfo *widgetLocationsInfo_;

    CITY_SUBMENU_STATE citySubMenuState_;
    QVariantAnimation expandingHeightAnimation_;
    int height_;
    int expandedHeight();
};

}

#endif // LOCATIONITEMREGIONWIDGET_H
