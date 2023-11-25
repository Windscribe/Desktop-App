#include "mainwindowcontroller.h"

#include <QApplication>
#include <QSequentialAnimationGroup>
#include <QScreen>
#include <QWindow>

#include "loginwindow/loginwindowitem.h"
#include "loginwindow/logginginwindowitem.h"
#include "loginwindow/initwindowitem.h"
#include "emergencyconnectwindow/emergencyconnectwindowitem.h"
#include "externalconfig/externalconfigwindowitem.h"
#include "twofactorauth/twofactorauthwindowitem.h"
#include "connectwindow/connectwindowitem.h"
#include "graphicresources/fontmanager.h"
#include "preferenceswindow/preferenceswindowitem.h"
#include "overlaysconnectwindow/updateappitem.h"
#include "overlaysconnectwindow/updatewindowitem.h"
#include "overlaysconnectwindow/upgradewindowitem.h"
#include "bottominfowidget/bottominfoitem.h"
#include "newsfeedwindow/newsfeedwindowitem.h"
#include "protocolwindow/protocolwindowitem.h"
#include "utils/ws_assert.h"
#include "utils/utils.h"
#include "languagecontroller.h"
#include "mainwindow.h"
#include "commongraphics/commongraphics.h"
#include "backend/persistentstate.h"
#include "dpiscalemanager.h"
#include "utils/logger.h"
#include "widgetutils/widgetutils.h"

#ifdef Q_OS_MAC
    #include "utils/macutils.h"
#endif

MainWindowController::MainWindowController(QWidget *parent, LocationsWindow *locationsWindow, PreferencesHelper *preferencesHelper,
                                           Preferences *preferences, AccountInfo *accountInfo) : QObject(parent),
    curWindow_(WINDOW_ID_UNINITIALIZED),
    mainWindow_(parent),
    preferences_(preferences),
    preferencesHelper_(preferencesHelper),
    locationsWindow_(locationsWindow),
    locationListAnimationState_(LOCATION_LIST_ANIMATION_COLLAPSED),
    isAtomicAnimationActive_(false),
    expandLocationsListAnimation_(NULL),
    collapseBottomInfoWindowAnimation_(NULL),
    expandLocationsAnimationGroup_(NULL),
    locationWindowHeightScaled_(0),
    locationsShadowOpacity_(0.0),
    initWindowInitHeight_(WINDOW_HEIGHT),
    vanGoghUpdateWidgetAnimationProgress_(0)
{
#ifdef Q_OS_WIN
    preferencesHelper->setIsDockedToTray(false);
#else
    preferencesHelper->setIsDockedToTray(true);
#endif

    shadowManager_ = new ShadowManager(this);
    windowSizeManager_ = new WindowSizeManager();

    view_ = new QGraphicsView(mainWindow_);
    scene_ = new QGraphicsScene(mainWindow_);
    view_->installEventFilter(this);
    view_->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);

    loginWindow_ = new LoginWindow::LoginWindowItem(nullptr, preferencesHelper);
    loggingInWindow_ = new LoginWindow::LoggingInWindowItem();
    initWindow_ = new LoginWindow::InitWindowItem();
    connectWindow_ = new ConnectWindow::ConnectWindowItem(nullptr, preferences, preferencesHelper);
    emergencyConnectWindow_ = new EmergencyConnectWindow::EmergencyConnectWindowItem(nullptr, preferencesHelper);
    externalConfigWindow_ = new ExternalConfigWindow::ExternalConfigWindowItem(nullptr, preferencesHelper);
    twoFactorAuthWindow_ = new TwoFactorAuthWindow::TwoFactorAuthWindowItem(nullptr, preferencesHelper);
    preferencesWindow_ = new PreferencesWindow::PreferencesWindowItem(nullptr, preferences, preferencesHelper, accountInfo);
    updateWindow_ = new UpdateWindowItem(preferences);
    upgradeAccountWindow_ = new UpgradeWindow::UpgradeWindowItem(preferences);
    bottomInfoWindow_ = new SharingFeatures::BottomInfoItem(preferences);
    generalMessageWindow_ = new GeneralMessageWindow::GeneralMessageWindowItem(
        nullptr, preferences, preferencesHelper, GeneralMessageWindow::kBright);
    exitWindow_ = new GeneralMessageWindow::GeneralMessageWindowItem(
        nullptr, preferences, preferencesHelper, GeneralMessageWindow::kDark, "SHUTDOWN_ICON", tr(kQuitTitle), "", tr(kQuit), tr(kCancel));
    exitWindow_->setTitleSize(14);
    logoutWindow_ = new GeneralMessageWindow::GeneralMessageWindowItem(
        nullptr, preferences, preferencesHelper, GeneralMessageWindow::kDark, "LOGOUT_ICON", tr(kLogOutTitle), "", tr(kLogOut), tr(kCancel));
    logoutWindow_->setTitleSize(14);

    updateAppItem_ = new UpdateApp::UpdateAppItem(preferences);
    newsFeedWindow_ = new NewsFeedWindow::NewsFeedWindowItem(nullptr, preferences, preferencesHelper);
    protocolWindow_ = new ProtocolWindow::ProtocolWindowItem(nullptr, connectWindow_, preferences, preferencesHelper);

    windowSizeManager_->addWindow(preferencesWindow_, ShadowManager::SHAPE_ID_PREFERENCES, EXPAND_PREFERENCES_RESIZE_DURATION);
    windowSizeManager_->addWindow(newsFeedWindow_, ShadowManager::SHAPE_ID_NEWS_FEED, EXPAND_WINDOW_RESIZE_DURATION);
    windowSizeManager_->addWindow(protocolWindow_, ShadowManager::SHAPE_ID_PROTOCOL, EXPAND_WINDOW_RESIZE_DURATION);
    windowSizeManager_->addWindow(generalMessageWindow_, ShadowManager::SHAPE_ID_GENERAL_MESSAGE, EXPAND_WINDOW_RESIZE_DURATION);
    windowSizeManager_->addWindow(logoutWindow_, ShadowManager::SHAPE_ID_EXIT, EXPAND_WINDOW_RESIZE_DURATION);
    windowSizeManager_->addWindow(exitWindow_, ShadowManager::SHAPE_ID_EXIT, EXPAND_WINDOW_RESIZE_DURATION);

    scene_->addItem(loginWindow_);
    scene_->addItem(loggingInWindow_);
    scene_->addItem(initWindow_);
    scene_->addItem(bottomInfoWindow_);
    scene_->addItem(connectWindow_);
    scene_->addItem(emergencyConnectWindow_);
    scene_->addItem(externalConfigWindow_);
    scene_->addItem(twoFactorAuthWindow_);
    scene_->addItem(preferencesWindow_);
    scene_->addItem(updateWindow_);
    scene_->addItem(upgradeAccountWindow_);
    scene_->addItem(updateAppItem_);
    scene_->addItem(newsFeedWindow_);
    scene_->addItem(protocolWindow_);
    scene_->addItem(generalMessageWindow_);
    scene_->addItem(exitWindow_);
    scene_->addItem(logoutWindow_);

    loginWindow_->setVisible(false);
    loggingInWindow_->setVisible(false);
    initWindow_->setVisible(false);
    bottomInfoWindow_->setVisible(false);
    connectWindow_->setVisible(false);
    emergencyConnectWindow_->setVisible(false);
    externalConfigWindow_->setVisible(false);
    twoFactorAuthWindow_->setVisible(false);
    preferencesWindow_->setVisible(false);
    updateWindow_->setVisible(false);
    upgradeAccountWindow_->setVisible(false);
    updateAppItem_->setVisible(false);
    newsFeedWindow_->setVisible(false);
    protocolWindow_->setVisible(false);
    generalMessageWindow_->setVisible(false);
    exitWindow_->setVisible(false);
    logoutWindow_->setVisible(false);

    // bottomInfoWindow_->setPos(0, BOTTOM_INFO_POS_Y_SHOWING);

    connect(locationsWindow_, &LocationsWindow::heightChanged, this, &MainWindowController::onLocationsWindowHeightChanged);
    connect(bottomInfoWindow_, &SharingFeatures::BottomInfoItem::heightChanged, this, &MainWindowController::onBottomInfoHeightChanged);
    connect(bottomInfoWindow_, &QGraphicsObject::yChanged, this, &MainWindowController::onBottomInfoPosChanged);
    connect(preferencesWindow_, &ResizableWindow::sizeChanged, this, &MainWindowController::onWindowResize);
    connect(preferencesWindow_, &ResizableWindow::resizeFinished, this, &MainWindowController::onWindowResizeFinished);
    connect(newsFeedWindow_, &ResizableWindow::sizeChanged, this, &MainWindowController::onWindowResize);
    connect(newsFeedWindow_, &ResizableWindow::resizeFinished, this, &MainWindowController::onWindowResizeFinished);
    connect(protocolWindow_, &ResizableWindow::sizeChanged, this, &MainWindowController::onWindowResize);
    connect(protocolWindow_, &ResizableWindow::resizeFinished, this, &MainWindowController::onWindowResizeFinished);
    connect(generalMessageWindow_, &ResizableWindow::sizeChanged, this, &MainWindowController::onWindowResize);
    connect(generalMessageWindow_, &ResizableWindow::resizeFinished, this, &MainWindowController::onWindowResizeFinished);
    connect(logoutWindow_, &ResizableWindow::sizeChanged, this, &MainWindowController::onWindowResize);
    connect(logoutWindow_, &ResizableWindow::resizeFinished, this, &MainWindowController::onWindowResizeFinished);
    connect(exitWindow_, &ResizableWindow::sizeChanged, this, &MainWindowController::onWindowResize);
    connect(exitWindow_, &ResizableWindow::resizeFinished, this, &MainWindowController::onWindowResizeFinished);

    connect(preferences_, &Preferences::appSkinChanged, this, &MainWindowController::onAppSkinChanged);

    if (PersistentState::instance().havePreferencesWindowHeight()) {
        windowSizeManager_->setWindowHeight(preferencesWindow_, PersistentState::instance().preferencesWindowHeight());
    }

    view_->setStyleSheet("background: transparent; border: none");
    view_->setScene(scene_);

    locationsWindow_->hide();

    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH) {
        shadowManager_->addRectangle(connectWindow_->boundingRect().toRect(), ShadowManager::SHAPE_ID_CONNECT_WINDOW, false);
    } else {
        shadowManager_->addPixmap(connectWindow_->getShadowPixmap(), 0, 0, ShadowManager::SHAPE_ID_CONNECT_WINDOW, false);
    }
    shadowManager_->addRectangle(QRect(0, 0, 0, 0), ShadowManager::SHAPE_ID_LOCATIONS, false);
    shadowManager_->addRectangle(loginWindow_->boundingRect().toRect(), ShadowManager::SHAPE_ID_LOGIN_WINDOW, false);
    shadowManager_->addRectangle(initWindow_->boundingRect().toRect(), ShadowManager::SHAPE_ID_INIT_WINDOW, false);
    shadowManager_->addRectangle(QRect(0, 0, 0, 0), ShadowManager::SHAPE_ID_PREFERENCES, false);
    shadowManager_->addRectangle(QRect(0, 0, 0, 0), ShadowManager::SHAPE_ID_NEWS_FEED, false);
    shadowManager_->addRectangle(QRect(0, 0, 0, 0), ShadowManager::SHAPE_ID_PROTOCOL, false);
    shadowManager_->addRectangle(QRect(0, 0, 0, 0), ShadowManager::SHAPE_ID_GENERAL_MESSAGE, false);
    shadowManager_->addRectangle(QRect(0, 0, 0, 0), ShadowManager::SHAPE_ID_EXIT, false);
    connect(shadowManager_, &ShadowManager::shadowUpdated, this, &MainWindowController::shadowUpdated);

    connect(&TooltipController::instance(), &TooltipController::sendServerRatingUp, this, &MainWindowController::onTooltipControllerSendServerRatingUp);
    connect(&TooltipController::instance(), &TooltipController::sendServerRatingDown, this, &MainWindowController::onTooltipControllerSendServerRatingDown);

    connect(&vanGoghUpdateWidgetAnimation_, &QVariantAnimation::valueChanged, this, &MainWindowController::onVanGoghAnimationProgressChanged);
    connect(&vanGoghUpdateWidgetAnimation_, &QVariantAnimation::finished, this, &MainWindowController::onVanGoghAnimationFinished);

    updateExpandAnimationParameters();

    // update window heights if we start in van gogh mode
    onAppSkinChanged(preferences_->appSkin());

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &MainWindowController::onLanguageChanged);
}

void MainWindowController::updateScaling()
{
    initWindow_->updateScaling();
    loginWindow_->updateScaling();
    loggingInWindow_->updateScaling();
    emergencyConnectWindow_->updateScaling();
    externalConfigWindow_->updateScaling();
    twoFactorAuthWindow_->updateScaling();
    exitWindow_->updateScaling();
    logoutWindow_->updateScaling();
    connectWindow_->updateScaling();
    locationsWindow_->updateScaling();
    preferencesWindow_->updateScaling();
    newsFeedWindow_->updateScaling();
    protocolWindow_->updateScaling();
    updateWindow_->updateScaling();
    upgradeAccountWindow_->updateScaling();
    bottomInfoWindow_->updateScaling();
    updateAppItem_->updateScaling();
    generalMessageWindow_->updateScaling();

    updateLocationsWindowAndTabGeometryStatic();
    updateMainAndViewGeometry(true);
    // Reposition windows if Van Gogh mode and update banner is visible
    onAppSkinChanged(preferences_->appSkin());
}

void MainWindowController::updateLocationsWindowAndTabGeometry()
{
    //locationsWindow_->updateLocationsTabGeometry();
    locationsWindow_->setGeometry(shadowManager_->getShadowMargin(),
                                  connectWindow_->boundingRect().height() + locationsYOffset() + shadowManager_->getShadowMargin(),
                                  LOCATIONS_WINDOW_WIDTH * G_SCALE,
                                  locationWindowHeightScaled_);

    shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_LOCATIONS,
                                        QRect(0,
                                              connectWindow_->boundingRect().height() + locationsYOffset(),
                                              LOCATIONS_WINDOW_WIDTH * G_SCALE,
                                              (locationsWindow_->geometry().height())));

}

void MainWindowController::updateLocationsWindowAndTabGeometryStatic()
{
    int height = locationsWindow_->tabAndFooterHeight();
    if (locationListAnimationState_ == LOCATION_LIST_ANIMATION_COLLAPSED) height = 0 ;

    locationsWindow_->updateLocationsTabGeometry();
    locationsWindow_->setGeometry(shadowManager_->getShadowMargin(),
                                  connectWindow_->boundingRect().height() + locationsYOffset() + shadowManager_->getShadowMargin(),
                                  LOCATIONS_WINDOW_WIDTH * G_SCALE,
                                  height * G_SCALE);

    locationWindowHeightScaled_ = height * G_SCALE;

    shadowManager_->removeObject(ShadowManager::SHAPE_ID_CONNECT_WINDOW);
    QPixmap shadow = connectWindow_->getShadowPixmap();
    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH) {
        int yOffset = 0;
        if (updateAppItem_->isVisible()) {
            yOffset = UPDATE_WIDGET_HEIGHT*G_SCALE;
        }
        shadowManager_->addRectangle(connectWindow_->boundingRect().toRect().adjusted(0, yOffset, 0, yOffset),
                                     ShadowManager::SHAPE_ID_CONNECT_WINDOW, shouldShowConnectBackground());
    } else {
        shadowManager_->addPixmap(shadow, 0, 0, ShadowManager::SHAPE_ID_CONNECT_WINDOW, shouldShowConnectBackground());
    }

    shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_LOCATIONS,
                                        QRect(0,
                                              connectWindow_->boundingRect().height() + locationsYOffset(),
                                              LOCATIONS_WINDOW_WIDTH * G_SCALE,
                                              (locationsWindow_->geometry().height())));

    shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_PREFERENCES,
                                        QRect(0,
                                              childWindowShadowOffsetY(true),
                                              preferencesWindow_->boundingRect().width(),
                                              preferencesWindow_->boundingRect().height() - childWindowShadowOffsetY(false)));

    shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_NEWS_FEED,
                                        QRect(0,
                                              childWindowShadowOffsetY(true),
                                              newsFeedWindow_->boundingRect().width(),
                                              newsFeedWindow_->boundingRect().height() - childWindowShadowOffsetY(false)));

    shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_PROTOCOL,
                                        QRect(0,
                                              childWindowShadowOffsetY(true),
                                              protocolWindow_->boundingRect().width(),
                                              protocolWindow_->boundingRect().height() - childWindowShadowOffsetY(false)));

    shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_GENERAL_MESSAGE,
                                        QRect(0,
                                              childWindowShadowOffsetY(true),
                                              generalMessageWindow_->boundingRect().width(),
                                              generalMessageWindow_->boundingRect().height() - childWindowShadowOffsetY(false)));

    shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_EXIT,
                                        QRect(0,
                                              childWindowShadowOffsetY(true),
                                              exitWindow_->boundingRect().width(),
                                              exitWindow_->boundingRect().height() - childWindowShadowOffsetY(false)));

    shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_LOGIN_WINDOW,
                                        QRect(0,
                                              0,
                                              loginWindow_->boundingRect().width(),
                                              loginWindow_->boundingRect().height()));

    shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_INIT_WINDOW,
                                        QRect(0,
                                              0,
                                              initWindow_->boundingRect().width(),
                                              initWindow_->boundingRect().height()));

    if (shadowManager_->isInShadowList(ShadowManager::SHAPE_ID_UPDATE_WIDGET)) {
        shadowManager_->removeObject(ShadowManager::SHAPE_ID_UPDATE_WIDGET);
        QPixmap currentShadow = updateAppItem_->getCurrentPixmapShape();
        shadowManager_->addPixmap(currentShadow, 0, 0, ShadowManager::SHAPE_ID_UPDATE_WIDGET, true);
    }

    if (bottomInfoWindow_->isUpgradeWidgetVisible() || bottomInfoWindow_->isSharingFeatureVisible()) {
        shadowManager_->removeObject(ShadowManager::SHAPE_ID_BOTTOM_INFO);
        bool bottomCollapsed = (locationListAnimationState_ != LOCATION_LIST_ANIMATION_COLLAPSED);
        if (windowSizeManager_->hasWindowInState(WindowSizeManager::kWindowCollapsed)) {
            bottomCollapsed = true;
        }
        QPoint posBottomInfoWindow = getCoordsOfBottomInfoWindow(bottomCollapsed);
        QPixmap bottomInfoShadow = bottomInfoWindow_->getCurrentPixmapShape();
        shadowManager_->addPixmap(bottomInfoShadow,
                                  posBottomInfoWindow.x(),
                                  posBottomInfoWindow.y(),
                                  ShadowManager::SHAPE_ID_BOTTOM_INFO, true);
    }

    updateExpandAnimationParameters();
}

