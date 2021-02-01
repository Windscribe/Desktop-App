#ifndef CITYITEMLISTWIDGET_H
#define CITYITEMLISTWIDGET_H

#include <QWidget>
#include "cursorupdatehelper.h"
#include "itemwidgetcity.h"

namespace GuiLocations {

class WidgetCitiesList : public QWidget
{
    Q_OBJECT
public:
    explicit WidgetCitiesList(IWidgetLocationsInfo * widgetLocationsInfo, QWidget *parent = nullptr);
    ~WidgetCitiesList();

    void clearWidgets();
    void addCity(const CityModelItem &city);

    void updateScaling();
    void selectWidgetContainingCursor();

    QVector<ItemWidgetCity *> itemWidgets();

    int selectableIndex(LocationID locationId);
    const LocationID lastAccentedLocationId() const;
    void accentItem(LocationID locationId);
    void accentFirstItem();
    bool hasAccentItem();
    void moveAccentUp();
    void moveAccentDown();
    int accentItemIndex();

    IItemWidget *lastAccentedItemWidget();
    IItemWidget *selectableWidget(LocationID locationId);

signals:
    void heightChanged(int height);
    void favoriteClicked(ItemWidgetCity *cityWidget, bool favorited);
    void cityItemClicked(ItemWidgetCity *cityWidget);
    void locationIdSelected(LocationID id);

private slots:
    void onCityItemAccented();
    void onCityItemClicked();

private:
    int height_;
    std::unique_ptr<CursorUpdateHelper> cursorUpdateHelper_;
    QVector<ItemWidgetCity *> itemWidgets_;
    IItemWidget *lastAccentedItemWidget_;
    QVector<IItemWidget *> recentlySelectedWidgets_;

    IWidgetLocationsInfo *widgetLocationsInfo_; // deleted elsewhere

    void recalcItemPositions();
    void updateCursorWithSelectableWidget(IItemWidget *widget);
};

} // namespace

#endif // CITYITEMLISTWIDGET_H
