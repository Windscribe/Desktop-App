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
    void expandAllLocations();
    void collapseAllLocations();
    void expandLocationIds(QVector<LocationID> locIds);

    QVector<LocationID> expandedOrExpandingLocationIds();
    QVector<ItemWidgetRegion *> itemWidgets();
    QVector<ItemWidgetCity *> cityWidgets();
    QVector<IItemWidget *> selectableWidgets(); // regions + expanded cities

    int selectableIndex(LocationID locationId);
    const LocationID lastSelectedLocationId() const;
    void selectItem(LocationID locationId);

    void accentFirstSelectableItem();
    bool hasAccentItem();
    void moveAccentUp();
    void moveAccentDown();
    int accentItemSelectableIndex();
    IItemWidget *lastAccentedItemWidget();
    IItemWidget *selectableWidget(LocationID locationId);

    enum ExpandReason { EXPAND_REASON_AUTO, EXPAND_REASON_USER };

signals:
    void heightChanged(int height);
    void favoriteClicked(ItemWidgetCity *cityWidget, bool favorited);
    void cityItemClicked(ItemWidgetCity *cityWidget);
    void locationIdSelected(LocationID id);
    void regionExpanding(ItemWidgetRegion *regionWidget, WidgetLocationsList::ExpandReason reason);

protected:
    virtual void paintEvent(QPaintEvent *event) override;

private slots:
    void onRegionWidgetHeightChanged(int height);

    void onLocationItemCityClicked(ItemWidgetCity *cityWidget);
    void onLocationItemRegionClicked(ItemWidgetRegion *regionWidget);
    void onSelectableLocationItemSelected(IItemWidget *itemWidget);

private:
    int height_;
    std::unique_ptr<CursorUpdateHelper> cursorUpdateHelper_;
    QVector<ItemWidgetRegion *> itemWidgets_; // Note: spinning up this many widgets is not scalable design. If locations list grows we may experience UI slowdowns. It seems okay for now.
    IItemWidget *lastAccentedItemWidget_;
    QVector<IItemWidget *> recentlySelectedWidgets_;

    IWidgetLocationsInfo *widgetLocationsInfo_; // deleted elsewhere

    void recalcItemPositions();
    void updateCursorWithSelectableWidget(IItemWidget *widget);

};

}

#endif // LOCATIONITEMLISTWIDGET_H
