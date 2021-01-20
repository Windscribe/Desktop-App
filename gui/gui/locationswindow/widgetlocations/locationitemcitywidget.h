#ifndef LOCATIONITEMCITYWIDGET_H
#define LOCATIONITEMCITYWIDGET_H

#include <QLabel>
#include "../backend/locationsmodel/basiclocationsmodel.h"
#include "selectablelocationitemwidget.h"
#include "iwidgetlocationsinfo.h"
#include "commonwidgets/iconwidget.h"
#include "commonwidgets/iconbuttonwidget.h"
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

    bool isForbidden() const override;
    bool isDisabled() const override;

    void setFavourited(bool favorited);
    void setSelectable(bool selectable) override;
    void setSelected(bool select) override;
    bool isSelected() const override;
    bool containsCursor() const override;

    void setLatencyMs(PingTime pingTime);
    void setShowLatencyMs(bool showLatencyMs);

signals:
    void selected(SelectableLocationItemWidget *itemWidget);
    void favoriteClicked(LocationItemCityWidget *itemWidget, bool favorited);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onPingBarIconHoverEnter();
    void onPingBarIconHoverLeave();
    void onFavoriteIconButtonClicked();

private:
    QSharedPointer<QLabel> cityLabel_;
    QSharedPointer<QLabel> nickLabel_;
    QSharedPointer<IconWidget> pingBarIcon_;
    QSharedPointer<CommonWidgets::IconButtonWidget> favoriteIconButton_;

    PingTime pingTime_;
    CityModelItem cityModelItem_;
    IWidgetLocationsInfo *widgetLocationsInfo_;

    bool showingPingBar_;
    bool selectable_;
    bool selected_;
    bool favorited_;
    const QString labelStyleSheetWithOpacity(double opacity);

    void recalcItemPositions();
    void updateFavoriteIcon();
    void updatePingBarIcon();
    const QString pingIconNameString(int connectionSpeedIndex);
};

}

#endif // LOCATIONITEMCITYWIDGET_H

