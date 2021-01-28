#ifndef LOCATIONSTAB_H
#define LOCATIONSTAB_H

#include <QObject>
#include <QPushButton>
#include <QWidget>
#include <QVariantAnimation>
#include <QTimer>
#include "widgetlocations.h"
#include "widgetcities.h"
#include "searchwidgetlocations.h"
#include "../backend/locationsmodel/locationsmodel.h"
#include "staticipdeviceinfo.h"
#include "configfooterinfo.h"
#include "commonwidgets/custommenulineedit.h"

namespace GuiLocations {

// switchable tabs of locations, includes widgets for all locations, favorite locations, configured locations, static IPs locations
class LocationsTab : public QWidget
{
    Q_OBJECT
public:
    explicit LocationsTab(QWidget *parent, LocationsModel *locationsModel);

    int unscaledHeight();
    void setCountVisibleItemSlots(int cnt);
    int getCountVisibleItems();
    void setOnlyConfigTabVisible(bool onlyConfig);

    void handleKeyReleaseEvent(QKeyEvent *event);

    void updateIconRectsAndLine();
    void updateLocationWidgetsGeometry(int newHeight);
    void updateScaling();

    void updateLanguage();

public slots:
    void setLatencyDisplay(ProtoTypes::LatencyDisplayType l);
    void setCustomConfigsPath(QString path);

protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void leaveEvent(QEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);

signals:
    void selected(LocationID id);
    void switchFavorite(LocationID id, bool isFavorite);
    void addStaticIpClicked();
    void clearCustomConfigClicked();
    void addCustomConfigClicked();

private slots:
    void onWhiteLinePosChanged(const QVariant &value);

    void onDeviceNameChanged(const QString &deviceName);
    void onAddCustomConfigClicked();

    void onSearchButtonClicked();
    void onSearchCancelButtonClicked();
    void onSearchButtonPosAnimationValueChanged(const QVariant &value);

    void onSearchLineEditTextChanged(QString text);
    void onSearchLineEditKeyEnterPressed();
    void onSearchLineEditFocusOut();
private:
    enum CurTabEnum {
        CUR_TAB_NONE = 0,
        CUR_TAB_ALL_LOCATIONS,
        CUR_TAB_FAVORITE_LOCATIONS,
        CUR_TAB_STATIC_IPS_LOCATIONS,
        CUR_TAB_CONFIGURED_LOCATIONS,
        CUR_TAB_SEARCH_LOCATIONS,
        CUR_TAB_FIRST = CUR_TAB_ALL_LOCATIONS,
        CUR_TAB_LAST = CUR_TAB_SEARCH_LOCATIONS
    };

    IWidgetLocationsInfo *currentWidgetLocations();
    IWidgetLocationsInfo *locationWidgetByEnum(CurTabEnum tabEnum);
    GuiLocations::WidgetLocations *widgetAllLocations_;
    GuiLocations::WidgetCities *widgetConfiguredLocations_;
    GuiLocations::WidgetCities *widgetStaticIpsLocations_;
    GuiLocations::WidgetCities *widgetFavoriteLocations_;
    GuiLocations::SearchWidgetLocations *widgetSearchLocations_;

    StaticIPDeviceInfo *staticIPDeviceInfo_; // footer
    ConfigFooterInfo *configFooterInfo_;     // footer

    //Backend &backend_;
    CurTabEnum curTab_;
    CurTabEnum lastTab_;
    CurTabEnum tabPress_;

    static constexpr int TOP_TAB_HEIGHT = 50;
    static constexpr int ANIMATION_DURATION = 150;
    static constexpr int WHITE_LINE_WIDTH = 18;
    static constexpr int TOP_TAB_MARGIN = 15;
    static constexpr double TAB_OPACITY_DIM = 0.5;
    static constexpr int SEARCH_BUTTON_POS_ANIMATION_DURATION = 200;
    static constexpr int FIRST_TAB_ICON_POS_X = 106;
    static constexpr int LAST_TAB_ICON_POS_X = 300;

    QElapsedTimer focusOutTimer_;
    bool searchTabSelected_; // better way to do this
    CommonWidgets::IconButtonWidget *searchButton_;
    CommonWidgets::IconButtonWidget *searchCancelButton_;
    CommonWidgets::CustomMenuLineEdit *searchLineEdit_;

    QRect rcAllLocationsIcon_;
    QRect rcConfiguredLocationsIcon_;
    QRect rcStaticIpsLocationsIcon_;
    QRect rcFavoriteLocationsIcon_;

    Qt::CursorShape curCursorShape_;
    CurTabEnum curTabMouseOver_; // TODO: use buttons for all tabs instead of mouse-over logic (should be similar to search button)

    int curWhiteLinePos_;
    QVariantAnimation whiteLineAnimation_;

    bool checkCustomConfigPathAccessRights_;

    int countOfVisibleItemSlots_;
    int currentLocationListHeight_;
    bool isRibbonVisible_;
    bool showAllTabs_;

    QColor backgroundColor_;

    void changeTab(CurTabEnum newTab);

    void onClickAllLocations();
    void onClickConfiguredLocations();
    void onClickStaticIpsLocations();
    void onClickFavoriteLocations();
    void onClickSearchLocations();

    void drawTab(QPainter &painter, const QRect &rc);
    void drawBottomLine(QPainter &painter, int left, int right, int bottom, int whiteLinePos);
    void setArrowCursor();
    void setPointingHandCursor();
    bool isWhiteAnimationActive();

    void rectHoverEnter(QRect buttonRect, QString text, int offsetX, int offsetY);

    void updateCustomConfigsEmptyListVisibility();
    void updateRibbonVisibility();

    int searchButtonPos_;
    QVariantAnimation searchButtonPosAnimation_;
    void updateTabIconRects();
    void passEventToLocationWidget(QKeyEvent *event);
};

} // namespace GuiLocations

#endif // LOCATIONSTAB_H
