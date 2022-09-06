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
#include "overlaysconnectwindow/igeneralmessagewindow.h"
#include "overlaysconnectwindow/igeneralmessagetwobuttonwindow.h"
#include "newsfeedwindow/inewsfeedwindow.h"
#include "externalconfig/iexternalconfigwindow.h"
#include "twofactorauth/itwofactorauthwindow.h"
#include "bottominfowidget/ibottominfoitem.h"
#include "tooltips/tooltipcontroller.h"

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
        WINDOW_CMD_CLOSE_EXIT,
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

    bool preferencesVisible();
    WINDOW_ID currentWindow();
    void changeWindow(WINDOW_ID windowId);

    void expandLocations();
    void collapseLocations();
    bool isLocationsExpanded() const;

    void expandPreferences();
    void collapsePreferences();

    void expandNewsFeed();
    void collapseNewsFeed();

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
    IEmergencyConnectWindow *getEmergencyConnectWindow() { return emergencyConnectWindow_; }
    IExternalConfigWindow *getExternalConfigWindow() { return externalConfigWindow_; }
    ITwoFactorAuthWindow *getTwoFactorAuthWindow() { return twoFactorAuthWindow_; }
    IInitWindow *getInitWindow() { return initWindow_; }
    IUpdateAppItem *getUpdateAppItem() { return updateAppItem_; }
    IUpdateWindow *getUpdateWindow() { return updateWindow_; }
    IUpgradeWindow *getUpgradeWindow() { return upgradeAccountWindow_; }
    IGeneralMessageWindow *getGeneralMessageWindow() { return generalMessageWindow_; }
    IGeneralMessageTwoButtonWindow *getExitWindow() { return exitWindow_; }
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

    void onPreferencesResize();
    void onPreferencesResizeFinished();
    void onNewsFeedResize();
    void onNewsFeedResizeFinished();

    void onBottomInfoHeightChanged();
    void onBottomInfoPosChanged();

    void onTooltipControllerSendServerRatingUp();
    void onTooltipControllerSendServerRatingDown();

private:
    WINDOW_ID curWindow_;
    WINDOW_ID windowBeforeExit_; // here save prev window before show exit window

    QWidget *mainWindow_;
    ShadowManager *shadowManager_;
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
    IBottomInfoItem *bottomInfoWindow_;
    IGeneralMessageWindow *generalMessageWindow_;
    IUpdateAppItem *updateAppItem_;
    IGeneralMessageTwoButtonWindow *exitWindow_;

    LocationsWindow *locationsWindow_;

    QString CLOSING_WINDSCRIBE;
    QString CLOSE_ACCEPT;
    QString CLOSE_REJECT;

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
    static constexpr int EXPAND_NEWS_FEED_RESIZE_DURATION = 200;
    static constexpr int EXPAND_ANIMATION_DURATION = 250;
    static constexpr int HIDE_BOTTOM_INFO_ANIMATION_DURATION = 150;
    static constexpr int BOTTOM_INFO_POS_Y_SHOWING = 287;
    static constexpr int BOTTOM_INFO_POS_Y_HIDING = 295;

    enum CHILD_WINDOW_STATE { CHILD_WINDOW_STATE_EXPANDED, CHILD_WINDOW_STATE_COLLAPSED, CHILD_WINDOW_STATE_ANIMATING };
    CHILD_WINDOW_STATE preferencesState_;
    CHILD_WINDOW_STATE newsFeedState_;
    int preferencesWindowHeight_;
    int newsFeedWindowHeight_;

    bool isAtomicAnimationActive_;      // animation which cannot be interrupted is active
    QQueue<WINDOW_ID> queueWindowChanges_;

    // TODO: check for leaks
    QPropertyAnimation *expandLocationsListAnimation_;
    QPropertyAnimation *collapseBottomInfoWindowAnimation_;
    QSequentialAnimationGroup *expandLocationsAnimationGroup_;

    // function used in onExpandLocationsAnimationGroupFinished for to do some actions after animation finished
    std::function<void()> functionOnAnimationFinished_;

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
    void gotoExitWindow();
    void closeExitWindow();

    void expandPreferencesFromLogin();
    void expandPreferencesFromConnect();

    void collapsePreferencesFromLogin();
    void collapsePreferencesFromConnect(bool bSkipBottomInfoWindowAnimate);

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

    int lastPreferencesWindowHeight_;
    int lastNewsFeedWindowHeight_;
    int locationWindowHeightScaled_; // Previously there were issues dynamically grabbing locationsWindow height... keeping a cache somehow helped. Not sure if the original issue persists

    void keepWindowInsideScreenCoordinates();

    void getGraphicsRegionWidthAndHeight(int &width, int &height, int &addHeightToGeometry);

    void updateLocationsWindowAndTabGeometry();
    void updateExpandAnimationParameters();

    bool shouldShowConnectBackground();

    qreal locationsShadowOpacity_;

    int childWindowShadowOffsetY();
    int initWindowInitHeight_;

#ifdef Q_OS_MAC
    void invalidateShadow_mac_impl();
    bool isNeedUpdateNativeShadow_ = false;
#endif

#ifdef Q_OS_WIN
    enum TaskbarLocation { TASKBAR_HIDDEN, TASKBAR_BOTTOM, TASKBAR_LEFT, TASKBAR_RIGHT, TASKBAR_TOP };
    TaskbarLocation primaryScreenTaskbarLocation_win();
    QRect taskbarAwareDockedGeometry_win(int width, int shadowSize, int widthWithShadow, int heightWithShadow);
#endif
};

#endif // MAINWINDOWCONTROLLER_H
