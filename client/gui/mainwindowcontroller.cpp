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
#include "overlaysconnectwindow/generalmessagewindowitem.h"
#include "overlaysconnectwindow/generalmessagetwobuttonwindowitem.h"
#include "bottominfowidget/bottominfoitem.h"
#include "newsfeedwindow/newsfeedwindowitem.h"
#include "utils/ws_assert.h"
#include "utils/utils.h"
#include "mainwindow.h"
#include "commongraphics/commongraphics.h"
#include "backend/persistentstate.h"
#include "dpiscalemanager.h"
#include "utils/logger.h"
#include "utils/widgetutils.h"

#ifdef Q_OS_MAC
    #include "utils/macutils.h"
#endif

MainWindowController::MainWindowController(QWidget *parent, LocationsWindow *locationsWindow, PreferencesHelper *preferencesHelper,
                                           Preferences *preferences, AccountInfo *accountInfo) : QObject(parent),
    curWindow_(WINDOW_ID_UNITIALIZED),
    mainWindow_(parent),
    preferences_(preferences),
    preferencesHelper_(preferencesHelper),
    locationsWindow_(locationsWindow),
    CLOSING_WINDSCRIBE(QT_TR_NOOP("Closing Windscribe")),
    CLOSE_ACCEPT(QT_TR_NOOP("Yes")),
    CLOSE_REJECT(QT_TR_NOOP("No")),
    locationListAnimationState_(LOCATION_LIST_ANIMATION_COLLAPSED),
    preferencesState_(CHILD_WINDOW_STATE_COLLAPSED),
    newsFeedState_(CHILD_WINDOW_STATE_COLLAPSED),
    isAtomicAnimationActive_(false),
    expandLocationsListAnimation_(NULL),
    collapseBottomInfoWindowAnimation_(NULL),
    expandLocationsAnimationGroup_(NULL),
    lastPreferencesWindowHeight_(0),
    lastNewsFeedWindowHeight_(0),
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
    preferencesWindow_ = new PreferencesWindow::PreferencesWindowItem(NULL, preferences, preferencesHelper, accountInfo);
    updateWindow_ = new UpdateWindowItem(preferences);
    upgradeAccountWindow_ = new UpgradeWindow::UpgradeWindowItem(preferences);
    bottomInfoWindow_ = new SharingFeatures::BottomInfoItem(preferences);
    generalMessageWindow_ = new GeneralMessage::GeneralMessageWindowItem(preferences, false);
    exitWindow_ = new GeneralMessage::GeneralMessageTwoButtonWindowItem(tr(CLOSING_WINDSCRIBE.toStdString().c_str()),
                                                                        "SHUTDOWN_ICON",
                                                                        tr(CLOSE_ACCEPT.toStdString().c_str()), tr(CLOSE_REJECT.toStdString().c_str()));

    updateAppItem_ = new UpdateApp::UpdateAppItem(preferences);
    newsFeedWindow_ = new NewsFeedWindow::NewsFeedWindowItem(nullptr, preferences, preferencesHelper);

    scene_->addItem(loginWindow_->getGraphicsObject());
    scene_->addItem(loggingInWindow_->getGraphicsObject());
    scene_->addItem(initWindow_->getGraphicsObject());
    scene_->addItem(bottomInfoWindow_->getGraphicsObject());
    scene_->addItem(connectWindow_->getGraphicsObject());
    scene_->addItem(emergencyConnectWindow_->getGraphicsObject());
    scene_->addItem(externalConfigWindow_->getGraphicsObject());
    scene_->addItem(twoFactorAuthWindow_->getGraphicsObject());
    scene_->addItem(preferencesWindow_->getGraphicsObject());
    scene_->addItem(updateWindow_->getGraphicsObject());
    scene_->addItem(upgradeAccountWindow_->getGraphicsObject());
    scene_->addItem(updateAppItem_->getGraphicsObject());
    scene_->addItem(newsFeedWindow_->getGraphicsObject());
    scene_->addItem(generalMessageWindow_->getGraphicsObject());
    scene_->addItem(exitWindow_->getGraphicsObject());

    loginWindow_->getGraphicsObject()->setVisible(false);
    loggingInWindow_->getGraphicsObject()->setVisible(false);
    initWindow_->getGraphicsObject()->setVisible(false);
    bottomInfoWindow_->getGraphicsObject()->setVisible(false);
    connectWindow_->getGraphicsObject()->setVisible(false);
    emergencyConnectWindow_->getGraphicsObject()->setVisible(false);
    externalConfigWindow_->getGraphicsObject()->setVisible(false);
    twoFactorAuthWindow_->getGraphicsObject()->setVisible(false);
    preferencesWindow_->getGraphicsObject()->setVisible(false);
    updateWindow_->getGraphicsObject()->setVisible(false);
    upgradeAccountWindow_->getGraphicsObject()->setVisible(false);
    updateAppItem_->getGraphicsObject()->setVisible(false);
    newsFeedWindow_->getGraphicsObject()->setVisible(false);
    generalMessageWindow_->getGraphicsObject()->setVisible(false);
    exitWindow_->getGraphicsObject()->setVisible(false);

    // bottomInfoWindow_->getGraphicsObject()->setPos(0, BOTTOM_INFO_POS_Y_SHOWING);

    connect(locationsWindow_, SIGNAL(heightChanged()), SLOT(onLocationsWindowHeightChanged()));
    connect(dynamic_cast<QObject*>(bottomInfoWindow_), SIGNAL(heightChanged(int)), SLOT(onBottomInfoHeightChanged()));
    connect(bottomInfoWindow_->getGraphicsObject(), SIGNAL(yChanged()), SLOT(onBottomInfoPosChanged()));
    connect(dynamic_cast<QObject*>(preferencesWindow_), SIGNAL(sizeChanged()), SLOT(onPreferencesResize()));
    connect(dynamic_cast<QObject*>(preferencesWindow_), SIGNAL(resizeFinished()), SLOT(onPreferencesResizeFinished()));
    connect(dynamic_cast<QObject*>(newsFeedWindow_), SIGNAL(sizeChanged()), SLOT(onNewsFeedResize()));
    connect(dynamic_cast<QObject*>(newsFeedWindow_), SIGNAL(resizeFinished()), SLOT(onNewsFeedResizeFinished()));
    connect(preferences_, &Preferences::appSkinChanged, this, &MainWindowController::onAppSkinChanged);

    preferencesWindowHeight_ = preferencesWindow_->recommendedHeight();
    newsFeedWindowHeight_ = newsFeedWindow_->recommendedHeight();

    view_->setStyleSheet("background: transparent; border: none");
    view_->setScene(scene_);

    locationsWindow_->hide();

    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH)
    {
        shadowManager_->addRectangle(connectWindow_->getGraphicsObject()->boundingRect().toRect(), ShadowManager::SHAPE_ID_CONNECT_WINDOW, false);
    }
    else
    {
        shadowManager_->addPixmap(connectWindow_->getShadowPixmap(), 0, 0, ShadowManager::SHAPE_ID_CONNECT_WINDOW, false);
    }
    shadowManager_->addRectangle(QRect(0, 0, 0, 0), ShadowManager::SHAPE_ID_LOCATIONS, false);
    shadowManager_->addRectangle(loginWindow_->getGraphicsObject()->boundingRect().toRect(), ShadowManager::SHAPE_ID_LOGIN_WINDOW, false);
    shadowManager_->addRectangle(initWindow_->getGraphicsObject()->boundingRect().toRect(), ShadowManager::SHAPE_ID_INIT_WINDOW, false);
    shadowManager_->addRectangle(QRect(0, 0, 0, 0), ShadowManager::SHAPE_ID_PREFERENCES, false);
    shadowManager_->addRectangle(QRect(0, 0, 0, 0), ShadowManager::SHAPE_ID_NEWS_FEED, false);
    connect(shadowManager_, SIGNAL(shadowUpdated()), SIGNAL(shadowUpdated()));

    connect(&TooltipController::instance(), SIGNAL(sendServerRatingUp()), SLOT(onTooltipControllerSendServerRatingUp()));
    connect(&TooltipController::instance(), SIGNAL(sendServerRatingDown()), SLOT(onTooltipControllerSendServerRatingDown()));

    connect(&vanGoghUpdateWidgetAnimation_, &QVariantAnimation::valueChanged, this, &MainWindowController::onVanGoghAnimationProgressChanged);

    updateExpandAnimationParameters();

    // update window heights if we start in van gogh mode
    onAppSkinChanged(preferences_->appSkin());
}

void MainWindowController::updateScaling()
{
    updateLocationsWindowAndTabGeometryStatic();
    updateMainAndViewGeometry(true);
}

void MainWindowController::updateLocationsWindowAndTabGeometry()
{
    //locationsWindow_->updateLocationsTabGeometry();
    locationsWindow_->setGeometry(shadowManager_->getShadowMargin(),
                                  connectWindow_->getGraphicsObject()->boundingRect().height() + locationsYOffset() + shadowManager_->getShadowMargin(),
                                  LOCATIONS_WINDOW_WIDTH * G_SCALE,
                                  locationWindowHeightScaled_);

    shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_LOCATIONS,
                                        QRect(0,
                                              connectWindow_->getGraphicsObject()->boundingRect().height() + locationsYOffset(),
                                              LOCATIONS_WINDOW_WIDTH * G_SCALE,
                                              (locationsWindow_->geometry().height())));

}