void MainWindowController::updateMaskForGraphicsView()
{
    if (curWindow_ == WINDOW_ID_EXIT || curWindow_ == WINDOW_ID_LOGOUT || curWindow_ == WINDOW_ID_GENERAL_MESSAGE) {
        clearMaskForGraphicsView();
    } else if (windowSizeManager_->allWindowsInState(WindowSizeManager::kWindowCollapsed)
            && curWindow_ != WINDOW_ID_INITIALIZATION
            && curWindow_ != WINDOW_ID_LOGIN
            && curWindow_ != WINDOW_ID_LOGGING_IN
            && curWindow_ != WINDOW_ID_EMERGENCY
            && curWindow_ != WINDOW_ID_EXTERNAL_CONFIG
            && curWindow_ != WINDOW_ID_TWO_FACTOR_AUTH
            && windowBeforeExit_ != WINDOW_ID_LOGIN
            && !bottomInfoWindow_->isVisible())
    {
        setMaskForGraphicsView();
    } else {
        clearMaskForGraphicsView();
    }
}

void MainWindowController::setWindowPosFromPersistent()
{
    // Leaving this here and active to debug future issues for customers.  I verfied no
    // startup cost is incurred, even on slow hardware.
    for (auto screen : qApp->screens()) {
        qCDebug(LOG_BASIC) << "setWindowPosFromPersistent() - screen" << screen->name() << "- geometry" << screen->geometry()
                           << "- virtualGeometry" << screen->virtualGeometry() << "- logicalDotsPerInch" << screen->logicalDotsPerInch()
                           << "- devicePixelRatio" << screen->devicePixelRatio();
    }

    if (PersistentState::instance().haveAppGeometry()) {
        if (mainWindow_->restoreGeometry(PersistentState::instance().appGeometry())) {
            qCDebug(LOG_BASIC) << "setWindowPosFromPersistent() - restored app geometry:" << mainWindow_->geometry();

            // Qt's restoreGeometry may restore the app onto a display that is no longer attached. According to the Qt
            // documentation, it is supposed to take detached displays into account, and appears to do a pretty good
            // job most of the time.  I did encounter sporadic occasions on a Windows laptop where Qt would restore
            // the app offscreen when the app was on an external display and the display was no longer attached (see
            // QTBUG-77385).  The check below should catch this edge case.  Note that we are using virtualGeometry()
            // here rather than geometry() due to the issues noted in ticket #411.
            for (auto screen : qApp->screens()) {
                if (screen->virtualGeometry().contains(mainWindow_->pos())) {
                    return;
                }
            }

            qCDebug(LOG_BASIC) << "setWindowPosFromPersistent() - did not locate a display containing restored app position:" << mainWindow_->pos();
        } else {
            qCDebug(LOG_BASIC) << "setWindowPosFromPersistent() - restoreGeometry failed";
        }
    } else {
        qCDebug(LOG_BASIC) << "setWindowPosFromPersistent() - saved app geometry was not found";
    }

    centerMainGeometryAndUpdateView();
}

void MainWindowController::setIsDockedToTray(bool isDocked)
{
    preferencesHelper_->setIsDockedToTray(isDocked);
    updateMainAndViewGeometry(true);
}

bool MainWindowController::isPreferencesVisible()
{
    return windowSizeManager_->state(preferencesWindow_) == WindowSizeManager::kWindowExpanded ||
           windowSizeManager_->state(preferencesWindow_) == WindowSizeManager::kWindowAnimating;
}

bool MainWindowController::isNewsFeedVisible()
{
    return windowSizeManager_->state(newsFeedWindow_) == WindowSizeManager::kWindowExpanded ||
           windowSizeManager_->state(newsFeedWindow_) == WindowSizeManager::kWindowAnimating;
}

MainWindowController::WINDOW_ID MainWindowController::currentWindow()
{
    return curWindow_;
}

MainWindowController::WINDOW_ID MainWindowController::windowBeforeExit()
{
    return windowBeforeExit_;
}

void MainWindowController::changeWindow(MainWindowController::WINDOW_ID windowId)
{
    if (isAtomicAnimationActive_) {
        queueWindowChanges_.enqueue(windowId);
        //qCDebug(LOG_BASIC) << "MainWindowController::changeWindow(enqueue):" << (int)windowId;
        return;
    }

    // for login window when preferences expanded, handle change window commands after preferences collapsed
    if (curWindow_ == WINDOW_ID_LOGIN && windowSizeManager_->state(preferencesWindow_) != WindowSizeManager::kWindowCollapsed) {
        queueWindowChanges_.enqueue(windowId);
        //qCDebug(LOG_BASIC) << "MainWindowController::changeWindow(enqueue):" << (int)windowId;
        return;
    }

    // for connect window when preferences/news expanded, handle change window commands after preferences collapsed,
    // except WINDOW_ID_UPDATE/WINDOW_ID_LOGIN/WINDOW_ID_INITIALIZATION
    if (curWindow_ == WINDOW_ID_CONNECT &&
        !windowSizeManager_->hasWindowInState(WindowSizeManager::kWindowCollapsed) &&
        windowId != WINDOW_ID_UPDATE &&
        windowId != WINDOW_ID_LOGIN &&
        windowId != WINDOW_ID_INITIALIZATION)
    {
        queueWindowChanges_.enqueue(windowId);
        //qCDebug(LOG_BASIC) << "MainWindowController::changeWindow(enqueue):" << (int)windowId;
        return;
    }

    //qCDebug(LOG_BASIC) << "MainWindowController::changeWindow:" << (int)curWindow_ << (int)windowId;

    // specific commands
    if (windowId == WINDOW_CMD_UPDATE_BOTTOM_INFO) {
        onBottomInfoHeightChanged();
        return;
    } else if (windowId == WINDOW_CMD_CLOSE_EXIT_FROM_PREFS) {
        closeExitWindow(true);
        return;
    } else if (windowId == WINDOW_CMD_CLOSE_EXIT) {
        closeExitWindow(false);
        return;
    }

    if (windowId == curWindow_) {
        return;
    }

    if (windowId == WINDOW_ID_INITIALIZATION) {
        gotoInitializationWindow();
    } else if (windowId == WINDOW_ID_LOGIN) {
        gotoLoginWindow();
    } else if (windowId == WINDOW_ID_EMERGENCY) {
        gotoEmergencyWindow();
    } else if (windowId == WINDOW_ID_LOGGING_IN) {
        gotoLoggingInWindow();
    } else if (windowId == WINDOW_ID_CONNECT) {
        gotoConnectWindow(false);
    } else if (windowId == WINDOW_ID_CONNECT_PREFERENCES) {
        gotoConnectWindow(true);
    } else if (windowId == WINDOW_ID_EXTERNAL_CONFIG) {
        gotoExternalConfigWindow();
    } else if (windowId == WINDOW_ID_TWO_FACTOR_AUTH) {
        gotoTwoFactorAuthWindow();
    } else if (windowId == WINDOW_ID_UPDATE) {
        gotoUpdateWindow();
    } else if (windowId == WINDOW_ID_UPGRADE) {
        gotoUpgradeWindow();
    } else if (windowId == WINDOW_ID_GENERAL_MESSAGE) {
        gotoGeneralMessageWindow();
    } else if (windowId == WINDOW_ID_LOGOUT) {
        gotoExitWindow(true);
    } else if (windowId == WINDOW_ID_EXIT) {
        gotoExitWindow(false);
    } else {
        WS_ASSERT(false);
    }
}

void MainWindowController::expandLocations()
{
    // qCDebug(LOG_LOCATION_LIST) << "MainWindowController::expandLocations";
    WS_ASSERT(curWindow_ == WINDOW_ID_CONNECT);
    WS_ASSERT(expandLocationsAnimationGroup_ != NULL);

    // do nothing if expanding or expanded
    if (expandLocationsAnimationGroup_->direction() == QAbstractAnimation::Forward &&
        expandLocationsAnimationGroup_->state() == QAbstractAnimation::Running)
    {
        return;
    }

    if (expandLocationsAnimationGroup_->state() == QAbstractAnimation::Stopped && locationListAnimationState_ == LOCATION_LIST_ANIMATION_EXPANDED) {
        return;
    }

    shadowManager_->setVisible(ShadowManager::SHAPE_ID_LOCATIONS, true);
    shadowManager_->setOpacity(ShadowManager::SHAPE_ID_LOCATIONS, 1.0, true);

    isAtomicAnimationActive_ = true;
    functionOnAnimationFinished_ = NULL;

    connectWindow_->updateLocationsState(true);

    updateExpandAnimationParameters();
    expandLocationsAnimationGroup_->setDirection(QAbstractAnimation::Forward);
    if (expandLocationsAnimationGroup_->state() != QAbstractAnimation::Running) {
        expandLocationsAnimationGroup_->start();
    }
    connectWindow_->setCornerColor(QColor(14, 25, 38));
}

void MainWindowController::collapseLocations()
{
    // qCDebug(LOG_LOCATION_LIST) << "MainWindowController::collapseLocations";
    WS_ASSERT(curWindow_ == WINDOW_ID_CONNECT);
    WS_ASSERT(expandLocationsAnimationGroup_ != NULL);

    if (expandLocationsAnimationGroup_->direction() == QAbstractAnimation::Backward &&
        expandLocationsAnimationGroup_->state() == QAbstractAnimation::Running)
    {
        return;
    }

    if (expandLocationsAnimationGroup_->state() == QAbstractAnimation::Stopped && locationListAnimationState_ == LOCATION_LIST_ANIMATION_COLLAPSED) {
        return;
    }

    TooltipController::instance().hideAllTooltips();

    isAtomicAnimationActive_ = true;
    functionOnAnimationFinished_ = NULL;

    connectWindow_->updateLocationsState(false);
    if (locationsWindow_->currentTab() == LOCATION_TAB_SEARCH_LOCATIONS) {
        locationsWindow_->hideSearchTabWithoutAnimation();
    }

    if (bottomInfoWindow_->isUpgradeWidgetVisible() ||
        bottomInfoWindow_->isSharingFeatureVisible()) {
        bottomInfoWindow_->setPos(getCoordsOfBottomInfoWindow(true));
        bottomInfoWindow_->setClickable(true);
        bottomInfoWindow_->show();
    }
    updateExpandAnimationParameters();
    expandLocationsAnimationGroup_->setDirection(QAbstractAnimation::Backward);
    if (expandLocationsAnimationGroup_->state() != QAbstractAnimation::Running)
    {
        expandLocationsAnimationGroup_->start();
    }

    connectWindow_->setCornerColor(Qt::transparent);
}

bool MainWindowController::isLocationsExpanded() const
{
    if (expandLocationsAnimationGroup_->direction() == QAbstractAnimation::Forward &&
        expandLocationsAnimationGroup_->state() == QAbstractAnimation::Running)
    {
        return true;
    } else if (expandLocationsAnimationGroup_->state() == QAbstractAnimation::Stopped && locationListAnimationState_ == LOCATION_LIST_ANIMATION_EXPANDED) {
        return true;
    }
    return false;
}

void MainWindowController::expandPreferences()
{
    if (curWindow_ == WINDOW_ID_LOGIN) {
        expandPreferencesFromLogin();
    } else if (curWindow_ == WINDOW_ID_CONNECT) {
        expandWindow(preferencesWindow_);
    }
}

void MainWindowController::collapsePreferences()
{
    if (curWindow_ == WINDOW_ID_LOGIN) {
        collapsePreferencesFromLogin();
    } else if (curWindow_ == WINDOW_ID_CONNECT) {
        collapseWindow(preferencesWindow_, true);
    }
}

void MainWindowController::expandNewsFeed()
{
    if (curWindow_ == WINDOW_ID_CONNECT) {
        expandWindow(newsFeedWindow_);
    }
}

void MainWindowController::collapseNewsFeed()
{
    if (curWindow_ == WINDOW_ID_CONNECT) {
        collapseWindow(newsFeedWindow_, false);
    }
}

void MainWindowController::expandProtocols(ProtocolWindowMode mode)
{
    if (curWindow_ == WINDOW_ID_CONNECT) {
        protocolWindow_->setMode(mode);
        expandWindow(protocolWindow_);
    }
}

void MainWindowController::collapseProtocols()
{
    collapseWindow(protocolWindow_, false);
}

void MainWindowController::showUpdateWidget()
{
    // Do not show update widget if in login screen or other window, since the
    // widget will overlay incorrectly.
    if (curWindow_ != WINDOW_ID_CONNECT) {
        return;
    }
    if (!updateAppItem_->isVisible()) {
        updateAppItem_->show();
        QPixmap shadow = updateAppItem_->getCurrentPixmapShape();
        shadowManager_->addPixmap(shadow, 0, 0, ShadowManager::SHAPE_ID_UPDATE_WIDGET, true);
        invalidateShadow_mac();
        if (preferences_->appSkin() == APP_SKIN_VAN_GOGH) {
            onAppSkinChanged(preferences_->appSkin());
            startAnAnimation(vanGoghUpdateWidgetAnimation_, vanGoghUpdateWidgetAnimationProgress_, 1.0, ANIMATION_SPEED_FAST);
        }
    }
}

void MainWindowController::hideUpdateWidget()
{
    if (updateAppItem_->isVisible()) {
        updateAppItem_->hide();
        shadowManager_->removeObject(ShadowManager::SHAPE_ID_UPDATE_WIDGET);
        invalidateShadow_mac();
        if (preferences_->appSkin() == APP_SKIN_VAN_GOGH) {
            onAppSkinChanged(preferences_->appSkin());
            startAnAnimation(vanGoghUpdateWidgetAnimation_, vanGoghUpdateWidgetAnimationProgress_, 0.0, ANIMATION_SPEED_FAST);
        }
    }
}

QWidget *MainWindowController::getViewport()
{
    return view_->viewport();
}

QPixmap &MainWindowController::getCurrentShadowPixmap()
{
    return shadowManager_->getCurrentShadowPixmap();
}

bool MainWindowController::isNeedShadow()
{
    return shadowManager_->isNeedShadow();
}

int MainWindowController::getShadowMargin()
{
    return shadowManager_->getShadowMargin();
}

void MainWindowController::hideAllToolTips()
{
    TooltipController::instance().hideAllTooltips();
}

void MainWindowController::onExpandLocationsListAnimationFinished()
{
    if (expandLocationsListAnimation_->direction() == QAbstractAnimation::Backward) {
        locationListAnimationState_ = LOCATION_LIST_ANIMATION_COLLAPSED;
        clearMaskForGraphicsView();
    } else {
        shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_LOCATIONS,
                                            QRect(0,
                                                  connectWindow_->boundingRect().height() + locationsYOffset(),
                                                  LOCATIONS_WINDOW_WIDTH * G_SCALE,
                                                  (locationsWindow_->geometry().height())));
        locationListAnimationState_ = LOCATION_LIST_ANIMATION_EXPANDED;
        setMaskForGraphicsView();
    }

    updateMainAndViewGeometry(false); // prevent out of sync app with viewport when proxy is showing

#ifdef Q_OS_MAC
    //QTimer::singleShot(1, this, SLOT(invalidateShadow_mac()));
#endif
}

void MainWindowController::onExpandLocationsListAnimationValueChanged(const QVariant &value)
{
    QSize sz = value.toSize();
    locationWindowHeightScaled_ = sz.height();

    updateLocationsWindowAndTabGeometry();
    updateMainAndViewGeometry(false);

    invalidateShadow_mac();
    keepWindowInsideScreenCoordinates();
}

void MainWindowController::onExpandLocationsListAnimationStateChanged(QAbstractAnimation::State newState, QAbstractAnimation::State oldState)
{
    if (newState == QAbstractAnimation::Running && oldState != QAbstractAnimation::Running) {
        if (expandLocationsListAnimation_->direction() == QAbstractAnimation::Backward) {
            locationListAnimationState_ = LOCATION_LIST_ANIMATION_COLLAPSING;
        } else {
            locationListAnimationState_ = LOCATION_LIST_ANIMATION_EXPANDING;
        }
    }
}

void MainWindowController::onExpandLocationsAnimationGroupFinished()
{
    if (functionOnAnimationFinished_ != NULL) {
        functionOnAnimationFinished_();
        functionOnAnimationFinished_ = NULL;
    } else {
        isAtomicAnimationActive_ = false;
        handleNextWindowChange();
    }

    updateBottomInfoWindowVisibilityAndPos();
}

void MainWindowController::onRevealConnectAnimationGroupFinished()
{
    isAtomicAnimationActive_ = false;
    handleNextWindowChange();
}

void MainWindowController::onCollapseBottomInfoWindowAnimationValueChanged(const QVariant & /*value*/)
{
    updateMainAndViewGeometry(false);
    invalidateShadow_mac();
}

