#pragma once

#include <QObject>
#include <QGraphicsView>
#include <QQueue>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>

#include "backend/preferences/preferenceshelper.h"
#include "backend/preferences/accountinfo.h"
#include "bottominfowidget/bottominfoitem.h"
#include "connectwindow/connectwindowitem.h"
#include "emergencyconnectwindow/emergencyconnectwindowitem.h"
#include "externalconfig/externalconfigwindowitem.h"
#include "generalmessage/generalmessagewindowitem.h"
#include "locationswindow/locationswindow.h"
#include "loginwindow/logginginwindowitem.h"
#include "loginwindow/loginwindowitem.h"
#include "loginwindow/initwindowitem.h"
#include "newsfeedwindow/newsfeedwindowitem.h"
#include "overlaysconnectwindow/updateappitem.h"
#include "overlaysconnectwindow/upgradewindowitem.h"
#include "overlaysconnectwindow/updatewindowitem.h"
#include "preferenceswindow/preferenceswindowitem.h"
#include "protocolwindow/protocolwindowitem.h"
#include "twofactorauth/twofactorauthwindowitem.h"
#include "tooltips/tooltipcontroller.h"
#include "utils/shadowmanager.h"
#include "windowsizemanager.h"

// Helper for MainWindow. Integrates all windows, controls the display of the current window, shadows, animations and transitions between windows
class MainWindowController : public QObject
{
    Q_OBJECT
public:
    enum WINDOW_ID {
        // use this consts in changeWindow(...)
        WINDOW_ID_UNINITIALIZED,
        WINDOW_ID_INITIALIZATION,
        WINDOW_ID_LOGIN,
        WINDOW_ID_LOGGING_IN,
        WINDOW_ID_CONNECT,
        WINDOW_ID_CONNECT_PREFERENCES,
        WINDOW_ID_EMERGENCY,
        WINDOW_ID_EXTERNAL_CONFIG,
        WINDOW_ID_TWO_FACTOR_AUTH,
        WINDOW_ID_UPDATE,
        WINDOW_ID_UPGRADE,
        WINDOW_ID_GENERAL_MESSAGE,
        WINDOW_ID_EXIT,
        WINDOW_ID_LOGOUT,
        WINDOW_CMD_CLOSE_EXIT,
        WINDOW_CMD_CLOSE_EXIT_FROM_PREFS,
        // internal states
        WINDOW_CMD_UPDATE_BOTTOM_INFO
    };

    explicit MainWindowController(QWidget *parent, LocationsWindow *locationsWindow, PreferencesHelper *preferencesHelper,
                                  Preferences *preferences, AccountInfo *accountInfo);

    void updateScaling();
    void updateMaskForGraphicsView();
    void updateMainAndViewGeometry(bool updateShadow);

    void setWindowPosFromPersistent();
    void setIsDockedToTray(bool isDocked);

    bool isPreferencesVisible();
    bool isNewsFeedVisible();
    WINDOW_ID currentWindow();
    WINDOW_ID currentWindowAfterAnimation();
    WINDOW_ID windowBeforeExit();
    void changeWindow(WINDOW_ID windowId);

    void expandLocations();
    void collapseLocations();
    bool isLocationsExpanded() const;

    void expandPreferences();
    void collapsePreferences();
    void expandNewsFeed();
    void collapseNewsFeed();
    void expandProtocols(ProtocolWindowMode mode = ProtocolWindowMode::kUninitialized);
    void collapseProtocols();

    void showUpdateWidget();
    void hideUpdateWidget();

    QWidget *getViewport();

    QPixmap &getCurrentShadowPixmap();
    bool isNeedShadow();
    int getShadowMargin();

    void updateMaximumHeight();

    void hideAllToolTips();

    LoginWindow::LoginWindowItem *getLoginWindow() { return loginWindow_; }
    LoginWindow::LoggingInWindowItem *getLoggingInWindow() { return loggingInWindow_; }
    ConnectWindow::ConnectWindowItem *getConnectWindow() { return connectWindow_; }
    PreferencesWindow::PreferencesWindowItem *getPreferencesWindow() { return preferencesWindow_; }
    SharingFeatures::BottomInfoItem *getBottomInfoWindow() { return bottomInfoWindow_; }
    NewsFeedWindow::NewsFeedWindowItem *getNewsFeedWindow() { return newsFeedWindow_; }
    ProtocolWindow::ProtocolWindowItem *getProtocolWindow() { return protocolWindow_; }
    EmergencyConnectWindow::EmergencyConnectWindowItem *getEmergencyConnectWindow() { return emergencyConnectWindow_; }
    ExternalConfigWindow::ExternalConfigWindowItem *getExternalConfigWindow() { return externalConfigWindow_; }
    TwoFactorAuthWindow::TwoFactorAuthWindowItem *getTwoFactorAuthWindow() { return twoFactorAuthWindow_; }
    LoginWindow::InitWindowItem *getInitWindow() { return initWindow_; }
    UpdateApp::UpdateAppItem *getUpdateAppItem() { return updateAppItem_; }
    UpdateWindowItem *getUpdateWindow() { return updateWindow_; }
    UpgradeWindow::UpgradeWindowItem *getUpgradeWindow() { return upgradeAccountWindow_; }
    GeneralMessageWindow::GeneralMessageWindowItem *getGeneralMessageWindow() { return generalMessageWindow_; }
    GeneralMessageWindow::GeneralMessageWindowItem *getExitWindow() { return exitWindow_; }
    GeneralMessageWindow::GeneralMessageWindowItem *getLogoutWindow() { return logoutWindow_; }
    LocationsWindow *getLocationsWindow() { return locationsWindow_; }

