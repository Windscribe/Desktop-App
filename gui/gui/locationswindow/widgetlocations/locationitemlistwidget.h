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

    int countRegions() const;
    void clearWidgets();
    void addRegionWidget(LocationModelItem *item);
    void addCityToRegion(CityModelItem city, LocationModelItem *region);

    void updateScaling();
    void selectWidgetContainingCursor();

    void expand(LocationID locId);
    void collapse(LocationID locId);
    void expandLocationIds(QVector<LocationID> locIds);
    QVector<LocationID> expandedOrExpandingLocationIds();
    const QVector<QSharedPointer<LocationItemRegionWidget>> &itemWidgets();
    const QVector<QSharedPointer<LocationItemCityWidget>> selectableCityWidgets();
    QVector<QSharedPointer<LocationItemCityWidget>> cityWidgets();

    const LocationID topSelectableLocationIdInViewport();
    int selectableIndex(LocationID locationId);
    int viewportIndex(LocationID locationId);

    void accentFirstItem();
    bool hasAccentItem();
    void moveAccentUp();
    void moveAccentDown();
    int accentItemSelectableIndex();
    int accentItemViewportIndex();

    SelectableLocationItemWidget *lastAccentedItemWidget();
    const LocationID lastSelectedLocationId() const;
    void selectItem(LocationID locationId);

    static const int ITEM_HEIGHT = 50;

signals:
    void heightChanged(int height);
    void favoriteClicked(LocationItemCityWidget *cityWidget, bool favorited);
    void cityItemClicked(LocationItemCityWidget *cityWidget);
    void locationIdSelected(LocationID id);
    void regionExpanding(LocationItemRegionWidget *regionWidget);

protected:
    virtual void paintEvent(QPaintEvent *event) override;

private slots:
    void onRegionWidgetHeightChanged(int height);

    void onLocationItemCityClicked(LocationItemCityWidget *cityWidget);
    void onLocationItemRegionClicked(LocationItemRegionWidget *regionWidget);
    void onSelectableLocationItemSelected(SelectableLocationItemWidget *itemWidget);

private:
    int height_;
    QVector<QSharedPointer<LocationItemRegionWidget>> itemWidgets_;
    void recalcItemPositions();
    QVector<QSharedPointer<SelectableLocationItemWidget>> selectableWidgets(); // regions + expanded cities

    IWidgetLocationsInfo *widgetLocationsInfo_;
    SelectableLocationItemWidget *lastAccentedItemWidget_;

    QVector<SelectableLocationItemWidget*> recentlySelectedWidgets_;

    std::unique_ptr<CursorUpdateHelper> cursorUpdateHelper_;
    void updateCursorWithSelectableWidget(SelectableLocationItemWidget *widget);

};

}

#endif // LOCATIONITEMLISTWIDGET_H