void MainWindowController::onLocationsWindowHeightChanged() // manual resizing
{
    // qCDebug(LOG_USER) << "MainController:: Resizing Locations";
    locationWindowHeightScaled_ = locationsWindow_->tabAndFooterHeight() * G_SCALE;
    updateLocationsWindowAndTabGeometry();
    updateMainAndViewGeometry(false);

    shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_LOCATIONS,
                                        QRect(0,
                                              connectWindow_->boundingRect().height() + locationsYOffset(),
                                              LOCATIONS_WINDOW_WIDTH * G_SCALE,
                                              (locationsWindow_->geometry().height())));

    keepWindowInsideScreenCoordinates();
}

void MainWindowController::onWindowResize(ResizableWindow *window)
{
    windowSizeManager_->setWindowHeight(window, window->boundingRect().height()/G_SCALE);
    if (window == preferencesWindow_) {
        PersistentState::instance().setPreferencesWindowHeight(windowSizeManager_->windowHeight(window));
    }
    updateMainAndViewGeometry(false);

    // only recalculate the shadow size at certain intervals since the changeRectangleSize is expensive
    if (abs(windowSizeManager_->windowHeight(window) - windowSizeManager_->previousWindowHeight(window)) > 5) {
        shadowManager_->changeRectangleSize(windowSizeManager_->shapeId(window),
                                            QRect(0,
                                                  childWindowShadowOffsetY(true),
                                                  window->boundingRect().width(),
                                                  window->boundingRect().height() - childWindowShadowOffsetY(false)));
        windowSizeManager_->setPreviousWindowHeight(window, windowSizeManager_->windowHeight(window));
    }

    keepWindowInsideScreenCoordinates();
}

void MainWindowController::onWindowResizeFinished(ResizableWindow *window)
{
    shadowManager_->changeRectangleSize(windowSizeManager_->shapeId(window),
                                        QRect(0,
                                              childWindowShadowOffsetY(true),
                                              window->boundingRect().width(),
                                              window->boundingRect().height() - childWindowShadowOffsetY(false)));
}

void MainWindowController::onBottomInfoHeightChanged()
{
    if (isAtomicAnimationActive_) {
        queueWindowChanges_.enqueue(WINDOW_CMD_UPDATE_BOTTOM_INFO);
    } else {
        updateBottomInfoWindowVisibilityAndPos();
        updateExpandAnimationParameters();
        invalidateShadow_mac();
        keepWindowInsideScreenCoordinates();
    }
}

void MainWindowController::onBottomInfoPosChanged()
{
    shadowManager_->changePixmapPos(ShadowManager::SHAPE_ID_BOTTOM_INFO,
                                    bottomInfoWindow_->pos().x(),
                                    bottomInfoWindow_->pos().y());
    keepWindowInsideScreenCoordinates();
}

void MainWindowController::onTooltipControllerSendServerRatingUp()
{
    emit sendServerRatingUp();
}

void MainWindowController::onTooltipControllerSendServerRatingDown()
{
    emit sendServerRatingDown();
}

void MainWindowController::gotoInitializationWindow()
{
    // qDebug() << "gotoInitializationWindow()";
    hideUpdateWidget();

    for (auto w : windowSizeManager_->windows()) {
        if (windowSizeManager_->state(w) != WindowSizeManager::kWindowCollapsed) {
            shadowManager_->setVisible(windowSizeManager_->shapeId(w), false);
            w->hide();
            windowSizeManager_->setState(w, WindowSizeManager::kWindowCollapsed);
            if (w == preferencesWindow_) {
                emit preferencesCollapsed();
            }
        }
    }

    shadowManager_->setVisible(ShadowManager::SHAPE_ID_CONNECT_WINDOW, false);
    shadowManager_->setVisible(ShadowManager::SHAPE_ID_GENERAL_MESSAGE, false);
    updateWindow_->hide();
    connectWindow_->hide();
    TooltipController::instance().hideAllTooltips();

    if (bottomInfoWindow_->isVisible()) {
        bottomInfoWindow_->hide();
        bottomInfoWindow_->setClickable(false);
        shadowManager_->removeObject(ShadowManager::SHAPE_ID_BOTTOM_INFO);
    }

    if (isLocationsExpanded()) {
        connectWindow_->updateLocationsState(false);
        locationsWindow_->hide();
        locationListAnimationState_ = LOCATION_LIST_ANIMATION_COLLAPSED;
        clearMaskForGraphicsView();
    }

    loginWindow_->setClickable(false);
    loginWindow_->setVisible(false);
    loggingInWindow_->setVisible(false);
    emergencyConnectWindow_->setClickable(false);
    emergencyConnectWindow_->setVisible(false);
    externalConfigWindow_->setClickable(false);
    externalConfigWindow_->setVisible(false);
    twoFactorAuthWindow_->setClickable(false);
    twoFactorAuthWindow_->setVisible(false);
    newsFeedWindow_->setVisible(false);
    protocolWindow_->setVisible(false);
    generalMessageWindow_->setVisible(false);
    exitWindow_->setVisible(false);
    logoutWindow_->setVisible(false);

    // init could be different height depending on if going to login or directly to connect screen
    int initHeight = WINDOW_HEIGHT;
    if (curWindow_ == WINDOW_ID_LOGIN) initHeight = LOGIN_HEIGHT;
    initWindow_->setHeight(initHeight);
    initWindowInitHeight_ = initHeight;

    shadowManager_->setVisible(ShadowManager::SHAPE_ID_INIT_WINDOW, true);
    shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_INIT_WINDOW,
                                        QRect(0,
                                              0,
                                              initWindow_->boundingRect().width(),
                                              initWindow_->boundingRect().height()));

    curWindow_ = WINDOW_ID_INITIALIZATION;
    initWindow_->setOpacity(1.0);
    initWindow_->setVisible(true);
    initWindow_->show();

    updateMainAndViewGeometry(false);

}

void MainWindowController::gotoLoginWindow()
{
    // qDebug() << "gotoLoginWindow()";
    WS_ASSERT(curWindow_ == WINDOW_ID_UNINITIALIZED
             || curWindow_ == WINDOW_ID_INITIALIZATION
             || curWindow_ == WINDOW_ID_EMERGENCY
             || curWindow_ == WINDOW_ID_LOGGING_IN
             || curWindow_ == WINDOW_ID_CONNECT
             || curWindow_ == WINDOW_ID_EXTERNAL_CONFIG
             || curWindow_ == WINDOW_ID_GENERAL_MESSAGE
             || curWindow_ == WINDOW_ID_TWO_FACTOR_AUTH
             || curWindow_ == WINDOW_ID_UPDATE
             || curWindow_ == WINDOW_ID_UPGRADE
             || curWindow_ == WINDOW_ID_LOGOUT);

    for (auto w : windowSizeManager_->windows()) {
        if (windowSizeManager_->state(w) != WindowSizeManager::kWindowCollapsed) {
            queueWindowChanges_.enqueue(WINDOW_ID_LOGIN);
            collapseWindow(w, true);
            return;
        }
    }

    if (curWindow_ == WINDOW_ID_INITIALIZATION) {
        // qDebug() << "Init -> Login";
        initWindow_->stackBefore(loginWindow_);

        loginWindow_->setOpacity(0.0);
        loginWindow_->setVisible(true);

        // login opacity
        QPropertyAnimation *loginOpacityAnim = new QPropertyAnimation(this);
        loginOpacityAnim->setTargetObject(loginWindow_);
        loginOpacityAnim->setPropertyName("opacity");
        loginOpacityAnim->setStartValue(0.0);
        loginOpacityAnim->setEndValue(1.0);
        loginOpacityAnim->setDuration(REVEAL_LOGIN_ANIMATION_DURATION);
        connect(loginOpacityAnim, &QPropertyAnimation::finished, [this]() {
            shadowManager_->setVisible(ShadowManager::SHAPE_ID_INIT_WINDOW, false);
            shadowManager_->setVisible(ShadowManager::SHAPE_ID_LOGIN_WINDOW, true);
            initWindow_->setVisible(false);
            initWindow_->resetState();
            loginWindow_->setClickable(true);
            loginWindow_->setFocus();
        });

        // Init size
        QVariantAnimation *initHeightAnim = new QVariantAnimation(this);
        initHeightAnim->setStartValue(initWindowInitHeight_);
        initHeightAnim->setEndValue(LOGIN_HEIGHT);
        initHeightAnim->setDuration(REVEAL_LOGIN_ANIMATION_DURATION);
        connect(initHeightAnim, &QVariantAnimation::valueChanged, [this](const QVariant &value) {
            initWindow_->setHeight(value.toInt());
            shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_INIT_WINDOW,
                                                QRect(0,0, initWindow_->boundingRect().width(),
                                                      initWindow_->boundingRect().height()));
            updateMainAndViewGeometry(true);
        });

        QSequentialAnimationGroup *revealLoginOpacitySeq = new QSequentialAnimationGroup(this);
        revealLoginOpacitySeq->addPause(REVEAL_LOGIN_ANIMATION_DURATION/2 );
        revealLoginOpacitySeq->addAnimation(loginOpacityAnim);

        // group
        QParallelAnimationGroup *revealLoginAnimationGroup = new QParallelAnimationGroup(this);
        revealLoginAnimationGroup->addAnimation(initHeightAnim);
        revealLoginAnimationGroup->addAnimation(revealLoginOpacitySeq);
        connect(revealLoginAnimationGroup, &QPropertyAnimation::finished,
                [this,revealLoginOpacitySeq, loginOpacityAnim, initHeightAnim]()
        {
            curWindow_ = WINDOW_ID_LOGIN;
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
            revealLoginOpacitySeq->deleteLater();
            loginOpacityAnim->deleteLater();
            initHeightAnim->deleteLater();
            QTimer::singleShot(0, this, [&]() {
                updateMainAndViewGeometry(true);
            });
        });

        isAtomicAnimationActive_ = true;
        revealLoginAnimationGroup->start(QPropertyAnimation::DeleteWhenStopped);
    } else if (curWindow_ == WINDOW_ID_EMERGENCY) {
        curWindow_ = WINDOW_ID_LOGIN;

        emergencyConnectWindow_->stackBefore(loginWindow_);
        emergencyConnectWindow_->setClickable(false);

        loginWindow_->setOpacity(0.0);
        loginWindow_->setVisible(true);

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(loginWindow_);
        anim->setPropertyName("opacity");
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        connect(anim, &QPropertyAnimation::finished, [this]() {
            emergencyConnectWindow_->setVisible(false);
            loginWindow_->setClickable(true);
            loginWindow_->setFocus();
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
        });

        isAtomicAnimationActive_ = true;
        anim->start(QPropertyAnimation::DeleteWhenStopped);
    } else if (curWindow_ == WINDOW_ID_EXTERNAL_CONFIG) {
        curWindow_ = WINDOW_ID_LOGIN;

        externalConfigWindow_->stackBefore(loginWindow_);

        loginWindow_->setOpacity(0.0);
        loginWindow_->setVisible(true);

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(loginWindow_);
        anim->setPropertyName("opacity");
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        connect(anim, &QPropertyAnimation::finished, [this]() {
            externalConfigWindow_->setVisible(false);
            loginWindow_->setClickable(true);
            loginWindow_->setFocus();
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
        });

        isAtomicAnimationActive_ = true;
        anim->start(QPropertyAnimation::DeleteWhenStopped);
    } else if (curWindow_ == WINDOW_ID_TWO_FACTOR_AUTH) {
        curWindow_ = WINDOW_ID_LOGIN;

        twoFactorAuthWindow_->stackBefore(loginWindow_);

        loginWindow_->setOpacity(0.0);
        loginWindow_->setVisible(true);

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(loginWindow_);
        anim->setPropertyName("opacity");
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        connect(anim, &QPropertyAnimation::finished, [this]() {
            twoFactorAuthWindow_->resetState();
            twoFactorAuthWindow_->setVisible(false);
            loginWindow_->setClickable(true);
            loginWindow_->setFocus();
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
        });

        isAtomicAnimationActive_ = true;
        anim->start(QPropertyAnimation::DeleteWhenStopped);
    } else if (curWindow_ == WINDOW_ID_LOGGING_IN) {
        // qDebug() << "LoggingIn -> Login";
        curWindow_ = WINDOW_ID_LOGIN;

        connectWindow_->stackBefore(loginWindow_);
        loggingInWindow_->stackBefore(loginWindow_);
        updateWindow_->stackBefore(loginWindow_);
        upgradeAccountWindow_->stackBefore(loginWindow_);

        loginWindow_->setOpacity(0.0);
        loginWindow_->setVisible(true);

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(loginWindow_);
        anim->setPropertyName("opacity");
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        // Init size
        QVariantAnimation *initHeightAnim = new QVariantAnimation(this);
        initHeightAnim->setStartValue(initWindowInitHeight_);
        initHeightAnim->setEndValue(LOGIN_HEIGHT);
        initHeightAnim->setDuration(REVEAL_LOGIN_ANIMATION_DURATION);
        connect(initHeightAnim, &QVariantAnimation::valueChanged, [this](const QVariant &value) {
            initWindow_->setHeight(value.toInt());
            shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_INIT_WINDOW,
                                                QRect(0,0, initWindow_->boundingRect().width(),
                                                      initWindow_->boundingRect().height()));
            updateMainAndViewGeometry(true);
        });

        QSequentialAnimationGroup *animGroup = new QSequentialAnimationGroup(this);
        animGroup->addPause(REVEAL_LOGIN_ANIMATION_DURATION/2 );
        animGroup->addAnimation(anim);
        animGroup->addAnimation(initHeightAnim);

        // group
        connect(animGroup, &QPropertyAnimation::finished, [this, animGroup, anim, initHeightAnim]() {
            loggingInWindow_->setVisible(false);
            loggingInWindow_->stopAnimation();
            loginWindow_->setClickable(true);
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
        });

        isAtomicAnimationActive_ = true;
        animGroup->start(QPropertyAnimation::DeleteWhenStopped);
    } else if (curWindow_ == WINDOW_ID_GENERAL_MESSAGE) {
        // qDebug() << "General -> Login";
        curWindow_ = WINDOW_ID_LOGIN;

        connectWindow_->stackBefore(loginWindow_);
        generalMessageWindow_->stackBefore(loginWindow_);
        updateWindow_->stackBefore(loginWindow_);
        upgradeAccountWindow_->stackBefore(loginWindow_);

        shadowManager_->setVisible(ShadowManager::SHAPE_ID_GENERAL_MESSAGE, false);

        loginWindow_->setOpacity(0.0);
        loginWindow_->setVisible(true);
        loggingInWindow_->setVisible(false);

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(loginWindow_);
        anim->setPropertyName("opacity");
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        connect(anim, &QPropertyAnimation::finished, [this]() {
            generalMessageWindow_->setVisible(false);
            loginWindow_->setClickable(true);
            loginWindow_->setFocus();
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
        });

        isAtomicAnimationActive_ = true;
        anim->start(QPropertyAnimation::DeleteWhenStopped);
    } else if (curWindow_ == WINDOW_ID_CONNECT || curWindow_ == WINDOW_ID_UPDATE || curWindow_ == WINDOW_ID_UPGRADE || curWindow_ == WINDOW_ID_LOGOUT) {
        // qDebug() << "Other -> Login";
        if (curWindow_ == WINDOW_ID_LOGOUT) {
            closeExitWindow(false);
        }

        hideUpdateWidget();
        newsFeedWindow_->hide();
        protocolWindow_->hide();
        updateWindow_->hide();
        upgradeAccountWindow_->hide();
        curWindow_ = WINDOW_ID_LOGIN;

        connectWindow_->stackBefore(loginWindow_);
        connectWindow_->setClickable(false);

        shadowManager_->setVisible(ShadowManager::SHAPE_ID_CONNECT_WINDOW, false);
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_LOGIN_WINDOW, true);

        loginWindow_->setOpacity(1.0);
        loginWindow_->setVisible(true);
        loginWindow_->setClickable(true);

        connectWindow_->hide();
        TooltipController::instance().hideAllTooltips();

        if (bottomInfoWindow_->isVisible()) {
            bottomInfoWindow_->hide();
            bottomInfoWindow_->setClickable(false);
            shadowManager_->removeObject(ShadowManager::SHAPE_ID_BOTTOM_INFO);
        }

        if (isLocationsExpanded()) {
            connectWindow_->updateLocationsState(false);
            locationsWindow_->hide();
            locationListAnimationState_ = LOCATION_LIST_ANIMATION_COLLAPSED;
            clearMaskForGraphicsView();
        }
        hideUpdateWidget();
        updateMainAndViewGeometry(false);
    }
}

void MainWindowController::gotoEmergencyWindow()
{
    WS_ASSERT(curWindow_ == WINDOW_ID_LOGIN);

    curWindow_ = WINDOW_ID_EMERGENCY;

    loginWindow_->setClickable(false);
    TooltipController::instance().hideAllTooltips();
    loginWindow_->stackBefore(emergencyConnectWindow_);

    emergencyConnectWindow_->setOpacity(0.0);
    emergencyConnectWindow_->setVisible(true);
    emergencyConnectWindow_->setClickable(true);

    QPropertyAnimation *anim = new QPropertyAnimation(this);
    anim->setTargetObject(emergencyConnectWindow_);
    anim->setPropertyName("opacity");
    anim->setStartValue(emergencyConnectWindow_->opacity());
    anim->setEndValue(0.96);
    anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

    connect(anim, &QPropertyAnimation::finished, [this]() {
        loginWindow_->setVisible(false);
        isAtomicAnimationActive_ = false;
        handleNextWindowChange();
        emergencyConnectWindow_->setFocus();
    });

    isAtomicAnimationActive_ = true;
    anim->start(QPropertyAnimation::DeleteWhenStopped);
}

