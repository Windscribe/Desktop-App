#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QPushButton>
#include <QTimer>
#include <QVariantAnimation>
#include <QWidget>

#include "backend/preferences/preferences.h"
#include "commonwidgets/custommenulineedit.h"
#include "configfooterinfo.h"
#include "locations/locationsmodel_manager.h"
#include "staticipdeviceinfo.h"
#include "widgetswitcher.h"


namespace GuiLocations {

// switchable tabs of locations, includes widgets for all locations, favorite locations, configured locations, static IPs locations
class LocationsTab : public QWidget
{
    Q_OBJECT
public:
    explicit LocationsTab(QWidget *parent, Preferences *preferences, gui_locations::LocationsModelManager *locationsModelManager);

    int unscaledHeightOfItemViewport();
    void setCountVisibleItemSlots(int cnt);
    int getCountVisibleItems();
    void setOnlyConfigTabVisible(bool onlyConfig);

    void updateIconRectsAndLine();
    void updateLocationWidgetsGeometry(int newHeight);
    void updateScaling();

    void updateLanguage();
    void showSearchTab();
    void hideSearchTab();
    void hideSearchTabWithoutAnimation();

    bool handleKeyPressEvent(QKeyEvent *event);

    LOCATION_TAB currentTab();

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
    void onAppSkinChanged(APP_SKIN s);
    void onLocationSelected(const LocationID &lid);
    void onClickedOnPremiumStarCity();

    void onLanguageChanged();

private:
    Preferences *preferences_;
    gui_locations::LocationsModelManager *locationsModelManager_;

    WidgetSwitcher *widgetAllLocations_;
    WidgetSwitcher *widgetConfiguredLocations_;
    WidgetSwitcher *widgetStaticIpsLocations_;
    WidgetSwitcher *widgetFavoriteLocations_;
    WidgetSwitcher *widgetSearchLocations_;

    // ribbons
    StaticIPDeviceInfo *staticIPDeviceInfo_;
    ConfigFooterInfo *configFooterInfo_;

    LOCATION_TAB curTab_;
    LOCATION_TAB lastTab_;
    LOCATION_TAB tabPress_;

    static constexpr int RIBBON_HEIGHT = 50;
    static constexpr int ANIMATION_DURATION = 150;
    static constexpr int WHITE_LINE_WIDTH = 18;
    static constexpr int TOP_TAB_MARGIN = 15;
    static constexpr double TAB_OPACITY_DIM = 0.5;
    static constexpr int SEARCH_BUTTON_POS_ANIMATION_DURATION = 200;
    static constexpr int FIRST_TAB_ICON_POS_X_VAN_GOGH = 24;
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
    LOCATION_TAB curTabMouseOver_;

    int curWhiteLinePos_;
    QVariantAnimation whiteLineAnimation_;

    int countOfVisibleItemSlots_;
    int currentLocationListHeight_;
    bool isRibbonVisible_;
    bool showAllTabs_;

    QColor tabBackgroundColor_;

    void changeTab(LOCATION_TAB newTab, bool animateChange = true);

    void onClickAllLocations();
    void onClickConfiguredLocations();
    void onClickStaticIpsLocations();
    void onClickFavoriteLocations();
    void onClickSearchLocations();

    void switchToTabAndRestoreCursorToAccentedItem(LOCATION_TAB locationTab);

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
    WidgetSwitcher *getCurrentWidget() const;
};

} // namespace GuiLocations
