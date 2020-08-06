#ifndef LOCATIONSTAB_H
#define LOCATIONSTAB_H

#include <QObject>
#include <QPushButton>
#include <QWidget>
#include <QVariantAnimation>
#include <QTimer>
#include "widgetlocations.h"
#include "widgetcities.h"
#include "../backend/locationsmodel/locationsmodel.h"
#include "staticipdeviceinfo.h"
#include "configfooterinfo.h"
#include "staticipdeviceitem.h"
#include "configfooteritem.h"

namespace GuiLocations {

// switchable tabs of locations, includes widgets for all locations, favorite locations, configured locations, static IPs locations
class LocationsTab : public QWidget
{
    Q_OBJECT
public:
    explicit LocationsTab(QWidget *parent, LocationsModel *locationsModel);

    // return new height
    int setCountVisibleItemSlots(int cnt);
    int getCountVisibleItems();
    void setOnlyConfigTabVisible(bool onlyConfig);

    void handleKeyReleaseEvent(QKeyEvent *event);

    void updateIconRectsAndLine();
    void updateLocationWidgetsGeometry(int newHeight);
    void updateScaling();

    void updateLanguage();

public slots:
    void setLatencyDisplay(ProtoTypes::LatencyDisplayType l);

protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void leaveEvent(QEvent *event);

signals:
    void selected(LocationID id);
    void switchFavorite(LocationID id, bool isFavorite);
    void addStaticIpClicked();
    void addCustomConfigClicked();
    void showTooltip(TooltipInfo info);
    void hideTooltip(TooltipId type);

private slots:
    void onWhiteLinePosChanged(const QVariant &value);
    void onAllLocationsHeightChanged(int oldHeight, int newHeight);
    void onDeviceNameChanged(const QString &deviceName);

private:
    IWidgetLocationsInfo *currentWidgetLocations();

    GuiLocations::WidgetLocations *widgetAllLocations_;
    GuiLocations::WidgetCities *widgetConfiguredLocations_;
    GuiLocations::WidgetCities *widgetStaticIpsLocations_;
    GuiLocations::WidgetCities *widgetFavoriteLocations_;

    StaticIPDeviceInfo *staticIPDeviceInfo_; // footer
    ConfigFooterInfo *configFooterInfo_;     // footer

    enum CurTabEnum {
        CUR_TAB_NONE = 0,
        CUR_TAB_ALL_LOCATIONS,
        CUR_TAB_FAVORITE_LOCATIONS,
        CUR_TAB_STATIC_IPS_LOCATIONS,
        CUR_TAB_CONFIGURED_LOCATIONS
    };

    //Backend &backend_;
    CurTabEnum curTab_;
    CurTabEnum tabPress_;

    const int TOP_TAB_HEIGHT = 50;
    const int ANIMATION_DURATION = 150;
    const int WHITE_LINE_WIDTH = 18;

    QRect rcAllLocationsIcon_;
    QRect rcConfiguredLocationsIcon_;
    QRect rcStaticIpsLocationsIcon_;
    QRect rcFavoriteLocationsIcon_;

    Qt::CursorShape curCursorShape_;
    CurTabEnum curTabMouseOver_;

    int curWhiteLinePos_;
    QVariantAnimation whileLineAnimation_;

    int countOfVisibleItemSlots_;
    bool showAllTabs_;
    void changeTab(CurTabEnum newTab);

    void onClickAllLocations();
    void onClickConfiguredLocations();
    void onClickStaticIpsLocations();
    void onClickFavoriteLocations();

    void drawTab(QPainter &painter, const QRect &rc);
    void drawBottomLine(QPainter &painter, int left, int right, int bottom, int whiteLinePos);
    void setArrowCursor();
    void setPointingHandCursor();
    bool isWhiteAnimationActive();

    void rectHoverEnter(QRect buttonRect, QString text, int offsetX, int offsetY);

    void updateRibbonVisibility();
};

} // namespace GuiLocations

#endif // LOCATIONSTAB_H
