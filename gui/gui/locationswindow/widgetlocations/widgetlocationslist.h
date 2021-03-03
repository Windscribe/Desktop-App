#ifndef LOCATIONITEMLISTWIDGET_H
#define LOCATIONITEMLISTWIDGET_H

#include <QWidget>
#include "itemwidgetregion.h"

namespace GuiLocations {

class CursorUpdateHelper;

class WidgetLocationsList : public QWidget
{
    Q_OBJECT
public:
    explicit WidgetLocationsList(IWidgetLocationsInfo * widgetLocationsInfo, QWidget *parent = nullptr);
    ~WidgetLocationsList() override;

    void clearWidgets();
    void addRegionWidget(LocationModelItem *item);

    void addCityToRegion(const CityModelItem &city, LocationModelItem *region);

    void updateScaling();
    void accentWidgetContainingCursor();
    void selectWidgetContainingGlobalPt(const QPoint &pt);

    void expand(LocationID locId);
    void collapse(LocationID locId);
    void expandAllLocationsWithoutAnimation();
    void collapseAllLocationsWithoutAnimation();
    void expandLocationIds(QVector<LocationID> locIds);

    QVector<LocationID> expandedOrExpandingLocationIds();
    QVector<ItemWidgetRegion *> itemWidgets();
    QVector<ItemWidgetCity *> cityWidgets();
    QVector<IItemWidget *> selectableWidgets(); // regions + expanded cities

    int selectableIndex(LocationID locationId);
    const LocationID lastAccentedLocationId() const;
    void accentItem(LocationID locationId);
    void accentItemWithoutAnimation(LocationID locationId);
    void setMuteAccentChanges(bool mute);

    void accentFirstSelectableItem();
    void accentFirstSelectableItemWithoutAnimation();
    bool hasAccentItem();
    void moveAccentUp();
    void moveAccentDown();
    int accentItemSelectableIndex();
    IItemWidget *lastAccentedItemWidget();
    IItemWidget *selectableWidget(LocationID locationId);
    ItemWidgetRegion *regionWidget(LocationID locationId);
    IItemWidget *itemWidget(LocationID locationId);

signals:
    void heightChanged(int height);
    void favoriteClicked(ItemWidgetCity *cityWidget, bool favorited);
    void cityItemClicked(ItemWidgetCity *cityWidget);
    void locationIdSelected(LocationID id);
    void regionExpanding(ItemWidgetRegion *regionWidget);

protected:
    virtual void paintEvent(QPaintEvent *event) override;

private slots:
    void onRegionWidgetHeightChanged(int height);

    void onLocationItemCityClicked(ItemWidgetCity *cityWidget);
    void onLocationItemRegionClicked(ItemWidgetRegion *regionWidget);
    void onSelectableLocationItemAccented(IItemWidget *itemWidget);

private:
    int height_;
    std::unique_ptr<CursorUpdateHelper> cursorUpdateHelper_;
    QVector<ItemWidgetRegion *> itemWidgets_; // Note: spinning up this many widgets is not scalable design. If locations list grows we may experience UI slowdowns. It seems okay for now.
    IItemWidget *lastAccentedItemWidget_;
    QVector<IItemWidget *> recentlyAccentedWidgets_;

    IWidgetLocationsInfo *widgetLocationsInfo_; // deleted elsewhere

    void recalcItemPositions();
    void updateCursorWithSelectableWidget(IItemWidget *widget);

    void safeEmitLocationIdSelected(IItemWidget *widget);
};

}

#endif // LOCATIONITEMLISTWIDGET_H