void MainWindowController::gotoLoggingInWindow()
{
    WS_ASSERT(curWindow_ == WINDOW_ID_LOGIN
             || curWindow_ == WINDOW_ID_INITIALIZATION
             || curWindow_ == WINDOW_ID_GENERAL_MESSAGE
             || curWindow_ == WINDOW_ID_EXTERNAL_CONFIG
             || curWindow_ == WINDOW_ID_TWO_FACTOR_AUTH);

    if (curWindow_ == WINDOW_ID_LOGIN) {
        curWindow_ = WINDOW_ID_LOGGING_IN;
        loginWindow_->setClickable(false);
        TooltipController::instance().hideAllTooltips();
        loginWindow_->stackBefore(loggingInWindow_);

        loggingInWindow_->setOpacity(0.0);
        loggingInWindow_->setVisible(true);
        loggingInWindow_->startAnimation();

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(loggingInWindow_);
        anim->setPropertyName("opacity");
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);
        connect(anim, &QPropertyAnimation::finished, [this]() {
            loginWindow_->setVisible(false);
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
        });

        isAtomicAnimationActive_ = true;
        anim->start(QPropertyAnimation::DeleteWhenStopped);
    } else if (curWindow_ == WINDOW_ID_INITIALIZATION) {
        curWindow_ = WINDOW_ID_LOGGING_IN;
        initWindow_->stackBefore(loggingInWindow_);

        loggingInWindow_->setOpacity(0.0);
        loggingInWindow_->setVisible(true);
        loggingInWindow_->startAnimation();

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(loggingInWindow_);
        anim->setPropertyName("opacity");
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);
        connect(anim, &QPropertyAnimation::finished, [this]() {
            initWindow_->setVisible(false);
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
        });

        isAtomicAnimationActive_ = true;
        anim->start(QPropertyAnimation::DeleteWhenStopped);
    } else if (curWindow_ == WINDOW_ID_EXTERNAL_CONFIG) {
        curWindow_ = WINDOW_ID_LOGGING_IN;
        externalConfigWindow_->setClickable(false);
        externalConfigWindow_->stackBefore(loggingInWindow_);

        loggingInWindow_->setOpacity(0.0);
        loggingInWindow_->setVisible(true);
        loggingInWindow_->startAnimation();

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(loggingInWindow_);
        anim->setPropertyName("opacity");
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);
        connect(anim, &QPropertyAnimation::finished, [this]() {
            externalConfigWindow_->setVisible(false);
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
        });

        isAtomicAnimationActive_ = true;
        anim->start(QPropertyAnimation::DeleteWhenStopped);
    } else if (curWindow_ == WINDOW_ID_TWO_FACTOR_AUTH) {
        curWindow_ = WINDOW_ID_LOGGING_IN;
        twoFactorAuthWindow_->setClickable(false);
        twoFactorAuthWindow_->stackBefore(loggingInWindow_);

        loggingInWindow_->setOpacity(0.0);
        loggingInWindow_->setVisible(true);
        loggingInWindow_->startAnimation();

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(loggingInWindow_);
        anim->setPropertyName("opacity");
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);
        connect(anim, &QPropertyAnimation::finished, [this]() {
            twoFactorAuthWindow_->resetState();
            twoFactorAuthWindow_->setVisible(false);
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
        });

        isAtomicAnimationActive_ = true;
        anim->start(QPropertyAnimation::DeleteWhenStopped);
    } else if (curWindow_ == WINDOW_ID_GENERAL_MESSAGE) {
        curWindow_ = WINDOW_ID_LOGGING_IN;
        connectWindow_->stackBefore(loggingInWindow_);
        generalMessageWindow_->stackBefore(loggingInWindow_);
        updateWindow_->stackBefore(loggingInWindow_);
        upgradeAccountWindow_->stackBefore(loggingInWindow_);

        shadowManager_->setVisible(ShadowManager::SHAPE_ID_GENERAL_MESSAGE, false);

        generalMessageWindow_->setVisible(false);

        loggingInWindow_->setOpacity(0.0);
        loggingInWindow_->setVisible(true);
        loggingInWindow_->startAnimation();

        if (bottomInfoWindow_->isVisible()) {
            bottomInfoWindow_->hide();
            bottomInfoWindow_->setClickable(false);
            shadowManager_->removeObject(ShadowManager::SHAPE_ID_BOTTOM_INFO);
        }
        hideUpdateWidget();

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(loggingInWindow_);
        anim->setPropertyName("opacity");
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);
        connect(anim, &QPropertyAnimation::finished, [this]() {
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
        });

        isAtomicAnimationActive_ = true;
        anim->start(QPropertyAnimation::DeleteWhenStopped);
    }
}

void MainWindowController::hideLocationsWindow()
{
    locationsWindow_->updateLocationsTabGeometry();
    locationsWindow_->setGeometry(shadowManager_->getShadowMargin(),
                                  scene_->height() + locationsYOffset() + shadowManager_->getShadowMargin(),
                                  LOCATIONS_WINDOW_WIDTH * G_SCALE,
                                  0);
    locationsWindow_->show();
}

void MainWindowController::clearServerRatingsTooltipState()
{
    TooltipController::instance().clearServerRatings();
}

bool MainWindowController::eventFilter(QObject *watched, QEvent *event)
{
    // Since the locations window has no focus, we have to send keyboard events there when the list of locations is expanded
    // Also, we first need to send an event to the main window, since it contains some logic for keyboard processing
    // It's a bit confusing - we  should do refactoring with control focuses
    if (watched == view_ && (event->type() == QEvent::KeyPress /*|| event->type() == QEvent::KeyRelease*/)) {
        const bool bLocationsExpanded = isLocationsExpanded();
        if (static_cast<MainWindow *>(mainWindow_)->handleKeyPressEvent(static_cast<QKeyEvent *>(event))) {
            return true;
        }
        if (bLocationsExpanded && locationsWindow_->handleKeyPressEvent(static_cast<QKeyEvent *>(event))) {
            return true;
        }
    }
    return QObject::eventFilter(watched, event);
}

#ifdef Q_OS_MAC
void MainWindowController::invalidateShadow_mac_impl()
{
    void *id = (void *)mainWindow_->winId();
    MacUtils::invalidateShadow(id);
}

void MainWindowController::updateNativeShadowIfNeeded()
{
    if (isNeedUpdateNativeShadow_) {
        isNeedUpdateNativeShadow_ = false;
        invalidateShadow_mac_impl();
    }
}
#endif

void MainWindowController::gotoConnectWindow(bool expandPrefs)
{
    // qDebug() << "gotoConnectWindow()";
    WS_ASSERT(curWindow_ == WINDOW_ID_LOGGING_IN
             || curWindow_ == WINDOW_ID_INITIALIZATION
             || curWindow_ == WINDOW_ID_UPDATE
             || curWindow_ == WINDOW_ID_UPGRADE
             || curWindow_ == WINDOW_ID_GENERAL_MESSAGE
             || curWindow_ == WINDOW_ID_EXTERNAL_CONFIG);

    if (curWindow_ == WINDOW_ID_LOGGING_IN) {
        // qDebug() << "LoggingIn -> Connect";
        loggingInWindow_->stackBefore(connectWindow_);

        curWindow_ = WINDOW_ID_CONNECT;
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_LOGIN_WINDOW, false);
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_INIT_WINDOW, false);
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_EXIT, false);
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_CONNECT_WINDOW, true);
        connectWindow_->setClickable(true);
        connectWindow_->setVisible(true);
        connectWindow_->show();

        loggingInWindow_->setVisible(false);
        loggingInWindow_->stopAnimation();

        updateMainAndViewGeometry(false);
        hideLocationsWindow();
        updateBottomInfoWindowVisibilityAndPos();

        updateExpandAnimationParameters();
        handleNextWindowChange();
    } else if (curWindow_ == WINDOW_ID_INITIALIZATION || (curWindow_ == WINDOW_ID_EXIT && windowBeforeExit_ == WINDOW_ID_INITIALIZATION)) {
        // qDebug() << "Init -> Connect";
        initWindow_->stackBefore(connectWindow_);

        isAtomicAnimationActive_ = true;

        // The second condition occurs when engine crashes and user tries to exit app using hot keys but then selects "No".
        // In this case it is necessary to set windowBeforeExit_ to WINDOW_ID_CONNECT because it will be necessary to enter
        // connect window after closing exit dialog.
        if(curWindow_ == WINDOW_ID_EXIT) {
            windowBeforeExit_ = WINDOW_ID_CONNECT;
        }

        shadowManager_->setVisible(ShadowManager::SHAPE_ID_INIT_WINDOW, false);
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_CONNECT_WINDOW, true);
        connectWindow_->setOpacity(OPACITY_HIDDEN);
        connectWindow_->setVisible(true);
        initWindow_->setCropHeight(0);
        emit revealConnectWindowStateChanged(true);

        // connect opacity
        QPropertyAnimation *revealConnectOpacityAnimation = new QPropertyAnimation(this);
        revealConnectOpacityAnimation->setTargetObject(connectWindow_);
        revealConnectOpacityAnimation->setPropertyName("opacity");
        revealConnectOpacityAnimation->setStartValue(OPACITY_HIDDEN);
        revealConnectOpacityAnimation->setEndValue(OPACITY_FULL);
        revealConnectOpacityAnimation->setDuration(REVEAL_CONNECT_ANIMATION_DURATION/2);
        connect(revealConnectOpacityAnimation, &QPropertyAnimation::finished, [this]() {
            emit revealConnectWindowStateChanged(false);
            connectWindow_->setClickable(true);
            initWindow_->setVisible(false);
            initWindow_->resetState();
            updateMainAndViewGeometry(false);
            hideLocationsWindow();
            updateExpandAnimationParameters();
        });

        // init shape
        QVariantAnimation *revealInitSizeAnimation = new QVariantAnimation(this);
        revealInitSizeAnimation->setStartValue(0);
        revealInitSizeAnimation->setEndValue(60);
        revealInitSizeAnimation->setDuration(REVEAL_CONNECT_ANIMATION_DURATION);
        connect(revealInitSizeAnimation, &QVariantAnimation::valueChanged, [this](const QVariant &value) {
            initWindow_->setCropHeight(value.toInt());
            updateMainAndViewGeometry(true);
        });

        QSequentialAnimationGroup *revealConnectOpacitySeq = new QSequentialAnimationGroup(this);
        revealConnectOpacitySeq->addPause(REVEAL_CONNECT_ANIMATION_DURATION/2);
        revealConnectOpacitySeq->addAnimation(revealConnectOpacityAnimation);

        // group
        QParallelAnimationGroup *revealConnectAnimationGroup = new QParallelAnimationGroup(this);
        revealConnectAnimationGroup->addAnimation(revealInitSizeAnimation);
        revealConnectAnimationGroup->addAnimation(revealConnectOpacitySeq);
        connect(revealConnectAnimationGroup, &QPropertyAnimation::finished,
                [this, revealConnectOpacityAnimation, revealConnectOpacitySeq, revealInitSizeAnimation]()
        {
            curWindow_ = WINDOW_ID_CONNECT;
            updateBottomInfoWindowVisibilityAndPos(/*forceCollapsed =*/ true);
            if (bottomInfoWindow_->isVisible()) {
                std::function<void()> finish_function = [this]() {
                    isAtomicAnimationActive_ = false;
                    handleNextWindowChange();
                    updateBottomInfoWindowVisibilityAndPos();
                };
                animateBottomInfoWindow(QAbstractAnimation::Backward, finish_function);
            } else {
                isAtomicAnimationActive_ = false;
                handleNextWindowChange();
            }
            revealConnectOpacityAnimation->deleteLater();
            revealConnectOpacitySeq->deleteLater();
            revealInitSizeAnimation->deleteLater();
        });

        revealConnectAnimationGroup->start(QPropertyAnimation::DeleteWhenStopped);
    } else if (curWindow_ == WINDOW_ID_UPDATE) {
        //qDebug() << "Update -> Connect";
        curWindow_ = WINDOW_ID_CONNECT;
        isAtomicAnimationActive_ = true;

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(updateWindow_);
        anim->setPropertyName("opacity");
        anim->setStartValue(updateWindow_->opacity());
        anim->setEndValue(0.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        connect(anim, &QPropertyAnimation::finished, [this]() {
            updateWindow_->hide();
            connectWindow_->setClickable(true);
            bottomInfoWindow_->setClickable(true);

            if (bottomInfoWindow_->isVisible()) {
                std::function<void()> finish_function = [this](){
                    isAtomicAnimationActive_ = false;
                    updateBottomInfoWindowVisibilityAndPos();
                    handleNextWindowChange();
                };

                animateBottomInfoWindow(QAbstractAnimation::Backward, finish_function);
            } else {
                isAtomicAnimationActive_ = false;
                handleNextWindowChange();
            }
        });

        anim->start(QPropertyAnimation::DeleteWhenStopped);
    } else if (curWindow_ == WINDOW_ID_UPGRADE) {
        // qDebug() << "Upgrade -> Connect";
        curWindow_ = WINDOW_ID_CONNECT;
        isAtomicAnimationActive_ = true;

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(upgradeAccountWindow_);
        anim->setPropertyName("opacity");
        anim->setStartValue(upgradeAccountWindow_->opacity());
        anim->setEndValue(0.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        connect(anim, &QPropertyAnimation::finished, [this]() {
            upgradeAccountWindow_->hide();
            connectWindow_->setClickable(true);
            bottomInfoWindow_->setClickable(true);

            if (bottomInfoWindow_->isVisible()) {
                std::function<void()> finish_function = [this](){
                    isAtomicAnimationActive_ = false;
                    updateBottomInfoWindowVisibilityAndPos();
                    handleNextWindowChange();
                };

                animateBottomInfoWindow(QAbstractAnimation::Backward, finish_function);
            } else {
                isAtomicAnimationActive_ = false;
                handleNextWindowChange();
            }
        });

        anim->start(QPropertyAnimation::DeleteWhenStopped);
    } else if (curWindow_ == WINDOW_ID_GENERAL_MESSAGE) {
        // qDebug() << "Message -> Connect";
        curWindow_ = WINDOW_ID_CONNECT;
        isAtomicAnimationActive_ = true;
        updateMaskForGraphicsView();

        shadowManager_->setVisible(ShadowManager::SHAPE_ID_GENERAL_MESSAGE, false);
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_CONNECT_WINDOW, true);

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(generalMessageWindow_);
        anim->setPropertyName("opacity");
        anim->setStartValue(generalMessageWindow_->opacity());
        anim->setEndValue(0.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        connect(anim, &QPropertyAnimation::finished, [this, expandPrefs]() {
            generalMessageWindow_->hide();
            connectWindow_->setClickable(true);
            bottomInfoWindow_->setClickable(true);

            if (expandPrefs) {
                bottomInfoWindow_->setVisible(false);
                expandWindow(preferencesWindow_);
            } else {
                if (bottomInfoWindow_->isVisible()) {
                    std::function<void()> finish_function = [this](){
                        isAtomicAnimationActive_ = false;
                        updateBottomInfoWindowVisibilityAndPos();
                        handleNextWindowChange();
                    };

                    animateBottomInfoWindow(QAbstractAnimation::Backward, finish_function);
                } else {
                    isAtomicAnimationActive_ = false;
                    handleNextWindowChange();
                }
            }
        });

        anim->start(QPropertyAnimation::DeleteWhenStopped);
    } else if (curWindow_ == WINDOW_ID_EXTERNAL_CONFIG) {
        // qDebug() << "External Config -> Connect";
        externalConfigWindow_->stackBefore(connectWindow_);

        curWindow_ = WINDOW_ID_CONNECT;
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_LOGIN_WINDOW, false);
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_CONNECT_WINDOW, true);
        connectWindow_->setClickable(true);
        connectWindow_->setVisible(true);
        externalConfigWindow_->setVisible(false);
        externalConfigWindow_->setClickable(false);

        updateMainAndViewGeometry(false);

        hideLocationsWindow();

        updateBottomInfoWindowVisibilityAndPos();

        updateExpandAnimationParameters();
        handleNextWindowChange();
    } else if (curWindow_ == WINDOW_ID_TWO_FACTOR_AUTH) {
        // qDebug() << "2FA -> Connect";
        twoFactorAuthWindow_->stackBefore(connectWindow_);

        curWindow_ = WINDOW_ID_CONNECT;
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_LOGIN_WINDOW, false);
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_CONNECT_WINDOW, true);
        connectWindow_->setClickable(true);
        connectWindow_->setVisible(true);
        twoFactorAuthWindow_->resetState();
        twoFactorAuthWindow_->setVisible(false);
        twoFactorAuthWindow_->setClickable(false);

        updateMainAndViewGeometry(false);

        hideLocationsWindow();

        updateBottomInfoWindowVisibilityAndPos();

        updateExpandAnimationParameters();
        handleNextWindowChange();
    }
}

void MainWindowController::gotoExternalConfigWindow()
{
    WS_ASSERT(curWindow_ == WINDOW_ID_LOGIN);

    isAtomicAnimationActive_ = true;
    curWindow_ = WINDOW_ID_EXTERNAL_CONFIG;

    loginWindow_->setClickable(false);
    TooltipController::instance().hideAllTooltips();
    loginWindow_->stackBefore(externalConfigWindow_);

    externalConfigWindow_->setOpacity(0.0);
    externalConfigWindow_->setVisible(true);

    QPropertyAnimation *anim = new QPropertyAnimation(this);
    anim->setTargetObject(externalConfigWindow_);
    anim->setPropertyName("opacity");
    anim->setStartValue(externalConfigWindow_->opacity());
    anim->setEndValue(0.96);
    anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

    connect(anim, &QPropertyAnimation::finished, [this]() {
        loginWindow_->setVisible(false);
        isAtomicAnimationActive_ = false;
        handleNextWindowChange();
        externalConfigWindow_->setClickable(true);
        externalConfigWindow_->setFocus();
    });

    isAtomicAnimationActive_ = true;

    anim->start(QPropertyAnimation::DeleteWhenStopped);
}

