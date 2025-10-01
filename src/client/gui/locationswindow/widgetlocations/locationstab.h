#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QPushButton>
#include <QTimer>
#include <QVariantAnimation>
#include <QWidget>

#include "backend/preferences/preferences.h"
#include "commonwidgets/custommenulineedit.h"
#include "commonwidgets/textbuttonwidget.h"
#include "configfooterinfo.h"
#include "locations/locationsmodel_manager.h"
#include "staticipdeviceinfo.h"
#include "types/connectstate.h"
#include "upgradebanner.h"
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

    void updateLocationWidgetsGeometry(int newHeight);
    void updateScaling();

    int headerHeight() const;
    static constexpr int COVER_LAST_ITEM_LINE = 4;

    void setDataRemaining(qint64 bytesUsed, qint64 bytesMax);

public slots:
    void setCustomConfigsPath(QString path);
    void setShowLocationLoad(bool showLocationLoad);
    void onConnectStateChanged(const types::ConnectState &connectState);
    void onPingsStarted();
    void onPingsFinished();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

signals:
    void selected(LocationID id);
    void clickedOnPremiumStarCity();
    void addStaticIpClicked();
    void clearCustomConfigClicked();
    void addCustomConfigClicked();
    void upgradeBannerClicked();
    void refreshClicked();

public slots:
    void onClickAllLocations();
    void onClickConfiguredLocations();
    void onClickStaticIpsLocations();
    void onClickFavoriteLocations();
    void onClickSearchLocations();
    void onSearchFilterChanged(const QString &filter);
    void onLocationsKeyPressed(QKeyEvent *event);

private slots:
    void onDeviceNameChanged(const QString &deviceName);
    void onAddCustomConfigClicked();
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

    QString refreshButtonText_;
    QRect refreshButtonRect_;
    QVariantAnimation *refreshButtonColorAnimation_;
    QColor refreshButtonColor_;
    double refreshButtonAnimationProgress_;
    bool pingsActive_;
    QVariantAnimation *spinnerRotationAnimation_;
    double currentSpinnerRotation_;

    // ribbons
    StaticIPDeviceInfo *staticIPDeviceInfo_;
    ConfigFooterInfo *configFooterInfo_;
    UpgradeBanner *upgradeBanner_;

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

    CONNECT_STATE connectState_;
    int countOfVisibleItemSlots_;
    int currentLocationListHeight_;
    bool isRibbonVisible_;
    bool showAllTabs_;
    bool isUnlimitedData_;

    QColor tabBackgroundColor_;

    void updateCustomConfigsEmptyListVisibility();
    void updateRibbonVisibility();

    WidgetSwitcher *getCurrentWidget() const;
    bool isRefreshButtonVisible() const;
};

} // namespace GuiLocations