    void hideLocationsWindow();

    void clearServerRatingsTooltipState();

    bool eventFilter(QObject *watched, QEvent *event) override;

#ifdef Q_OS_MACOS
    void updateNativeShadowIfNeeded();
#endif

signals:
    void shadowUpdated();
    void revealConnectWindowStateChanged(bool revealing);
    void preferencesCollapsed();

    void sendServerRatingUp();
    void sendServerRatingDown();

private slots:
    void onExpandLocationsListAnimationFinished();
    void onExpandLocationsListAnimationValueChanged(const QVariant &value);
    void onExpandLocationsListAnimationStateChanged(QAbstractAnimation::State newState, QAbstractAnimation::State oldState);

    void onExpandLocationsAnimationGroupFinished();
    void onRevealConnectAnimationGroupFinished();

    void onCollapseBottomInfoWindowAnimationValueChanged(const QVariant &value);

    void onLocationsWindowHeightChanged();

    void onWindowResize(ResizableWindow *window);
    void onWindowResizeFinished(ResizableWindow *window);

    void onBottomInfoHeightChanged();
    void onBottomInfoPosChanged();

    void onTooltipControllerSendServerRatingUp();
    void onTooltipControllerSendServerRatingDown();

    void onAppSkinChanged(APP_SKIN s);
    void onUpdateWidgetAnimationProgressChanged(QVariant value);
    void onVanGoghAnimationFinished();
    void onLanguageChanged();

private:
    WINDOW_ID curWindow_;
    WINDOW_ID windowBeforeExit_; // here save prev window before show exit window

    QWidget *mainWindow_;
    ShadowManager *shadowManager_;
    WindowSizeManager *windowSizeManager_;
    Preferences *preferences_;
    PreferencesHelper *preferencesHelper_;

    QGraphicsView *view_;
    QGraphicsScene* scene_;

    LoginWindow::LoginWindowItem *loginWindow_;
    LoginWindow::LoggingInWindowItem *loggingInWindow_;
    LoginWindow::InitWindowItem *initWindow_;
    ConnectWindow::ConnectWindowItem *connectWindow_;
    EmergencyConnectWindow::EmergencyConnectWindowItem *emergencyConnectWindow_;
    ExternalConfigWindow::ExternalConfigWindowItem *externalConfigWindow_;
    TwoFactorAuthWindow::TwoFactorAuthWindowItem *twoFactorAuthWindow_;
    PreferencesWindow::PreferencesWindowItem *preferencesWindow_;
    UpdateWindowItem *updateWindow_;
    UpgradeWindow::UpgradeWindowItem *upgradeAccountWindow_;
    NewsFeedWindow::NewsFeedWindowItem *newsFeedWindow_;
    ProtocolWindow::ProtocolWindowItem *protocolWindow_;
    SharingFeatures::BottomInfoItem *bottomInfoWindow_;
    UpdateApp::UpdateAppItem *updateAppItem_;
    GeneralMessageWindow::GeneralMessageWindowItem *generalMessageWindow_;
    GeneralMessageWindow::GeneralMessageWindowItem *exitWindow_;
    GeneralMessageWindow::GeneralMessageWindowItem *logoutWindow_;

    LocationsWindow *locationsWindow_;

    const char *kQuitTitle = QT_TR_NOOP("Quit Windscribe?");
    const char *kLogOutTitle = QT_TR_NOOP("Log Out of Windscribe?");
    const char *kQuit = QT_TR_NOOP("Quit");
    const char *kLogOut = QT_TR_NOOP("Log Out");
    const char *kCancel = QT_TR_NOOP("Cancel");

    static constexpr int LOCATIONS_WINDOW_TOP_OFFS = 27;
    static constexpr int LOCATIONS_WINDOW_WIDTH = WINDOW_WIDTH;
    static constexpr int LOCATIONS_WINDOW_HEIGHT = 417;

    enum LOCATION_LIST_ANIMATION_STATE { LOCATION_LIST_ANIMATION_COLLAPSED, LOCATION_LIST_ANIMATION_EXPANDED, LOCATION_LIST_ANIMATION_EXPANDING, LOCATION_LIST_ANIMATION_COLLAPSING };
    LOCATION_LIST_ANIMATION_STATE locationListAnimationState_;