void MainWindowController::updateLocationsWindowAndTabGeometryStatic()
{
    static double prevScale = 1.0;

    initWindow_->updateScaling();
    loginWindow_->updateScaling();
    loggingInWindow_->updateScaling();
    emergencyConnectWindow_->updateScaling();
    externalConfigWindow_->updateScaling();
    twoFactorAuthWindow_->updateScaling();
    exitWindow_->updateScaling();
    connectWindow_->updateScaling();
    locationsWindow_->updateScaling();
    preferencesWindow_->updateScaling();
    newsFeedWindow_->updateScaling();
    updateWindow_->updateScaling();
    upgradeAccountWindow_->updateScaling();
    preferencesWindowHeight_ = preferencesWindowHeight_ * (G_SCALE / prevScale);
    newsFeedWindowHeight_ = newsFeedWindowHeight_ * (G_SCALE / prevScale);
    prevScale = G_SCALE;

    bottomInfoWindow_->updateScaling();
    updateAppItem_->updateScaling();
    generalMessageWindow_->updateScaling();


    int height = locationsWindow_->tabAndFooterHeight();
    if (locationListAnimationState_ == LOCATION_LIST_ANIMATION_COLLAPSED) height = 0 ;

    locationsWindow_->updateLocationsTabGeometry();
    locationsWindow_->setGeometry(shadowManager_->getShadowMargin(),
                                  connectWindow_->getGraphicsObject()->boundingRect().height() + locationsYOffset() + shadowManager_->getShadowMargin(),
                                  LOCATIONS_WINDOW_WIDTH * G_SCALE,
                                  height * G_SCALE);

    locationWindowHeightScaled_ = height * G_SCALE;

    shadowManager_->removeObject(ShadowManager::SHAPE_ID_CONNECT_WINDOW);
    QPixmap shadow = connectWindow_->getShadowPixmap();
    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH)
    {
        shadowManager_->addRectangle(connectWindow_->getGraphicsObject()->boundingRect().toRect(), ShadowManager::SHAPE_ID_CONNECT_WINDOW, shouldShowConnectBackground());
    }
    else
    {
        shadowManager_->addPixmap(shadow, 0, 0, ShadowManager::SHAPE_ID_CONNECT_WINDOW, shouldShowConnectBackground());
    }

    shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_LOCATIONS,
                                        QRect(0,
                                              connectWindow_->getGraphicsObject()->boundingRect().height() + locationsYOffset(),
                                              LOCATIONS_WINDOW_WIDTH * G_SCALE,
                                              (locationsWindow_->geometry().height())));

    shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_PREFERENCES,
                                        QRect(0, childWindowShadowOffsetY(),
                                              preferencesWindow_->getGraphicsObject()->boundingRect().width(),
                                              preferencesWindow_->getGraphicsObject()->boundingRect().height() - childWindowShadowOffsetY()));

    shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_NEWS_FEED,
                                        QRect(0, childWindowShadowOffsetY(),
                                              newsFeedWindow_->getGraphicsObject()->boundingRect().width(),
                                              newsFeedWindow_->getGraphicsObject()->boundingRect().height() - childWindowShadowOffsetY()));

    shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_LOGIN_WINDOW,
                                        QRect(0,0, loginWindow_->getGraphicsObject()->boundingRect().width(),
                                              loginWindow_->getGraphicsObject()->boundingRect().height()));

    shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_INIT_WINDOW,
                                        QRect(0, 0, initWindow_->getGraphicsObject()->boundingRect().width(),
                                              initWindow_->getGraphicsObject()->boundingRect().height()));

    if (shadowManager_->isInShadowList(ShadowManager::SHAPE_ID_UPDATE_WIDGET))
    {
        shadowManager_->removeObject(ShadowManager::SHAPE_ID_UPDATE_WIDGET);
        QPixmap currentShadow = updateAppItem_->getCurrentPixmapShape();
        shadowManager_->addPixmap(currentShadow, 0, 0, ShadowManager::SHAPE_ID_UPDATE_WIDGET, true);
    }


    if (bottomInfoWindow_->isUpgradeWidgetVisible() || bottomInfoWindow_->isSharingFeatureVisible())
    {
        shadowManager_->removeObject(ShadowManager::SHAPE_ID_BOTTOM_INFO);
        bool bottomCollapsed = (locationListAnimationState_ != LOCATION_LIST_ANIMATION_COLLAPSED);
        if (preferencesState_ != CHILD_WINDOW_STATE_COLLAPSED || newsFeedState_ != CHILD_WINDOW_STATE_COLLAPSED)
        {
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
    if (preferencesState_ == CHILD_WINDOW_STATE_COLLAPSED
            && newsFeedState_ == CHILD_WINDOW_STATE_COLLAPSED
            && curWindow_ != WINDOW_ID_INITIALIZATION
            && curWindow_ != WINDOW_ID_LOGIN
            && curWindow_ != WINDOW_ID_EMERGENCY
            && curWindow_ != WINDOW_ID_EXTERNAL_CONFIG
            && curWindow_ != WINDOW_ID_TWO_FACTOR_AUTH
            && windowBeforeExit_ != WINDOW_ID_LOGIN
            && !bottomInfoWindow_->getGraphicsObject()->isVisible())
    {
        setMaskForGraphicsView();
    }
    else
    {
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

    if (PersistentState::instance().haveAppGeometry())
    {
        if (mainWindow_->restoreGeometry(PersistentState::instance().appGeometry()))
        {
            qCDebug(LOG_BASIC) << "setWindowPosFromPersistent() - restored app geometry:" << mainWindow_->geometry();

            // Qt's restoreGeometry may restore the app onto a display that is no longer attached. According to the Qt
            // documentation, it is supposed to take detached displays into account, and appears to do a pretty good
            // job most of the time.  I did encounter sporadic occasions on a Windows laptop where Qt would restore
            // the app offscreen when the app was on an external display and the display was no longer attached (see
            // QTBUG-77385).  The check below should catch this edge case.  Note that we are using virtualGeometry()
            // here rather than geometry() due to the issues noted in ticket #411.
            for (auto screen : qApp->screens())
            {
                if (screen->virtualGeometry().contains(mainWindow_->pos())) {
                    return;
                }
            }

            qCDebug(LOG_BASIC) << "setWindowPosFromPersistent() - did not locate a display containing restored app position:" << mainWindow_->pos();
        }
        else {
            qCDebug(LOG_BASIC) << "setWindowPosFromPersistent() - restoreGeometry failed";
        }
    }
    else {
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
    return preferencesState_ == CHILD_WINDOW_STATE_EXPANDED || preferencesState_ == CHILD_WINDOW_STATE_ANIMATING;
}

bool MainWindowController::isNewsFeedVisible()
{
    return newsFeedState_ == CHILD_WINDOW_STATE_EXPANDED || newsFeedState_ == CHILD_WINDOW_STATE_ANIMATING;
}

MainWindowController::WINDOW_ID MainWindowController::currentWindow()
{
    return curWindow_;
}

void MainWindowController::changeWindow(MainWindowController::WINDOW_ID windowId)
{
    if (isAtomicAnimationActive_)
    {
        queueWindowChanges_.enqueue(windowId);
        //qCDebug(LOG_BASIC) << "MainWindowController::changeWindow(enqueue):" << (int)windowId;
        return;
    }

    // for login window when preferences expanded, handle change window commands after preferences collapsed
    if (curWindow_ == WINDOW_ID_LOGIN && preferencesState_ != CHILD_WINDOW_STATE_COLLAPSED)
    {
        queueWindowChanges_.enqueue(windowId);
        //qCDebug(LOG_BASIC) << "MainWindowController::changeWindow(enqueue):" << (int)windowId;
        return;
    }

    // for connect window when preferences/news expanded, handle change window commands after preferences collapsed,
    // except WINDOW_ID_UPDATE/WINDOW_ID_LOGIN/WINDOW_ID_INITIALIZATION
    if (curWindow_ == WINDOW_ID_CONNECT &&
        (preferencesState_ != CHILD_WINDOW_STATE_COLLAPSED || newsFeedState_ != CHILD_WINDOW_STATE_COLLAPSED) &&
        windowId != WINDOW_ID_UPDATE &&
        windowId != WINDOW_ID_LOGIN &&
        windowId != WINDOW_ID_INITIALIZATION)
    {
        queueWindowChanges_.enqueue(windowId);
        //qCDebug(LOG_BASIC) << "MainWindowController::changeWindow(enqueue):" << (int)windowId;
        return;
    }

    //qCDebug(LOG_BASIC) << "MainWindowController::changeWindow:" << (int)windowId;

    // specific commands
    if (windowId == WINDOW_CMD_UPDATE_BOTTOM_INFO)
    {
        onBottomInfoHeightChanged();
        return;
    }
    else if (windowId == WINDOW_CMD_CLOSE_EXIT)
    {
        closeExitWindow();
        return;
    }

    if (windowId == curWindow_)
    {
        return;
    }

    if (windowId == WINDOW_ID_INITIALIZATION)
    {
        gotoInitializationWindow();
    }
    else if (windowId == WINDOW_ID_LOGIN)
    {
        gotoLoginWindow();
    }
    else if (windowId == WINDOW_ID_EMERGENCY)
    {
        gotoEmergencyWindow();
    }
    else if (windowId == WINDOW_ID_LOGGING_IN)
    {
        gotoLoggingInWindow();
    }
    else if (windowId == WINDOW_ID_CONNECT)
    {
        gotoConnectWindow();
    }
    else if (windowId == WINDOW_ID_EXTERNAL_CONFIG)
    {
        gotoExternalConfigWindow();
    }
    else if (windowId == WINDOW_ID_TWO_FACTOR_AUTH)
    {
        gotoTwoFactorAuthWindow();
    }
    else if (windowId == WINDOW_ID_UPDATE)
    {
        gotoUpdateWindow();
    }
    else if (windowId == WINDOW_ID_UPGRADE)
    {
        gotoUpgradeWindow();
    }
    else if (windowId == WINDOW_ID_GENERAL_MESSAGE)
    {
        gotoGeneralMessageWindow();
    }
    else if (windowId == WINDOW_ID_EXIT)
    {
        gotoExitWindow();
    }
    else
    {
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

    if (expandLocationsAnimationGroup_->state() == QAbstractAnimation::Stopped && locationListAnimationState_ == LOCATION_LIST_ANIMATION_EXPANDED)
    {
        return;
    }

    shadowManager_->setVisible(ShadowManager::SHAPE_ID_LOCATIONS, true);
    shadowManager_->setOpacity(ShadowManager::SHAPE_ID_LOCATIONS, 1.0, true);

    isAtomicAnimationActive_ = true;
    functionOnAnimationFinished_ = NULL;

    connectWindow_->updateLocationsState(true);

    updateExpandAnimationParameters();
    expandLocationsAnimationGroup_->setDirection(QAbstractAnimation::Forward);
    if (expandLocationsAnimationGroup_->state() != QAbstractAnimation::Running)
    {
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

    if (expandLocationsAnimationGroup_->state() == QAbstractAnimation::Stopped && locationListAnimationState_ == LOCATION_LIST_ANIMATION_COLLAPSED)
    {
        return;
    }

    TooltipController::instance().hideAllTooltips();

    isAtomicAnimationActive_ = true;
    functionOnAnimationFinished_ = NULL;

    connectWindow_->updateLocationsState(false);
    if (locationsWindow_->currentTab() == GuiLocations::LocationsTab::LOCATION_TAB_SEARCH_LOCATIONS)
    {
        locationsWindow_->hideSearchTabWithoutAnimation();
    }

    if (bottomInfoWindow_->isUpgradeWidgetVisible() ||
        bottomInfoWindow_->isSharingFeatureVisible()) {
        bottomInfoWindow_->getGraphicsObject()->setPos(getCoordsOfBottomInfoWindow(true));
        bottomInfoWindow_->setClickable(true);
        bottomInfoWindow_->getGraphicsObject()->show();
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
    }
    else if (expandLocationsAnimationGroup_->state() == QAbstractAnimation::Stopped && locationListAnimationState_ == LOCATION_LIST_ANIMATION_EXPANDED)
    {
        return true;
    }
    return false;
}

void MainWindowController::expandPreferences()
{
    if (curWindow_ == WINDOW_ID_LOGIN)
    {
        expandPreferencesFromLogin();
    }
    else if (curWindow_ == WINDOW_ID_CONNECT)
    {
        expandPreferencesFromConnect();
    }
}

void MainWindowController::collapsePreferences()
{
    if (curWindow_ == WINDOW_ID_LOGIN)
    {
        collapsePreferencesFromLogin();
    }
    else if (curWindow_ == WINDOW_ID_CONNECT)
    {
        collapsePreferencesFromConnect(false);
    }
}

void MainWindowController::showUpdateWidget()
{
    //WS_ASSERT(curWindow_ == WINDOW_ID_CONNECT
    if (!updateAppItem_->getGraphicsObject()->isVisible())
    {
        updateAppItem_->getGraphicsObject()->show();
        QPixmap shadow = updateAppItem_->getCurrentPixmapShape();
        shadowManager_->addPixmap(shadow, 0, 0, ShadowManager::SHAPE_ID_UPDATE_WIDGET, true);
        invalidateShadow_mac();
        if (preferences_->appSkin() == APP_SKIN_VAN_GOGH)
        {
            startAnAnimation(vanGoghUpdateWidgetAnimation_, vanGoghUpdateWidgetAnimationProgress_, 1.0, ANIMATION_SPEED_FAST);
            // trigger resizes
            onAppSkinChanged(preferences_->appSkin());
        }
    }
}

void MainWindowController::hideUpdateWidget()
{
    if (updateAppItem_->getGraphicsObject()->isVisible())
    {
        updateAppItem_->getGraphicsObject()->hide();
        shadowManager_->removeObject(ShadowManager::SHAPE_ID_UPDATE_WIDGET);
        invalidateShadow_mac();
        if (preferences_->appSkin() == APP_SKIN_VAN_GOGH)
        {
            startAnAnimation(vanGoghUpdateWidgetAnimation_, vanGoghUpdateWidgetAnimationProgress_, 0.0, ANIMATION_SPEED_FAST);
            // trigger resizes
            onAppSkinChanged(preferences_->appSkin());
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
    if (expandLocationsListAnimation_->direction() == QAbstractAnimation::Backward)
    {
        locationListAnimationState_ = LOCATION_LIST_ANIMATION_COLLAPSED;
        clearMaskForGraphicsView();
    }
    else
    {
        shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_LOCATIONS,
                                            QRect(0,
                                                  connectWindow_->getGraphicsObject()->boundingRect().height() + locationsYOffset(),
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
    if (newState == QAbstractAnimation::Running && oldState != QAbstractAnimation::Running)
    {
        if (expandLocationsListAnimation_->direction() == QAbstractAnimation::Backward)
        {
            locationListAnimationState_ = LOCATION_LIST_ANIMATION_COLLAPSING;
        }
        else
        {
            locationListAnimationState_ = LOCATION_LIST_ANIMATION_EXPANDING;
        }
    }
}

void MainWindowController::onExpandLocationsAnimationGroupFinished()
{
    if (functionOnAnimationFinished_ != NULL)
    {
        functionOnAnimationFinished_();
        functionOnAnimationFinished_ = NULL;
    }
    else
    {
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
                                              connectWindow_->getGraphicsObject()->boundingRect().height() + locationsYOffset(),
                                              LOCATIONS_WINDOW_WIDTH * G_SCALE,
                                              (locationsWindow_->geometry().height())));

    keepWindowInsideScreenCoordinates();
}

void MainWindowController::onPreferencesResize() // manual resizing
{
    preferencesWindowHeight_ = preferencesWindow_->getGraphicsObject()->boundingRect().height();
    updateMainAndViewGeometry(false);

    // only recalculate the shadow size at certain intervals since the changeRectangleSize is expensive
    if (abs(preferencesWindowHeight_ - lastPreferencesWindowHeight_) > 5)
    {
        shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_PREFERENCES,
                                            QRect(0, childWindowShadowOffsetY(),
                                                  preferencesWindow_->getGraphicsObject()->boundingRect().width(),
                                                  preferencesWindow_->getGraphicsObject()->boundingRect().height() - childWindowShadowOffsetY()));
        lastPreferencesWindowHeight_ = preferencesWindowHeight_;
    }

    keepWindowInsideScreenCoordinates();
}

void MainWindowController::onPreferencesResizeFinished()
{
    shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_PREFERENCES,
                                        QRect(0, childWindowShadowOffsetY(),
                                              preferencesWindow_->getGraphicsObject()->boundingRect().width(),
                                              preferencesWindow_->getGraphicsObject()->boundingRect().height() - childWindowShadowOffsetY()));
}

void MainWindowController::onNewsFeedResize() // manual resizing
{
    newsFeedWindowHeight_ = newsFeedWindow_->getGraphicsObject()->boundingRect().height();
    updateMainAndViewGeometry(false);

    // only recalculate the shadow size at certain intervals since the changeRectangleSize is expensive
    if (abs(newsFeedWindowHeight_ - lastNewsFeedWindowHeight_) > 5)
    {
        shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_NEWS_FEED,
                                            QRect(0, childWindowShadowOffsetY(),
                                                  newsFeedWindow_->getGraphicsObject()->boundingRect().width(),
                                                  newsFeedWindow_->getGraphicsObject()->boundingRect().height() - childWindowShadowOffsetY()));
        lastNewsFeedWindowHeight_ = newsFeedWindowHeight_;
    }

    keepWindowInsideScreenCoordinates();
}

void MainWindowController::onNewsFeedResizeFinished()
{
    shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_NEWS_FEED,
                                        QRect(0, childWindowShadowOffsetY(),
                                              newsFeedWindow_->getGraphicsObject()->boundingRect().width(),
                                              newsFeedWindow_->getGraphicsObject()->boundingRect().height() - childWindowShadowOffsetY()));
}

void MainWindowController::onBottomInfoHeightChanged()
{
    if (isAtomicAnimationActive_)
    {
        queueWindowChanges_.enqueue(WINDOW_CMD_UPDATE_BOTTOM_INFO);
    }
    else
    {
        updateBottomInfoWindowVisibilityAndPos();
        updateExpandAnimationParameters();
        invalidateShadow_mac();
        keepWindowInsideScreenCoordinates();
    }
}

void MainWindowController::onBottomInfoPosChanged()
{
    shadowManager_->changePixmapPos(ShadowManager::SHAPE_ID_BOTTOM_INFO,
                                    bottomInfoWindow_->getGraphicsObject()->pos().x(),
                                    bottomInfoWindow_->getGraphicsObject()->pos().y());
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

    if (preferencesState_ != CHILD_WINDOW_STATE_COLLAPSED)
    {
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_PREFERENCES, false);
        preferencesWindow_->getGraphicsObject()->hide();
        preferencesState_ = CHILD_WINDOW_STATE_COLLAPSED;
        emit preferencesCollapsed();
    }

    if (newsFeedState_ != CHILD_WINDOW_STATE_COLLAPSED)
    {
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_NEWS_FEED, false);
        newsFeedWindow_->getGraphicsObject()->hide();
        newsFeedState_ = CHILD_WINDOW_STATE_COLLAPSED;
    }

    shadowManager_->setVisible(ShadowManager::SHAPE_ID_CONNECT_WINDOW, false);
    updateWindow_->getGraphicsObject()->hide();
    connectWindow_->getGraphicsObject()->hide();
    TooltipController::instance().hideAllTooltips();

    if (bottomInfoWindow_->getGraphicsObject()->isVisible())
    {
        bottomInfoWindow_->getGraphicsObject()->hide();
        bottomInfoWindow_->setClickable(false);
        shadowManager_->removeObject(ShadowManager::SHAPE_ID_BOTTOM_INFO);
    }

    if (isLocationsExpanded())
    {
        connectWindow_->updateLocationsState(false);
        locationsWindow_->hide();
        locationListAnimationState_ = LOCATION_LIST_ANIMATION_COLLAPSED;
        clearMaskForGraphicsView();
    }

    loginWindow_->setClickable(false);
    loginWindow_->getGraphicsObject()->setVisible(false);
    loggingInWindow_->getGraphicsObject()->setVisible(false);
    emergencyConnectWindow_->setClickable(false);
    emergencyConnectWindow_->getGraphicsObject()->setVisible(false);
    externalConfigWindow_->setClickable(false);
    externalConfigWindow_->getGraphicsObject()->setVisible(false);
    twoFactorAuthWindow_->setClickable(false);
    twoFactorAuthWindow_->getGraphicsObject()->setVisible(false);
    newsFeedWindow_->getGraphicsObject()->setVisible(false);

    exitWindow_->getGraphicsObject()->setVisible(false);

	// init could be different height depending on if going to login or directly to connect screen
    int initHeight = WINDOW_HEIGHT;
    if (curWindow_ == WINDOW_ID_LOGIN) initHeight = LOGIN_HEIGHT;
    initWindow_->setHeight(initHeight);
    initWindowInitHeight_ = initHeight;

    shadowManager_->setVisible(ShadowManager::SHAPE_ID_INIT_WINDOW, true);
    shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_INIT_WINDOW,
                                        QRect(0,0, initWindow_->getGraphicsObject()->boundingRect().width(),
                                              initWindow_->getGraphicsObject()->boundingRect().height()));

    curWindow_ = WINDOW_ID_INITIALIZATION;
    initWindow_->getGraphicsObject()->setOpacity(1.0);
    initWindow_->getGraphicsObject()->setVisible(true);
    initWindow_->getGraphicsObject()->show();

    updateMainAndViewGeometry(false);

}

void MainWindowController::gotoLoginWindow()
{
    // qDebug() << "gotoLoginWindow()";
    WS_ASSERT(curWindow_ == WINDOW_ID_UNITIALIZED
             || curWindow_ == WINDOW_ID_INITIALIZATION
             || curWindow_ == WINDOW_ID_EMERGENCY
             || curWindow_ == WINDOW_ID_LOGGING_IN
             || curWindow_ == WINDOW_ID_CONNECT
             || curWindow_ == WINDOW_ID_EXTERNAL_CONFIG
             || curWindow_ == WINDOW_ID_TWO_FACTOR_AUTH
             || curWindow_ == WINDOW_ID_UPDATE
             || curWindow_ == WINDOW_ID_UPGRADE);

    // if preferences expanded then collapse preferences first
    if (preferencesState_ != CHILD_WINDOW_STATE_COLLAPSED)
    {
        queueWindowChanges_.enqueue(WINDOW_ID_LOGIN);
        collapsePreferencesFromConnect(true);
        return;
    }

    // if news expanded then collapse it first
    if (newsFeedState_ != CHILD_WINDOW_STATE_COLLAPSED)
    {
        queueWindowChanges_.enqueue(WINDOW_ID_LOGIN);
        collapseNewsFeed();
        return;
    }

    if (curWindow_ == WINDOW_ID_INITIALIZATION)
    {
        // qDebug() << "Init -> Login";
        initWindow_->setClickable(false);
        initWindow_->getGraphicsObject()->stackBefore(loginWindow_->getGraphicsObject());

        loginWindow_->getGraphicsObject()->setOpacity(0.0);
        loginWindow_->getGraphicsObject()->setVisible(true);

        // login opacity
        QPropertyAnimation *loginOpacityAnim = new QPropertyAnimation(this);
        loginOpacityAnim->setTargetObject(loginWindow_->getGraphicsObject());
        loginOpacityAnim->setPropertyName("opacity");
        loginOpacityAnim->setStartValue(0.0);
        loginOpacityAnim->setEndValue(1.0);
        loginOpacityAnim->setDuration(REVEAL_LOGIN_ANIMATION_DURATION);
        connect(loginOpacityAnim, &QPropertyAnimation::finished, [this]()
        {
            shadowManager_->setVisible(ShadowManager::SHAPE_ID_INIT_WINDOW, false);
            shadowManager_->setVisible(ShadowManager::SHAPE_ID_LOGIN_WINDOW, true);
            initWindow_->getGraphicsObject()->setVisible(false);
            initWindow_->resetState();
            loginWindow_->setClickable(true);
            loginWindow_->getGraphicsObject()->setFocus();
        });

        // Init size
        QVariantAnimation *initHeightAnim = new QVariantAnimation(this);
        initHeightAnim->setStartValue(initWindowInitHeight_);
        initHeightAnim->setEndValue(LOGIN_HEIGHT);
        initHeightAnim->setDuration(REVEAL_LOGIN_ANIMATION_DURATION);
        connect(initHeightAnim, &QVariantAnimation::valueChanged, [this](const QVariant &value)
        {
            initWindow_->setHeight(value.toInt());
            shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_INIT_WINDOW,
                                                QRect(0,0, initWindow_->getGraphicsObject()->boundingRect().width(),
                                                      initWindow_->getGraphicsObject()->boundingRect().height()));
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
    }
    else if (curWindow_ == WINDOW_ID_EMERGENCY)
    {
        curWindow_ = WINDOW_ID_LOGIN;

        emergencyConnectWindow_->getGraphicsObject()->stackBefore(loginWindow_->getGraphicsObject());
        emergencyConnectWindow_->setClickable(false);

        loginWindow_->getGraphicsObject()->setOpacity(0.0);
        loginWindow_->getGraphicsObject()->setVisible(true);

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(loginWindow_->getGraphicsObject());
        anim->setPropertyName("opacity");
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        connect(anim, &QPropertyAnimation::finished, [this]()
        {
            emergencyConnectWindow_->getGraphicsObject()->setVisible(false);
            loginWindow_->setClickable(true);
            loginWindow_->getGraphicsObject()->setFocus();
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
        });

        isAtomicAnimationActive_ = true;
        anim->start(QPropertyAnimation::DeleteWhenStopped);
    }
    else if (curWindow_ == WINDOW_ID_EXTERNAL_CONFIG)
    {
        curWindow_ = WINDOW_ID_LOGIN;

        externalConfigWindow_->getGraphicsObject()->stackBefore(loginWindow_->getGraphicsObject());

        loginWindow_->getGraphicsObject()->setOpacity(0.0);
        loginWindow_->getGraphicsObject()->setVisible(true);

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(loginWindow_->getGraphicsObject());
        anim->setPropertyName("opacity");
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        connect(anim, &QPropertyAnimation::finished, [this]()
        {
            externalConfigWindow_->getGraphicsObject()->setVisible(false);
            loginWindow_->setClickable(true);
            loginWindow_->getGraphicsObject()->setFocus();
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
        });

        isAtomicAnimationActive_ = true;
        anim->start(QPropertyAnimation::DeleteWhenStopped);
    }
    else if (curWindow_ == WINDOW_ID_TWO_FACTOR_AUTH)
    {
        curWindow_ = WINDOW_ID_LOGIN;

        twoFactorAuthWindow_->getGraphicsObject()->stackBefore(loginWindow_->getGraphicsObject());

        loginWindow_->getGraphicsObject()->setOpacity(0.0);
        loginWindow_->getGraphicsObject()->setVisible(true);

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(loginWindow_->getGraphicsObject());
        anim->setPropertyName("opacity");
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        connect(anim, &QPropertyAnimation::finished, [this]()
        {
            twoFactorAuthWindow_->resetState();
            twoFactorAuthWindow_->getGraphicsObject()->setVisible(false);
            loginWindow_->setClickable(true);
            loginWindow_->getGraphicsObject()->setFocus();
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
        });

        isAtomicAnimationActive_ = true;
        anim->start(QPropertyAnimation::DeleteWhenStopped);
    }
    else if (curWindow_ == WINDOW_ID_LOGGING_IN)
    {
        // qDebug() << "LoggingIn -> Login";
        curWindow_ = WINDOW_ID_LOGIN;

        loggingInWindow_->getGraphicsObject()->stackBefore(loginWindow_->getGraphicsObject());

        loginWindow_->getGraphicsObject()->setOpacity(0.0);
        loginWindow_->getGraphicsObject()->setVisible(true);

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(loginWindow_->getGraphicsObject());
        anim->setPropertyName("opacity");
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        connect(anim, &QPropertyAnimation::finished, [this]()
        {
            loggingInWindow_->getGraphicsObject()->setVisible(false);
            loggingInWindow_->stopAnimation();
            loginWindow_->setClickable(true);
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
        });

        isAtomicAnimationActive_ = true;
        anim->start(QPropertyAnimation::DeleteWhenStopped);
    }
    else if (curWindow_ == WINDOW_ID_CONNECT || curWindow_ == WINDOW_ID_UPDATE || curWindow_ == WINDOW_ID_UPGRADE)
    {
        // qDebug() << "Other -> Login";
        hideUpdateWidget();
        newsFeedWindow_->getGraphicsObject()->hide();
        updateWindow_->getGraphicsObject()->hide();
        upgradeAccountWindow_->getGraphicsObject()->hide();
        curWindow_ = WINDOW_ID_LOGIN;

        connectWindow_->getGraphicsObject()->stackBefore(loginWindow_->getGraphicsObject());
        connectWindow_->setClickable(false);

        shadowManager_->setVisible(ShadowManager::SHAPE_ID_CONNECT_WINDOW, false);
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_LOGIN_WINDOW, true);

        loginWindow_->getGraphicsObject()->setOpacity(1.0);
        loginWindow_->getGraphicsObject()->setVisible(true);
        loginWindow_->setClickable(true);

        connectWindow_->getGraphicsObject()->hide();
        TooltipController::instance().hideAllTooltips();

        if (bottomInfoWindow_->getGraphicsObject()->isVisible())
        {
            bottomInfoWindow_->getGraphicsObject()->hide();
            bottomInfoWindow_->setClickable(false);
            shadowManager_->removeObject(ShadowManager::SHAPE_ID_BOTTOM_INFO);
        }

        if (isLocationsExpanded())
        {
            connectWindow_->updateLocationsState(false);
            locationsWindow_->hide();
            locationListAnimationState_ = LOCATION_LIST_ANIMATION_COLLAPSED;
            clearMaskForGraphicsView();
        }

        updateMainAndViewGeometry(false);
    }
}

void MainWindowController::gotoEmergencyWindow()
{
    WS_ASSERT(curWindow_ == WINDOW_ID_LOGIN);

    curWindow_ = WINDOW_ID_EMERGENCY;

    loginWindow_->setClickable(false);
    TooltipController::instance().hideAllTooltips();
    loginWindow_->getGraphicsObject()->stackBefore(emergencyConnectWindow_->getGraphicsObject());

    emergencyConnectWindow_->getGraphicsObject()->setOpacity(0.0);
    emergencyConnectWindow_->getGraphicsObject()->setVisible(true);
    emergencyConnectWindow_->setClickable(true);

    QPropertyAnimation *anim = new QPropertyAnimation(this);
    anim->setTargetObject(emergencyConnectWindow_->getGraphicsObject());
    anim->setPropertyName("opacity");
    anim->setStartValue(emergencyConnectWindow_->getGraphicsObject()->opacity());
    anim->setEndValue(0.96);
    anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

    connect(anim, &QPropertyAnimation::finished, [this]()
    {
        loginWindow_->getGraphicsObject()->setVisible(false);
        isAtomicAnimationActive_ = false;
        handleNextWindowChange();
        emergencyConnectWindow_->getGraphicsObject()->setFocus();
    });

    isAtomicAnimationActive_ = true;
    anim->start(QPropertyAnimation::DeleteWhenStopped);
}

void MainWindowController::gotoLoggingInWindow()
{
    WS_ASSERT(curWindow_ == WINDOW_ID_LOGIN
             || curWindow_ == WINDOW_ID_INITIALIZATION
             || curWindow_ == WINDOW_ID_EXTERNAL_CONFIG
             || curWindow_ == WINDOW_ID_TWO_FACTOR_AUTH);

    if (curWindow_ == WINDOW_ID_LOGIN)
    {
        curWindow_ = WINDOW_ID_LOGGING_IN;
        loginWindow_->setClickable(false);
        TooltipController::instance().hideAllTooltips();
        loginWindow_->getGraphicsObject()->stackBefore(loggingInWindow_->getGraphicsObject());

        loggingInWindow_->getGraphicsObject()->setOpacity(0.0);
        loggingInWindow_->getGraphicsObject()->setVisible(true);
        loggingInWindow_->startAnimation();

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(loggingInWindow_->getGraphicsObject());
        anim->setPropertyName("opacity");
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);
        connect(anim, &QPropertyAnimation::finished, [this]()
        {
            loginWindow_->getGraphicsObject()->setVisible(false);
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
        });

        isAtomicAnimationActive_ = true;
        anim->start(QPropertyAnimation::DeleteWhenStopped);
    }
    else if (curWindow_ == WINDOW_ID_INITIALIZATION)
    {
        curWindow_ = WINDOW_ID_LOGGING_IN;
        initWindow_->setClickable(false);
        initWindow_->getGraphicsObject()->stackBefore(loggingInWindow_->getGraphicsObject());

        loggingInWindow_->getGraphicsObject()->setOpacity(0.0);
        loggingInWindow_->getGraphicsObject()->setVisible(true);
        loggingInWindow_->startAnimation();

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(loggingInWindow_->getGraphicsObject());
        anim->setPropertyName("opacity");
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);
        connect(anim, &QPropertyAnimation::finished, [this]()
        {
            initWindow_->getGraphicsObject()->setVisible(false);
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
        });

        isAtomicAnimationActive_ = true;
        anim->start(QPropertyAnimation::DeleteWhenStopped);
    }
    else if (curWindow_ == WINDOW_ID_EXTERNAL_CONFIG)
    {
        curWindow_ = WINDOW_ID_LOGGING_IN;
        externalConfigWindow_->setClickable(false);
        externalConfigWindow_->getGraphicsObject()->stackBefore(loggingInWindow_->getGraphicsObject());

        loggingInWindow_->getGraphicsObject()->setOpacity(0.0);
        loggingInWindow_->getGraphicsObject()->setVisible(true);
        loggingInWindow_->startAnimation();

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(loggingInWindow_->getGraphicsObject());
        anim->setPropertyName("opacity");
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);
        connect(anim, &QPropertyAnimation::finished, [this]()
        {
            externalConfigWindow_->getGraphicsObject()->setVisible(false);
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
        });

        isAtomicAnimationActive_ = true;
        anim->start(QPropertyAnimation::DeleteWhenStopped);
    }
    else if (curWindow_ == WINDOW_ID_TWO_FACTOR_AUTH)
    {
        curWindow_ = WINDOW_ID_LOGGING_IN;
        twoFactorAuthWindow_->setClickable(false);
        twoFactorAuthWindow_->getGraphicsObject()->stackBefore(loggingInWindow_->getGraphicsObject());

        loggingInWindow_->getGraphicsObject()->setOpacity(0.0);
        loggingInWindow_->getGraphicsObject()->setVisible(true);
        loggingInWindow_->startAnimation();

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(loggingInWindow_->getGraphicsObject());
        anim->setPropertyName("opacity");
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);
        connect(anim, &QPropertyAnimation::finished, [this]()
        {
            twoFactorAuthWindow_->resetState();
            twoFactorAuthWindow_->getGraphicsObject()->setVisible(false);
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

void MainWindowController::gotoConnectWindow()
{
    // qDebug() << "gotoConnectWindow()";
    WS_ASSERT(curWindow_ == WINDOW_ID_LOGGING_IN
             || curWindow_ == WINDOW_ID_INITIALIZATION
             || curWindow_ == WINDOW_ID_UPDATE
             || curWindow_ == WINDOW_ID_UPGRADE
             || curWindow_ == WINDOW_ID_GENERAL_MESSAGE
             || curWindow_ == WINDOW_ID_EXTERNAL_CONFIG);

    if (curWindow_ == WINDOW_ID_LOGGING_IN)
    {
        // qDebug() << "LoggingIn -> Connect";
        loggingInWindow_->getGraphicsObject()->stackBefore(connectWindow_->getGraphicsObject());

        curWindow_ = WINDOW_ID_CONNECT;
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_LOGIN_WINDOW, false);
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_INIT_WINDOW, false);
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_CONNECT_WINDOW, true);
        connectWindow_->setClickable(true);
        connectWindow_->getGraphicsObject()->setVisible(true);
        connectWindow_->getGraphicsObject()->show();

        loggingInWindow_->getGraphicsObject()->setVisible(false);
        loggingInWindow_->stopAnimation();

        updateMainAndViewGeometry(false);
        hideLocationsWindow();
        updateBottomInfoWindowVisibilityAndPos();

        updateExpandAnimationParameters();
        handleNextWindowChange();
    }
    else if (curWindow_ == WINDOW_ID_INITIALIZATION
             || (curWindow_ == WINDOW_ID_EXIT && windowBeforeExit_ == WINDOW_ID_INITIALIZATION))
    {
        // qDebug() << "Init -> Connect";
        initWindow_->getGraphicsObject()->stackBefore(connectWindow_->getGraphicsObject());

        isAtomicAnimationActive_ = true;

        // The second condition occurs when engine crashes and user tries to exit app using hot keys but then selects "No".
        // In this case it is necessary to set windowBeforeExit_ to WINDOW_ID_CONNECT because it will be necessary to enter
        // connect window after closing exit dialog.
        if(curWindow_ == WINDOW_ID_EXIT) {
            windowBeforeExit_ = WINDOW_ID_CONNECT;
        }

        shadowManager_->setVisible(ShadowManager::SHAPE_ID_INIT_WINDOW, false);
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_CONNECT_WINDOW, true);
        connectWindow_->getGraphicsObject()->setOpacity(OPACITY_HIDDEN);
        connectWindow_->getGraphicsObject()->setVisible(true);
        initWindow_->setCropHeight(0);
        emit revealConnectWindowStateChanged(true);

        // connect opacity
        QPropertyAnimation *revealConnectOpacityAnimation = new QPropertyAnimation(this);
        revealConnectOpacityAnimation->setTargetObject(connectWindow_->getGraphicsObject());
        revealConnectOpacityAnimation->setPropertyName("opacity");
        revealConnectOpacityAnimation->setStartValue(OPACITY_HIDDEN);
        revealConnectOpacityAnimation->setEndValue(OPACITY_FULL);
        revealConnectOpacityAnimation->setDuration(REVEAL_CONNECT_ANIMATION_DURATION/2);
        connect(revealConnectOpacityAnimation, &QPropertyAnimation::finished, [this]()
        {
            emit revealConnectWindowStateChanged(false);
            connectWindow_->setClickable(true);
            initWindow_->getGraphicsObject()->setVisible(false);
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
        connect(revealInitSizeAnimation, &QVariantAnimation::valueChanged, [this](const QVariant &value)
        {
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
            if (bottomInfoWindow_->getGraphicsObject()->isVisible()) {
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
    }
    else if (curWindow_ == WINDOW_ID_UPDATE)
    {
        //qDebug() << "Update -> Connect";
        curWindow_ = WINDOW_ID_CONNECT;
        isAtomicAnimationActive_ = true;

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(updateWindow_->getGraphicsObject());
        anim->setPropertyName("opacity");
        anim->setStartValue(updateWindow_->getGraphicsObject()->opacity());
        anim->setEndValue(0.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        connect(anim, &QPropertyAnimation::finished, [this]() {
            updateWindow_->getGraphicsObject()->hide();
            connectWindow_->setClickable(true);
            bottomInfoWindow_->setClickable(true);

            if (bottomInfoWindow_->getGraphicsObject()->isVisible())
            {
                std::function<void()> finish_function = [this](){
                    isAtomicAnimationActive_ = false;
                    updateBottomInfoWindowVisibilityAndPos();
                    handleNextWindowChange();
                };

                animateBottomInfoWindow(QAbstractAnimation::Backward, finish_function);
            }
            else
            {
                isAtomicAnimationActive_ = false;
                handleNextWindowChange();
            }
        });

        anim->start(QPropertyAnimation::DeleteWhenStopped);
    }
    else if (curWindow_ == WINDOW_ID_UPGRADE)
    {
        // qDebug() << "Upgrade -> Connect";
        curWindow_ = WINDOW_ID_CONNECT;
        isAtomicAnimationActive_ = true;

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(upgradeAccountWindow_->getGraphicsObject());
        anim->setPropertyName("opacity");
        anim->setStartValue(upgradeAccountWindow_->getGraphicsObject()->opacity());
        anim->setEndValue(0.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        connect(anim, &QPropertyAnimation::finished, [this]() {
            upgradeAccountWindow_->getGraphicsObject()->hide();
            connectWindow_->setClickable(true);
            bottomInfoWindow_->setClickable(true);

            if (bottomInfoWindow_->getGraphicsObject()->isVisible())
            {
                std::function<void()> finish_function = [this](){
                    isAtomicAnimationActive_ = false;
                    updateBottomInfoWindowVisibilityAndPos();
                    handleNextWindowChange();
                };

                animateBottomInfoWindow(QAbstractAnimation::Backward, finish_function);
            }
            else
            {
                isAtomicAnimationActive_ = false;
                handleNextWindowChange();
            }
        });

        anim->start(QPropertyAnimation::DeleteWhenStopped);
    }

    else if (curWindow_ == WINDOW_ID_GENERAL_MESSAGE)
    {
        // qDebug() << "Message -> Connect";
        curWindow_ = WINDOW_ID_CONNECT;
        isAtomicAnimationActive_ = true;

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(generalMessageWindow_->getGraphicsObject());
        anim->setPropertyName("opacity");
        anim->setStartValue(generalMessageWindow_->getGraphicsObject()->opacity());
        anim->setEndValue(0.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        connect(anim, &QPropertyAnimation::finished, [this]() {
            generalMessageWindow_->getGraphicsObject()->hide();
            connectWindow_->setClickable(true);
            bottomInfoWindow_->setClickable(true);

            if (bottomInfoWindow_->getGraphicsObject()->isVisible())
            {
                std::function<void()> finish_function = [this](){
                    isAtomicAnimationActive_ = false;
                    updateBottomInfoWindowVisibilityAndPos();
                    handleNextWindowChange();
                };

                animateBottomInfoWindow(QAbstractAnimation::Backward, finish_function);
            }
            else
            {
                isAtomicAnimationActive_ = false;
                handleNextWindowChange();
            }
        });

        anim->start(QPropertyAnimation::DeleteWhenStopped);
    }
    else if (curWindow_ == WINDOW_ID_EXTERNAL_CONFIG)
    {
        // qDebug() << "External Config -> Connect";
        externalConfigWindow_->getGraphicsObject()->stackBefore(connectWindow_->getGraphicsObject());

        curWindow_ = WINDOW_ID_CONNECT;
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_LOGIN_WINDOW, false);
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_CONNECT_WINDOW, true);
        connectWindow_->setClickable(true);
        connectWindow_->getGraphicsObject()->setVisible(true);
        externalConfigWindow_->getGraphicsObject()->setVisible(false);
        externalConfigWindow_->setClickable(false);

        updateMainAndViewGeometry(false);

        hideLocationsWindow();

        updateBottomInfoWindowVisibilityAndPos();

        updateExpandAnimationParameters();
        handleNextWindowChange();
    }
    else if (curWindow_ == WINDOW_ID_TWO_FACTOR_AUTH)
    {
        // qDebug() << "2FA -> Connect";
        twoFactorAuthWindow_->getGraphicsObject()->stackBefore(connectWindow_->getGraphicsObject());

        curWindow_ = WINDOW_ID_CONNECT;
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_LOGIN_WINDOW, false);
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_CONNECT_WINDOW, true);
        connectWindow_->setClickable(true);
        connectWindow_->getGraphicsObject()->setVisible(true);
        twoFactorAuthWindow_->resetState();
        twoFactorAuthWindow_->getGraphicsObject()->setVisible(false);
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
    loginWindow_->getGraphicsObject()->stackBefore(externalConfigWindow_->getGraphicsObject());

    externalConfigWindow_->getGraphicsObject()->setOpacity(0.0);
    externalConfigWindow_->getGraphicsObject()->setVisible(true);

    QPropertyAnimation *anim = new QPropertyAnimation(this);
    anim->setTargetObject(externalConfigWindow_->getGraphicsObject());
    anim->setPropertyName("opacity");
    anim->setStartValue(externalConfigWindow_->getGraphicsObject()->opacity());
    anim->setEndValue(0.96);
    anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

    connect(anim, &QPropertyAnimation::finished, [this]()
    {
        loginWindow_->getGraphicsObject()->setVisible(false);
        isAtomicAnimationActive_ = false;
        handleNextWindowChange();
        externalConfigWindow_->setClickable(true);
        externalConfigWindow_->getGraphicsObject()->setFocus();
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
        loginWindow_->getGraphicsObject()->stackBefore(twoFactorAuthWindow_->getGraphicsObject());
        break;
    case WINDOW_ID_LOGGING_IN:
        loggingInWindow_->getGraphicsObject()->stackBefore(twoFactorAuthWindow_->getGraphicsObject());
        break;
    default:
        break;
    }

    twoFactorAuthWindow_->resetState();
    twoFactorAuthWindow_->getGraphicsObject()->setOpacity(0.0);
    twoFactorAuthWindow_->getGraphicsObject()->setVisible(true);

    QPropertyAnimation *anim = new QPropertyAnimation(this);
    anim->setTargetObject(twoFactorAuthWindow_->getGraphicsObject());
    anim->setPropertyName("opacity");
    anim->setStartValue(twoFactorAuthWindow_->getGraphicsObject()->opacity());
    anim->setEndValue(1.0);
    anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

    connect(anim, &QPropertyAnimation::finished, [this, prevWindow]()
    {
        switch (prevWindow) {
        case WINDOW_ID_LOGIN:
            loginWindow_->getGraphicsObject()->setVisible(false);
            break;
        case WINDOW_ID_LOGGING_IN:
            loggingInWindow_->getGraphicsObject()->setVisible(false);
            loggingInWindow_->stopAnimation();
        default:
            break;
        }
        isAtomicAnimationActive_ = false;
        handleNextWindowChange();
        twoFactorAuthWindow_->setClickable(true);
        twoFactorAuthWindow_->getGraphicsObject()->setFocus();
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

    // if preferences expanded then collapse preferences first
    if (preferencesState_ != CHILD_WINDOW_STATE_COLLAPSED)
    {
        queueWindowChanges_.enqueue(WINDOW_ID_UPDATE);
        collapsePreferencesFromConnect(true);
        return;
    }

    // if news expanded then collapse news first
    if (newsFeedState_ != CHILD_WINDOW_STATE_COLLAPSED)
    {
        queueWindowChanges_.enqueue(WINDOW_ID_UPDATE);
        collapseNewsFeed();
        return;
    }

    isAtomicAnimationActive_ = true;
    WINDOW_ID saveCurWindow = curWindow_;
    curWindow_ = WINDOW_ID_UPDATE;

    TooltipController::instance().hideAllTooltips();
    connectWindow_->setClickable(false);
    bottomInfoWindow_->setClickable(false);

    functionOnAnimationFinished_ = [this, saveCurWindow]()
    {
        updateWindow_->getGraphicsObject()->setOpacity(0.0);
        if (saveCurWindow == WINDOW_ID_GENERAL_MESSAGE)
        {
            generalMessageWindow_->getGraphicsObject()->stackBefore(updateWindow_->getGraphicsObject());
        }
        else
        {
            connectWindow_->getGraphicsObject()->stackBefore(updateWindow_->getGraphicsObject());
        }
        updateWindow_->getGraphicsObject()->show();

        QPropertyAnimation *anim1 = new QPropertyAnimation(this);
        anim1->setTargetObject(updateWindow_->getGraphicsObject());
        anim1->setPropertyName("opacity");
        anim1->setStartValue(updateWindow_->getGraphicsObject()->opacity());
        anim1->setEndValue(0.96);
        anim1->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        // anim2 was removed since WINDOW_ID_NOTIFICATION is no longer valid

        QPropertyAnimation *anim3 = NULL;
        if (saveCurWindow == WINDOW_ID_UPGRADE)
        {
            anim3 = new QPropertyAnimation(this);
            anim3->setTargetObject(upgradeAccountWindow_->getGraphicsObject());
            anim3->setPropertyName("opacity");
            anim3->setStartValue(upgradeAccountWindow_->getGraphicsObject()->opacity());
            anim3->setEndValue(0.0);
            anim3->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

            connect(anim3, &QPropertyAnimation::finished, [this]()
            {
                upgradeAccountWindow_->getGraphicsObject()->hide();
            });
        }

        QPropertyAnimation *anim4 = NULL;
        if (saveCurWindow == WINDOW_ID_GENERAL_MESSAGE)
        {
            anim4 = new QPropertyAnimation(this);
            anim4->setTargetObject(generalMessageWindow_->getGraphicsObject());
            //anim4->setPropertyName("opacity");
            anim4->setStartValue(generalMessageWindow_->getGraphicsObject()->opacity());
            anim4->setEndValue(0.0);
            anim4->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

            connect(anim4, &QPropertyAnimation::finished, [this]()
            {
                generalMessageWindow_->getGraphicsObject()->hide();
                updateWindow_->setClickable(true);
            });
        }

        QPropertyAnimation *anim5 = NULL;
        if (saveCurWindow == WINDOW_ID_EXIT)
        {
            anim5 = new QPropertyAnimation(this);
            anim5->setTargetObject(exitWindow_->getGraphicsObject());
            anim5->setPropertyName("opacity");
            anim5->setStartValue(exitWindow_->getGraphicsObject()->opacity());
            anim5->setEndValue(0.0);
            anim5->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

            connect(anim5, &QPropertyAnimation::finished, [this]()
            {
                exitWindow_->getGraphicsObject()->hide();
            });
        }

        QParallelAnimationGroup *group = new QParallelAnimationGroup(this);
        group->addAnimation(anim1);
        if (anim3 != NULL)
        {
            group->addAnimation(anim3);
        }
        if (anim4 != NULL)
        {
            group->addAnimation(anim4);
        }
        if (anim5 != NULL)
        {
            group->addAnimation(anim5);
        }

        connect(group, &QPropertyAnimation::finished, [this]()
        {
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
            updateWindow_->getGraphicsObject()->setFocus();
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

    functionOnAnimationFinished_ = [this]()
    {
        upgradeAccountWindow_->getGraphicsObject()->setOpacity(0.0);
        connectWindow_->getGraphicsObject()->stackBefore(upgradeAccountWindow_->getGraphicsObject());
        upgradeAccountWindow_->getGraphicsObject()->show();

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(upgradeAccountWindow_->getGraphicsObject());
        anim->setPropertyName("opacity");
        anim->setStartValue(upgradeAccountWindow_->getGraphicsObject()->opacity());
        anim->setEndValue(0.96);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        connect(anim, &QPropertyAnimation::finished, [this]()
        {
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
            upgradeAccountWindow_->getGraphicsObject()->setFocus();
        });

        anim->start(QPropertyAnimation::DeleteWhenStopped);
    };

    collapseAllExpandedOnBottom();
}

void MainWindowController::gotoGeneralMessageWindow()
{
    WS_ASSERT(curWindow_ == WINDOW_ID_CONNECT || curWindow_ == WINDOW_ID_UPDATE);

    isAtomicAnimationActive_ = true;
    WINDOW_ID saveCurWindow = curWindow_;
    curWindow_ = WINDOW_ID_GENERAL_MESSAGE;

    TooltipController::instance().hideAllTooltips();
    if (saveCurWindow == WINDOW_ID_CONNECT)
    {
        connectWindow_->setClickable(false);
    }
    else if (saveCurWindow == WINDOW_ID_UPDATE)
    {
        updateWindow_->setClickable(false);
    }
    else
    {
        WS_ASSERT(false);
    }

    bottomInfoWindow_->setClickable(false);

    functionOnAnimationFinished_ = [this, saveCurWindow]()
    {
        generalMessageWindow_->getGraphicsObject()->setOpacity(0.0);
        if (saveCurWindow == WINDOW_ID_CONNECT)
        {
            connectWindow_->getGraphicsObject()->stackBefore(generalMessageWindow_->getGraphicsObject());
        }
        else if (saveCurWindow == WINDOW_ID_UPDATE)
        {
            updateWindow_->getGraphicsObject()->stackBefore(generalMessageWindow_->getGraphicsObject());
        }
        else
        {
            WS_ASSERT(false);
        }
        generalMessageWindow_->getGraphicsObject()->show();

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(generalMessageWindow_->getGraphicsObject());
        anim->setPropertyName("opacity");
        anim->setStartValue(generalMessageWindow_->getGraphicsObject()->opacity());
        anim->setEndValue(0.96);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        connect(anim, &QPropertyAnimation::finished, [this]()
        {
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
            generalMessageWindow_->getGraphicsObject()->setFocus();
        });

        anim->start(QPropertyAnimation::DeleteWhenStopped);
    };

    collapseAllExpandedOnBottom();
}

void MainWindowController::gotoExitWindow()
{
    WS_ASSERT(curWindow_ == WINDOW_ID_CONNECT
             || curWindow_ == WINDOW_ID_LOGIN
             || curWindow_ == WINDOW_ID_EMERGENCY
             || curWindow_ == WINDOW_ID_EXTERNAL_CONFIG
             || curWindow_ == WINDOW_ID_TWO_FACTOR_AUTH);

    windowBeforeExit_ = curWindow_;

    if (curWindow_ == WINDOW_ID_CONNECT)
    {
        if (preferences_->appSkin() == APP_SKIN_VAN_GOGH)
        {
            exitWindow_->setHeight(connectWindow_->getGraphicsObject()->boundingRect().height()/G_SCALE);
        }
        else
        {
            exitWindow_->setHeight(LOGIN_HEIGHT);
        }
        exitWindow_->setBackgroundShapedToConnectWindow(preferences_->appSkin() == APP_SKIN_ALPHA);

        TooltipController::instance().hideAllTooltips();
        connectWindow_->setClickable(false);
        bottomInfoWindow_->setClickable(false);
    }
    else if (curWindow_ == WINDOW_ID_LOGIN)
    {
        exitWindow_->setHeight(LOGIN_HEIGHT);
        exitWindow_->setBackgroundShapedToConnectWindow(false);
        TooltipController::instance().hideAllTooltips();
        loginWindow_->setClickable(false);
    }
    else if (curWindow_ == WINDOW_ID_EMERGENCY)
    {
        exitWindow_->setHeight(LOGIN_HEIGHT);
        exitWindow_->setBackgroundShapedToConnectWindow(false);
        emergencyConnectWindow_->setClickable(false);
    }
    else if (curWindow_ == WINDOW_ID_EXTERNAL_CONFIG)
    {
        exitWindow_->setHeight(LOGIN_HEIGHT);
        exitWindow_->setBackgroundShapedToConnectWindow(false);
        externalConfigWindow_->setClickable(false);
    }
    else if (curWindow_ == WINDOW_ID_TWO_FACTOR_AUTH)
    {
        exitWindow_->setHeight(LOGIN_HEIGHT);
        exitWindow_->setBackgroundShapedToConnectWindow(false);
        twoFactorAuthWindow_->setClickable(false);
    }

    WINDOW_ID saveCurWindow = curWindow_;
    curWindow_ = WINDOW_ID_EXIT;
    isAtomicAnimationActive_ = true;

    functionOnAnimationFinished_ = [this, saveCurWindow]()
    {
        exitWindow_->getGraphicsObject()->setOpacity(0.0);
        if (saveCurWindow == WINDOW_ID_CONNECT)
        {
            connectWindow_->getGraphicsObject()->stackBefore(exitWindow_->getGraphicsObject());
        }
        else if (saveCurWindow == WINDOW_ID_LOGIN)
        {
            loginWindow_->getGraphicsObject()->stackBefore(exitWindow_->getGraphicsObject());
        }
        else if (saveCurWindow == WINDOW_ID_EMERGENCY)
        {
            emergencyConnectWindow_->getGraphicsObject()->stackBefore(exitWindow_->getGraphicsObject());
        }
        else if (saveCurWindow == WINDOW_ID_EXTERNAL_CONFIG)
        {
            externalConfigWindow_->getGraphicsObject()->stackBefore(exitWindow_->getGraphicsObject());
        }

        exitWindow_->changeSelection(IGeneralMessageTwoButtonWindow::ACCEPT);
        exitWindow_->getGraphicsObject()->show();

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(exitWindow_->getGraphicsObject());
        anim->setPropertyName("opacity");
        anim->setStartValue(exitWindow_->getGraphicsObject()->opacity());
        anim->setEndValue(0.96);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        connect(anim, &QPropertyAnimation::finished, [this]()
        {
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
            exitWindow_->getGraphicsObject()->setFocus();
        });

        anim->start(QPropertyAnimation::DeleteWhenStopped);
    };

    collapseAllExpandedOnBottom();
}

void MainWindowController::closeExitWindow()
{
    WS_ASSERT(curWindow_ == WINDOW_ID_EXIT);

    if (windowBeforeExit_ == WINDOW_ID_CONNECT)
    {
        curWindow_ = WINDOW_ID_CONNECT;
        isAtomicAnimationActive_ = true;

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(exitWindow_->getGraphicsObject());
        anim->setPropertyName("opacity");
        anim->setStartValue(exitWindow_->getGraphicsObject()->opacity());
        anim->setEndValue(0.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        connect(anim, &QPropertyAnimation::finished, [this]() {
            exitWindow_->getGraphicsObject()->hide();
            connectWindow_->setClickable(true);
            bottomInfoWindow_->setClickable(true);

            if (bottomInfoWindow_->getGraphicsObject()->isVisible())
            {
                std::function<void()> finish_function = [this](){
                    isAtomicAnimationActive_ = false;
                    updateBottomInfoWindowVisibilityAndPos();
                    handleNextWindowChange();
                };

                animateBottomInfoWindow(QAbstractAnimation::Backward, finish_function);
            }
            else
            {
                isAtomicAnimationActive_ = false;
                handleNextWindowChange();
            }
        });

        anim->start(QPropertyAnimation::DeleteWhenStopped);
    }
    else if (windowBeforeExit_ == WINDOW_ID_LOGIN)
    {
        curWindow_ = WINDOW_ID_LOGIN;
        isAtomicAnimationActive_ = true;

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(exitWindow_->getGraphicsObject());
        anim->setPropertyName("opacity");
        anim->setStartValue(exitWindow_->getGraphicsObject()->opacity());
        anim->setEndValue(0.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        connect(anim, &QPropertyAnimation::finished, [this]() {
            exitWindow_->getGraphicsObject()->hide();
            loginWindow_->setClickable(true);
            loginWindow_->getGraphicsObject()->setFocus();
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
        });

        anim->start(QPropertyAnimation::DeleteWhenStopped);
    }
    else if (windowBeforeExit_ == WINDOW_ID_EMERGENCY)
    {
        curWindow_ = WINDOW_ID_EMERGENCY;
        isAtomicAnimationActive_ = true;

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(exitWindow_->getGraphicsObject());
        anim->setPropertyName("opacity");
        anim->setStartValue(exitWindow_->getGraphicsObject()->opacity());
        anim->setEndValue(0.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        connect(anim, &QPropertyAnimation::finished, [this]() {
            exitWindow_->getGraphicsObject()->hide();
            emergencyConnectWindow_->setClickable(true);
            emergencyConnectWindow_->getGraphicsObject()->setFocus();
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
        });

        anim->start(QPropertyAnimation::DeleteWhenStopped);
    }
    else if (windowBeforeExit_ == WINDOW_ID_EXTERNAL_CONFIG)
    {
        curWindow_ = WINDOW_ID_EXTERNAL_CONFIG;
        isAtomicAnimationActive_ = true;

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(exitWindow_->getGraphicsObject());
        anim->setPropertyName("opacity");
        anim->setStartValue(exitWindow_->getGraphicsObject()->opacity());
        anim->setEndValue(0.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        connect(anim, &QPropertyAnimation::finished, [this]() {
            exitWindow_->getGraphicsObject()->hide();
            externalConfigWindow_->setClickable(true);
            externalConfigWindow_->getGraphicsObject()->setFocus();
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
        });

        anim->start(QPropertyAnimation::DeleteWhenStopped);
    }
    else if (windowBeforeExit_ == WINDOW_ID_TWO_FACTOR_AUTH)
    {
        curWindow_ = WINDOW_ID_TWO_FACTOR_AUTH;
        isAtomicAnimationActive_ = true;

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(exitWindow_->getGraphicsObject());
        anim->setPropertyName("opacity");
        anim->setStartValue(exitWindow_->getGraphicsObject()->opacity());
        anim->setEndValue(0.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        connect(anim, &QPropertyAnimation::finished, [this]() {
            exitWindow_->getGraphicsObject()->hide();
            twoFactorAuthWindow_->setClickable(true);
            twoFactorAuthWindow_->getGraphicsObject()->setFocus();
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
        });

        anim->start(QPropertyAnimation::DeleteWhenStopped);
    }
    else if (windowBeforeExit_ == WINDOW_ID_INITIALIZATION)
    {
        curWindow_ = WINDOW_ID_INITIALIZATION;
        isAtomicAnimationActive_ = true;

        QPropertyAnimation *anim = new QPropertyAnimation(this);
        anim->setTargetObject(exitWindow_->getGraphicsObject());
        anim->setPropertyName("opacity");
        anim->setStartValue(exitWindow_->getGraphicsObject()->opacity());
        anim->setEndValue(0.0);
        anim->setDuration(SCREEN_SWITCH_OPACITY_ANIMATION_DURATION);

        connect(anim, &QPropertyAnimation::finished, [this]() {
            exitWindow_->getGraphicsObject()->hide();
            newsFeedWindow_->setClickable(true);
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

    if (preferencesState_ != CHILD_WINDOW_STATE_COLLAPSED)
    {
        return;
    }

    isAtomicAnimationActive_ = true;
    preferencesState_ = CHILD_WINDOW_STATE_ANIMATING;

    preferencesWindow_->setScrollBarVisibility(false);
    preferencesWindow_->getGraphicsObject()->setOpacity(0.0);
    preferencesWindow_->getGraphicsObject()->show();
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
        preferencesWindow_->getGraphicsObject()->setOpacity(prefOpacity);
        shadowManager_->setOpacity(ShadowManager::SHAPE_ID_PREFERENCES, prefOpacity, true);
        shadowManager_->setOpacity(ShadowManager::SHAPE_ID_LOGIN_WINDOW, 1.0 - prefOpacity, true);
        loginWindow_->getGraphicsObject()->setOpacity(1.0 - prefOpacity);
        shadowManager_->setOpacity(ShadowManager::SHAPE_ID_CONNECT_WINDOW, prefOpacity, false);
    });

    int start = (int)loginWindow_->getGraphicsObject()->boundingRect().height();
    int target =  preferencesWindowHeight_;
    QVariantAnimation *animResize = new QVariantAnimation(this);
    animResize->setStartValue(start);
    animResize->setEndValue(target);
    animResize->setDuration(EXPAND_PREFERENCES_RESIZE_DURATION);
    connect(animResize, &QVariantAnimation::valueChanged, [this](const QVariant &value) {
        preferencesWindow_->setHeight(value.toInt());
        updateMainAndViewGeometry(false);
        shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_PREFERENCES,
                                            QRect(0, childWindowShadowOffsetY(), preferencesWindow_->getGraphicsObject()->boundingRect().width(),
                                                  preferencesWindow_->getGraphicsObject()->boundingRect().height() - childWindowShadowOffsetY()));
        invalidateShadow_mac();
        keepWindowInsideScreenCoordinates();
    });

    // resize finished
    connect(animResize, &QVariantAnimation::finished, [this]() {
        loginWindow_->getGraphicsObject()->hide();
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_LOGIN_WINDOW, false);
        preferencesWindow_->setScrollBarVisibility(true);
        preferencesWindow_->getGraphicsObject()->setFocus();
    });

    loginWindow_->setClickable(false);

    // group finished
    QParallelAnimationGroup *animGroup = new QParallelAnimationGroup(this);
    connect(animGroup, &QVariantAnimation::finished, [this]() {
        preferencesState_ = CHILD_WINDOW_STATE_EXPANDED;
        updateCursorInViewport();
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_PREFERENCES, true);
        shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_PREFERENCES,
                                            QRect(0, childWindowShadowOffsetY(),
                                                  preferencesWindow_->getGraphicsObject()->boundingRect().width(),
                                                  preferencesWindow_->getGraphicsObject()->boundingRect().height() - childWindowShadowOffsetY()));
        invalidateShadow_mac();

        isAtomicAnimationActive_ = false;
        handleNextWindowChange();
    });

    animGroup->addAnimation(animOpacity);
    animGroup->addAnimation(animResize);

    animGroup->start(QVariantAnimation::DeleteWhenStopped);

    //preferencesWindow_->getGraphicsObject()->setFocus();
}

void MainWindowController::expandPreferencesFromConnect()
{
    // qCDebug(LOG_BASIC) << "MainWindowController::expandPreferencesFromConnect";

    if (preferencesState_ != CHILD_WINDOW_STATE_COLLAPSED)
    {
        return;
    }

    // expand preferences
    functionOnAnimationFinished_ = [this]()
    {
        if (expandLocationsAnimationGroup_ && expandLocationsAnimationGroup_->state() == QAbstractAnimation::Running)
        {
            WS_ASSERT(false);
            return;
        }

        TooltipController::instance().hideAllTooltips();

        preferencesState_ = CHILD_WINDOW_STATE_ANIMATING;

        int start = (int)connectWindow_->getGraphicsObject()->boundingRect().height();
        if (newsFeedState_ == CHILD_WINDOW_STATE_EXPANDED)
        {
            start = (int)newsFeedWindow_->getGraphicsObject()->boundingRect().height();
        }
        int target = preferencesWindowHeight_;
        preferencesWindow_->setHeight(start);

        preferencesWindow_->setScrollBarVisibility(false);
        preferencesWindow_->getGraphicsObject()->setOpacity(0.0);
        preferencesWindow_->getGraphicsObject()->show();
        updateMainAndViewGeometry(false);

        shadowManager_->setOpacity(ShadowManager::SHAPE_ID_PREFERENCES, 0.0, true);
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_PREFERENCES, true);
        //shadowManager_->setVisible(WINDOW_ID_LOGIN, false);

        // opacity changed
        QVariantAnimation *animOpacity = new QVariantAnimation(this);
        animOpacity->setStartValue(0.0);
        animOpacity->setEndValue(1.0);
        animOpacity->setDuration(EXPAND_CHILD_WINDOW_OPACITY_DURATION);
        connect(animOpacity, &QVariantAnimation::valueChanged, [this](const QVariant &value) {
            preferencesWindow_->getGraphicsObject()->setOpacity(value.toDouble());
            shadowManager_->setOpacity(ShadowManager::SHAPE_ID_PREFERENCES, value.toDouble(), true);
            if (newsFeedState_ == CHILD_WINDOW_STATE_EXPANDED)
            {
                newsFeedWindow_->getGraphicsObject()->setOpacity(1 - value.toDouble());
                shadowManager_->setOpacity(ShadowManager::SHAPE_ID_NEWS_FEED, 1 - value.toDouble(), true);
            }
        });

        QVariantAnimation *animResize = new QVariantAnimation(this);
        animResize->setStartValue(start);
        animResize->setEndValue(target);
        animResize->setDuration(EXPAND_PREFERENCES_RESIZE_DURATION);
        connect(animResize, &QVariantAnimation::valueChanged, [this](const QVariant &value) {
            preferencesWindow_->setHeight(value.toInt());
            // updateLocationsWindowAndTabGeometry();
            updateMainAndViewGeometry(false);
            shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_PREFERENCES,
                                                QRect(0, childWindowShadowOffsetY(), preferencesWindow_->getGraphicsObject()->boundingRect().width(),
                                                      preferencesWindow_->getGraphicsObject()->boundingRect().height() - childWindowShadowOffsetY()));

            invalidateShadow_mac();
            keepWindowInsideScreenCoordinates();
        });

        // resize finished
        connect(animResize, &QVariantAnimation::finished, [this]() {
            if (newsFeedState_ == CHILD_WINDOW_STATE_EXPANDED)
            {
                newsFeedWindow_->getGraphicsObject()->hide();
                newsFeedState_ = CHILD_WINDOW_STATE_COLLAPSED;
            }
            connectWindow_->getGraphicsObject()->hide();
        });

        // group finished
        QParallelAnimationGroup *animGroup = new QParallelAnimationGroup(this);
        connect(animGroup, &QVariantAnimation::finished, [this]() {
            preferencesState_ = CHILD_WINDOW_STATE_EXPANDED;
            clearMaskForGraphicsView();
            updateBottomInfoWindowVisibilityAndPos();
            bottomInfoWindow_->setClickable(false);
            updateExpandAnimationParameters();
            invalidateShadow_mac();
            preferencesWindow_->setScrollBarVisibility(true);
            preferencesWindow_->getGraphicsObject()->setFocus();
            
			shadowManager_->setVisible(ShadowManager::SHAPE_ID_PREFERENCES, true);
            shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_PREFERENCES,
                                                QRect(0, childWindowShadowOffsetY(),
                                                      preferencesWindow_->getGraphicsObject()->boundingRect().width(),
                                                      preferencesWindow_->getGraphicsObject()->boundingRect().height() - childWindowShadowOffsetY()));

            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
        });

        // delay opacity
        QSequentialAnimationGroup *seqGroup = new QSequentialAnimationGroup(this);
        seqGroup->addPause(EXPAND_PREFERENCES_RESIZE_DURATION - EXPAND_CHILD_WINDOW_OPACITY_DURATION);
        seqGroup->addAnimation(animOpacity);

        animGroup->addAnimation(animResize);
        animGroup->addAnimation(seqGroup);
        animGroup->start(QVariantAnimation::DeleteWhenStopped);
    };

    isAtomicAnimationActive_ = true;

    connectWindow_->setClickable(false);

    // hide locations or bottomInfo before running preferences expand
    if (isLocationsExpanded())
    {
        connectWindow_->updateLocationsState(false);
        expandLocationsAnimationGroup_->setDirection(QAbstractAnimation::Backward);
        if (expandLocationsAnimationGroup_->state() != QAbstractAnimation::Running)
        {
            expandLocationsAnimationGroup_->start();
        }
    }
    else if (bottomInfoWindow_->getGraphicsObject()->isVisible())
    {
        std::function<void()> finish_function = [this](){
            if (functionOnAnimationFinished_ != NULL)
            {
                functionOnAnimationFinished_();
                functionOnAnimationFinished_ = NULL;
            }
        };

        animateBottomInfoWindow(QAbstractAnimation::Forward, finish_function);
    }
    else
    {
        if (functionOnAnimationFinished_ != NULL)
        {
            functionOnAnimationFinished_();
            functionOnAnimationFinished_ = NULL;
        }
    }
}

void MainWindowController::expandNewsFeed()
{
    // qCDebug(LOG_BASIC) << "MainWindowController::expandNewsFeed";

    if (newsFeedState_ != CHILD_WINDOW_STATE_COLLAPSED)
    {
        return;
    }

    // expand news feed
    functionOnAnimationFinished_ = [this]()
    {
        if (expandLocationsAnimationGroup_ && expandLocationsAnimationGroup_->state() == QAbstractAnimation::Running)
        {
            WS_ASSERT(false);
            return;
        }

        TooltipController::instance().hideAllTooltips();

        newsFeedState_ = CHILD_WINDOW_STATE_ANIMATING;

        int start = (int)connectWindow_->getGraphicsObject()->boundingRect().height();
        if (preferencesState_ == CHILD_WINDOW_STATE_EXPANDED)
        {
            start = (int)preferencesWindow_->getGraphicsObject()->boundingRect().height();
        }
        int target = newsFeedWindowHeight_;
        newsFeedWindow_->setHeight(start);

        newsFeedWindow_->setScrollBarVisibility(false);
        newsFeedWindow_->getGraphicsObject()->setOpacity(0.0);
        newsFeedWindow_->getGraphicsObject()->show();
        updateMainAndViewGeometry(false);

        shadowManager_->setOpacity(ShadowManager::SHAPE_ID_NEWS_FEED, 0.0, true);
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_NEWS_FEED, true);

        QVariantAnimation *animOpacity = new QVariantAnimation(this);
        animOpacity->setStartValue(0.0);
        animOpacity->setEndValue(1.0);
        animOpacity->setDuration(EXPAND_CHILD_WINDOW_OPACITY_DURATION);
        connect(animOpacity, &QVariantAnimation::valueChanged, [this](const QVariant &value) {
            newsFeedWindow_->getGraphicsObject()->setOpacity(value.toDouble());
            shadowManager_->setOpacity(ShadowManager::SHAPE_ID_NEWS_FEED, value.toDouble(), true);
            if (preferencesState_ == CHILD_WINDOW_STATE_EXPANDED)
            {
                preferencesWindow_->getGraphicsObject()->setOpacity(1 - value.toDouble());
                shadowManager_->setOpacity(ShadowManager::SHAPE_ID_PREFERENCES, 1 - value.toDouble(), true);
            }

        });

        QVariantAnimation *animResize = new QVariantAnimation(this);
        animResize->setStartValue(start);
        animResize->setEndValue(target);
        animResize->setDuration(EXPAND_NEWS_FEED_RESIZE_DURATION);
        connect(animResize, &QVariantAnimation::valueChanged, [this](const QVariant &value) {
            newsFeedWindow_->setHeight(value.toInt());
            updateMainAndViewGeometry(false);
            shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_NEWS_FEED,
                                                QRect(0, childWindowShadowOffsetY(),
                                                      newsFeedWindow_->getGraphicsObject()->boundingRect().width(),
                                                      newsFeedWindow_->getGraphicsObject()->boundingRect().height() - childWindowShadowOffsetY()));
            invalidateShadow_mac();
            keepWindowInsideScreenCoordinates();
        });

        // resize finished
        connect(animResize, &QVariantAnimation::finished, [this]() {
            connectWindow_->getGraphicsObject()->hide();
            if (preferencesState_ == CHILD_WINDOW_STATE_EXPANDED)
            {
                preferencesWindow_->getGraphicsObject()->hide();
                preferencesState_ = CHILD_WINDOW_STATE_COLLAPSED;
            }
        });

        // group finished
        QParallelAnimationGroup *animGroup = new QParallelAnimationGroup(this);
        connect(animGroup, &QVariantAnimation::finished, [this]() {
            newsFeedState_ = CHILD_WINDOW_STATE_EXPANDED;
            clearMaskForGraphicsView();
            updateBottomInfoWindowVisibilityAndPos();
            bottomInfoWindow_->setClickable(false);
            updateExpandAnimationParameters();
            invalidateShadow_mac();
            newsFeedWindow_->setScrollBarVisibility(true);
            newsFeedWindow_->getGraphicsObject()->setFocus();
            newsFeedWindow_->updateRead();

			shadowManager_->setVisible(ShadowManager::SHAPE_ID_NEWS_FEED, true);
            shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_NEWS_FEED,
                                                QRect(0, childWindowShadowOffsetY(),
                                                      newsFeedWindow_->getGraphicsObject()->boundingRect().width(),
                                                      newsFeedWindow_->getGraphicsObject()->boundingRect().height() - childWindowShadowOffsetY()));

            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
        });

        // delay opacity
        QSequentialAnimationGroup *seqGroup = new QSequentialAnimationGroup(this);
        seqGroup->addPause(EXPAND_NEWS_FEED_RESIZE_DURATION - EXPAND_CHILD_WINDOW_OPACITY_DURATION);
        seqGroup->addAnimation(animOpacity);

        animGroup->addAnimation(animResize);
        animGroup->addAnimation(seqGroup);
        animGroup->start(QVariantAnimation::DeleteWhenStopped);
    };

    isAtomicAnimationActive_ = true;

    connectWindow_->setClickable(false);

    // hide locations or bottomInfo before running news feed expand
    if (isLocationsExpanded())
    {
        connectWindow_->updateLocationsState(false);
        expandLocationsAnimationGroup_->setDirection(QAbstractAnimation::Backward);
        if (expandLocationsAnimationGroup_->state() != QAbstractAnimation::Running)
        {
            expandLocationsAnimationGroup_->start();
        }
    }
    else if (bottomInfoWindow_->getGraphicsObject()->isVisible())
    {
        std::function<void()> finish_function = [this](){
            if (functionOnAnimationFinished_ != NULL)
            {
                functionOnAnimationFinished_();
                functionOnAnimationFinished_ = NULL;
            }
        };

        animateBottomInfoWindow(QAbstractAnimation::Forward, finish_function);
    }
    else
    {
        if (functionOnAnimationFinished_ != NULL)
        {
            functionOnAnimationFinished_();
            functionOnAnimationFinished_ = NULL;
        }
    }
}

void MainWindowController::collapsePreferencesFromLogin()
{
    // qCDebug(LOG_BASIC) << "MainWindowController::collapsePreferencesFromLogin";

    if (preferencesState_ != CHILD_WINDOW_STATE_EXPANDED)
    {
        return;
    }

    isAtomicAnimationActive_ = true;

    preferencesWindow_->setScrollBarVisibility(false);
    TooltipController::instance().hideAllTooltips();

    preferencesState_ = CHILD_WINDOW_STATE_ANIMATING;
    loginWindow_->getGraphicsObject()->show();


    shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_LOGIN_WINDOW,
                                        QRect(0,0, loginWindow_->getGraphicsObject()->boundingRect().width(),
                                              loginWindow_->getGraphicsObject()->boundingRect().height()));

    shadowManager_->setVisible(ShadowManager::SHAPE_ID_LOGIN_WINDOW, true);
    shadowManager_->setOpacity(ShadowManager::SHAPE_ID_LOGIN_WINDOW, 0.0, false);

    // Opacity
    QVariantAnimation *animOpacity = new QVariantAnimation(this);
    animOpacity->setStartValue(1.0);
    animOpacity->setEndValue(0.0);
    animOpacity->setDuration(EXPAND_CHILD_WINDOW_OPACITY_DURATION);
    connect(animOpacity, &QVariantAnimation::valueChanged, [this](const QVariant &value) {
        double prefOpacity = value.toDouble();
        preferencesWindow_->getGraphicsObject()->setOpacity(prefOpacity);
        shadowManager_->setOpacity(ShadowManager::SHAPE_ID_PREFERENCES, prefOpacity, true);
        shadowManager_->setOpacity(ShadowManager::SHAPE_ID_LOGIN_WINDOW, 1.0 - prefOpacity, false);
        loginWindow_->getGraphicsObject()->setOpacity(1.0 - prefOpacity);
        shadowManager_->setOpacity(ShadowManager::SHAPE_ID_CONNECT_WINDOW, prefOpacity, false);
    });

    // Resizing
    QVariantAnimation *animResize = new QVariantAnimation(this);
    animResize->setStartValue((int)preferencesWindow_->getGraphicsObject()->boundingRect().height());
    animResize->setEndValue((int)loginWindow_->getGraphicsObject()->boundingRect().height());
    animResize->setDuration(EXPAND_PREFERENCES_RESIZE_DURATION);
    connect(animResize, &QVariantAnimation::valueChanged, [this](const QVariant &value) {
        preferencesWindow_->setHeight(value.toInt());
        shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_NEWS_FEED,
                                            QRect(0, childWindowShadowOffsetY(), preferencesWindow_->getGraphicsObject()->boundingRect().width(),
                                                  preferencesWindow_->getGraphicsObject()->boundingRect().height() - childWindowShadowOffsetY()));
        updateMainAndViewGeometry(false);
        invalidateShadow_mac();
    });

    // resize finished
    connect(animResize, &QVariantAnimation::finished, [this]() {
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_PREFERENCES, true);
        preferencesWindow_->getGraphicsObject()->hide();
    });

    // group finished
    QParallelAnimationGroup *animGroup = new QParallelAnimationGroup(this);
    connect(animGroup, &QVariantAnimation::finished, [this]() {
        loginWindow_->setClickable(true);
        loginWindow_->getGraphicsObject()->setFocus();
        preferencesState_ = CHILD_WINDOW_STATE_COLLAPSED;
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

void MainWindowController::collapsePreferencesFromConnect(bool bSkipBottomInfoWindowAnimate)
{
    // qCDebug(LOG_BASIC) << "MainWindowController::collapsePreferencesFromConnect";

    if (preferencesState_ != CHILD_WINDOW_STATE_EXPANDED)
    {
        return;
    }
    preferencesWindow_->setScrollBarVisibility(false);
    TooltipController::instance().hideAllTooltips();

    preferencesState_ = CHILD_WINDOW_STATE_ANIMATING;
    connectWindow_->getGraphicsObject()->show();

    if (locationsWindow_->currentTab() == GuiLocations::LocationsTab::LOCATION_TAB_SEARCH_LOCATIONS)
    {
        locationsWindow_->hideSearchTabWithoutAnimation();
    }

    // opacity change
    QVariantAnimation *animOpacity = new QVariantAnimation(this);
    animOpacity->setStartValue(1.0);
    animOpacity->setEndValue(0.0);
    animOpacity->setDuration(EXPAND_CHILD_WINDOW_OPACITY_DURATION);
    connect(animOpacity, &QVariantAnimation::valueChanged, [this](const QVariant &value) {
        double opacity = value.toDouble();
        preferencesWindow_->getGraphicsObject()->setOpacity(opacity);
        shadowManager_->setOpacity(ShadowManager::SHAPE_ID_PREFERENCES, opacity, true);
    });

    int start = (int)preferencesWindow_->getGraphicsObject()->boundingRect().height();
    int target =  (int)connectWindow_->getGraphicsObject()->boundingRect().height();
    QVariantAnimation *animResize = new QVariantAnimation(this);
    animResize->setStartValue(start);
    animResize->setEndValue(target);
    animResize->setDuration(EXPAND_PREFERENCES_RESIZE_DURATION);
    connect(animResize, &QVariantAnimation::valueChanged, [this](const QVariant &value) {
        preferencesWindow_->setHeight(value.toInt());
        updateMainAndViewGeometry(false);
        shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_PREFERENCES,
                                            QRect(0, childWindowShadowOffsetY(),
                                                  preferencesWindow_->getGraphicsObject()->boundingRect().width(),
                                                  preferencesWindow_->getGraphicsObject()->boundingRect().height() - childWindowShadowOffsetY()));
        invalidateShadow_mac();
    });

    // resize finished
    connect(animResize, &QVariantAnimation::finished, [this]() {
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_PREFERENCES, true);
        preferencesWindow_->getGraphicsObject()->hide();
    });

    // group finished
    QParallelAnimationGroup *animGroup = new QParallelAnimationGroup(this);
    connect(animGroup, &QVariantAnimation::finished, [this, bSkipBottomInfoWindowAnimate]() {
        preferencesState_ = CHILD_WINDOW_STATE_COLLAPSED;
        emit preferencesCollapsed();
        TooltipController::instance().hideAllTooltips();
        updateMainAndViewGeometry(false);

        connectWindow_->setClickable(true);
        bottomInfoWindow_->setClickable(true);
        invalidateShadow_mac();

        if (!bSkipBottomInfoWindowAnimate && bottomInfoWindow_->getGraphicsObject()->isVisible())
        {
            std::function<void()> finish_function = [this](){
                isAtomicAnimationActive_ = false;
                handleNextWindowChange();
                updateBottomInfoWindowVisibilityAndPos();
            };
            animateBottomInfoWindow(QAbstractAnimation::Backward, finish_function);
        }
        else
        {
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
        }
    });

    // delay opacity
    QSequentialAnimationGroup *seqGroup = new QSequentialAnimationGroup(this);
    seqGroup->addPause(EXPAND_PREFERENCES_RESIZE_DURATION - EXPAND_CHILD_WINDOW_OPACITY_DURATION);
    seqGroup->addAnimation(animOpacity);

    animGroup->addAnimation(animResize);
    animGroup->addAnimation(seqGroup);
    animGroup->start(QVariantAnimation::DeleteWhenStopped);
}

void MainWindowController::collapseNewsFeed()
{
    // qCDebug(LOG_BASIC) << "MainWindowController::collapseNewsFeedFromConnect";

    if (newsFeedState_ != CHILD_WINDOW_STATE_EXPANDED)
    {
        return;
    }
    newsFeedWindow_->setScrollBarVisibility(false);
    TooltipController::instance().hideAllTooltips();

    newsFeedState_ = CHILD_WINDOW_STATE_ANIMATING;
    connectWindow_->getGraphicsObject()->show();

    if (locationsWindow_->currentTab() == GuiLocations::LocationsTab::LOCATION_TAB_SEARCH_LOCATIONS)
    {
        locationsWindow_->hideSearchTabWithoutAnimation();
    }

    // opacity change
    QVariantAnimation *animOpacity = new QVariantAnimation(this);
    animOpacity->setStartValue(1.0);
    animOpacity->setEndValue(0.0);
    animOpacity->setDuration(EXPAND_CHILD_WINDOW_OPACITY_DURATION);
    connect(animOpacity, &QVariantAnimation::valueChanged, [this](const QVariant &value) {
        double opacity = value.toDouble();
        newsFeedWindow_->getGraphicsObject()->setOpacity(opacity);
        shadowManager_->setOpacity(ShadowManager::SHAPE_ID_NEWS_FEED, opacity, true);
    });

    int start = (int)newsFeedWindow_->getGraphicsObject()->boundingRect().height();
    int target =  (int)connectWindow_->getGraphicsObject()->boundingRect().height();
    QVariantAnimation *animResize = new QVariantAnimation(this);
    animResize->setStartValue(start);
    animResize->setEndValue(target);
    animResize->setDuration(EXPAND_NEWS_FEED_RESIZE_DURATION);
    connect(animResize, &QVariantAnimation::valueChanged, [this](const QVariant &value) {
        newsFeedWindow_->setHeight(value.toInt());
        updateMainAndViewGeometry(false);
        shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_NEWS_FEED,
                                            QRect(0, childWindowShadowOffsetY(),
                                                  newsFeedWindow_->getGraphicsObject()->boundingRect().width(),
                                                  newsFeedWindow_->getGraphicsObject()->boundingRect().height() - childWindowShadowOffsetY()));
        invalidateShadow_mac();
    });

    // resize finished
    connect(animResize, &QVariantAnimation::finished, [this]() {
        shadowManager_->setVisible(ShadowManager::SHAPE_ID_NEWS_FEED, true);
        newsFeedWindow_->getGraphicsObject()->hide();
    });

    // group finished
    QParallelAnimationGroup *animGroup = new QParallelAnimationGroup(this);
    connect(animGroup, &QVariantAnimation::finished, [this]() {
        newsFeedState_ = CHILD_WINDOW_STATE_COLLAPSED;
        TooltipController::instance().hideAllTooltips();
        updateMainAndViewGeometry(false);

        connectWindow_->setClickable(true);
        bottomInfoWindow_->setClickable(true);
        invalidateShadow_mac();

        if (bottomInfoWindow_->getGraphicsObject()->isVisible())
        {
            std::function<void()> finish_function = [this](){
                isAtomicAnimationActive_ = false;
                handleNextWindowChange();
                updateBottomInfoWindowVisibilityAndPos();
            };
            animateBottomInfoWindow(QAbstractAnimation::Backward, finish_function);
        }
        else
        {
            isAtomicAnimationActive_ = false;
            handleNextWindowChange();
        }
    });

    // delay opacity
    QSequentialAnimationGroup *seqGroup = new QSequentialAnimationGroup(this);
    seqGroup->addPause(EXPAND_NEWS_FEED_RESIZE_DURATION - EXPAND_CHILD_WINDOW_OPACITY_DURATION);
    seqGroup->addAnimation(animOpacity);

    animGroup->addAnimation(animResize);
    animGroup->addAnimation(seqGroup);
    animGroup->start(QVariantAnimation::DeleteWhenStopped);
}

void MainWindowController::collapseAllExpandedOnBottom()
{
    if (isLocationsExpanded())
    {
        connectWindow_->updateLocationsState(false);
        updateExpandAnimationParameters();
        expandLocationsAnimationGroup_->removeAnimation(collapseBottomInfoWindowAnimation_);
        expandLocationsAnimationGroup_->setDirection(QAbstractAnimation::Backward);
        if (expandLocationsAnimationGroup_->state() != QAbstractAnimation::Running)
        {
            expandLocationsAnimationGroup_->start();
        }
    }
    else if (bottomInfoWindow_->getGraphicsObject()->isVisible() && !isBottomInfoCollapsed())
    {
        std::function<void()> finished_function = [this]() {
            if (functionOnAnimationFinished_ != NULL)
            {
                functionOnAnimationFinished_();
                functionOnAnimationFinished_ = NULL;
            }
        };

        animateBottomInfoWindow(QAbstractAnimation::Forward, finished_function);
    }
    else
    {
        if (functionOnAnimationFinished_ != NULL)
        {
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
    collapseBottomInfo->setTargetObject(bottomInfoWindow_->getGraphicsObject());
    collapseBottomInfo->setPropertyName("pos");

    collapseBottomInfo->setStartValue(getCoordsOfBottomInfoWindow(false));
    collapseBottomInfo->setEndValue(getCoordsOfBottomInfoWindow(true));
    collapseBottomInfo->setDuration(HIDE_BOTTOM_INFO_ANIMATION_DURATION);

    collapseBottomInfo->setDirection(direction);

    connect(collapseBottomInfo, &QPropertyAnimation::valueChanged, [this](){
        updateMainAndViewGeometry(false);
        invalidateShadow_mac();
    });

    connect(collapseBottomInfo, &QPropertyAnimation::finished, [this]() {
        updateExpandAnimationParameters();
        invalidateShadow_mac();
    });

    if (finished_function != nullptr)
    {
        connect(collapseBottomInfo, &QPropertyAnimation::finished, [=]() {
            finished_function();
        });
    }

    return collapseBottomInfo;
}

void MainWindowController::updateExpandAnimationParameters()
{
    if (expandLocationsAnimationGroup_ != NULL)
    {
        // TODO: sometimes this assert can fail
        WS_ASSERT(expandLocationsAnimationGroup_->state() != QAbstractAnimation::Running);
    }

    SAFE_DELETE(expandLocationsAnimationGroup_);

    expandLocationsListAnimation_ = new QPropertyAnimation(this);
    expandLocationsListAnimation_->setTargetObject(locationsWindow_);
    expandLocationsListAnimation_->setPropertyName("size");
    expandLocationsListAnimation_->setStartValue(QSize(locationsWindow_->width(), 0));
    expandLocationsListAnimation_->setEndValue(QSize(LOCATIONS_WINDOW_WIDTH*G_SCALE, locationsWindow_->tabAndFooterHeight() * G_SCALE));
    expandLocationsListAnimation_->setDuration(EXPAND_ANIMATION_DURATION);
    connect(expandLocationsListAnimation_, SIGNAL(finished()), SLOT(onExpandLocationsListAnimationFinished()));
    connect(expandLocationsListAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onExpandLocationsListAnimationValueChanged(QVariant)));
    connect(expandLocationsListAnimation_, SIGNAL(stateChanged(QAbstractAnimation::State,QAbstractAnimation::State)), SLOT(onExpandLocationsListAnimationStateChanged(QAbstractAnimation::State,QAbstractAnimation::State)));

    collapseBottomInfoWindowAnimation_ = new QPropertyAnimation(this);
    collapseBottomInfoWindowAnimation_->setTargetObject(bottomInfoWindow_->getGraphicsObject());
    collapseBottomInfoWindowAnimation_->setPropertyName("pos");
    connect(collapseBottomInfoWindowAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onCollapseBottomInfoWindowAnimationValueChanged(QVariant)));

    expandLocationsAnimationGroup_ = new QSequentialAnimationGroup(this);
    connect(expandLocationsAnimationGroup_, SIGNAL(finished()), SLOT(onExpandLocationsAnimationGroupFinished()));

    if (bottomInfoWindow_->getGraphicsObject()->isVisible())
    {
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

int MainWindowController::childWindowShadowOffsetY()
{
    return 50 * G_SCALE;
}

#ifdef Q_OS_WIN
MainWindowController::TaskbarLocation MainWindowController::primaryScreenTaskbarLocation_win()
{
    TaskbarLocation taskbarLocation = TASKBAR_HIDDEN;

    // use system tray icon as anchor for primary screen since QGuiApplication::primaryScreen() doesn't update after app startup
    QRect rcIcon = static_cast<MainWindow*>(mainWindow_)->trayIconRect();
    QScreen *screen = QGuiApplication::screenAt(rcIcon.center());
    if (!screen)
    {
        qCDebug(LOG_BASIC) << "Couldn't find screen at system icon location";
        return taskbarLocation;
    }

    QRect desktopAvailableRc = screen->availableGeometry();
    QRect desktopScreenRc = screen->geometry();

    // qDebug() << "Desktop Available: " << desktopAvailableRc;
    // qDebug() << "Desktop Screen: "    << desktopScreenRc;

    if (desktopAvailableRc.x() > desktopScreenRc.x())
    {
        taskbarLocation = TASKBAR_LEFT;
    }
    else if (desktopAvailableRc.y() > desktopScreenRc.y())
    {
        taskbarLocation = TASKBAR_TOP;
    }
    else if (desktopAvailableRc.width() < desktopScreenRc.width())
    {
        taskbarLocation = TASKBAR_RIGHT;
    }
    else if (desktopAvailableRc.height() < desktopScreenRc.height())
    {
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
    if (!screen)
    {
        // qDebug() << "Still no screen found -- not updating geometry and scene";
        return QRect();
    }

    QRect desktopAvailableRc = screen->availableGeometry();
    const int kRightOffset = 16 * G_SCALE;
    const int kVerticalOffset = 16 * G_SCALE;

    QRect geo;
    if (taskbarLocation == TASKBAR_TOP)
    {
        geo = QRect(desktopAvailableRc.right() - width - shadowSize - kRightOffset,
                    desktopAvailableRc.top() - shadowSize + kVerticalOffset,
                    widthWithShadow, heightWithShadow);

        if (geo.right() > desktopAvailableRc.right() + shadowSize)
        {
            geo.moveRight(desktopAvailableRc.right() + shadowSize);
        }
    }
    else if (taskbarLocation == TASKBAR_LEFT)
    {
        geo = QRect(desktopAvailableRc.left() - shadowSize,
                    desktopAvailableRc.bottom() - heightWithShadow + shadowSize,
                    widthWithShadow, heightWithShadow);
    }
    else if (taskbarLocation == TASKBAR_RIGHT)
    {
        geo = QRect(desktopAvailableRc.right() - width - shadowSize + 1*G_SCALE, // not sure why its off by 1 pixel
                    desktopAvailableRc.bottom() - heightWithShadow + shadowSize,
                    widthWithShadow, heightWithShadow);
    }
    else if (taskbarLocation == TASKBAR_BOTTOM)
    {
        geo = QRect(desktopAvailableRc.right() - width - shadowSize - kRightOffset,
                    desktopAvailableRc.bottom() - heightWithShadow + shadowSize - kVerticalOffset,
                    widthWithShadow, heightWithShadow);

        if (geo.right() > desktopAvailableRc.right() + shadowSize)
        {
            geo.moveRight(desktopAvailableRc.right() + shadowSize);
        }
    }
    else // TASKBAR_HIDDEN
    {
//        qDebug() << "Coundn't determine taskbar location -- putting app on primary screen bottom right";
//        qDebug() << "Desktop Available: " << desktopAvailableRc;

        // determine hidden taskbar location from tray icon
        // BOTTOM and RIGHT use bottom-right corner
        int posX = desktopAvailableRc.right() - width - shadowSize - kRightOffset;
        int posY = desktopAvailableRc.bottom() - heightWithShadow + shadowSize - kVerticalOffset;
        if (rcIcon.y() == 0) // TOP
        {
            posY = -shadowSize + kVerticalOffset;
        }
        else if (rcIcon.x() < desktopAvailableRc.width()/2) // LEFT
        {
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
    if (preferencesState_ == CHILD_WINDOW_STATE_EXPANDED &&
        newsFeedState_ == CHILD_WINDOW_STATE_COLLAPSED)
    {
        width = preferencesWindow_->getGraphicsObject()->boundingRect().width();
        height = preferencesWindow_->getGraphicsObject()->boundingRect().height();
    }
    else if (preferencesState_ == CHILD_WINDOW_STATE_COLLAPSED &&
             newsFeedState_ == CHILD_WINDOW_STATE_EXPANDED)
    {
        width = newsFeedWindow_->getGraphicsObject()->boundingRect().width();
        height = newsFeedWindow_->getGraphicsObject()->boundingRect().height();
    }
    else if (curWindow_ == WINDOW_ID_CONNECT || curWindow_ == WINDOW_ID_UPDATE ||
             curWindow_ == WINDOW_ID_UPGRADE || curWindow_ == WINDOW_ID_GENERAL_MESSAGE )
    {
        // here, the geometry depends on the state preferences window, bottomInfoWindow_, locationsWindow_
        if (locationListAnimationState_ == LOCATION_LIST_ANIMATION_COLLAPSED)
        {
            width = connectWindow_->getGraphicsObject()->boundingRect().width();
            height = connectWindow_->getGraphicsObject()->boundingRect().height();

            if (bottomInfoWindow_->getGraphicsObject()->isVisible() && bottomInfoWindow_->getGraphicsObject()->boundingRect().height() > 0)
            {

                int bottomInfoHeight = bottomInfoWindow_->getGraphicsObject()->boundingRect().height() + getCoordsOfBottomInfoWindow(false).y();

                if (bottomInfoHeight > height)
                {
                    height = bottomInfoHeight;
                }
            }

            if (preferencesState_ == CHILD_WINDOW_STATE_ANIMATING)
            {
                height = preferencesWindow_->getGraphicsObject()->boundingRect().height();
            }
            else if (newsFeedState_ == CHILD_WINDOW_STATE_ANIMATING)
            {
                height = newsFeedWindow_->getGraphicsObject()->boundingRect().height();
            }
        }
        else
        {
            width = connectWindow_->getGraphicsObject()->boundingRect().width();
            height = connectWindow_->getGraphicsObject()->boundingRect().height();
            addHeightToGeometry = locationWindowHeightScaled_ - locationsYOffset();
            if (addHeightToGeometry < 0 ) addHeightToGeometry = 0;
        }
    }
    else if (curWindow_ == WINDOW_ID_EXIT)
    {
        if (windowBeforeExit_ == WINDOW_ID_CONNECT)
        {
            width = connectWindow_->getGraphicsObject()->boundingRect().width();
            height = connectWindow_->getGraphicsObject()->boundingRect().height();

            if (locationListAnimationState_ != LOCATION_LIST_ANIMATION_COLLAPSED) // Animating
            {
                addHeightToGeometry = locationWindowHeightScaled_ + locationsYOffset();
                if (addHeightToGeometry < 0 ) addHeightToGeometry = 0;
            }
        }
        else // login exit
        {
            width = loginWindow_->getGraphicsObject()->boundingRect().width();
            height = loginWindow_->getGraphicsObject()->boundingRect().height();
        }
    }
    else if (curWindow_ == WINDOW_ID_INITIALIZATION)
    {
        width = initWindow_->getGraphicsObject()->boundingRect().width();
        height = initWindow_->getGraphicsObject()->boundingRect().height();
    }
    else if (curWindow_ == WINDOW_ID_LOGIN || curWindow_ == WINDOW_ID_LOGGING_IN ||
             curWindow_ == WINDOW_ID_EMERGENCY || curWindow_ == WINDOW_ID_EXTERNAL_CONFIG ||
             curWindow_ == WINDOW_ID_TWO_FACTOR_AUTH)
    {
        width = loginWindow_->getGraphicsObject()->boundingRect().width();
        height = loginWindow_->getGraphicsObject()->boundingRect().height();

        if (preferencesState_ == CHILD_WINDOW_STATE_ANIMATING)
        {
            height = preferencesWindow_->getGraphicsObject()->boundingRect().height();
        }
    }
    else
    {
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
    if (window)
    {
        int screenWidth = window->screen()->geometry().width();
        int screenHeight = window->screen()->geometry().height();
        mainWindow_->setGeometry((screenWidth - widthWithShadow) / 2, (screenHeight - heightWithShadow) / 2, widthWithShadow, heightWithShadow);
    }
    else // should only happen on startup if there are no persistentGuiSettings in registry
    {
        QScreen *primaryScreen = qApp->primaryScreen();
        if (primaryScreen)
        {
            qCDebug(LOG_BASIC) << "centerMainGeometryAndUpdateView() - screen" << primaryScreen->name() << "geometry" << primaryScreen->geometry();
            int screenWidth = primaryScreen->geometry().width();
            int screenHeight = primaryScreen->geometry().height();
            mainWindow_->setGeometry((screenWidth - widthWithShadow) / 2, (screenHeight - heightWithShadow) / 2, widthWithShadow, heightWithShadow);
        }
        else
        {
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

    if (preferencesHelper_->isDockedToTray())
    {
        const QRect rcIcon = static_cast<MainWindow*>(mainWindow_)->trayIconRect();
        const QPoint iconCenter(qMax(0, rcIcon.center().x()), qMax(0, rcIcon.center().y()));

        // On Windows and Mac: screen is sometimes not found because QSystemTrayIcon is invalid or not
        // contained in screen list during monitor change, screen resolution change, or opening/closing
        // laptop lid.
        // Safer screen check prevents a crash when the trayIcon is invalid
        QScreen *screen = WidgetUtils::slightlySaferScreenAt(iconCenter);
        if (!screen)
        {
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
        if (geo.right() > kMaxGeometryRightPosition)
        {
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
    if (view_->geometry().width() != width || view_->geometry().height() != height)
    {
        view_->setGeometry(shadowSize, shadowSize, width, height);
    }

    // update scene geometry
    int sceneRectWidth = scene_->sceneRect().width();
    int sceneRectHeight = scene_->sceneRect().height();
    if (sceneRectWidth != width || sceneRectHeight != height)
    {
        scene_->setSceneRect(QRectF(0, 0, width, height));
    }

    updateMaskForGraphicsView();

    if (updateShadow)
    {
        shadowManager_->updateShadow();
        invalidateShadow_mac();
    }

    // qDebug() << "MainWindow Geo (after view/scene update): " << mainWindow_->geometry();

}

void MainWindowController::handleNextWindowChange()
{
    if (!queueWindowChanges_.isEmpty())
    {
        changeWindow(queueWindowChanges_.dequeue());
    }
    else
    {
        invalidateShadow_mac();
    }
}

void MainWindowController::updateBottomInfoWindowVisibilityAndPos(bool forceCollapsed)
{
    if (expandLocationsAnimationGroup_ != NULL && expandLocationsAnimationGroup_->state() == QAbstractAnimation::Running)
    {
        // TODO: can fire when quickly opening locations list on app startup
        WS_ASSERT(false);
    }
    else
    {
        if ((!bottomInfoWindow_->isUpgradeWidgetVisible() && !bottomInfoWindow_->isSharingFeatureVisible())
                || curWindow_ != WINDOW_ID_CONNECT
                || isLocationsExpanded())
        {
            bottomInfoWindow_->getGraphicsObject()->hide();
            bottomInfoWindow_->setClickable(false);
            shadowManager_->removeObject(ShadowManager::SHAPE_ID_BOTTOM_INFO);
            if (expandLocationsAnimationGroup_->state() != QAbstractAnimation::Stopped || locationListAnimationState_ != LOCATION_LIST_ANIMATION_COLLAPSED)
            {
                connectWindow_->setCornerColor(QColor(14, 25, 38));
            }
            else
            {
                connectWindow_->setCornerColor(Qt::transparent);
            }
        }
        else
        {
            bool bottomCollapsed = forceCollapsed || (locationListAnimationState_ != LOCATION_LIST_ANIMATION_COLLAPSED);
            if (preferencesState_ != CHILD_WINDOW_STATE_COLLAPSED || newsFeedState_ != CHILD_WINDOW_STATE_COLLAPSED)
            {
                bottomCollapsed = true;
            }

            QPoint posBottomInfoWindow = getCoordsOfBottomInfoWindow(bottomCollapsed);
            bottomInfoWindow_->getGraphicsObject()->setPos(posBottomInfoWindow);
            bottomInfoWindow_->setClickable(true);
            shadowManager_->removeObject(ShadowManager::SHAPE_ID_BOTTOM_INFO);

            QPixmap shadow = bottomInfoWindow_->getCurrentPixmapShape();
            shadowManager_->addPixmap(shadow,
                                      posBottomInfoWindow.x(),
                                      posBottomInfoWindow.y(),
                                      ShadowManager::SHAPE_ID_BOTTOM_INFO, true);
            bottomInfoWindow_->getGraphicsObject()->show();

            bottomInfoWindow_->getGraphicsObject()->stackBefore(connectWindow_->getGraphicsObject());
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
    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH && bottomInfoWindow_->isUpgradeWidgetVisible() && !bottomInfoWindow_->isSharingFeatureVisible())
    {
        pt = QPoint(0, TOP_OFFS_WHEN_SHARING_FEATURES_HIDDEN_VAN_GOGH);
    }
    else if (preferences_->appSkin() == APP_SKIN_VAN_GOGH)
    {
        pt = QPoint(0, TOP_OFFS_WHEN_SHARING_FEATURES_VISIBLE_VAN_GOGH);
    }
    else if (bottomInfoWindow_->isUpgradeWidgetVisible() && !bottomInfoWindow_->isSharingFeatureVisible())
    {
        pt = QPoint(LEFT_OFFS_WHEN_SHARING_FEATURES_HIDDEN, TOP_OFFS_WHEN_SHARING_FEATURES_HIDDEN);
    }
    else
    {
        pt = QPoint(LEFT_OFFS_WHEN_SHARING_FEATURES_VISIBLE, TOP_OFFS_WHEN_SHARING_FEATURES_VISIBLE);
    }

    if (isBottomInfoWindowCollapsed)
    {
        int bottomInfoWindowEdge = pt.y() + bottomInfoWindow_->getGraphicsObject()->boundingRect().height();
        int bottomConnectEdge = preferences_->appSkin() == APP_SKIN_VAN_GOGH ? (264 + yOffset)*G_SCALE : 290*G_SCALE;
        int moveHeight = bottomInfoWindowEdge - bottomConnectEdge;
        pt.setY(pt.y() - moveHeight);
    }

    return pt;
}

bool MainWindowController::isBottomInfoCollapsed() const
{
    return bottomInfoWindow_->getGraphicsObject()->pos().toPoint() == getCoordsOfBottomInfoWindow(true);
}

void MainWindowController::updateCursorInViewport()
{
    QMouseEvent event(QEvent::MouseMove, QCursor::pos(), Qt::NoButton, Qt::MouseButtons(), Qt::KeyboardModifiers());
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
    if (!static_cast<MainWindow*>(mainWindow_)->isActiveState())
        isNeedUpdateNativeShadow_ = true;
#endif
}

void MainWindowController::setMaskForGraphicsView()
{
    // Mask allows clicking locations tab buttons when expanded
    view_->setMask(connectWindow_->getMask());
}

void MainWindowController::clearMaskForGraphicsView()
{
    view_->clearMask();
}

void MainWindowController::keepWindowInsideScreenCoordinates()
{
    QRect rcWindow = mainWindow_->geometry();
    QRect rcScreen = mainWindow_->screen()->availableGeometry();

    if (rcWindow.bottom() > (rcScreen.bottom()))
    {
       // qDebug() << "KEEPING MAINWINDOW INSIDE SCREEN COORDINATES";
       rcWindow.moveBottom(rcScreen.bottom());
       mainWindow_->setGeometry(rcWindow);
    }
}

void MainWindowController::onAppSkinChanged(APP_SKIN s)
{
    // if turning Van Gogh mode on, and the update banner is visible, we shift everything down to accommodate the banner
    vanGoghUpdateWidgetAnimationProgress_ = (s == APP_SKIN_VAN_GOGH && updateAppItem_->getGraphicsObject()->isVisible()) ? 1.0 : 0.0;
    onVanGoghAnimationProgressChanged(vanGoghUpdateWidgetAnimationProgress_);

    // we also take into account the offset to remove it from the height
    int yOffset = 0;
    if (s == APP_SKIN_VAN_GOGH && updateAppItem_->getGraphicsObject()->isVisible())
    {
        yOffset = UPDATE_WIDGET_HEIGHT;
    }

    // determine news feed window height
    if (newsFeedWindowHeight_ - yOffset < newsFeedWindow_->recommendedHeight())
    {
        newsFeedWindowHeight_ = newsFeedWindow_->recommendedHeight();
    }
    else
    {
        newsFeedWindowHeight_ -= yOffset;
    }
    newsFeedWindow_->setHeight(newsFeedWindowHeight_);

    // determine preferences window height
    int preferencesWindowHeightOld = preferencesWindowHeight_;
    if (preferencesWindowHeight_ - yOffset < preferencesWindow_->recommendedHeight())
    {
        preferencesWindowHeight_ = preferencesWindow_->recommendedHeight();
    }
    else
    {
        preferencesWindowHeight_ -= yOffset;
    }

    preferencesWindow_->setHeight(preferencesWindowHeight_);
    // if preferences is open, update shadows and scroll etc
    if (preferencesState_ == CHILD_WINDOW_STATE_EXPANDED)
    {
        updateMainAndViewGeometry(false);
        shadowManager_->changeRectangleSize(ShadowManager::SHAPE_ID_PREFERENCES,
                                            QRect(0, childWindowShadowOffsetY(), preferencesWindow_->getGraphicsObject()->boundingRect().width(),
                                                  preferencesWindow_->getGraphicsObject()->boundingRect().height() - childWindowShadowOffsetY()));
        invalidateShadow_mac();
        keepWindowInsideScreenCoordinates();

        int scrollOffset = 2*UPDATE_WIDGET_HEIGHT;
        if (s != APP_SKIN_VAN_GOGH && !updateAppItem_->getGraphicsObject()->isVisible())
        {
            scrollOffset = 0;
        }
        preferencesWindow_->setScrollOffset(scrollOffset);
    }

    // update window heights
    if (s == APP_SKIN_VAN_GOGH)
    {
        updateWindow_->setHeight(connectWindow_->getGraphicsObject()->boundingRect().height()/G_SCALE);
        upgradeAccountWindow_->setHeight(connectWindow_->getGraphicsObject()->boundingRect().height()/G_SCALE);
        generalMessageWindow_->setHeight(connectWindow_->getGraphicsObject()->boundingRect().height()/G_SCALE);
    }
    else
    {
        updateWindow_->setHeight(WINDOW_HEIGHT);
        upgradeAccountWindow_->setHeight(WINDOW_HEIGHT);
        generalMessageWindow_->setHeight(WINDOW_HEIGHT);
    }

    if (curWindow_ != WINDOW_ID_UNITIALIZED)
    {
        updateMainAndViewGeometry(true);
        updateLocationsWindowAndTabGeometryStatic();
        updateBottomInfoWindowVisibilityAndPos();
    }
}

void MainWindowController::onVanGoghAnimationProgressChanged(QVariant value)
{
    vanGoghUpdateWidgetAnimationProgress_ = value.toDouble();
    int yOffset = UPDATE_WIDGET_HEIGHT * vanGoghUpdateWidgetAnimationProgress_;

    connectWindow_->getGraphicsObject()->setPos(0, yOffset*G_SCALE);
    newsFeedWindow_->getGraphicsObject()->setPos(0, yOffset*G_SCALE);
    preferencesWindow_->getGraphicsObject()->setPos(0, yOffset*G_SCALE);
    exitWindow_->getGraphicsObject()->setPos(0, yOffset*G_SCALE);
    updateWindow_->getGraphicsObject()->setPos(0, yOffset*G_SCALE);
    upgradeAccountWindow_->getGraphicsObject()->setPos(0, yOffset*G_SCALE);
    generalMessageWindow_->getGraphicsObject()->setPos(0, yOffset*G_SCALE);

    // update shadows etc
    if (curWindow_ != WINDOW_ID_UNITIALIZED)
    {
        updateMainAndViewGeometry(true);
        updateLocationsWindowAndTabGeometryStatic();
        updateBottomInfoWindowVisibilityAndPos();
    }
}

int MainWindowController::locationsYOffset()
{
    if (preferences_->appSkin() == APP_SKIN_VAN_GOGH)
    {
        if (updateAppItem_->getGraphicsObject()->isVisible())
        {
            return UPDATE_WIDGET_HEIGHT * vanGoghUpdateWidgetAnimationProgress_;
        }
        return 0;
    }
    return -LOCATIONS_WINDOW_TOP_OFFS*G_SCALE;
}