void MainWindowController::gotoTwoFactorAuthWindow()
{
    WS_ASSERT(curWindow_ == WINDOW_ID_LOGIN || curWindow_ == WINDOW_ID_LOGGING_IN);

    const auto prevWindow = curWindow_;
    curWindow_ = WINDOW_ID_TWO_FACTOR_AUTH;

    TooltipController::instance().hideAllTooltips();

    switch (prevWindow) {
    case WINDOW_ID_LOGIN:
        loginWindow_->setClickable(false);
        loginWindow_->stackBefore(twoFactorAuthWindow_);
        break;
    case WINDOW_ID_LOGGING_IN:
        loggingInWindow_->stackBefore(twoFactorAuthWindow_);
        break;
    default:
        break;
    }

    twoFactorAuthWindow_->resetState();
    twoFactorAuthWindow_->setOpacity(0.0);
    twoFactorAuthWindow_->setVisible(true);

    QPropertyAnimation *anim = new QPropertyAnimation(this);
    anim->setTargetObject(twoFactorAuthWindow_);
    anim->setPropertyName("opacity");
    anim->setStartValue(twoFactorAuthWindow_->opacity());
    anim->setEndValue(1.0);
    anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

    connect(anim, &QPropertyAnimation::finished, [this, prevWindow]() {
        switch (prevWindow) {
        case WINDOW_ID_LOGIN:
            loginWindow_->setVisible(false);
            break;
        case WINDOW_ID_LOGGING_IN:
            loggingInWindow_->setVisible(false);
            loggingInWindow_->stopAnimation();
        default:
            break;
        }
        isAtomicAnimationActive_ = false;
        handleNextWindowChange();
        twoFactorAuthWindow_->setClickable(true);
        twoFactorAuthWindow_->setFocus();
    });

    isAtomicAnimationActive_ = true;

    anim->start(QPropertyAnimation::DeleteWhenStopped);
}

void MainWindowController::gotoUpdateWindow()
{
    WS_ASSERT(curWindow_ == WINDOW_ID_CONNECT
             || curWindow_ == WINDOW_ID_UPGRADE
             || curWindow_ == WINDOW_ID_GENERAL_MESSAGE
             || curWindow_ == WINDOW_ID_EXIT);

    for (auto w : windowSizeManager_->windows()) {
        if (windowSizeManager_->state(w) != WindowSizeManager::kWindowCollapsed) {
            queueWindowChanges_.enqueue(WINDOW_ID_UPDATE);
            collapseWindow(w, true);
            return;
        }
    }

    isAtomicAnimationActive_ = true;
    WINDOW_ID saveCurWindow = curWindow_;
    curWindow_ = WINDOW_ID_UPDATE;
    if (saveCurWindow == WINDOW_ID_GENERAL_MESSAGE) {
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_GENERAL_MESSAGE, false);
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_CONNECT_WINDOW, true);
        updateMaskForGraphicsView();
    }

    TooltipController::instance().hideAllTooltips();
    connectWindow_->setClickable(false);
    bottomInfoWindow_->setClickable(false);

    functionOnAnimationFinished_ = [this, saveCurWindow]() {
        updateWindow_->setOpacity(0.0);
        if (saveCurWindow == WINDOW_ID_GENERAL_MESSAGE) {
            generalMessageWindow_->stackBefore(updateWindow_);
        } else {
            connectWindow_->stackBefore(updateWindow_);
        }
        updateWindow_->show();

        QPropertyAnimation *anim1 = new QPropertyAnimation(this);
        anim1->setTargetObject(updateWindow_);
        anim1->setPropertyName("opacity");
        anim1->setStartValue(updateWindow_->opacity());
        anim1->setEndValue(0.96);
        anim1->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        // anim2 was removed since WINDOW_ID_NOTIFICATION is no longer valid

        QPropertyAnimation *anim3 = NULL;
        if (saveCurWindow == WINDOW_ID_UPGRADE) {
            anim3 = new QPropertyAnimation(this);
            anim3->setTargetObject(upgradeAccountWindow_);
            anim3->setPropertyName("opacity");
            anim3->setStartValue(upgradeAccountWindow_->opacity());
            anim3->setEndValue(0.0);
            anim3->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

            connect(anim3, &QPropertyAnimation::finished, [this]() {
                upgradeAccountWindow_->hide();
            });
        }

        QPropertyAnimation *anim4 = NULL;
        if (saveCurWindow == WINDOW_ID_GENERAL_MESSAGE) {
            anim4 = new QPropertyAnimation(this);
            anim4->setTargetObject(generalMessageWindow_);
            //anim4->setPropertyName("opacity");
            anim4->setStartValue(generalMessageWindow_->opacity());
            anim4->setEndValue(0.0);
            anim4->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

            connect(anim4, &QPropertyAnimation::finished, [this]()
            {
                generalMessageWindow_->hide();
                updateWindow_->setClickable(true);
            });
        }

        QPropertyAnimation *anim5 = NULL;
        if (saveCurWindow == WINDOW_ID_EXIT) {
            anim5 = new QPropertyAnimation(this);
            anim5->setTargetObject(exitWindow_);
            anim5->setPropertyName("opacity");
            anim5->setStartValue(exitWindow_->opacity());
            anim5->setEndValue(0.0);
            anim5->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

            connect(anim5, &QPropertyAnimation::finished, [this]() {
                exitWindow_->hide();
            });
        }

        QPropertyAnimation *anim6 = NULL;
        if (saveCurWindow == WINDOW_ID_LOGOUT) {
            anim6 = new QPropertyAnimation(this);
            anim6->setTargetObject(logoutWindow_);
            anim6->setPropertyName("opacity");
            anim6->setStartValue(logoutWindow_->opacity());
            anim6->setEndValue(0.0);
            anim6->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

            connect(anim6, &QPropertyAnimation::finished, [this]() {
                logoutWindow_->hide();
            });
        }

        QParallelAnimationGroup *group = new QParallelAnimationGroup(this);
        group->addAnimation(anim1);
        if (anim3 != NULL) {
            group->addAnimation(anim3);
        }
        if (anim4 != NULL) {
            group->addAnimation(anim4);
        }
        if (anim5 != NULL) {
            group->addAnimation(anim5);
        }
        if (anim6 != NULL) {
            group->addAnimation(anim6);
        }

        connect(group, &QPropertyAnimation::finished, [this]() {
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
            updateWindow_->setFocus();
        });

        group->start(QPropertyAnimation::DeleteWhenStopped);
    };

    collapseAllExpandedOnBottom();
}

void MainWindowController::gotoUpgradeWindow()
{
    WS_ASSERT(curWindow_ == WINDOW_ID_CONNECT);

    isAtomicAnimationActive_ = true;
    curWindow_ = WINDOW_ID_UPGRADE;

    TooltipController::instance().hideAllTooltips();
    connectWindow_->setClickable(false);
    bottomInfoWindow_->setClickable(false);

    functionOnAnimationFinished_ = [this]() {
        upgradeAccountWindow_->setOpacity(0.0);
        connectWindow_->stackBefore(upgradeAccountWindow_);
        upgradeAccountWindow_->show();

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(upgradeAccountWindow_);
        anim->setPropertyName("opacity");
        anim->setStartValue(upgradeAccountWindow_->opacity());
        anim->setEndValue(0.96);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        connect(anim, &QPropertyAnimation::finished, [this]() {
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
            upgradeAccountWindow_->setFocus();
        });

        anim->start(QPropertyAnimation::DeleteWhenStopped);
    };

    collapseAllExpandedOnBottom();
}

void MainWindowController::gotoGeneralMessageWindow()
{
    WS_ASSERT(curWindow_ == WINDOW_ID_INITIALIZATION ||
              curWindow_ == WINDOW_ID_LOGIN ||
              curWindow_ == WINDOW_ID_LOGGING_IN ||
              curWindow_ == WINDOW_ID_CONNECT ||
              curWindow_ == WINDOW_ID_UPDATE ||
              curWindow_ == WINDOW_ID_LOGOUT ||
              curWindow_ == WINDOW_ID_EXIT);

    if (curWindow_ == WINDOW_ID_LOGOUT || curWindow_ == WINDOW_ID_EXIT) {
        // Do not transition while on logout/exit screen
        return;
    }

    isAtomicAnimationActive_ = true;
    WINDOW_ID saveCurWindow = curWindow_;

    if (curWindow_ == WINDOW_ID_INITIALIZATION || curWindow_ == WINDOW_ID_LOGGING_IN || curWindow_ == WINDOW_ID_LOGIN) {
        generalMessageWindow_->setBackgroundShape(GeneralMessageWindow::kLoginScreenShape);
    } else if (preferences_->appSkin() == APP_SKIN_VAN_GOGH) {
        generalMessageWindow_->setBackgroundShape(GeneralMessageWindow::kConnectScreenVanGoghShape);
    } else {
        generalMessageWindow_->setBackgroundShape(GeneralMessageWindow::kConnectScreenAlphaShape);
    }

    shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_GENERAL_MESSAGE,
                                        QRect(0,
                                              childWindowShadowOffsetY(true),
                                              generalMessageWindow_->boundingRect().width(),
                                              generalMessageWindow_->boundingRect().height() - childWindowShadowOffsetY(false)));

    TooltipController::instance().hideAllTooltips();
    for (auto w : windowSizeManager_->windows()) {
        collapseWindow(w, false, true);
    }

    curWindow_ = WINDOW_ID_GENERAL_MESSAGE;
    updateMaskForGraphicsView();
    shadowManager_->setVisible(ShadowManager::SHAPE_ID_CONNECT_WINDOW, false);
    shadowManager_->setVisible(ShadowManager::SHAPE_ID_GENERAL_MESSAGE, true);

    connectWindow_->setClickable(false);
    bottomInfoWindow_->setClickable(false);
    updateWindow_->setClickable(false);
    loginWindow_->setClickable(false);

    functionOnAnimationFinished_ = [this, saveCurWindow]() {
        generalMessageWindow_->setOpacity(0.0);
        preferencesWindow_->stackBefore(generalMessageWindow_);
        connectWindow_->stackBefore(generalMessageWindow_);
        updateWindow_->stackBefore(generalMessageWindow_);
        loginWindow_->stackBefore(generalMessageWindow_);
        initWindow_->stackBefore(generalMessageWindow_);
        loggingInWindow_->stackBefore(generalMessageWindow_);
        generalMessageWindow_->show();

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(generalMessageWindow_);
        anim->setPropertyName("opacity");
        anim->setStartValue(generalMessageWindow_->opacity());
        anim->setEndValue(1.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        connect(anim, &QPropertyAnimation::finished, [this]() {
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
            generalMessageWindow_->setFocus();
        });

        anim->start(QPropertyAnimation::DeleteWhenStopped);
    };

    updateMainAndViewGeometry(false);
    collapseAllExpandedOnBottom();
}

void MainWindowController::gotoExitWindow(bool isLogout)
{
    WS_ASSERT(curWindow_ == WINDOW_ID_CONNECT
             || curWindow_ == WINDOW_ID_LOGIN
             || curWindow_ == WINDOW_ID_EMERGENCY
             || curWindow_ == WINDOW_ID_EXTERNAL_CONFIG
             || curWindow_ == WINDOW_ID_TWO_FACTOR_AUTH
             || curWindow_ == WINDOW_ID_GENERAL_MESSAGE
             || curWindow_ == WINDOW_ID_LOGOUT);

    // If we're overriding a logout with a quit (e.g. user pressed alt-f4 while on the logout prompt),
    // close the previous logout window first
    if (curWindow_ == WINDOW_ID_LOGOUT) {
        // Suppress expanding preferences window even though originally logout came from prefs.
        closeExitWindow(false);
    }
    windowBeforeExit_ = curWindow_;
    GeneralMessageWindow::GeneralMessageWindowItem *win = (isLogout ? logoutWindow_ : exitWindow_);

    TooltipController::instance().hideAllTooltips();
    for (auto w : windowSizeManager_->windows()) {
        collapseWindow(w, false, true);
    }

    if (curWindow_ == WINDOW_ID_CONNECT) {
        if (preferences_->appSkin() == APP_SKIN_VAN_GOGH) {
            win->setBackgroundShape(GeneralMessageWindow::kConnectScreenVanGoghShape);
        } else {
            win->setBackgroundShape(GeneralMessageWindow::kConnectScreenAlphaShape);
        }
        connectWindow_->setClickable(false);
        bottomInfoWindow_->setClickable(false);
    } else if (curWindow_ == WINDOW_ID_GENERAL_MESSAGE) {
        win->setBackgroundShape(generalMessageWindow_->backgroundShape());
    } else {
        win->setBackgroundShape(GeneralMessageWindow::kLoginScreenShape);
        if (curWindow_ == WINDOW_ID_LOGIN) {
            loginWindow_->setClickable(false);
        } else if (curWindow_ == WINDOW_ID_EMERGENCY) {
            emergencyConnectWindow_->setClickable(false);
        } else if (curWindow_ == WINDOW_ID_EXTERNAL_CONFIG) {
            externalConfigWindow_->setClickable(false);
        } else if (curWindow_ == WINDOW_ID_TWO_FACTOR_AUTH) {
            twoFactorAuthWindow_->setClickable(false);
        }
    }

    shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_EXIT,
                                        QRect(0,
                                            childWindowShadowOffsetY(true),
                                            win->boundingRect().width(),
                                            win->boundingRect().height() - childWindowShadowOffsetY(false)));
    if (win->backgroundShape() == GeneralMessageWindow::Shape::kConnectScreenAlphaShape ||
        curWindow_ == WINDOW_ID_GENERAL_MESSAGE)
    {
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_EXIT, true);
    }

    WINDOW_ID saveCurWindow = curWindow_;
    curWindow_ = isLogout ? WINDOW_ID_LOGOUT : WINDOW_ID_EXIT;
    updateMaskForGraphicsView();
    isAtomicAnimationActive_ = true;

    shadowManager_->setVisible(ShadowManager::SHAPE_ID_GENERAL_MESSAGE, false);

    functionOnAnimationFinished_ = [this, win, saveCurWindow]() {
        win->setOpacity(0.0);
        connectWindow_->stackBefore(win);
        loginWindow_->stackBefore(win);
        emergencyConnectWindow_->stackBefore(win);
        externalConfigWindow_->stackBefore(win);
        generalMessageWindow_->stackBefore(win);
        win->show();

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(win);
        anim->setPropertyName("opacity");
        anim->setStartValue(win->opacity());
        anim->setEndValue(1.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        connect(anim, &QPropertyAnimation::finished, [this, win]() {
            isAtomicAnimationActive_ = false;
            generalMessageWindow_->hide();
            handleNextWindowChange();
            win->setFocus();
        });

        anim->start(QPropertyAnimation::DeleteWhenStopped);
    };

    updateMainAndViewGeometry(false);
    collapseAllExpandedOnBottom();
}