    static constexpr int REVEAL_CONNECT_ANIMATION_DURATION = 300;
    static constexpr int REVEAL_LOGIN_ANIMATION_DURATION = 300;
    static constexpr int SCREEN_SWITCH_OPACITY_ANIMATION_DURATION = 150;
    static constexpr int EXPAND_CHILD_WINDOW_OPACITY_DURATION = 200;
    static constexpr int EXPAND_PREFERENCES_RESIZE_DURATION = 250;
    static constexpr int EXPAND_WINDOW_RESIZE_DURATION = 200;
    static constexpr int EXPAND_ANIMATION_DURATION = 250;
    static constexpr int HIDE_BOTTOM_INFO_ANIMATION_DURATION = 150;
    static constexpr int BOTTOM_INFO_POS_Y_SHOWING = 260;
    static constexpr int BOTTOM_INFO_POS_Y_HIDING = 260;
    static constexpr int BOTTOM_INFO_POS_Y_VAN_GOGH = 244;
    static constexpr int UPDATE_WIDGET_HEIGHT = 24;

    bool isAtomicAnimationActive_;      // animation which cannot be interrupted is active
    QQueue<WINDOW_ID> queueWindowChanges_;

    // TODO: check for leaks
    QPropertyAnimation *expandLocationsListAnimation_;
    QPropertyAnimation *collapseBottomInfoWindowAnimation_;
    QSequentialAnimationGroup *expandLocationsAnimationGroup_;

    // function used in onExpandLocationsAnimationGroupFinished for to do some actions after animation finished
    std::function<void()> functionOnAnimationFinished_;

    QVariantAnimation updateWidgetAnimation_;
    double updateWidgetAnimationProgress_;

    void gotoInitializationWindow();
    void gotoLoginWindow();
    void gotoEmergencyWindow();
    void gotoLoggingInWindow();
    void gotoConnectWindow(bool expandPrefs);
    void gotoNotificationsWindow();
    void gotoExternalConfigWindow();
    void gotoTwoFactorAuthWindow();
    void gotoUpdateWindow();
    void gotoUpgradeWindow();
    void gotoGeneralMessageWindow();
    void gotoExitWindow(bool isLogout);
    void closeExitWindow(bool fromPrefs);

    void expandPreferencesFromLogin();
    void collapsePreferencesFromLogin();

    void collapseAllExpandedOnBottom();

    void animateBottomInfoWindow(QAbstractAnimation::Direction direction, std::function<void()> finished_function = nullptr);
    QPropertyAnimation *getAnimateBottomInfoWindowAnimation(QAbstractAnimation::Direction direction, std::function<void()> finished_function = nullptr);

    void handleNextWindowChange();

    void updateBottomInfoWindowVisibilityAndPos(bool forceCollapsed = false);
    QPoint getCoordsOfBottomInfoWindow(bool isBottomInfoWindowCollapsed) const;
    bool isBottomInfoCollapsed() const;

    void updateCursorInViewport();

    void centerMainGeometryAndUpdateView();
    void updateViewAndScene(int width, int height, int shadowSize, bool updateShadow);
    void updateLocationsWindowAndTabGeometryStatic();

    void invalidateShadow_mac();
    void setMaskForGraphicsView();
    void clearMaskForGraphicsView();

    int locationWindowHeightScaled_; // Previously there were issues dynamically grabbing locationsWindow height... keeping a cache somehow helped. Not sure if the original issue persists

    void keepWindowInsideScreenCoordinates();

    void getGraphicsRegionWidthAndHeight(int &width, int &height, int &addHeightToGeometry);

    void updateLocationsWindowAndTabGeometry();
    void updateExpandAnimationParameters();

    bool shouldShowConnectBackground();

    qreal locationsShadowOpacity_;

    int childWindowShadowOffsetY(bool withVanGoghOffset);
    int initWindowInitHeight_;

    int locationsYOffset();

#ifdef Q_OS_MACOS
    void invalidateShadow_mac_impl();
    bool isNeedUpdateNativeShadow_ = false;
#endif

#ifdef Q_OS_WIN
    enum TaskbarLocation { TASKBAR_HIDDEN, TASKBAR_BOTTOM, TASKBAR_LEFT, TASKBAR_RIGHT, TASKBAR_TOP };
    TaskbarLocation primaryScreenTaskbarLocation_win();
    QRect taskbarAwareDockedGeometry_win(int width, int shadowSize, int widthWithShadow, int heightWithShadow);
#endif

    void expandWindow(ResizableWindow *window);
    void collapseWindow(ResizableWindow *window, bool bSkipBottomInfoWindowAnimate = false, bool bSkipSetClickable = false);

    int getUpdateWidgetOffset() const;
};
