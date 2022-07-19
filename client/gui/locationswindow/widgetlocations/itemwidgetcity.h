#ifndef LOCATIONITEMCITYWIDGET_H
#define LOCATIONITEMCITYWIDGET_H

#include <QLabel>
#include "backend/locationsmodel/basiclocationsmodel.h"
#include "iitemwidget.h"
#include "iwidgetlocationsinfo.h"
#include "commonwidgets/iconwidget.h"
#include "commonwidgets/iconbuttonwidget.h"
#include "types/pingtime.h"
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
    bool isBrokenConfig() const override;

    void setFavourited(bool favorited);
    void setSelectable(bool selectable) override;
    void setAccented(bool accent) override;
    void setAccentedWithoutAnimation(bool accent) override;
    bool isAccented() const override;

    bool containsCursor() const override;
    bool containsGlobalPoint(const QPoint &pt) override;
    QRect globalGeometry() const override;

    void setLatencyMs(PingTime pingTime);
    void setShowLatencyMs(bool showLatencyMs);

    void updateScaling();
signals:
    void accented();
    void favoriteClicked(ItemWidgetCity *itemWidget, bool favorited);
    void hoverEnter();

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onPingIconLightWidgetHoveringChanged(bool hovering);
    void onCityLightWidgetHoveringChanged(bool hovering);
    void onTextOpacityAnimationValueChanged(const QVariant &value);

private:
    IWidgetLocationsInfo *widgetLocationsInfo_;
    PingTime pingTime_;
    CityModelItem cityModelItem_;

    bool showFavIcon_;
    bool showPingIcon_;
    bool show10gbpsIcon_;
    bool showingLatencyAsPingBar_;
    bool selectable_;
    bool accented_;
    bool favorited_;
    bool pressingFav_;

    const QString pingIconNameString(int connectionSpeedIndex);
    void updatePingBarIcon();
    void updateFavoriteIcon();
    void update10gbpsIcon();
    void clickFavorite();

    QSharedPointer<LightWidget> favLightWidget_;
    QSharedPointer<LightWidget> pingIconLightWidget_;
    QSharedPointer<LightWidget> tenGbpsLightWidget_;

    QSharedPointer<QTextLayout> cityTextLayout_;
    QSharedPointer<LightWidget> cityLightWidget_;
    QSharedPointer<QTextLayout> nickTextLayout_;
    QSharedPointer<LightWidget> nickLightWidget_;

    QSharedPointer<QTextLayout> staticIpTextLayout_;
    QSharedPointer<LightWidget> staticIpLightWidget_;

    double curTextOpacity_;
    void recreateTextLayouts();
    QVariantAnimation textOpacityAnimation_;

    const int CITY_CAPTION_MAX_WIDTH = 210;
};

}

#endif // LOCATIONITEMCITYWIDGET_H

