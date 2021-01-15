#ifndef LOCATIONITEMLISTWIDGET_H
#define LOCATIONITEMLISTWIDGET_H

#include <QWidget>
#include "locationitemregionwidget.h"

namespace GuiLocations {

class LocationItemListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LocationItemListWidget(QWidget *parent = nullptr);
    ~LocationItemListWidget();

    int countRegions() const;
    void clearWidgets();
    void addRegionWidget(LocationModelItem *item);
    void addCityToRegion(CityModelItem city, LocationModelItem *region);

    void updateScaling();
    void selectWidgetContainingCursor();

    void expandLocationIds(QVector<LocationID> locIds);
    QVector<LocationID> expandedLocationIds(); // TODO: add expanding
    const QVector<QSharedPointer<LocationItemRegionWidget>> &itemWidgets();
    const QVector<QSharedPointer<SelectableLocationItemWidget>> &visibleItemWidgets();

    const LocationID topSelectableLocationIdInViewport();
    int selectableIndex(LocationID locationId);

    const LocationID lastSelectedLocationId() const;
    void selectItem(LocationID locationId);

    static const int ITEM_HEIGHT = 50;

signals:
    void heightChanged(int height);

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

    LocationID lastSelectedLocationId_;

};

}

#endif // LOCATIONITEMLISTWIDGET_H
