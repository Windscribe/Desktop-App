#ifndef CITYITEMLISTWIDGET_H
#define CITYITEMLISTWIDGET_H

#include <QWidget>
#include "cursorupdatehelper.h"
#include "locationitemcitywidget.h"

namespace GuiLocations {

class CityItemListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CityItemListWidget(IWidgetLocationsInfo * widgetLocationsInfo, QWidget *parent = nullptr);
    ~CityItemListWidget();

    void clearWidgets();
    void addCity(CityModelItem city);

    void updateScaling();
    void selectWidgetContainingCursor();

    QVector<LocationItemCityWidget *> itemWidgets();

    int selectableIndex(LocationID locationId);
    const LocationID lastAccentedLocationId() const;
    void accentItem(LocationID locationId);
    void accentFirstItem();
    bool hasAccentItem();
    void moveAccentUp();
    void moveAccentDown();
    int accentItemIndex();

    SelectableLocationItemWidget *lastAccentedItemWidget();
    SelectableLocationItemWidget *selectableWidget(LocationID locationId);

signals:
    void heightChanged(int height);
    void favoriteClicked(LocationItemCityWidget *cityWidget, bool favorited);
    void cityItemClicked(LocationItemCityWidget *cityWidget);
    void locationIdSelected(LocationID id);

private slots:
    void onCityItemAccented();
    void onCityItemClicked();

private:
    int height_;
    std::unique_ptr<CursorUpdateHelper> cursorUpdateHelper_;
    QVector<LocationItemCityWidget *> itemWidgets_;
    SelectableLocationItemWidget *lastAccentedItemWidget_;
    QVector<SelectableLocationItemWidget *> recentlySelectedWidgets_;

    IWidgetLocationsInfo *widgetLocationsInfo_; // deleted elsewhere

    void recalcItemPositions();
    void updateCursorWithSelectableWidget(SelectableLocationItemWidget *widget);
};

} // namespace

#endif // CITYITEMLISTWIDGET_H
