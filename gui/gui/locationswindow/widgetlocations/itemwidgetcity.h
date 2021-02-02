#ifndef LOCATIONITEMCITYWIDGET_H
#define LOCATIONITEMCITYWIDGET_H

#include <QLabel>
#include "../backend/locationsmodel/basiclocationsmodel.h"
#include "iitemwidget.h"
#include "iwidgetlocationsinfo.h"
#include "commonwidgets/iconwidget.h"
#include "commonwidgets/iconbuttonwidget.h"
#include "../backend/types/pingtime.h"
#include "itemtimems.h"
#include "commonwidgets/lightwidget.h"

namespace GuiLocations {

class ItemWidgetCity : public IItemWidget
{
    Q_OBJECT
public:
    explicit ItemWidgetCity(IWidgetLocationsInfo *widgetLocationsInfo, const CityModelItem &cityModelItem, QWidget *parent = nullptr);
    ~ItemWidgetCity() override;

    bool isExpanded() const override;

    const LocationID getId() const override;
    const QString name() const override;
    ItemWidgetType type() override;

    bool isForbidden() const override;
    bool isDisabled() const override;

    void setFavourited(bool favorited);
    void setSelectable(bool selectable) override;
    void setSelected(bool select) override;
    bool isSelected() const override;

    bool containsCursor() const override;
    QRect globalGeometry() const override;

    void setLatencyMs(PingTime pingTime);
    void setShowLatencyMs(bool showLatencyMs);

    void updateScaling();
signals:
    void selected();
    void favoriteClicked(ItemWidgetCity *itemWidget, bool favorited);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void onPingIconLightWidgetHoveringChanged(bool hovering);
    void onFavoriteIconButtonClicked(); // TODO: fix me
    void onTextOpacityAnimationValueChanged(const QVariant &value);

private:
    IWidgetLocationsInfo *widgetLocationsInfo_;
    PingTime pingTime_;
    CityModelItem cityModelItem_;

    bool showFavIcon_;
    bool showPingIcon_;
    bool showingLatencyAsPingBar_;
    bool selectable_;
    bool selected_;
    bool favorited_;

    const QString pingIconNameString(int connectionSpeedIndex);
    void updatePingBarIcon();
    void updateFavoriteIcon();

    QSharedPointer<LightWidget> favLightWidget_;
    QSharedPointer<LightWidget> pingIconLightWidget_;

    QSharedPointer<QTextLayout> cityTextLayout_;
    QSharedPointer<LightWidget> cityLightWidget_;
    QSharedPointer<QTextLayout> nickTextLayout_;
    QSharedPointer<LightWidget> nickLightWidget_;

    double curTextOpacity_;
    void recreateTextLayouts();
    QVariantAnimation textOpacityAnimation_;
};

}

#endif // LOCATIONITEMCITYWIDGET_H