void MainWindowController::closeExitWindow(bool fromPrefs)
{
    GeneralMessageWindow::GeneralMessageWindowItem *win;
    if (curWindow_ == WINDOW_ID_LOGOUT) {
        win = logoutWindow_;
    } else if (curWindow_ == WINDOW_ID_EXIT) {
        win = exitWindow_;
    } else {
        WS_ASSERT(false);
        return;
    }

    if (windowBeforeExit_ == WINDOW_ID_CONNECT) {
        curWindow_ = WINDOW_ID_CONNECT;
        isAtomicAnimationActive_ = true;

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(win);
        anim->setPropertyName("opacity");
        anim->setStartValue(win->opacity());
        anim->setEndValue(0.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        shadowManager_->setVisible(ShadowManager::SHAPE_ID_EXIT, false);
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_CONNECT_WINDOW, true);

        connect(anim, &QPropertyAnimation::finished, [this, fromPrefs, win]() {
            win->hide();
            connectWindow_->setClickable(true);
            bottomInfoWindow_->setClickable(true);

            if (GeneralMessageController::instance().hasMessages()) {
                gotoGeneralMessageWindow();
            } else if (fromPrefs) {
                bottomInfoWindow_->setVisible(false);
                expandWindow(preferencesWindow_);
            } else {
                if (bottomInfoWindow_->isVisible()) {
                    std::function<void()> finish_function = [this]() {
                        isAtomicAnimationActive_ = false;
                        updateBottomInfoWindowVisibilityAndPos();
                        handleNextWindowChange();
                    };

                    animateBottomInfoWindow(QAbstractAnimation::Backward, finish_function);
                } else {
                    isAtomicAnimationActive_ = false;
                    handleNextWindowChange();
                }
            }
        });

        anim->start(QPropertyAnimation::DeleteWhenStopped);
    } else if (windowBeforeExit_ == WINDOW_ID_LOGIN) {
        curWindow_ = WINDOW_ID_LOGIN;
        isAtomicAnimationActive_ = true;

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(win);
        anim->setPropertyName("opacity");
        anim->setStartValue(win->opacity());
        anim->setEndValue(0.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        shadowManager_->setVisible(ShadowManager::SHAPE_ID_EXIT, false);

        connect(anim, &QPropertyAnimation::finished, [this, win]() {
            if (GeneralMessageController::instance().hasMessages()) {
                gotoGeneralMessageWindow();
            } else {
                win->hide();
                loginWindow_->setClickable(true);
                loginWindow_->setFocus();
                isAtomicAnimationActive_ = false;
                handleNextWindowChange();
            }
        });

        anim->start(QPropertyAnimation::DeleteWhenStopped);
    } else if (windowBeforeExit_ == WINDOW_ID_EMERGENCY) {
        curWindow_ = WINDOW_ID_EMERGENCY;
        isAtomicAnimationActive_ = true;

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(win);
        anim->setPropertyName("opacity");
        anim->setStartValue(win->opacity());
        anim->setEndValue(0.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        shadowManager_->setVisible(ShadowManager::SHAPE_ID_EXIT, false);

        connect(anim, &QPropertyAnimation::finished, [this, win]() {
            if (GeneralMessageController::instance().hasMessages()) {
                gotoGeneralMessageWindow();
            } else {
                win->hide();
                emergencyConnectWindow_->setClickable(true);
                emergencyConnectWindow_->setFocus();
                isAtomicAnimationActive_ = false;
                handleNextWindowChange();
            }
        });

        anim->start(QPropertyAnimation::DeleteWhenStopped);
    } else if (windowBeforeExit_ == WINDOW_ID_EXTERNAL_CONFIG) {
        curWindow_ = WINDOW_ID_EXTERNAL_CONFIG;
        isAtomicAnimationActive_ = true;

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(win);
        anim->setPropertyName("opacity");
        anim->setStartValue(win->opacity());
        anim->setEndValue(0.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        shadowManager_->setVisible(ShadowManager::SHAPE_ID_EXIT, false);

        connect(anim, &QPropertyAnimation::finished, [this, win]() {
            if (GeneralMessageController::instance().hasMessages()) {
                gotoGeneralMessageWindow();
            } else {
                win->hide();
                externalConfigWindow_->setClickable(true);
                externalConfigWindow_->setFocus();
                isAtomicAnimationActive_ = false;
                handleNextWindowChange();
            }
        });

        anim->start(QPropertyAnimation::DeleteWhenStopped);
    } else if (windowBeforeExit_ == WINDOW_ID_TWO_FACTOR_AUTH) {
        curWindow_ = WINDOW_ID_TWO_FACTOR_AUTH;
        isAtomicAnimationActive_ = true;

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(win);
        anim->setPropertyName("opacity");
        anim->setStartValue(win->opacity());
        anim->setEndValue(0.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        shadowManager_->setVisible(ShadowManager::SHAPE_ID_EXIT, false);

        connect(anim, &QPropertyAnimation::finished, [this, win]() {
            if (GeneralMessageController::instance().hasMessages()) {
                gotoGeneralMessageWindow();
            } else {
                win->hide();
                twoFactorAuthWindow_->setClickable(true);
                twoFactorAuthWindow_->setFocus();
                isAtomicAnimationActive_ = false;
                handleNextWindowChange();
            }
        });

        anim->start(QPropertyAnimation::DeleteWhenStopped);
    } else if (windowBeforeExit_ == WINDOW_ID_INITIALIZATION) {
        curWindow_ = WINDOW_ID_INITIALIZATION;
        isAtomicAnimationActive_ = true;

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(win);
        anim->setPropertyName("opacity");
        anim->setStartValue(win->opacity());
        anim->setEndValue(0.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        shadowManager_->setVisible(ShadowManager::SHAPE_ID_EXIT, false);

        connect(anim, &QPropertyAnimation::finished, [this, win]() {
            if (GeneralMessageController::instance().hasMessages()) {
                gotoGeneralMessageWindow();
            } else {
                win->hide();
                isAtomicAnimationActive_ = false;
                updateBottomInfoWindowVisibilityAndPos();
                handleNextWindowChange();
            }
        });

        anim->start(QPropertyAnimation::DeleteWhenStopped);
    } else if (windowBeforeExit_ == WINDOW_ID_GENERAL_MESSAGE) {
        curWindow_ = WINDOW_ID_GENERAL_MESSAGE;
        isAtomicAnimationActive_ = true;

        generalMessageWindow_->stackBefore(win);
        generalMessageWindow_->show();
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_EXIT, false);
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_GENERAL_MESSAGE, true);

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(win);
        anim->setPropertyName("opacity");
        anim->setStartValue(win->opacity());
        anim->setEndValue(0.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        connect(anim, &QPropertyAnimation::finished, [this, win]() {
            win->hide();
            isAtomicAnimationActive_ = false;
            updateBottomInfoWindowVisibilityAndPos();
            handleNextWindowChange();
        });

        anim->start(QPropertyAnimation::DeleteWhenStopped);
    }

}

void MainWindowController::expandPreferencesFromLogin()
{
    // qCDebug(LOG_BASIC) << "MainWindowController::expandPreferencesFromLogin";

    if (windowSizeManager_->state(preferencesWindow_) != WindowSizeManager::kWindowCollapsed) {
        return;
    }

    isAtomicAnimationActive_ = true;
    windowSizeManager_->setState(preferencesWindow_, WindowSizeManager::kWindowAnimating);

    preferencesWindow_->setScrollBarVisibility(false);
    preferencesWindow_->setOpacity(0.0);
    preferencesWindow_->show();
    updateMainAndViewGeometry(false);

    shadowManager_->setOpacity(ShadowManager::SHAPE_ID_PREFERENCES, 0.0, true);
    shadowManager_->setVisible(ShadowManager::SHAPE_ID_PREFERENCES, true);

    shadowManager_->setOpacity(ShadowManager::SHAPE_ID_CONNECT_WINDOW, 0.0, true);
    shadowManager_->setVisible(ShadowManager::SHAPE_ID_CONNECT_WINDOW, true);

    // opacity
    QVariantAnimation *animOpacity = new QVariantAnimation(this);
    animOpacity->setStartValue(0.0);
    animOpacity->setEndValue(1.0);
    animOpacity->setDuration(EXPAND_CHILD_WINDOW_OPACITY_DURATION);
    connect(animOpacity, &QVariantAnimation::valueChanged, [this](const QVariant &value) {
        double prefOpacity = value.toDouble();
        preferencesWindow_->setOpacity(prefOpacity);
        shadowManager_->setOpacity(ShadowManager::SHAPE_ID_PREFERENCES, prefOpacity, true);
        shadowManager_->setOpacity(ShadowManager::SHAPE_ID_LOGIN_WINDOW, 1.0 - prefOpacity, true);
        loginWindow_->setOpacity(1.0 - prefOpacity);
        shadowManager_->setOpacity(ShadowManager::SHAPE_ID_CONNECT_WINDOW, prefOpacity, false);
    });

    int start = (int)loginWindow_->boundingRect().height();
    int target =  windowSizeManager_->windowHeight(preferencesWindow_)*G_SCALE;
    QVariantAnimation *animResize = new QVariantAnimation(this);
    animResize->setStartValue(start);
    animResize->setEndValue(target);
    animResize->setDuration(EXPAND_PREFERENCES_RESIZE_DURATION);
    connect(animResize, &QVariantAnimation::valueChanged, [this](const QVariant &value) {
        preferencesWindow_->setHeight(value.toInt());
        updateMainAndViewGeometry(false);
        shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_PREFERENCES,
                                            QRect(0, childWindowShadowOffsetY(true), preferencesWindow_->boundingRect().width(),
                                                  preferencesWindow_->boundingRect().height() - childWindowShadowOffsetY(false)));
        invalidateShadow_mac();
        keepWindowInsideScreenCoordinates();
    });

    // resize finished
    connect(animResize, &QVariantAnimation::finished, [this]() {
        loginWindow_->hide();
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_LOGIN_WINDOW, false);
        preferencesWindow_->setScrollBarVisibility(true);
        preferencesWindow_->setFocus();
    });

    loginWindow_->setClickable(false);

    // group finished
    QParallelAnimationGroup *animGroup = new QParallelAnimationGroup(this);
    connect(animGroup, &QVariantAnimation::finished, [this]() {
        windowSizeManager_->setState(preferencesWindow_, WindowSizeManager::kWindowExpanded);
        updateCursorInViewport();
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_PREFERENCES, true);
        shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_PREFERENCES,
                                            QRect(0, childWindowShadowOffsetY(true),
                                                  preferencesWindow_->boundingRect().width(),
                                                  preferencesWindow_->boundingRect().height() - childWindowShadowOffsetY(false)));
        preferencesWindow_->setScrollPos(windowSizeManager_->scrollPos(preferencesWindow_));
        invalidateShadow_mac();

        isAtomicAnimationActive_ = false;
        handleNextWindowChange();
    });

    animGroup->addAnimation(animOpacity);
    animGroup->addAnimation(animResize);

    animGroup->start(QVariantAnimation::DeleteWhenStopped);

    //preferencesWindow_->setFocus();
}

void MainWindowController::expandWindow(ResizableWindow *window)
{
    // qCDebug(LOG_BASIC) << "MainWindowController::expandWindow";

    if (windowSizeManager_->state(window) != WindowSizeManager::kWindowCollapsed) {
        return;
    }

    isAtomicAnimationActive_ = true;

    functionOnAnimationFinished_ = [this, window]() {
        if (expandLocationsAnimationGroup_ && expandLocationsAnimationGroup_->state() == QAbstractAnimation::Running) {
            WS_ASSERT(false);
            return;
        }

        TooltipController::instance().hideAllTooltips();

        windowSizeManager_->setState(window, WindowSizeManager::kWindowAnimating);

        int start = (int)connectWindow_->boundingRect().height();
        for (auto w : windowSizeManager_->windows()) {
            if (w != window && windowSizeManager_->state(w) == WindowSizeManager::kWindowExpanded) {
                 start = (int)w->boundingRect().height();
                 break;
            }
        }
        int target = windowSizeManager_->windowHeight(window)*G_SCALE;
        window->setHeight(start);

        window->setScrollBarVisibility(false);
        window->setOpacity(0.0);
        window->show();
        updateMainAndViewGeometry(false);

        shadowManager_->setOpacity(windowSizeManager_->shapeId(window), 0.0, true);
        shadowManager_->setVisible(windowSizeManager_->shapeId(window), true);

        // opacity changed
        QVariantAnimation *animOpacity = new QVariantAnimation(this);
        animOpacity->setStartValue(0.0);
        animOpacity->setEndValue(1.0);
        animOpacity->setDuration(EXPAND_CHILD_WINDOW_OPACITY_DURATION);
        connect(animOpacity, &QVariantAnimation::valueChanged, [this, window](const QVariant &value) {
            window->setOpacity(value.toDouble());
            shadowManager_->setOpacity(windowSizeManager_->shapeId(window), value.toDouble(), true);
            for (auto w : windowSizeManager_->windows()) {
                if (w != window && windowSizeManager_->state(w) == WindowSizeManager::kWindowExpanded) {
                    w->setOpacity(1 - value.toDouble());
                    shadowManager_->setOpacity(windowSizeManager_->shapeId(w), 1 - value.toDouble(), true);
                }
            }
        });

        QVariantAnimation *animResize = new QVariantAnimation(this);
        animResize->setStartValue(start);
        animResize->setEndValue(target);
        animResize->setDuration(windowSizeManager_->resizeDurationMs(window));
        connect(animResize, &QVariantAnimation::valueChanged, [this, window](const QVariant &value) {
            window->setHeight(value.toInt());
            updateMainAndViewGeometry(false);
            shadowManager_->changeRectangleSize(windowSizeManager_->shapeId(window),
                                                QRect(0,
                                                      childWindowShadowOffsetY(true),
                                                      window->boundingRect().width(),
                                                      window->boundingRect().height() - childWindowShadowOffsetY(false)));
            invalidateShadow_mac();
            keepWindowInsideScreenCoordinates();
        });

        // resize finished
        connect(animResize, &QVariantAnimation::finished, [this, window]() {
            for (auto w : windowSizeManager_->windows()) {
                if (w != window && windowSizeManager_->state(w) == WindowSizeManager::kWindowExpanded) {
                    w->hide();
                    windowSizeManager_->setState(w, WindowSizeManager::kWindowCollapsed);
                }
            }
            connectWindow_->hide();
        });

        // group finished
        QParallelAnimationGroup *animGroup = new QParallelAnimationGroup(this);
        connect(animGroup, &QVariantAnimation::finished, [this, window]() {
            windowSizeManager_->setState(window, WindowSizeManager::kWindowExpanded);
            clearMaskForGraphicsView();
            updateBottomInfoWindowVisibilityAndPos();
            bottomInfoWindow_->setClickable(false);
            updateExpandAnimationParameters();
            invalidateShadow_mac();
            window->setScrollBarVisibility(true);
            window->setFocus();
            if (window == newsFeedWindow_) {
                newsFeedWindow_->updateRead();
            }
            window->setScrollPos(windowSizeManager_->scrollPos(window));

            shadowManager_->setVisible(windowSizeManager_->shapeId(window), true);
            shadowManager_->changeRectangleSize(windowSizeManager_->shapeId(window),
                                                QRect(0,
                                                      childWindowShadowOffsetY(true),
                                                      window->boundingRect().width(),
                                                      window->boundingRect().height() - childWindowShadowOffsetY(false)));

            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
        });

        // delay opacity
        QSequentialAnimationGroup *seqGroup = new QSequentialAnimationGroup(this);
        seqGroup->addPause(windowSizeManager_->resizeDurationMs(window) - EXPAND_CHILD_WINDOW_OPACITY_DURATION);
        seqGroup->addAnimation(animOpacity);

        animGroup->addAnimation(animResize);
        animGroup->addAnimation(seqGroup);
        animGroup->start(QVariantAnimation::DeleteWhenStopped);
    };

    isAtomicAnimationActive_ = true;

    connectWindow_->setClickable(false);

    // hide locations or bottomInfo before running window expand
    if (isLocationsExpanded()) {
        connectWindow_->updateLocationsState(false);
        expandLocationsAnimationGroup_->setDirection(QAbstractAnimation::Backward);
        if (expandLocationsAnimationGroup_->state() != QAbstractAnimation::Running) {
            expandLocationsAnimationGroup_->start();
        }
    } else if (bottomInfoWindow_->isVisible()) {
        std::function<void()> finish_function = [this]() {
            if (functionOnAnimationFinished_ != NULL) {
                functionOnAnimationFinished_();
                functionOnAnimationFinished_ = NULL;
            }
        };
        animateBottomInfoWindow(QAbstractAnimation::Forward, finish_function);
    } else {
        if (functionOnAnimationFinished_ != NULL) {
            functionOnAnimationFinished_();
            functionOnAnimationFinished_ = NULL;
        }
    }
}

void MainWindowController::collapsePreferencesFromLogin()
{
    // qCDebug(LOG_BASIC) << "MainWindowController::collapsePreferencesFromLogin";

    if (windowSizeManager_->state(preferencesWindow_) != WindowSizeManager::kWindowExpanded) {
        return;
    }

    isAtomicAnimationActive_ = true;

    hideUpdateWidget();

    preferencesWindow_->setScrollBarVisibility(false);
    TooltipController::instance().hideAllTooltips();

    windowSizeManager_->setScrollPos(preferencesWindow_, preferencesWindow_->scrollPos());
    windowSizeManager_->setState(preferencesWindow_, WindowSizeManager::kWindowAnimating);
    loginWindow_->show();

    shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_LOGIN_WINDOW,
                                        QRect(0,0, loginWindow_->boundingRect().width(),
                                              loginWindow_->boundingRect().height()));

    shadowManager_->setVisible(ShadowManager::SHAPE_ID_LOGIN_WINDOW, true);
    shadowManager_->setOpacity(ShadowManager::SHAPE_ID_LOGIN_WINDOW, 0.0, false);

    // Opacity
    QVariantAnimation *animOpacity = new QVariantAnimation(this);
    animOpacity->setStartValue(1.0);
    animOpacity->setEndValue(0.0);
    animOpacity->setDuration(EXPAND_CHILD_WINDOW_OPACITY_DURATION);
    connect(animOpacity, &QVariantAnimation::valueChanged, [this](const QVariant &value) {
        double prefOpacity = value.toDouble();
        preferencesWindow_->setOpacity(prefOpacity);
        shadowManager_->setOpacity(ShadowManager::SHAPE_ID_PREFERENCES, prefOpacity, true);
        shadowManager_->setOpacity(ShadowManager::SHAPE_ID_LOGIN_WINDOW, 1.0 - prefOpacity, false);
        loginWindow_->setOpacity(1.0 - prefOpacity);
        shadowManager_->setOpacity(ShadowManager::SHAPE_ID_CONNECT_WINDOW, prefOpacity, false);
    });

    // Resizing
    QVariantAnimation *animResize = new QVariantAnimation(this);
    animResize->setStartValue((int)preferencesWindow_->boundingRect().height());
    animResize->setEndValue((int)loginWindow_->boundingRect().height());
    animResize->setDuration(EXPAND_PREFERENCES_RESIZE_DURATION);
    connect(animResize, &QVariantAnimation::valueChanged, [this](const QVariant &value) {
        preferencesWindow_->setHeight(value.toInt());
        shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_NEWS_FEED,
                                            QRect(0, childWindowShadowOffsetY(true), preferencesWindow_->boundingRect().width(),
                                                  preferencesWindow_->boundingRect().height() - childWindowShadowOffsetY(false)));
        updateMainAndViewGeometry(false);
        invalidateShadow_mac();
    });

    // resize finished
    connect(animResize, &QVariantAnimation::finished, [this]() {
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_PREFERENCES, true);
        preferencesWindow_->hide();
    });

    // group finished
    QParallelAnimationGroup *animGroup = new QParallelAnimationGroup(this);
    connect(animGroup, &QVariantAnimation::finished, [this]() {
        loginWindow_->setClickable(true);
        loginWindow_->setFocus();
        windowSizeManager_->setState(preferencesWindow_, WindowSizeManager::kWindowCollapsed);
        emit preferencesCollapsed();
        shadowManager_->setOpacity(ShadowManager::SHAPE_ID_CONNECT_WINDOW, 1.0, false);
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_LOGIN_WINDOW, true);
        updateMainAndViewGeometry(false);
        invalidateShadow_mac();
        isAtomicAnimationActive_ = false;
        handleNextWindowChange();
    });

    QSequentialAnimationGroup *seqGroup = new QSequentialAnimationGroup(this);
    seqGroup->addPause(50);
    seqGroup->addAnimation(animOpacity);

    animGroup->addAnimation(animResize);
    animGroup->addAnimation(seqGroup);

    animGroup->start(QVariantAnimation::DeleteWhenStopped);
}

