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
    explicit LocationItemRegionWidget(LocationModelItem *locationItem, QWidget *parent = nullptr);
    ~LocationItemRegionWidget();

    const LocationID getId() const;
    bool expandable() const;
    bool expanded() const;
    void setExpanded(bool expand);
    void setShowLatencyMs(bool showLatencyMs);

    void addCity(CityModelItem city);
    QVector<QSharedPointer<SelectableLocationItemWidget>> selectableWidgets();

    void recalcItemPos();

signals:
    void heightChanged(int height);
    void selected(SelectableLocationItemWidget *itemWidget); // TODO: re-word to "accent"
    void clicked(LocationItemCityWidget *cityWidget);
    void clicked(LocationItemRegionWidget *regionWidget);

//protected:
//    void enterEvent(QEvent *event) override;
//    void leaveEvent(QEvent *event) override;

private slots:
    void onRegionItemSelected(SelectableLocationItemWidget *regionWidget);
    void onRegionItemClicked();
    void onCityItemSelected(SelectableLocationItemWidget *cityWidget);
    void onCityItemClicked(LocationItemCityWidget *cityWidget);

private:
    QSharedPointer<LocationItemRegionHeaderWidget> regionHeaderWidget_;
    QVector<QSharedPointer<LocationItemCityWidget>> cities_;

    bool expanded_;
    int height_;
};

}

#endif // LOCATIONITEMREGIONWIDGET_H
