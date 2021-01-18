#ifndef LOCATIONITEMCITYWIDGET_H
#define LOCATIONITEMCITYWIDGET_H

#include <QLabel>
#include "../backend/locationsmodel/basiclocationsmodel.h"
#include "selectablelocationitemwidget.h"
#include "iwidgetlocationsinfo.h"
#include "commonwidgets/iconwidget.h"
#include "../backend/types/pingtime.h"
#include "itemtimems.h"

namespace GuiLocations {

class LocationItemCityWidget : public SelectableLocationItemWidget
{
    Q_OBJECT
public:
    explicit LocationItemCityWidget(IWidgetLocationsInfo *widgetLocationsInfo, CityModelItem cityModelItem, QWidget *parent = nullptr);
    ~LocationItemCityWidget();

    const LocationID getId() const override;
    const QString name() const override;
    SelectableLocationItemWidgetType type() override;

    void setSelectable(bool selectable) override;
    void setSelected(bool select) override;
    bool isSelected() const override;
    bool containsCursor() const override;

    // TODO: fix display of time in MS
    // TODO: fix display of correct ping bar icon (based on speed)
    void setLatencyMs(PingTime pingTime);
    void setShowLatencyMs(bool showLatencyMs);

    // TODO: add click prevention for forbidden city
signals:
    void selected(SelectableLocationItemWidget *itemWidget);
    void clicked(LocationItemCityWidget *itemWidget);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onPingBarIconHoverEnter();
    void onPingBarIconHoverLeave();

private:
    QSharedPointer<QLabel> cityLabel_;
    QSharedPointer<QLabel> nickLabel_;
    QSharedPointer<IconWidget> pingBarIcon_;

    PingTime pingTime_;
    CityModelItem cityModelItem_;
    IWidgetLocationsInfo *widgetLocationsInfo_;

    bool showingPingBar_;
    bool selectable_;
    bool selected_;
    const QString labelStyleSheetWithOpacity(double opacity);

    void recalcItemPositions();

};

}

#endif // LOCATIONITEMCITYWIDGET_H