void MainWindowController::collapseWindow(ResizableWindow *window, bool bSkipBottomInfoWindowAnimate, bool bSkipSetClickable)
{
    // qCDebug(LOG_BASIC) << "MainWindowController::collapseWindow";

    if (windowSizeManager_->state(window) != WindowSizeManager::kWindowExpanded) {
        return;
    }

    isAtomicAnimationActive_ = true;

    window->setScrollBarVisibility(false);
    TooltipController::instance().hideAllTooltips();

    windowSizeManager_->setScrollPos(window, window->scrollPos());
    windowSizeManager_->setState(window, WindowSizeManager::kWindowAnimating);
    connectWindow_->show();

    if (locationsWindow_->currentTab() == LOCATION_TAB_SEARCH_LOCATIONS) {
        locationsWindow_->hideSearchTabWithoutAnimation();
    }

    // opacity change
    QVariantAnimation *animOpacity = new QVariantAnimation(this);
    animOpacity->setStartValue(1.0);
    animOpacity->setEndValue(0.0);
    animOpacity->setDuration(EXPAND_CHILD_WINDOW_OPACITY_DURATION);
    connect(animOpacity, &QVariantAnimation::valueChanged, [this, window](const QVariant &value) {
        double opacity = value.toDouble();
        window->setOpacity(opacity);
        shadowManager_->setOpacity(windowSizeManager_->shapeId(window), opacity, true);
    });

    int start = (int)window->boundingRect().height();
    int target =  (int)connectWindow_->boundingRect().height();
    QVariantAnimation *animResize = new QVariantAnimation(this);
    animResize->setStartValue(start);
    animResize->setEndValue(target);
    animResize->setDuration(windowSizeManager_->resizeDurationMs(window));
    connect(animResize, &QVariantAnimation::valueChanged, [this, window](const QVariant &value) {
        window->setHeight(value.toInt());
        updateMainAndViewGeometry(false);
        shadowManager_->changeRectangleSize(windowSizeManager_->shapeId(window),
                                            QRect(0,
                                                  childWindowShadowOffsetY(true),
                                                  window->boundingRect().width(),
                                                  window->boundingRect().height() - childWindowShadowOffsetY(false)));
        invalidateShadow_mac();
    });

    // resize finished
    connect(animResize, &QVariantAnimation::finished, [this, window]() {
        shadowManager_->setVisible(windowSizeManager_->shapeId(window), true);
        window->hide();
    });

    // group finished
    QParallelAnimationGroup *animGroup = new QParallelAnimationGroup(this);
    connect(animGroup, &QVariantAnimation::finished, [this, window, bSkipBottomInfoWindowAnimate, bSkipSetClickable]() {
        windowSizeManager_->setState(window, WindowSizeManager::kWindowCollapsed);
        if (window == preferencesWindow_) {
            preferencesWindow_->onCollapse();
            emit preferencesCollapsed();
        }
        TooltipController::instance().hideAllTooltips();
        updateMainAndViewGeometry(false);

        if (!bSkipSetClickable) {
            connectWindow_->setClickable(true);
            bottomInfoWindow_->setClickable(true);
        }
        invalidateShadow_mac();

        if (!bSkipBottomInfoWindowAnimate && bottomInfoWindow_->isVisible()) {
            std::function<void()> finish_function = [this, window]() {
                isAtomicAnimationActive_ = false;
                handleNextWindowChange();
                updateBottomInfoWindowVisibilityAndPos();
            };
            animateBottomInfoWindow(QAbstractAnimation::Backward, finish_function);
        } else {
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
        }
    });

    // delay opacity
    QSequentialAnimationGroup *seqGroup = new QSequentialAnimationGroup(this);
    seqGroup->addPause(windowSizeManager_->resizeDurationMs(window) - EXPAND_CHILD_WINDOW_OPACITY_DURATION);
    seqGroup->addAnimation(animOpacity);

    animGroup->addAnimation(animResize);
    animGroup->addAnimation(seqGroup);
    animGroup->start(QVariantAnimation::DeleteWhenStopped);
}

void MainWindowController::collapseAllExpandedOnBottom()
{
    if (isLocationsExpanded()) {
        connectWindow_->updateLocationsState(false);
        updateExpandAnimationParameters();
        expandLocationsAnimationGroup_->removeAnimation(collapseBottomInfoWindowAnimation_);
        expandLocationsAnimationGroup_->setDirection(QAbstractAnimation::Backward);
        if (expandLocationsAnimationGroup_->state() != QAbstractAnimation::Running) {
            expandLocationsAnimationGroup_->start();
        }
    } else if (bottomInfoWindow_->isVisible() && !isBottomInfoCollapsed()) {
        std::function<void()> finished_function = [this]() {
            if (functionOnAnimationFinished_ != NULL) {
                functionOnAnimationFinished_();
                functionOnAnimationFinished_ = NULL;
            }
        };

        animateBottomInfoWindow(QAbstractAnimation::Forward, finished_function);
    } else {
        if (functionOnAnimationFinished_ != NULL) {
            functionOnAnimationFinished_();
            functionOnAnimationFinished_ = NULL;
        }
    }
}

void MainWindowController::animateBottomInfoWindow(QAbstractAnimation::Direction direction, std::function<void ()> finished_function)
{
    QPropertyAnimation *collapseBottomInfo = getAnimateBottomInfoWindowAnimation(direction, finished_function);
    collapseBottomInfo->start(QAbstractAnimation::DeleteWhenStopped);
}

QPropertyAnimation *MainWindowController::getAnimateBottomInfoWindowAnimation(QAbstractAnimation::Direction direction, std::function<void ()> finished_function)
{
    QPropertyAnimation *collapseBottomInfo = new QPropertyAnimation(this);
    collapseBottomInfo->setTargetObject(bottomInfoWindow_);
    collapseBottomInfo->setPropertyName("pos");

    collapseBottomInfo->setStartValue(getCoordsOfBottomInfoWindow(false));
    collapseBottomInfo->setEndValue(getCoordsOfBottomInfoWindow(true));
    collapseBottomInfo->setDuration(HIDE_BOTTOM_INFO_ANIMATION_DURATION);

    collapseBottomInfo->setDirection(direction);

    connect(collapseBottomInfo, &QPropertyAnimation::valueChanged, [this]() {
        updateMainAndViewGeometry(false);
        invalidateShadow_mac();
    });

    connect(collapseBottomInfo, &QPropertyAnimation::finished, [this]() {
        updateExpandAnimationParameters();
        invalidateShadow_mac();
    });

    if (finished_function != nullptr) {
        connect(collapseBottomInfo, &QPropertyAnimation::finished, [=]() {
            finished_function();
        });
    }

    return collapseBottomInfo;
}

void MainWindowController::updateExpandAnimationParameters()
{
    if (expandLocationsAnimationGroup_ != NULL) {
        // TODO: sometimes this assert can fail
        WS_ASSERT(expandLocationsAnimationGroup_->state() != QAbstractAnimation::Running);
    }

    SAFE_DELETE(expandLocationsAnimationGroup_);

    expandLocationsListAnimation_ = new QPropertyAnimation(this);
    expandLocationsListAnimation_->setTargetObject(locationsWindow_);
    expandLocationsListAnimation_->setPropertyName("size");
    expandLocationsListAnimation_->setStartValue(QSize(locationsWindow_->width(), 0));
    expandLocationsListAnimation_->setEndValue(QSize(LOCATIONS_WINDOW_WIDTH*G_SCALE, locationsWindow_->tabAndFooterHeight()*G_SCALE));
    expandLocationsListAnimation_->setDuration(EXPAND_ANIMATION_DURATION);
    connect(expandLocationsListAnimation_, &QPropertyAnimation::finished, this, &MainWindowController::onExpandLocationsListAnimationFinished);
    connect(expandLocationsListAnimation_, &QPropertyAnimation::valueChanged, this, &MainWindowController::onExpandLocationsListAnimationValueChanged);
    connect(expandLocationsListAnimation_, &QPropertyAnimation::stateChanged, this, &MainWindowController::onExpandLocationsListAnimationStateChanged);

    collapseBottomInfoWindowAnimation_ = new QPropertyAnimation(this);
    collapseBottomInfoWindowAnimation_->setTargetObject(bottomInfoWindow_);
    collapseBottomInfoWindowAnimation_->setPropertyName("pos");
    connect(collapseBottomInfoWindowAnimation_, &QPropertyAnimation::valueChanged, this, &MainWindowController::onCollapseBottomInfoWindowAnimationValueChanged);

    expandLocationsAnimationGroup_ = new QSequentialAnimationGroup(this);
    connect(expandLocationsAnimationGroup_, &QSequentialAnimationGroup::finished, this, &MainWindowController::onExpandLocationsAnimationGroupFinished);

    if (bottomInfoWindow_->isVisible()) {
        expandLocationsAnimationGroup_->addAnimation(collapseBottomInfoWindowAnimation_);

        collapseBottomInfoWindowAnimation_->setStartValue(getCoordsOfBottomInfoWindow(false));
        collapseBottomInfoWindowAnimation_->setEndValue(getCoordsOfBottomInfoWindow(true));
        collapseBottomInfoWindowAnimation_->setDuration(HIDE_BOTTOM_INFO_ANIMATION_DURATION);
    }

    expandLocationsAnimationGroup_->addAnimation(expandLocationsListAnimation_);
}

bool MainWindowController::shouldShowConnectBackground()
{
    return curWindow_ == WINDOW_ID_CONNECT ||
            curWindow_ == WINDOW_ID_UPDATE ||
            curWindow_ == WINDOW_ID_UPGRADE ||
            curWindow_ == WINDOW_ID_GENERAL_MESSAGE ||
            (curWindow_ == WINDOW_ID_EXIT && windowBeforeExit_ == WINDOW_ID_CONNECT);
}

int MainWindowController::childWindowShadowOffsetY(bool withVanGoghOffset)
{
    int yOffset = 0;

    if (withVanGoghOffset && preferences_->appSkin() == APP_SKIN_VAN_GOGH &&
            updateAppItem_->isVisible()) {
        yOffset = UPDATE_WIDGET_HEIGHT*G_SCALE;
    }

    return yOffset + 50*G_SCALE;
}

#ifdef Q_OS_WIN
MainWindowController::TaskbarLocation MainWindowController::primaryScreenTaskbarLocation_win()
{
    TaskbarLocation taskbarLocation = TASKBAR_HIDDEN;

    QRect rcIcon = static_cast<MainWindow*>(mainWindow_)->trayIconRect();
    QScreen *screen = WidgetUtils::slightlySaferScreenAt(rcIcon.center());
    if (!screen) {
        qCDebug(LOG_BASIC) << "Couldn't find screen at system icon location";
        return taskbarLocation;
    }

    QRect desktopAvailableRc = screen->availableGeometry();
    QRect desktopScreenRc = screen->geometry();

    // qDebug() << "Desktop Available: " << desktopAvailableRc;
    // qDebug() << "Desktop Screen: "    << desktopScreenRc;

    if (desktopAvailableRc.x() > desktopScreenRc.x()) {
        taskbarLocation = TASKBAR_LEFT;
    } else if (desktopAvailableRc.y() > desktopScreenRc.y()) {
        taskbarLocation = TASKBAR_TOP;
    } else if (desktopAvailableRc.width() < desktopScreenRc.width()) {
        taskbarLocation = TASKBAR_RIGHT;
    } else if (desktopAvailableRc.height() < desktopScreenRc.height()) {
        taskbarLocation = TASKBAR_BOTTOM;
    }

    return taskbarLocation;
}

QRect MainWindowController::taskbarAwareDockedGeometry_win(int width, int shadowSize, int widthWithShadow, int heightWithShadow)
{
    TaskbarLocation taskbarLocation = primaryScreenTaskbarLocation_win();
    // qDebug() << "TaskbarLocation: " << taskbarLocation;

    QRect rcIcon = static_cast<MainWindow*>(mainWindow_)->trayIconRect();

    // Screen is sometimes not found because QSystemTrayIcon is invalid or not contained in screen
    // list during monitor change, screen resolution change, or opening/closing laptop lid.
    QScreen *screen = WidgetUtils::slightlySaferScreenAt(rcIcon.center());
    if (!screen) {
        // qDebug() << "Still no screen found -- not updating geometry and scene";
        return QRect();
    }

    QRect desktopAvailableRc = screen->availableGeometry();
    const int kRightOffset = 16 * G_SCALE;
    const int kVerticalOffset = 16 * G_SCALE;

    QRect geo;
    if (taskbarLocation == TASKBAR_TOP) {
        geo = QRect(desktopAvailableRc.right() - width - shadowSize - kRightOffset,
                    desktopAvailableRc.top() - shadowSize + kVerticalOffset,
                    widthWithShadow,
                    heightWithShadow);
        if (geo.right() > desktopAvailableRc.right() + shadowSize) {
            geo.moveRight(desktopAvailableRc.right() + shadowSize);
        }
    } else if (taskbarLocation == TASKBAR_LEFT) {
        geo = QRect(desktopAvailableRc.left() - shadowSize,
                    desktopAvailableRc.bottom() - heightWithShadow + shadowSize,
                    widthWithShadow,
                    heightWithShadow);
    } else if (taskbarLocation == TASKBAR_RIGHT) {
        geo = QRect(desktopAvailableRc.right() - width - shadowSize + 1*G_SCALE, // not sure why its off by 1 pixel
                    desktopAvailableRc.bottom() - heightWithShadow + shadowSize,
                    widthWithShadow,
                    heightWithShadow);
    } else if (taskbarLocation == TASKBAR_BOTTOM) {
        geo = QRect(desktopAvailableRc.right() - width - shadowSize - kRightOffset,
                    desktopAvailableRc.bottom() - heightWithShadow + shadowSize - kVerticalOffset,
                    widthWithShadow,
                    heightWithShadow);
        if (geo.right() > desktopAvailableRc.right() + shadowSize) {
            geo.moveRight(desktopAvailableRc.right() + shadowSize);
        }
    } else { // TASKBAR_HIDDEN
//        qDebug() << "Coundn't determine taskbar location -- putting app on primary screen bottom right";
//        qDebug() << "Desktop Available: " << desktopAvailableRc;

        // determine hidden taskbar location from tray icon
        // BOTTOM and RIGHT use bottom-right corner
        int posX = desktopAvailableRc.right() - width - shadowSize - kRightOffset;
        int posY = desktopAvailableRc.bottom() - heightWithShadow + shadowSize - kVerticalOffset;
        if (rcIcon.y() == 0) { // TOP
            posY = -shadowSize + kVerticalOffset;
        } else if (rcIcon.x() < desktopAvailableRc.width()/2) { // LEFT
            posX = -shadowSize + kRightOffset;
            posY = desktopAvailableRc.bottom() - heightWithShadow + shadowSize;
        }

        geo = QRect(posX, posY, widthWithShadow, heightWithShadow);
    }

    // qDebug() << "New GEO: " << geo;

    return geo;
}
#endif

// update main window geometry, view_ geometry, scene rect, depending current window, child item pos, etc...
void MainWindowController::getGraphicsRegionWidthAndHeight(int &width, int &height, int &addHeightToGeometry)
{
    for (auto w : windowSizeManager_->windows()) {
        if (windowSizeManager_->isExclusivelyExpanded(w)) {
            width = w->boundingRect().width();
            height = w->boundingRect().height();
            height += preferences_->appSkin() == APP_SKIN_VAN_GOGH ? (UPDATE_WIDGET_HEIGHT * vanGoghUpdateWidgetAnimationProgress_)*G_SCALE : 0;
            return;
        }
    }

    if (curWindow_ == WINDOW_ID_CONNECT || curWindow_ == WINDOW_ID_UPDATE || curWindow_ == WINDOW_ID_UPGRADE)
    {
        // here, the geometry depends on the state preferences window, bottomInfoWindow_, locationsWindow_
        if (locationListAnimationState_ == LOCATION_LIST_ANIMATION_COLLAPSED) {
            width = connectWindow_->boundingRect().width();
            height = connectWindow_->boundingRect().height();

            if (bottomInfoWindow_->isVisible() && bottomInfoWindow_->boundingRect().height() > 0) {
                int bottomInfoHeight = bottomInfoWindow_->boundingRect().height() + getCoordsOfBottomInfoWindow(false).y();

                if (bottomInfoHeight > height) {
                    height = bottomInfoHeight;
                }
            }

            for (auto w : windowSizeManager_->windows()) {
                if (windowSizeManager_->state(w) == WindowSizeManager::kWindowAnimating) {
                    height = w->boundingRect().height();
                    break;
                }
            }
        } else {
            width = connectWindow_->boundingRect().width();
            height = connectWindow_->boundingRect().height();
            addHeightToGeometry = locationWindowHeightScaled_ + locationsYOffset();
            if (addHeightToGeometry < 0 ) {
                addHeightToGeometry = 0;
            }
        }

    } else if (curWindow_ == WINDOW_ID_GENERAL_MESSAGE) {
        width = generalMessageWindow_->boundingRect().width();
        height = generalMessageWindow_->boundingRect().height();
    } else if (curWindow_ == WINDOW_ID_EXIT || curWindow_ == WINDOW_ID_LOGOUT) {
        if (windowBeforeExit_ == WINDOW_ID_CONNECT) {

            width = connectWindow_->boundingRect().width();
            height = connectWindow_->boundingRect().height();

            if (locationListAnimationState_ != LOCATION_LIST_ANIMATION_COLLAPSED) { // Animating
                addHeightToGeometry = locationWindowHeightScaled_ + locationsYOffset();
                if (addHeightToGeometry < 0 ) {
                    addHeightToGeometry = 0;
                }
            }
        } else { // login exit
            width = loginWindow_->boundingRect().width();
            height = loginWindow_->boundingRect().height();
        }
    } else if (curWindow_ == WINDOW_ID_INITIALIZATION) {
        width = initWindow_->boundingRect().width();
        height = initWindow_->boundingRect().height();
    } else if (curWindow_ == WINDOW_ID_LOGIN || curWindow_ == WINDOW_ID_LOGGING_IN ||
               curWindow_ == WINDOW_ID_EMERGENCY || curWindow_ == WINDOW_ID_EXTERNAL_CONFIG ||
               curWindow_ == WINDOW_ID_TWO_FACTOR_AUTH)
    {
        width = loginWindow_->boundingRect().width();
        height = loginWindow_->boundingRect().height();

        if (windowSizeManager_->state(preferencesWindow_) == WindowSizeManager::kWindowAnimating) {
            height = preferencesWindow_->boundingRect().height();
        }
    } else if (curWindow_ == WINDOW_ID_UNINITIALIZED) {
        // nothing
    } else {
        WS_ASSERT(false);
    }

    height += preferences_->appSkin() == APP_SKIN_VAN_GOGH ? (UPDATE_WIDGET_HEIGHT * vanGoghUpdateWidgetAnimationProgress_)*G_SCALE : 0;
}

