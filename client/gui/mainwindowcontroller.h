#ifndef MAINWINDOWCONTROLLER_H
#define MAINWINDOWCONTROLLER_H

#include <QObject>
#include <QGraphicsView>
#include <QQueue>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include "utils/shadowmanager.h"
#include "locationswindow/locationswindow.h"
#include "loginwindow/iloginwindow.h"
#include "loginwindow/ilogginginwindow.h"
#include "loginwindow/iinitwindow.h"
#include "emergencyconnectwindow/iemergencyconnectwindow.h"
#include "connectwindow/iconnectwindow.h"
#include "preferenceswindow/ipreferenceswindow.h"
#include "overlaysconnectwindow/iupdateappitem.h"
#include "overlaysconnectwindow/iupgradewindow.h"
#include "overlaysconnectwindow/iupdatewindow.h"
#include "generalmessage/generalmessagewindowitem.h"
#include "newsfeedwindow/inewsfeedwindow.h"
#include "protocolwindow/iprotocolwindow.h"
#include "externalconfig/iexternalconfigwindow.h"
#include "twofactorauth/itwofactorauthwindow.h"
#include "bottominfowidget/ibottominfoitem.h"
#include "tooltips/tooltipcontroller.h"
#include "windowsizemanager.h"

#include "backend/preferences/preferenceshelper.h"
#include "backend/preferences/accountinfo.h"

// Helper for MainWindow. Integrates all windows, controls the display of the current window, shadows, animations and transitions between windows
class MainWindowController : public QObject
{
    Q_OBJECT
public:
    enum WINDOW_ID {
        // use this consts in changeWindow(...)
        WINDOW_ID_UNITIALIZED,
        WINDOW_ID_INITIALIZATION,
        WINDOW_ID_LOGIN,
        WINDOW_ID_LOGGING_IN,
        WINDOW_ID_CONNECT,
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

    void hideAllToolTips();

    ILoginWindow *getLoginWindow() { return loginWindow_; }
    ILoggingInWindow *getLoggingInWindow() { return loggingInWindow_; }
    IConnectWindow *getConnectWindow() { return connectWindow_; }
    IPreferencesWindow *getPreferencesWindow() { return preferencesWindow_; }
    IBottomInfoItem *getBottomInfoWindow() { return bottomInfoWindow_; }
    INewsFeedWindow *getNewsFeedWindow() { return newsFeedWindow_; }
    IProtocolWindow *getProtocolWindow() { return protocolWindow_; }
    IEmergencyConnectWindow *getEmergencyConnectWindow() { return emergencyConnectWindow_; }
    IExternalConfigWindow *getExternalConfigWindow() { return externalConfigWindow_; }
    ITwoFactorAuthWindow *getTwoFactorAuthWindow() { return twoFactorAuthWindow_; }
    IInitWindow *getInitWindow() { return initWindow_; }
    IUpdateAppItem *getUpdateAppItem() { return updateAppItem_; }
    IUpdateWindow *getUpdateWindow() { return updateWindow_; }
    IUpgradeWindow *getUpgradeWindow() { return upgradeAccountWindow_; }
    IGeneralMessageWindow *getGeneralMessageWindow() { return generalMessageWindow_; }
    IGeneralMessageWindow *getExitWindow() { return exitWindow_; }
    IGeneralMessageWindow *getLogoutWindow() { return logoutWindow_; }
    QWidget *getLocationsWindow() { return locationsWindow_; }

    void hideLocationsWindow();

    void clearServerRatingsTooltipState();

    bool eventFilter(QObject *watched, QEvent *event) override;

#ifdef Q_OS_MAC
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
    void onVanGoghAnimationProgressChanged(QVariant value);
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

    ILoginWindow *loginWindow_;
    ILoggingInWindow *loggingInWindow_;
    IInitWindow *initWindow_;
    IConnectWindow *connectWindow_;
    IEmergencyConnectWindow *emergencyConnectWindow_;
    IExternalConfigWindow *externalConfigWindow_;
    ITwoFactorAuthWindow *twoFactorAuthWindow_;
    IPreferencesWindow *preferencesWindow_;
    IUpdateWindow *updateWindow_;
    IUpgradeWindow *upgradeAccountWindow_;
    INewsFeedWindow *newsFeedWindow_;
    IProtocolWindow *protocolWindow_;
    IBottomInfoItem *bottomInfoWindow_;
    IUpdateAppItem *updateAppItem_;
    IGeneralMessageWindow *generalMessageWindow_;
    IGeneralMessageWindow *exitWindow_;
    IGeneralMessageWindow *logoutWindow_;

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
    static constexpr int BOTTOM_INFO_POS_Y_SHOWING = 287;
    static constexpr int BOTTOM_INFO_POS_Y_HIDING = 295;
    static constexpr int BOTTOM_INFO_POS_Y_VAN_GOGH = 264;
    static constexpr int UPDATE_WIDGET_HEIGHT = 28;

    bool isAtomicAnimationActive_;      // animation which cannot be interrupted is active
    QQueue<WINDOW_ID> queueWindowChanges_;

    // TODO: check for leaks
    QPropertyAnimation *expandLocationsListAnimation_;
    QPropertyAnimation *collapseBottomInfoWindowAnimation_;
    QSequentialAnimationGroup *expandLocationsAnimationGroup_;

    // function used in onExpandLocationsAnimationGroupFinished for to do some actions after animation finished
    std::function<void()> functionOnAnimationFinished_;

    QVariantAnimation vanGoghUpdateWidgetAnimation_;
    double vanGoghUpdateWidgetAnimationProgress_;

    void gotoInitializationWindow();
    void gotoLoginWindow();
    void gotoEmergencyWindow();
    void gotoLoggingInWindow();
    void gotoConnectWindow();
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

#ifdef Q_OS_MAC
    void invalidateShadow_mac_impl();
    bool isNeedUpdateNativeShadow_ = false;
#endif

#ifdef Q_OS_WIN
    enum TaskbarLocation { TASKBAR_HIDDEN, TASKBAR_BOTTOM, TASKBAR_LEFT, TASKBAR_RIGHT, TASKBAR_TOP };
    TaskbarLocation primaryScreenTaskbarLocation_win();
    QRect taskbarAwareDockedGeometry_win(int width, int shadowSize, int widthWithShadow, int heightWithShadow);
#endif

    void expandWindow(ResizableWindow *window);
    void collapseWindow(ResizableWindow *window, bool bSkipBottomInfoWindowAnimate = false);

};

#endif // MAINWINDOWCONTROLLER_H
