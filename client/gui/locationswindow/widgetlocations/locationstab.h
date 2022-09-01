#ifndef LOCATIONSTAB_H
#define LOCATIONSTAB_H

#include <QObject>
#include <QPushButton>
#include <QWidget>
#include <QVariantAnimation>
#include <QTimer>
#include <QElapsedTimer>

#include "locations/locationsmodel_manager.h"
#include "staticipdeviceinfo.h"
#include "configfooterinfo.h"
#include "commonwidgets/custommenulineedit.h"
#include "widgetswitcher.h"


namespace GuiLocations {

// switchable tabs of locations, includes widgets for all locations, favorite locations, configured locations, static IPs locations
class LocationsTab : public QWidget
{
    Q_OBJECT
public:
    explicit LocationsTab(QWidget *parent, gui_locations::LocationsModelManager *locationsModelManager);

    int unscaledHeightOfItemViewport();
    void setCountVisibleItemSlots(int cnt);
    int getCountVisibleItems();
    void setOnlyConfigTabVisible(bool onlyConfig);

    void handleKeyReleaseEvent(QKeyEvent *event);
    void handleKeyPressEvent(QKeyEvent *event);

    void updateIconRectsAndLine();
    void updateLocationWidgetsGeometry(int newHeight);
    void updateScaling();

    void updateLanguage();
    void showSearchTab();
    void hideSearchTab();
    void hideSearchTabWithoutAnimation();

    void setMuteAccentChanges(bool mute);

    enum LocationTabEnum {
        LOCATION_TAB_NONE = 0,
        LOCATION_TAB_ALL_LOCATIONS,
        LOCATION_TAB_FAVORITE_LOCATIONS,
        LOCATION_TAB_STATIC_IPS_LOCATIONS,
        LOCATION_TAB_CONFIGURED_LOCATIONS,
        LOCATION_TAB_SEARCH_LOCATIONS,
        LOCATION_TAB_FIRST = LOCATION_TAB_ALL_LOCATIONS,
        LOCATION_TAB_LAST = LOCATION_TAB_SEARCH_LOCATIONS
    };
    LocationTabEnum currentTab();

    static constexpr int TAB_HEADER_HEIGHT = 48;
    static constexpr int COVER_LAST_ITEM_LINE = 4;

public slots:
    void setLatencyDisplay(LATENCY_DISPLAY_TYPE l);
    void setCustomConfigsPath(QString path);
    void setShowLocationLoad(bool showLocationLoad);

protected:
    void paintEvent(QPaintEvent *event)        override;
    void mouseMoveEvent(QMouseEvent *event)    override;
    void mousePressEvent(QMouseEvent *event)   override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event)             override;
    bool eventFilter(QObject *object, QEvent *event) override;

signals:
    void selected(LocationID id);
    void clickedOnPremiumStarCity();
    void addStaticIpClicked();
    void clearCustomConfigClicked();
    void addCustomConfigClicked();

private slots:
    void onWhiteLinePosChanged(const QVariant &value);

    void onDeviceNameChanged(const QString &deviceName);
    void onAddCustomConfigClicked();

    void onSearchButtonHoverEnter();
    void onSearchButtonHoverLeave();
    void onSearchButtonClicked();
    void onSearchCancelButtonClicked();
    void onSearchButtonPosAnimationValueChanged(const QVariant &value);

    void onDelayShowIconsTimerTimeout();
    void onSearchTypingDelayTimerTimeout();
    void onSearchLineEditTextChanged(QString text);
    void onSearchLineEditFocusOut();
private:

    WidgetSwitcher *widgetAllLocations_;
    WidgetSwitcher *widgetConfiguredLocations_;
    WidgetSwitcher *widgetStaticIpsLocations_;
    WidgetSwitcher *widgetFavoriteLocations_;

    // ribbons
    StaticIPDeviceInfo *staticIPDeviceInfo_;
    ConfigFooterInfo *configFooterInfo_;

    //Backend &backend_;
    LocationTabEnum curTab_;
    LocationTabEnum lastTab_;
    LocationTabEnum tabPress_;

    static constexpr int RIBBON_HEIGHT = 50;
    static constexpr int ANIMATION_DURATION = 150;
    static constexpr int WHITE_LINE_WIDTH = 18;
    static constexpr int TOP_TAB_MARGIN = 15;
    static constexpr double TAB_OPACITY_DIM = 0.5;
    static constexpr int SEARCH_BUTTON_POS_ANIMATION_DURATION = 200;
    static constexpr int FIRST_TAB_ICON_POS_X = 106;
    static constexpr int LAST_TAB_ICON_POS_X = 300;

    QString filterText_;
    QTimer searchTypingDelayTimer_; // saves some drawing cycles for fast typers -- most significant slowdown should be fixed by hide/show widgets instead of create/destroy (as it does now)
    QElapsedTimer focusOutTimer_;
    bool searchTabSelected_; // better way to do this
    CommonWidgets::IconButtonWidget *searchButton_;
    CommonWidgets::IconButtonWidget *searchCancelButton_;
    CommonWidgets::CustomMenuLineEdit *searchLineEdit_;
    QTimer delayShowIconsTimer_;

    QRect rcAllLocationsIcon_;
    QRect rcConfiguredLocationsIcon_;
    QRect rcStaticIpsLocationsIcon_;
    QRect rcFavoriteLocationsIcon_;

    Qt::CursorShape curCursorShape_;
    LocationTabEnum curTabMouseOver_;

    int curWhiteLinePos_;
    QVariantAnimation whiteLineAnimation_;

    int countOfVisibleItemSlots_;
    int currentLocationListHeight_;
    bool isRibbonVisible_;
    bool showAllTabs_;

    QColor tabBackgroundColor_;

    void changeTab(LocationTabEnum newTab, bool animateChange = true);

    void onClickAllLocations();
    void onClickConfiguredLocations();
    void onClickStaticIpsLocations();
    void onClickFavoriteLocations();
    void onClickSearchLocations();

    void switchToTabAndRestoreCursorToAccentedItem(LocationTabEnum locationTab);

    void drawTabRegion(QPainter &painter, const QRect &rc);
    void drawBottomLine(QPainter &painter, int bottom, int whiteLinePos);
    void setArrowCursor();
    void setPointingHandCursor();
    bool isWhiteAnimationActive();

    void rectHoverEnter(QRect buttonRect, QString text, int offsetX, int offsetY);

    void updateCustomConfigsEmptyListVisibility();
    void updateRibbonVisibility();

    int searchButtonPosUnscaled_;
    QVariantAnimation searchButtonPosAnimation_;
    void updateTabIconRects();
    void passEventToLocationWidget(QKeyEvent *event);
};

} // namespace GuiLocations

#endif // LOCATIONSTAB_H
