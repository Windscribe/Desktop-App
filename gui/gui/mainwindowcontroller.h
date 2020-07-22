#ifndef MAINWINDOWCONTROLLER_H
#define MAINWINDOWCONTROLLER_H

#include <QObject>
#include <QGraphicsView>
#include <QQueue>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include "Utils/shadowmanager.h"
#include "LocationsWindow/locationswindow.h"
#include "LoginWindow/iloginwindow.h"
#include "LoginWindow/ilogginginwindow.h"
#include "LoginWindow/iinitwindow.h"
#include "EmergencyConnectWindow/iemergencyconnectwindow.h"
#include "ConnectWindow/iconnectwindow.h"
#include "PreferencesWindow/ipreferenceswindow.h"
#include "UpdateApp/iupdateappitem.h"
#include "NewsFeedWindow/inewsfeedwindow.h"
#include "Update/iupdatewindow.h"
#include "ExternalConfig/iexternalconfigwindow.h"
#include "BottomInfoWidget/ibottominfoitem.h"
#include "GeneralMessage/igeneralmessagewindow.h"
#include "GeneralMessage/igeneralmessagetwobuttonwindow.h"
#include "Tooltips/tooltipcontroller.h"

#include "../Backend/Preferences/preferenceshelper.h"
#include "../Backend/Preferences/accountinfo.h"

// Helper for MainWindow. Integrates all windows, controls the display of the current window, shadows, animations and transitions between windows
class MainWindowController : public QObject
{
    Q_OBJECT
public:
    enum WINDOW_ID { // use this consts in changeWindow(...)
                     WINDOW_ID_UNITIALIZED, WINDOW_ID_INITIALIZATION, WINDOW_ID_LOGIN, WINDOW_ID_LOGGING_IN,
                     WINDOW_ID_CONNECT, WINDOW_ID_EMERGENCY, WINDOW_ID_EXTERNAL_CONFIG, WINDOW_ID_NOTIFICATIONS,
                     WINDOW_ID_UPDATE, WINDOW_ID_UPGRADE, WINDOW_ID_GENERAL_MESSAGE, WINDOW_ID_EXIT, WINDOW_CMD_CLOSE_EXIT,
                     // internal states
                     WINDOW_CMD_UPDATE_BOTTOM_INFO};

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
    IInitWindow *getInitWindow() { return initWindow_; }
    IUpdateAppItem *getUpdateAppItem() { return updateAppItem_; }
    IUpdateWindow *getUpdateWindow() { return updateWindow_; }
    IUpdateWindow *getUpgradeWindow() { return upgradeAccountWindow_; }
    IGeneralMessageWindow *getGeneralMessageWindow() { return generalMessageWindow_; }
    IGeneralMessageTwoButtonWindow *getExitWindow() { return exitWindow_; }

    void handleKeyReleaseEvent(QKeyEvent *event);

    void hideLocationsWindow();

    void clearServerRatingsTooltipState();

signals:
    void shadowUpdated();
    void revealConnectWindowStateChanged(bool revealing);

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

    void onPreferencesWindowResize();
    void onPreferencesWindowResizeFinished();

    void onBottomInfoHeightChanged();
    void onBottomInfoPosChanged();

    void onChildWindowShowTooltip(TooltipInfo info);
    void onChildWindowHideTooltip(TooltipId type);

    void onTooltipControllerSendServerRatingUp();
    void onTooltipControllerSendServerRatingDown();

private:
    WINDOW_ID curWindow_;
    WINDOW_ID windowBeforeExit_; // here save prev window before show exit window

    QWidget *mainWindow_;
    ShadowManager *shadowManager_;

    QGraphicsView *view_;
    QGraphicsScene* scene_;

    ILoginWindow *loginWindow_;
    ILoggingInWindow *loggingInWindow_;
    IInitWindow *initWindow_;
    IConnectWindow *connectWindow_;
    IEmergencyConnectWindow *emergencyConnectWindow_;
    IExternalConfigWindow *externalConfigWindow_;
    IPreferencesWindow *preferencesWindow_;
    IUpdateWindow *updateWindow_;
    IUpdateWindow *upgradeAccountWindow_;
    INewsFeedWindow *newsFeedWindow_;
    IBottomInfoItem *bottomInfoWindow_;
    IGeneralMessageWindow *generalMessageWindow_;
    IUpdateAppItem *updateAppItem_;
    IGeneralMessageTwoButtonWindow *exitWindow_;

    TooltipController *tooltipController_;

    LocationsWindow *locationsWindow_;

    QString CLOSING_WINDSCRIBE;
    QString CLOSE_ACCEPT;
    QString CLOSE_REJECT;

    const int LOCATIONS_WINDOW_TOP_OFFS = 27;
    const int LOCATIONS_WINDOW_WIDTH = WINDOW_WIDTH;
    const int LOCATIONS_WINDOW_HEIGHT = 417;

    enum LOCATION_LIST_ANIMATION_STATE { LOCATION_LIST_ANIMATION_COLLAPSED, LOCATION_LIST_ANIMATION_EXPANDED, LOCATION_LIST_ANIMATION_EXPANDING, LOCATION_LIST_ANIMATION_COLLAPSING };
    LOCATION_LIST_ANIMATION_STATE locationListAnimationState_;

    const int REVEAL_CONNECT_ANIMATION_DURATION = 300;
    const int REVEAL_LOGIN_ANIMATION_DURATION = 300;
    const int SCREEN_SWITCH_OPACITY_ANIMATION_DURATION = 150;
    const int EXPAND_PREFERENCES_OPACITY_DURATION = 200;
    const int EXPAND_PREFERENCES_RESIZE_DURATION = 250;
    //const int EXPAND_PREFERENCES_OPACITY_DURATION = 2000;
    //const int EXPAND_PREFERENCES_RESIZE_DURATION = 2500;
    const int EXPAND_ANIMATION_DURATION = 250;
    const int HIDE_BOTTOM_INFO_ANIMATION_DURATION = 150;
    const int BOTTOM_INFO_POS_Y_SHOWING = 287;
    const int BOTTOM_INFO_POS_Y_HIDING = 295;

    enum PREFERENCES_STATE { PREFERENCES_STATE_EXPANDED, PREFERENCES_STATE_COLLAPSED, PREFERENCES_STATE_ANIMATING };
    PREFERENCES_STATE preferencesState_;
    int preferencesWindowHeight_;

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

    int lastPreferencesHeight_;
    int locationWindowHeightScaled_; //  TODO: use should be replaced by direct call to current size
    void keepWindowInsideScreenCoordinates();

    void getGraphicsRegionWidthAndHeight(int &width, int &height, int &addHeightToGeometry);

    void updateLocationsWindowAndTabGeometry();
    void updateExpandAnimationParameters();

    bool shouldShowConnectBackground();

    qreal locationsShadowOpacity_;

    int preferencesShadowOffsetY();
    int initWindowInitHeight_;

    bool isDocked_;
};

#endif // MAINWINDOWCONTROLLER_H
