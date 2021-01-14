#ifndef LOCATIONITEMREGIONWIDGET_H
#define LOCATIONITEMREGIONWIDGET_H

#include <QList>
#include "../backend/locationsmodel/basiclocationsmodel.h"
#include "locationitemcitywidget.h"
#include "locationitemregionheaderwidget.h"

namespace GuiLocations {

class LocationItemRegionWidget : public QAbstractButton
{
    Q_OBJECT
public:
    explicit LocationItemRegionWidget(LocationModelItem *locationItem, QWidget *parent = nullptr);
    ~LocationItemRegionWidget();

    LocationID getId();
    bool expandable() const;
    bool expanded() const;
    void setExpanded(bool expand);
    void setShowLatencyMs(bool showLatencyMs);

    void addCity(CityModelItem city);
    QList<QSharedPointer<SelectableLocationItemWidget>> selectableWidgets();

    void updateScaling();
    void recalcItemPos();

signals:
    void heightChanged(int height);
    void selected(SelectableLocationItemWidget *itemWidget);
    void clicked(LocationItemCityWidget *itemWidget);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void onRegionItemSelected(SelectableLocationItemWidget *regionWidget);
    void onRegionItemClicked();
    void onCityItemSelected(SelectableLocationItemWidget *cityWidget);
    void onCityItemClicked(LocationItemCityWidget *cityWidget);

private:
    QSharedPointer<LocationItemRegionHeaderWidget> regionHeaderWidget_;
    QList<QSharedPointer<LocationItemCityWidget>> cities_;

    bool expanded_;
    int height_;
};

}

#endif // LOCATIONITEMREGIONWIDGET_H
