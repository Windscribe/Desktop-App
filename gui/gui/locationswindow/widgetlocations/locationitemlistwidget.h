#ifndef LOCATIONITEMLISTWIDGET_H
#define LOCATIONITEMLISTWIDGET_H

#include <QWidget>
#include "locationitemregionwidget.h"

namespace GuiLocations {

class CursorUpdateHelper;

class LocationItemListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LocationItemListWidget(IWidgetLocationsInfo * widgetLocationsInfo, QWidget *parent = nullptr);
    ~LocationItemListWidget() override;

    void clearWidgets();
    void addRegionWidget(LocationModelItem *item);

    // TODO: can speed things up with by removing copies?
    void addCityToRegion(CityModelItem city, LocationModelItem *region);

    void updateScaling();
    void selectWidgetContainingCursor();

    void expand(LocationID locId);
    void collapse(LocationID locId);
    void expandAllLocations();
    void collapseAllLocations();
    void expandLocationIds(QVector<LocationID> locIds);

    QVector<LocationID> expandedOrExpandingLocationIds();
    QVector<LocationItemRegionWidget *> itemWidgets();
    QVector<LocationItemCityWidget *> cityWidgets();
    QVector<SelectableLocationItemWidget *> selectableWidgets(); // regions + expanded cities

    int selectableIndex(LocationID locationId);
    const LocationID lastSelectedLocationId() const;
    void selectItem(LocationID locationId);

    void accentFirstSelectableItem();
    bool hasAccentItem();
    void moveAccentUp();
    void moveAccentDown();
    int accentItemSelectableIndex();
    SelectableLocationItemWidget *lastAccentedItemWidget();
    SelectableLocationItemWidget *selectableWidget(LocationID locationId);

    enum ExpandReason { EXPAND_REASON_AUTO, EXPAND_REASON_USER };

signals:
    void heightChanged(int height);
    void favoriteClicked(LocationItemCityWidget *cityWidget, bool favorited);
    void cityItemClicked(LocationItemCityWidget *cityWidget);
    void locationIdSelected(LocationID id);
    void regionExpanding(LocationItemRegionWidget *regionWidget, LocationItemListWidget::ExpandReason reason);

protected:
    virtual void paintEvent(QPaintEvent *event) override;

private slots:
    void onRegionWidgetHeightChanged(int height);

    void onLocationItemCityClicked(LocationItemCityWidget *cityWidget);
    void onLocationItemRegionClicked(LocationItemRegionWidget *regionWidget);
    void onSelectableLocationItemSelected(SelectableLocationItemWidget *itemWidget);

private:
    int height_;
    std::unique_ptr<CursorUpdateHelper> cursorUpdateHelper_;
    QVector<LocationItemRegionWidget *> itemWidgets_;
    SelectableLocationItemWidget *lastAccentedItemWidget_;
    QVector<SelectableLocationItemWidget *> recentlySelectedWidgets_;

    IWidgetLocationsInfo *widgetLocationsInfo_; // deleted elsewhere

    void recalcItemPositions();
    void updateCursorWithSelectableWidget(SelectableLocationItemWidget *widget);

};

}

#endif // LOCATIONITEMLISTWIDGET_H