void MainWindowController::centerMainGeometryAndUpdateView()
{
    int width, height;
    int addHeightToGeometry = 0;
    getGraphicsRegionWidthAndHeight(width, height, addHeightToGeometry);

    int shadowSize = shadowManager_->getShadowMargin();
    int widthWithShadow = width +  shadowSize * 2;
    int heightWithShadow = height + shadowSize * 2 + addHeightToGeometry;

    QWindow *window = mainWindow_->window()->windowHandle();
    if (window) {
        int screenWidth = window->screen()->geometry().width();
        int screenHeight = window->screen()->geometry().height();
        mainWindow_->setGeometry((screenWidth - widthWithShadow) / 2, (screenHeight - heightWithShadow) / 2, widthWithShadow, heightWithShadow);
    } else { // should only happen on startup if there are no persistentGuiSettings in registry
        QScreen *primaryScreen = qApp->primaryScreen();
        if (primaryScreen) {
            qCDebug(LOG_BASIC) << "centerMainGeometryAndUpdateView() - screen" << primaryScreen->name() << "geometry" << primaryScreen->geometry();
            int screenWidth = primaryScreen->geometry().width();
            int screenHeight = primaryScreen->geometry().height();
            mainWindow_->setGeometry((screenWidth - widthWithShadow) / 2, (screenHeight - heightWithShadow) / 2, widthWithShadow, heightWithShadow);
        } else {
            mainWindow_->setGeometry(0, 0, widthWithShadow, heightWithShadow);
        }
    }

    updateViewAndScene(width, height, shadowSize, true);
}

void MainWindowController::updateMainAndViewGeometry(bool updateShadow)
{
    int width, height;
    int addHeightToGeometry = 0;
    getGraphicsRegionWidthAndHeight(width, height, addHeightToGeometry);

    int shadowSize = shadowManager_->getShadowMargin();
    int widthWithShadow = width +  shadowSize * 2;
    int heightWithShadow = height + shadowSize * 2 + addHeightToGeometry;

    QRect geo = QRect(mainWindow_->pos().x(), mainWindow_->pos().y(), widthWithShadow, heightWithShadow);

    if (preferencesHelper_->isDockedToTray()) {
        const QRect rcIcon = static_cast<MainWindow*>(mainWindow_)->trayIconRect();
        const QPoint iconCenter(qMax(0, rcIcon.center().x()), qMax(0, rcIcon.center().y()));

        // On Windows and Mac: screen is sometimes not found because QSystemTrayIcon is invalid or not
        // contained in screen list during monitor change, screen resolution change, or opening/closing
        // laptop lid.
        // Safer screen check prevents a crash when the trayIcon is invalid
        QScreen *screen = WidgetUtils::slightlySaferScreenAt(iconCenter);
        if (!screen) {
            // qDebug() << "Still no screen found -- not updating geometry and scene";
            return;
        }

        const QRect desktopAvailableRc = screen->availableGeometry();

#ifdef Q_OS_WIN

        geo = taskbarAwareDockedGeometry_win(width, shadowSize, widthWithShadow, heightWithShadow);
        if (!geo.isValid()) {
            return;
        }

        const int kMaxGeometryRightPosition = desktopAvailableRc.right() + shadowSize;
        if (geo.right() > kMaxGeometryRightPosition)
            geo.moveRight(kMaxGeometryRightPosition);

#elif defined Q_OS_MAC

        // center ear on tray
        int rightEarCenterOffset = static_cast<int>(41 * G_SCALE);
        int posX = rcIcon.left() + rcIcon.width() / 2 - (width + shadowSize*2 - rightEarCenterOffset);
        geo = QRect(posX, desktopAvailableRc.top(), widthWithShadow, heightWithShadow);

        // keeps mainwindow on-screen
        const int kMaxGeometryRightPosition = desktopAvailableRc.right() + shadowSize;
        if (geo.right() > kMaxGeometryRightPosition) {
            // qDebug() << "Keeping mainwindow on screen!";
            geo.moveRight(kMaxGeometryRightPosition);
        }
#endif

    }

    // qDebug() << "Updating mainwindow geo: " << geo;
    mainWindow_->setGeometry(geo);
    updateViewAndScene(width, height, shadowSize, updateShadow);
}

void MainWindowController::updateViewAndScene(int width, int height, int shadowSize, bool updateShadow)
{
    // update view geometry
    if (view_->geometry().width() != width || view_->geometry().height() != height) {
        view_->setGeometry(shadowSize, shadowSize, width, height);
    }

    // update scene geometry
    int sceneRectWidth = scene_->sceneRect().width();
    int sceneRectHeight = scene_->sceneRect().height();
    if (sceneRectWidth != width || sceneRectHeight != height) {
        scene_->setSceneRect(QRectF(0, 0, width, height));
    }

    updateMaskForGraphicsView();

    if (updateShadow) {
        shadowManager_->updateShadow();
        invalidateShadow_mac();
    }

    // qDebug() << "MainWindow Geo (after view/scene update): " << mainWindow_->geometry();
}

void MainWindowController::handleNextWindowChange()
{
    if (!queueWindowChanges_.isEmpty()) {
        changeWindow(queueWindowChanges_.dequeue());
    } else {
        invalidateShadow_mac();
    }
}

void MainWindowController::updateBottomInfoWindowVisibilityAndPos(bool forceCollapsed)
{
    if (expandLocationsAnimationGroup_ != NULL && expandLocationsAnimationGroup_->state() == QAbstractAnimation::Running) {
        // TODO: can fire when quickly opening locations list on app startup
        WS_ASSERT(false);
    } else {
        if ((!bottomInfoWindow_->isUpgradeWidgetVisible() && !bottomInfoWindow_->isSharingFeatureVisible())
                || curWindow_ != WINDOW_ID_CONNECT
                || isLocationsExpanded())
        {
            bottomInfoWindow_->hide();
            bottomInfoWindow_->setClickable(false);
            shadowManager_->removeObject(ShadowManager::SHAPE_ID_BOTTOM_INFO);
            if (expandLocationsAnimationGroup_->state() != QAbstractAnimation::Stopped || locationListAnimationState_ != LOCATION_LIST_ANIMATION_COLLAPSED) {
                connectWindow_->setCornerColor(QColor(14, 25, 38));
            } else {
                connectWindow_->setCornerColor(Qt::transparent);
            }
        } else {
            bool bottomCollapsed = forceCollapsed || (locationListAnimationState_ != LOCATION_LIST_ANIMATION_COLLAPSED);
            if (!windowSizeManager_->hasWindowInState(WindowSizeManager::kWindowCollapsed)) {
                bottomCollapsed = true;
            }

            QPoint posBottomInfoWindow = getCoordsOfBottomInfoWindow(bottomCollapsed);
            bottomInfoWindow_->setPos(posBottomInfoWindow);
            bottomInfoWindow_->setClickable(true);
            shadowManager_->removeObject(ShadowManager::SHAPE_ID_BOTTOM_INFO);

            QPixmap shadow = bottomInfoWindow_->getCurrentPixmapShape();
            shadowManager_->addPixmap(shadow,
                                      posBottomInfoWindow.x(),
                                      posBottomInfoWindow.y(),
                                      ShadowManager::SHAPE_ID_BOTTOM_INFO, true);
            bottomInfoWindow_->show();

            bottomInfoWindow_->stackBefore(connectWindow_);
            connectWindow_->setCornerColor(FontManager::instance().getMidnightColor());
        }
        updateMainAndViewGeometry(false);
    }
}

// calc position of bottom info window, depending isBottomInfoWindowCollapsed state
QPoint MainWindowController::getCoordsOfBottomInfoWindow(bool isBottomInfoWindowCollapsed) const
{
    const int yOffset = UPDATE_WIDGET_HEIGHT * vanGoghUpdateWidgetAnimationProgress_;
    const int LEFT_OFFS_WHEN_SHARING_FEATURES_VISIBLE = 0;
    const int TOP_OFFS_WHEN_SHARING_FEATURES_VISIBLE = BOTTOM_INFO_POS_Y_SHOWING*G_SCALE;
    const int LEFT_OFFS_WHEN_SHARING_FEATURES_HIDDEN = 100*G_SCALE;
    const int TOP_OFFS_WHEN_SHARING_FEATURES_HIDDEN = BOTTOM_INFO_POS_Y_HIDING*G_SCALE;
    const int TOP_OFFS_WHEN_SHARING_FEATURES_HIDDEN_VAN_GOGH = (BOTTOM_INFO_POS_Y_VAN_GOGH + 8 + yOffset)*G_SCALE;
    const int TOP_OFFS_WHEN_SHARING_FEATURES_VISIBLE_VAN_GOGH = (BOTTOM_INFO_POS_Y_VAN_GOGH + yOffset)*G_SCALE;

    WS_ASSERT(bottomInfoWindow_->isUpgradeWidgetVisible() || bottomInfoWindow_->isSharingFeatureVisible());

    QPoint pt;
    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH && bottomInfoWindow_->isUpgradeWidgetVisible() && !bottomInfoWindow_->isSharingFeatureVisible()) {
        pt = QPoint(0, TOP_OFFS_WHEN_SHARING_FEATURES_HIDDEN_VAN_GOGH);
    } else if (preferences_->appSkin() == APP_SKIN_VAN_GOGH) {
        pt = QPoint(0, TOP_OFFS_WHEN_SHARING_FEATURES_VISIBLE_VAN_GOGH);
    } else if (bottomInfoWindow_->isUpgradeWidgetVisible() && !bottomInfoWindow_->isSharingFeatureVisible()) {
        pt = QPoint(LEFT_OFFS_WHEN_SHARING_FEATURES_HIDDEN, TOP_OFFS_WHEN_SHARING_FEATURES_HIDDEN);
    } else {
        pt = QPoint(LEFT_OFFS_WHEN_SHARING_FEATURES_VISIBLE, TOP_OFFS_WHEN_SHARING_FEATURES_VISIBLE);
    }

    if (isBottomInfoWindowCollapsed) {
        int bottomInfoWindowEdge = pt.y() + bottomInfoWindow_->boundingRect().height();
        int bottomConnectEdge = preferences_->appSkin() == APP_SKIN_VAN_GOGH ? (WINDOW_HEIGHT_VAN_GOGH + yOffset)*G_SCALE : 290*G_SCALE;
        int moveHeight = bottomInfoWindowEdge - bottomConnectEdge;
        pt.setY(pt.y() - moveHeight);
    }

    return pt;
}

bool MainWindowController::isBottomInfoCollapsed() const
{
    return bottomInfoWindow_->pos().toPoint() == getCoordsOfBottomInfoWindow(true);
}

void MainWindowController::updateCursorInViewport()
{
    QMouseEvent event(QEvent::MouseMove, QCursor::pos(), QCursor::pos(), Qt::NoButton, Qt::MouseButtons(), Qt::KeyboardModifiers());
    QApplication::sendEvent(view_->viewport(), &event);
}

void MainWindowController::invalidateShadow_mac()
{
#ifdef Q_OS_MAC
    // setPos() and similar methods do not change widget geometry immediately. Instead, all the
    // changes are postponed till the next redraw, which happens in the main loop. So, to get
    // correct shadow, accounting for all the recent changes, we have to place the invalidation call
    // into the main loop as well.
    QTimer::singleShot(0, this, [&]() {
        invalidateShadow_mac_impl();
    });
    // If the window is not in active state, it may be occluded. In this case, OSX won't invalidate
    // the native shadow at all, so we have to postpone the call till the next paint event.
    if (!static_cast<MainWindow*>(mainWindow_)->isActiveState()) {
        isNeedUpdateNativeShadow_ = true;
    }
#endif
}

void MainWindowController::setMaskForGraphicsView()
{
    // Mask allows clicking locations tab buttons when expanded
    if (preferences_->appSkin() == APP_SKIN_ALPHA) {
        view_->setMask(connectWindow_->getMask());
    }
}

void MainWindowController::clearMaskForGraphicsView()
{
    view_->clearMask();
}

void MainWindowController::keepWindowInsideScreenCoordinates()
{
    QRect rcWindow = mainWindow_->geometry();
    QRect rcScreen = mainWindow_->screen()->availableGeometry();

    if (rcWindow.bottom() > (rcScreen.bottom())) {
       // qDebug() << "KEEPING MAINWINDOW INSIDE SCREEN COORDINATES";
       rcWindow.moveBottom(rcScreen.bottom());
       mainWindow_->setGeometry(rcWindow);
    }
}

void MainWindowController::onAppSkinChanged(APP_SKIN s)
{
    // if turning Van Gogh mode on, and the update banner is visible, we shift everything down to accommodate the banner
    vanGoghUpdateWidgetAnimationProgress_ = (s == APP_SKIN_VAN_GOGH && updateAppItem_->isVisible()) ? 1.0 : 0.0;
    onVanGoghAnimationProgressChanged(vanGoghUpdateWidgetAnimationProgress_);

    // if in Van Gogh mode, and the update banner is visible, everything needs to be offset
    int yOffset = 0;
    if (s == APP_SKIN_VAN_GOGH && updateAppItem_->isVisible()) {
        yOffset = UPDATE_WIDGET_HEIGHT;
    }

    for (auto w : windowSizeManager_->windows()) {
        windowSizeManager_->setWindowHeight(w, std::max(windowSizeManager_->windowHeight(w) - yOffset, w->minimumHeight()));
        w->setHeight(windowSizeManager_->windowHeight(w)*G_SCALE);
        if (w == preferencesWindow_) {
            PersistentState::instance().setPreferencesWindowHeight(windowSizeManager_->windowHeight(preferencesWindow_));
        }
    }

    // if preferences is open, update shadows and scroll etc
    if (windowSizeManager_->state(preferencesWindow_) == WindowSizeManager::kWindowExpanded) {
        updateMainAndViewGeometry(false);
        shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_PREFERENCES,
                                            QRect(0,
                                                  childWindowShadowOffsetY(true),
                                                  preferencesWindow_->boundingRect().width(),
                                                  preferencesWindow_->boundingRect().height() - childWindowShadowOffsetY(false)));
        invalidateShadow_mac();
        keepWindowInsideScreenCoordinates();

        int scrollOffset = 2*UPDATE_WIDGET_HEIGHT;
        if (s != APP_SKIN_VAN_GOGH && !updateAppItem_->isVisible()) {
            scrollOffset = 0;
        }
        preferencesWindow_->setScrollOffset(scrollOffset);
        windowSizeManager_->setScrollPos(preferencesWindow_, scrollOffset);
    }

    // update window heights
    if (s == APP_SKIN_VAN_GOGH) {
        updateWindow_->setHeight(connectWindow_->boundingRect().height()/G_SCALE);
        upgradeAccountWindow_->setHeight(connectWindow_->boundingRect().height()/G_SCALE);
    } else {
        updateWindow_->setHeight(WINDOW_HEIGHT);
        upgradeAccountWindow_->setHeight(WINDOW_HEIGHT);
    }

    if (curWindow_ != WINDOW_ID_UNINITIALIZED) {
        updateMainAndViewGeometry(true);
        updateLocationsWindowAndTabGeometryStatic();
        updateBottomInfoWindowVisibilityAndPos();
    }
}

void MainWindowController::onVanGoghAnimationProgressChanged(QVariant value)
{
    vanGoghUpdateWidgetAnimationProgress_ = value.toDouble();
    int yOffset = UPDATE_WIDGET_HEIGHT*vanGoghUpdateWidgetAnimationProgress_*G_SCALE;

    connectWindow_->setPos(0, yOffset);
    newsFeedWindow_->setPos(0, yOffset);
    protocolWindow_->setPos(0, yOffset);
    preferencesWindow_->setPos(0, yOffset);
    exitWindow_->setPos(0, yOffset);
    logoutWindow_->setPos(0, yOffset);
    updateWindow_->setPos(0, yOffset);
    upgradeAccountWindow_->setPos(0, yOffset);
    generalMessageWindow_->setPos(0, yOffset);
}

void MainWindowController::onVanGoghAnimationFinished()
{
    if (curWindow_ != WINDOW_ID_UNINITIALIZED) {
        updateMainAndViewGeometry(true);
        updateLocationsWindowAndTabGeometryStatic();
        updateBottomInfoWindowVisibilityAndPos();
    }
}

int MainWindowController::locationsYOffset()
{
    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH) {
        if (updateAppItem_->isVisible()) {
            return UPDATE_WIDGET_HEIGHT*vanGoghUpdateWidgetAnimationProgress_*G_SCALE;
        }
        return 0;
    }
    return -LOCATIONS_WINDOW_TOP_OFFS*G_SCALE;
}

void MainWindowController::onLanguageChanged()
{
    exitWindow_->setTitle(tr(kQuitTitle));
    exitWindow_->setAcceptText(tr(kQuit));
    exitWindow_->setRejectText(tr(kCancel));
    logoutWindow_->setTitle(tr(kLogOutTitle));
    logoutWindow_->setAcceptText(tr(kLogOut));
    logoutWindow_->setRejectText(tr(kCancel));
}
