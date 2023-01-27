#include "mainwindow.h"

#include <QMouseEvent>
#include <QTimer>
#include <QApplication>
#include <QMessageBox>
#include <QDesktopServices>
#include <QThread>
#include <QFileDialog>
#include <QWindow>
#include <QScreen>
#include <QWidgetAction>
#include <QCommandLineParser>
#include <QScreen>

#include "application/windscribeapplication.h"
#include "commongraphics/commongraphics.h"
#include "backend/persistentstate.h"

#include "utils/hardcodedsettings.h"
#include "utils/utils.h"
#include "utils/logger.h"
#include "utils/writeaccessrightschecker.h"
#include "utils/mergelog.h"
#include "languagecontroller.h"
#include "multipleaccountdetection/multipleaccountdetectionfactory.h"
#include "dialogs/dialoggetusernamepassword.h"
#include "dialogs/dialogmessagecpuusage.h"

#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/imageresourcesjpg.h"
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"
#include "launchonstartup/launchonstartup.h"
#include "showingdialogstate.h"
#include "mainwindowstate.h"
#include "utils/interfaceutils.h"
#include "utils/iauthchecker.h"
#include "utils/authcheckerfactory.h"
#include "systemtray/locationstraymenuscalemanager.h"

#if defined(Q_OS_WIN)
    #include "utils/winutils.h"
    #include "utils/widgetutils_win.h"
    #include <windows.h>
#elif defined(Q_OS_LINUX)
    #include "utils/authchecker_linux.h"
#else
    #include "utils/macutils.h"
    #include "utils/widgetutils_mac.h"
    #include "utils/authchecker_mac.h"
#endif
#include "utils/widgetutils.h"

QWidget *g_mainWindow = NULL;

MainWindow::MainWindow() :
    QWidget(nullptr),
    backend_(NULL),
    logViewerWindow_(nullptr),
    advParametersWindow_(nullptr),
#ifndef Q_OS_LINUX
    locationsMenu_(),
#if !defined(USE_LOCATIONS_TRAY_MENU_NATIVE)
    listWidgetAction_(),
    locationsTrayMenuWidget_(),
#endif
#endif
    currentAppIconType_(AppIconType::DISCONNECTED),
    trayIcon_(),
    bNotificationConnectedShowed_(false),
    bytesTransferred_(0),
    bMousePressed_(false),
    bMoveEnabled_(true),
    signOutReason_(SIGN_OUT_UNDEFINED),
    isPrevSessionStatusInitialized_(false),
    bDisconnectFromTrafficExceed_(false),
    isInitializationAborted_(false),
    isLoginOkAndConnectWindowVisible_(false),
    revealingConnectWindow_(false),
    internetConnected_(false),
    currentlyShowingUserWarningMessage_(false),
    bGotoUpdateWindowAfterGeneralMessage_(false),
    backendAppActiveState_(true),
#ifdef Q_OS_MAC
    hideShowDockIconTimer_(this),
    currentDockIconVisibility_(true),
    desiredDockIconVisibility_(true),
    lastScreenName_(""),
#endif
    activeState_(true),
    lastWindowStateChange_(0),
    isExitingFromPreferences_(false),
    isSpontaneousCloseEvent_(false),
    isExitingAfterUpdate_(false),
    downloadRunning_(false),
    ignoreUpdateUntilNextRun_(false)
{
    g_mainWindow = this;

    // Initialize "fallback" tray icon geometry.
    const QScreen *screen = qApp->screens().at(0);
    if (!screen) qDebug() << "No screen for fallback tray icon init"; // This shouldn't ever happen

    const QRect desktopAvailableRc = screen->availableGeometry();
    savedTrayIconRect_.setTopLeft(QPoint(desktopAvailableRc.right() - WINDOW_WIDTH * G_SCALE, 0));
    savedTrayIconRect_.setSize(QSize(22, 22));

    isRunningInDarkMode_ = InterfaceUtils::isDarkMode();
    qCDebug(LOG_BASIC) << "OS in dark mode: " << isRunningInDarkMode_;

    // Init and show tray icon.
    trayIcon_.setIcon(*IconManager::instance().getDisconnectedTrayIcon(isRunningInDarkMode_));
    trayIcon_.show();
#ifdef Q_OS_MAC
    const QRect desktopScreenRc = screen->geometry();
    if (desktopScreenRc.top() != desktopAvailableRc.top()) {
        while (trayIcon_.geometry().isEmpty())
            qApp->processEvents();
    }
#elif defined Q_OS_LINUX
    //todo Linux
#endif

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinimizeButtonHint);
#if defined(Q_OS_WIN)
    // Fix resize problem on DPI change by assigning a fixed size flag, because the main window is
    // already fixed size by design.
    setWindowFlag(Qt::MSWindowsFixedSizeDialogHint, true);
#endif
    setAttribute(Qt::WA_TranslucentBackground, true);

    multipleAccountDetection_ = QSharedPointer<IMultipleAccountDetection>(MultipleAccountDetectionFactory::create());
    freeTrafficNotificationController_ = new FreeTrafficNotificationController(this);
    connect(freeTrafficNotificationController_, SIGNAL(freeTrafficNotification(QString)), SLOT(onFreeTrafficNotification(QString)));

    unsigned long guiPid = Utils::getCurrentPid();
    qCDebug(LOG_BASIC) << "GUI pid: " << guiPid;
    backend_ = new Backend(this);

    connect(backend_, SIGNAL(initFinished(ProtoTypes::InitState)), SLOT(onBackendInitFinished(ProtoTypes::InitState)));
    connect(backend_, SIGNAL(initTooLong()), SLOT(onBackendInitTooLong()));

    connect(backend_, SIGNAL(loginFinished(bool)), SLOT(onBackendLoginFinished(bool)));
    connect(backend_, SIGNAL(loginStepMessage(ProtoTypes::LoginMessage)), SLOT(onBackendLoginStepMessage(ProtoTypes::LoginMessage)));
    connect(backend_, &Backend::loginError, this, &MainWindow::onBackendLoginError);

    connect(backend_, SIGNAL(signOutFinished()), SLOT(onBackendSignOutFinished()));

    connect(backend_, SIGNAL(sessionStatusChanged(ProtoTypes::SessionStatus)), SLOT(onBackendSessionStatusChanged(ProtoTypes::SessionStatus)));
    connect(backend_, SIGNAL(checkUpdateChanged(ProtoTypes::CheckUpdateInfo)), SLOT(onBackendCheckUpdateChanged(ProtoTypes::CheckUpdateInfo)));
    connect(backend_, SIGNAL(myIpChanged(QString, bool)), SLOT(onBackendMyIpChanged(QString, bool)));
    connect(backend_, SIGNAL(connectStateChanged(ProtoTypes::ConnectState)), SLOT(onBackendConnectStateChanged(ProtoTypes::ConnectState)));
    connect(backend_, SIGNAL(emergencyConnectStateChanged(ProtoTypes::ConnectState)), SLOT(onBackendEmergencyConnectStateChanged(ProtoTypes::ConnectState)));
    connect(backend_, SIGNAL(firewallStateChanged(bool)), SLOT(onBackendFirewallStateChanged(bool)));
    connect(backend_, SIGNAL(confirmEmailResult(bool)), SLOT(onBackendConfirmEmailResult(bool)));
    connect(backend_, SIGNAL(debugLogResult(bool)), SLOT(onBackendDebugLogResult(bool)));
    connect(backend_, SIGNAL(networkChanged(ProtoTypes::NetworkInterface)), SLOT(onNetworkChanged(ProtoTypes::NetworkInterface)));
    connect(backend_, SIGNAL(splitTunnelingStateChanged(bool)), SLOT(onSplitTunnelingStateChanged(bool)));
    connect(backend_, SIGNAL(statisticsUpdated(quint64, quint64, bool)), SLOT(onBackendStatisticsUpdated(quint64, quint64, bool)));
    connect(backend_, SIGNAL(requestCustomOvpnConfigCredentials()), SLOT(onBackendRequestCustomOvpnConfigCredentials()));
    notificationsController_.connect(backend_,SIGNAL(notificationsChanged(ProtoTypes::ArrayApiNotification)), SLOT(updateNotifications(ProtoTypes::ArrayApiNotification)));

#ifdef Q_OS_MAC
    WidgetUtils_mac::allowMinimizeForFramelessWindow(this);
#endif

    connect(backend_, SIGNAL(proxySharingInfoChanged(ProtoTypes::ProxySharingInfo)), SLOT(onBackendProxySharingInfoChanged(ProtoTypes::ProxySharingInfo)));
    connect(backend_, SIGNAL(wifiSharingInfoChanged(ProtoTypes::WifiSharingInfo)), SLOT(onBackendWifiSharingInfoChanged(ProtoTypes::WifiSharingInfo)));
    connect(backend_, SIGNAL(cleanupFinished()), SLOT(onBackendCleanupFinished()));
    connect(backend_, SIGNAL(gotoCustomOvpnConfigModeFinished()), SLOT(onBackendGotoCustomOvpnConfigModeFinished()));
    connect(backend_, SIGNAL(sessionDeleted()), SLOT(onBackendSessionDeleted()));
    connect(backend_, SIGNAL(testTunnelResult(bool)), SLOT(onBackendTestTunnelResult(bool)));
    connect(backend_, SIGNAL(lostConnectionToHelper()), SLOT(onBackendLostConnectionToHelper()));
    connect(backend_, SIGNAL(highCpuUsage(QStringList)), SLOT(onBackendHighCpuUsage(QStringList)));
    connect(backend_, SIGNAL(userWarning(ProtoTypes::UserWarningType)), SLOT(onBackendUserWarning(ProtoTypes::UserWarningType)));
    connect(backend_, SIGNAL(internetConnectivityChanged(bool)), SLOT(onBackendInternetConnectivityChanged(bool)));
    connect(backend_, SIGNAL(protocolPortChanged(ProtoTypes::Protocol, uint)), SLOT(onBackendProtocolPortChanged(ProtoTypes::Protocol, uint)));
    connect(backend_, SIGNAL(packetSizeDetectionStateChanged(bool, bool)), SLOT(onBackendPacketSizeDetectionStateChanged(bool, bool)));
    connect(backend_, SIGNAL(updateVersionChanged(uint, ProtoTypes::UpdateVersionState, ProtoTypes::UpdateVersionError)),
            SLOT(onBackendUpdateVersionChanged(uint, ProtoTypes::UpdateVersionState, ProtoTypes::UpdateVersionError)));
    connect(backend_, SIGNAL(webSessionTokenForEditAccountDetails(QString)), SLOT(onBackendWebSessionTokenForEditAccountDetails(QString)));
    connect(backend_, SIGNAL(webSessionTokenForAddEmail(QString)), SLOT(onBackendWebSessionTokenForAddEmail(QString)));
    connect(backend_, SIGNAL(engineCrash()), SLOT(onBackendEngineCrash()));
    connect(backend_, SIGNAL(locationsUpdated()), SLOT(onBackendLocationsUpdated()));
    connect(backend_, &Backend::wireGuardAtKeyLimit, this, &MainWindow::onWireGuardAtKeyLimit);
    connect(this, &MainWindow::wireGuardKeyLimitUserResponse, backend_, &Backend::wireGuardKeyLimitUserResponse);

    locationsWindow_ = new LocationsWindow(this, backend_->getLocationsModel());
    connect(locationsWindow_, SIGNAL(selected(LocationID)), SLOT(onLocationSelected(LocationID)));
    connect(locationsWindow_, SIGNAL(clickedOnPremiumStarCity()), SLOT(onClickedOnPremiumStarCity()));
    connect(locationsWindow_, SIGNAL(switchFavorite(LocationID,bool)), SLOT(onLocationSwitchFavorite(LocationID,bool)));
    connect(locationsWindow_, SIGNAL(addStaticIpClicked()), SLOT(onLocationsAddStaticIpClicked()));
    connect(locationsWindow_, SIGNAL(clearCustomConfigClicked()), SLOT(onLocationsClearCustomConfigClicked()));
    connect(locationsWindow_, SIGNAL(addCustomConfigClicked()), SLOT(onLocationsAddCustomConfigClicked()));
    locationsWindow_->setLatencyDisplay(backend_->getPreferences()->latencyDisplay());
    locationsWindow_->connect(backend_->getPreferences(), SIGNAL(latencyDisplayChanged(ProtoTypes::LatencyDisplayType)), SLOT(setLatencyDisplay(ProtoTypes::LatencyDisplayType)) );
    locationsWindow_->setShowLocationLoad(backend_->getPreferences()->isShowLocationLoad());
    connect(backend_->getPreferences(), &Preferences::showLocationLoadChanged, locationsWindow_, &LocationsWindow::setShowLocationLoad);

    localIpcServer_ = new LocalIPCServer(backend_, this);
    connect(localIpcServer_, &LocalIPCServer::showLocations, this, &MainWindow::onReceivedOpenLocationsMessage);
    connect(localIpcServer_, &LocalIPCServer::connectToLocation, this, &MainWindow::onConnectToLocation);

    mainWindowController_ = new MainWindowController(this, locationsWindow_, backend_->getPreferencesHelper(), backend_->getPreferences(), backend_->getAccountInfo());

    mainWindowController_->getConnectWindow()->updateMyIp(PersistentState::instance().lastExternalIp());
    mainWindowController_->getConnectWindow()->updateNotificationsState(notificationsController_.totalMessages(), notificationsController_.unreadMessages());
    dynamic_cast<QObject*>(mainWindowController_->getConnectWindow())->connect(&notificationsController_, SIGNAL(stateChanged(int, int)), SLOT(updateNotificationsState(int, int)));
    dynamic_cast<QObject*>(mainWindowController_->getConnectWindow())->connect(
        backend_->getLocationsModel(), SIGNAL(locationSpeedChanged(LocationID, PingTime)),
        SLOT(updateLocationSpeed(LocationID, PingTime)));
    connect(&notificationsController_, SIGNAL(newPopupMessage(int)), SLOT(onNotificationControllerNewPopupMessage(int)));

    connect(backend_->getLocationsModel(), SIGNAL(bestLocationChanged(LocationID)), SLOT(onBestLocationChanged(LocationID)));

    connect(dynamic_cast<QObject*>(mainWindowController_->getNewsFeedWindow()), SIGNAL(escClick()), SLOT(onEscapeNotificationsClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getNewsFeedWindow()), SIGNAL(messageReaded(qint64)),
            &notificationsController_, SLOT(setNotificationReaded(qint64)));
    mainWindowController_->getNewsFeedWindow()->setMessages(
        notificationsController_.messages(), notificationsController_.shownIds());

    // init window signals
    connect(dynamic_cast<QObject*>(mainWindowController_->getInitWindow()), SIGNAL(abortClicked()), SLOT(onAbortInitialization()));

    // login window signals
    connect(dynamic_cast<QObject*>(mainWindowController_->getLoginWindow()), SIGNAL(loginClick(QString,QString,QString)), SLOT(onLoginClick(QString,QString,QString)));
    connect(dynamic_cast<QObject*>(mainWindowController_->getLoginWindow()), SIGNAL(minimizeClick()), SLOT(onMinimizeClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getLoginWindow()), SIGNAL(closeClick()), SLOT(onCloseClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getLoginWindow()), SIGNAL(preferencesClick()), SLOT(onLoginPreferencesClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getLoginWindow()), SIGNAL(haveAccountYesClick()), SLOT(onLoginHaveAccountYesClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getLoginWindow()), SIGNAL(backToWelcomeClick()), SLOT(onLoginBackToWelcomeClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getLoginWindow()), SIGNAL(emergencyConnectClick()), SLOT(onLoginEmergencyWindowClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getLoginWindow()), SIGNAL(externalConfigModeClick()), SLOT(onLoginExternalConfigWindowClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getLoginWindow()), SIGNAL(twoFactorAuthClick(QString, QString)), SLOT(onLoginTwoFactorAuthWindowClick(QString, QString)));
    connect(dynamic_cast<QObject*>(mainWindowController_->getLoginWindow()), SIGNAL(firewallTurnOffClick()), SLOT(onLoginFirewallTurnOffClick()));

    // connect window signals
    connect(dynamic_cast<QObject*>(mainWindowController_->getConnectWindow()), SIGNAL(minimizeClick()), SLOT(onMinimizeClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getConnectWindow()), SIGNAL(closeClick()), SLOT(onCloseClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getConnectWindow()), SIGNAL(connectClick()), SLOT(onConnectWindowConnectClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getConnectWindow()), SIGNAL(firewallClick()), SLOT(onConnectWindowFirewallClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getConnectWindow()), SIGNAL(networkButtonClick()), this, SLOT(onConnectWindowNetworkButtonClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getConnectWindow()), SIGNAL(locationsClick()), SLOT(onConnectWindowLocationsClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getConnectWindow()), SIGNAL(preferencesClick()), SLOT(onConnectWindowPreferencesClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getConnectWindow()), SIGNAL(notificationsClick()), SLOT(onConnectWindowNotificationsClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getConnectWindow()), SIGNAL(splitTunnelingButtonClick()), SLOT(onConnectWindowSplitTunnelingClick()));

    dynamic_cast<QObject*>(mainWindowController_->getConnectWindow())->connect(dynamic_cast<QObject*>(backend_), SIGNAL(firewallStateChanged(bool)), SLOT(updateFirewallState(bool)));

    // news feed window signals
    connect(dynamic_cast<QObject*>(mainWindowController_->getNewsFeedWindow()), SIGNAL(escClick()), SLOT(onEscapeNotificationsClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getNewsFeedWindow()), SIGNAL(closeClick()), SLOT(onCloseClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getNewsFeedWindow()), SIGNAL(minimizeClick()), SLOT(onMinimizeClick()));

    // preferences window signals
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(quitAppClick()), SLOT(onPreferencesQuitAppClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(escape()), SLOT(onPreferencesEscapeClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(helpClick()), SLOT(onPreferencesHelpClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(signOutClick()), SLOT(onPreferencesSignOutClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(loginClick()), SLOT(onPreferencesLoginClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(viewLogClick()), SLOT(onPreferencesViewLogClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(advancedParametersClicked()), SLOT(onPreferencesAdvancedParametersClicked()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(currentNetworkUpdated(ProtoTypes::NetworkInterface)), SLOT(onCurrentNetworkUpdated(ProtoTypes::NetworkInterface)));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(sendConfirmEmailClick()), SLOT(onPreferencesSendConfirmEmailClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(sendDebugLogClick()), SLOT(onPreferencesSendDebugLogClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(editAccountDetailsClick()), SLOT(onPreferencesEditAccountDetailsClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(addEmailButtonClick()), SLOT(onPreferencesAddEmailButtonClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(noAccountLoginClick()), SLOT(onPreferencesNoAccountLoginClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(cycleMacAddressClick()), SLOT(onPreferencesCycleMacAddressClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(detectAppropriatePacketSizeButtonClicked()), SLOT(onPreferencesWindowDetectAppropriatePacketSizeButtonClicked()));
#ifdef Q_OS_WIN
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(setIpv6StateInOS(bool, bool)), SLOT(onPreferencesSetIpv6StateInOS(bool, bool)));
#endif
    // emergency window signals
    connect(dynamic_cast<QObject*>(mainWindowController_->getEmergencyConnectWindow()), SIGNAL(minimizeClick()), SLOT(onMinimizeClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getEmergencyConnectWindow()), SIGNAL(closeClick()), SLOT(onCloseClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getEmergencyConnectWindow()), SIGNAL(escapeClick()), SLOT(onEscapeClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getEmergencyConnectWindow()), SIGNAL(connectClick()), SLOT(onEmergencyConnectClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getEmergencyConnectWindow()), SIGNAL(disconnectClick()), SLOT(onEmergencyDisconnectClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getEmergencyConnectWindow()), SIGNAL(windscribeLinkClick()), SLOT(onEmergencyWindscribeLinkClick()));

    // external config window signals
    connect(dynamic_cast<QObject*>(mainWindowController_->getExternalConfigWindow()), SIGNAL(buttonClick()), SLOT(onExternalConfigWindowNextClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getExternalConfigWindow()), SIGNAL(escapeClick()), SLOT(onEscapeClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getExternalConfigWindow()), SIGNAL(closeClick()), SLOT(onCloseClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getExternalConfigWindow()), SIGNAL(minimizeClick()), SLOT(onMinimizeClick()));

    // 2FA window signals
    connect(dynamic_cast<QObject*>(mainWindowController_->getTwoFactorAuthWindow()), SIGNAL(addClick(QString)), SLOT(onTwoFactorAuthWindowButtonAddClick(QString)));
    connect(dynamic_cast<QObject*>(mainWindowController_->getTwoFactorAuthWindow()), SIGNAL(loginClick(QString, QString, QString)), SLOT(onLoginClick(QString, QString, QString)));
    connect(dynamic_cast<QObject*>(mainWindowController_->getTwoFactorAuthWindow()), SIGNAL(escapeClick()), SLOT(onEscapeClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getTwoFactorAuthWindow()), SIGNAL(closeClick()), SLOT(onCloseClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getTwoFactorAuthWindow()), SIGNAL(minimizeClick()), SLOT(onMinimizeClick()));

    // bottom window signals
    connect(dynamic_cast<QObject*>(mainWindowController_->getBottomInfoWindow()), SIGNAL(upgradeClick()), SLOT(onUpgradeAccountAccept()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getBottomInfoWindow()), SIGNAL(renewClick()), SLOT(onBottomWindowRenewClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getBottomInfoWindow()), SIGNAL(loginClick()), SLOT(onBottomWindowExternalConfigLoginClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getBottomInfoWindow()), SIGNAL(proxyGatewayClick()), this, SLOT(onBottomWindowSharingFeaturesClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getBottomInfoWindow()), SIGNAL(secureHotspotClick()), this, SLOT(onBottomWindowSharingFeaturesClick()));

    // update app item signals
    connect(dynamic_cast<QObject*>(mainWindowController_->getUpdateAppItem()), SIGNAL(updateClick()), SLOT(onUpdateAppItemClick()));

    // update window signals
    connect(dynamic_cast<QObject*>(mainWindowController_->getUpdateWindow()), SIGNAL(acceptClick()), SLOT(onUpdateWindowAccept()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getUpdateWindow()), SIGNAL(cancelClick()), SLOT(onUpdateWindowCancel()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getUpdateWindow()), SIGNAL(laterClick()), SLOT(onUpdateWindowLater()));

    // upgrade window signals
    connect(dynamic_cast<QObject*>(mainWindowController_->getUpgradeWindow()), SIGNAL(acceptClick()), SLOT(onUpgradeAccountAccept()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getUpgradeWindow()), SIGNAL(cancelClick()), SLOT(onUpgradeAccountCancel()));

    // general window signals
    connect(dynamic_cast<QObject*>(mainWindowController_->getGeneralMessageWindow()), SIGNAL(acceptClick()), SLOT(onGeneralMessageWindowAccept()));

    // exit window signals
    connect(dynamic_cast<QObject*>(mainWindowController_->getExitWindow()), SIGNAL(acceptClick()), SLOT(onExitWindowAccept()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getExitWindow()), SIGNAL(rejectClick()), SLOT(onExitWindowReject()));

    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(nativeInfoErrorMessage(QString,QString)), SLOT(onNativeInfoErrorMessage(QString, QString)));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(splitTunnelingAppsAddButtonClick()), SLOT(onSplitTunnelingAppsAddButtonClick()));

    connect(mainWindowController_, SIGNAL(sendServerRatingUp()), SLOT(onMainWindowControllerSendServerRatingUp()));
    connect(mainWindowController_, SIGNAL(sendServerRatingDown()), SLOT(onMainWindowControllerSendServerRatingDown()));
    connect(mainWindowController_, SIGNAL(preferencesCollapsed()), SLOT(onPreferencesCollapsed()));

    // preferences changes signals
    connect(backend_->getPreferences(), SIGNAL(firewallSettingsChanged(ProtoTypes::FirewallSettings)), SLOT(onPreferencesFirewallSettingsChanged(ProtoTypes::FirewallSettings)));
    connect(backend_->getPreferences(), SIGNAL(shareProxyGatewayChanged(ProtoTypes::ShareProxyGateway)), SLOT(onPreferencesShareProxyGatewayChanged(ProtoTypes::ShareProxyGateway)));
    connect(backend_->getPreferences(), SIGNAL(shareSecureHotspotChanged(ProtoTypes::ShareSecureHotspot)), SLOT(onPreferencesShareSecureHotspotChanged(ProtoTypes::ShareSecureHotspot)));
    connect(backend_->getPreferences(), SIGNAL(locationOrderChanged(ProtoTypes::OrderLocationType)), SLOT(onPreferencesLocationOrderChanged(ProtoTypes::OrderLocationType)));
    connect(backend_->getPreferences(), SIGNAL(splitTunnelingChanged(ProtoTypes::SplitTunneling)), SLOT(onPreferencesSplitTunnelingChanged(ProtoTypes::SplitTunneling)));
    connect(backend_->getPreferences(), SIGNAL(updateEngineSettings()), SLOT(onPreferencesUpdateEngineSettings()));
    connect(backend_->getPreferences(), SIGNAL(isLaunchOnStartupChanged(bool)), SLOT(onPreferencesLaunchOnStartupChanged(bool)));
    connect(backend_->getPreferences(), SIGNAL(connectionSettingsChanged(ProtoTypes::ConnectionSettings)), SLOT(onPreferencesConnectionSettingsChanged(ProtoTypes::ConnectionSettings)));
    connect(backend_->getPreferences(), SIGNAL(isDockedToTrayChanged(bool)), SLOT(onPreferencesIsDockedToTrayChanged(bool)));
    connect(backend_->getPreferences(), SIGNAL(updateChannelChanged(ProtoTypes::UpdateChannel)), SLOT(onPreferencesUpdateChannelChanged(ProtoTypes::UpdateChannel)));
    connect(backend_->getPreferences(), SIGNAL(customConfigsPathChanged(QString)), SLOT(onPreferencesCustomConfigsPathChanged(QString)));
    connect(backend_->getPreferences(), SIGNAL(debugAdvancedParametersChanged(QString)), SLOT(onPreferencesdebugAdvancedParametersChanged(QString)));


    connect(backend_->getPreferences(), SIGNAL(reportErrorToUser(QString,QString)), SLOT(onPreferencesReportErrorToUser(QString,QString)));
#ifdef Q_OS_MAC
    connect(backend_->getPreferences(), SIGNAL(hideFromDockChanged(bool)), SLOT(onPreferencesHideFromDockChanged(bool)));
#endif

    // WindscribeApplication signals
    WindscribeApplication * app = WindscribeApplication::instance();
    connect(app, SIGNAL(clickOnDock()), SLOT(toggleVisibilityIfDocked()));
    connect(app, SIGNAL(activateFromAnotherInstance()), SLOT(onAppActivateFromAnotherInstance()));
    connect(app, SIGNAL(shouldTerminate_mac()), SLOT(onAppShouldTerminate_mac()));
    connect(app, SIGNAL(focusWindowChanged(QWindow*)), SLOT(onFocusWindowChanged(QWindow*)));
    connect(app, SIGNAL(applicationCloseRequest()), SLOT(onAppCloseRequest()));
#if defined(Q_OS_WIN)
    connect(app, SIGNAL(winIniChanged()), SLOT(onAppWinIniChanged()));
#endif
    /*connect(&LanguageController::instance(), SIGNAL(languageChanged()), SLOT(onLanguageChanged())); */

    mainWindowController_->getViewport()->installEventFilter(this);
    connect(mainWindowController_, SIGNAL(shadowUpdated()), SLOT(update()));
    connect(mainWindowController_, SIGNAL(revealConnectWindowStateChanged(bool)), this, SLOT(onRevealConnectStateChanged(bool)));

    setupTrayIcon();

    backend_->getLocationsModel()->setOrderLocationsType(backend_->getPreferences()->locationOrder());

    connect(&DpiScaleManager::instance(), SIGNAL(scaleChanged(double)), SLOT(onScaleChanged()));
    connect(&DpiScaleManager::instance(), SIGNAL(newScreen(QScreen*)), SLOT(onDpiScaleManagerNewScreen(QScreen*)));

    backend_->init();

    mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_INITIALIZATION);
    mainWindowController_->getInitWindow()->startWaitingAnimation();

    mainWindowController_->setIsDockedToTray(backend_->getPreferences()->isDockedToTray());
    bMoveEnabled_ = !backend_->getPreferences()->isDockedToTray();

    if (bMoveEnabled_)
        mainWindowController_->setWindowPosFromPersistent();

#if defined(Q_OS_MAC)
    hideShowDockIconTimer_.setSingleShot(true);
    connect(&hideShowDockIconTimer_, &QTimer::timeout, this, [this]() {
        hideShowDockIconImpl(true);
    });
#endif
    deactivationTimer_.setSingleShot(true);
    connect(&deactivationTimer_, SIGNAL(timeout()), SLOT(onWindowDeactivateAndHideImpl()));

    QTimer::singleShot(0, this, SLOT(setWindowToDpiScaleManager()));
}

MainWindow::~MainWindow()
{

}

void MainWindow::showAfterLaunch()
{
    if (!backend_) {
        qCDebug(LOG_BASIC) << "Backend is nullptr!";
    }

    // Report the tray geometry after we've given the app some startup time.
    qCDebug(LOG_BASIC) << "Tray Icon geometry:" << trayIcon_.geometry();

    #ifdef Q_OS_MACOS
    // Do not showMinimized if hide from dock is enabled.  Otherwise, the app will fail to show
    // itself when the user selects 'Show' in the app's system tray menu.
    if (backend_ && backend_->getPreferences()->isHideFromDock()) {
        desiredDockIconVisibility_ = false;
        hideShowDockIconImpl(!backend_->getPreferences()->isStartMinimized());
        return;
    }
    #endif

    if (backend_ && backend_->getPreferences()->isStartMinimized()) {
        showMinimized();
        return;
    }
    #if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    else if (backend_ && backend_->getPreferences()->isMinimizeAndCloseToTray()) {
        QCommandLineParser cmdParser;
        cmdParser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
        QCommandLineOption osRestartOption("os_restart");
        cmdParser.addOption(osRestartOption);
        cmdParser.process(*WindscribeApplication::instance());
        if (cmdParser.isSet(osRestartOption)) {
            showMinimized();
            return;
        }
    }
    #endif
    show();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // qDebug() << "MainWindow eventFilter: " << event->type();

    if (watched == mainWindowController_->getViewport() && event->type() == QEvent::MouseMove)
    {
        mouseMoveEvent((QMouseEvent *)event);
    }
    else if (watched == mainWindowController_->getViewport() && event->type() == QEvent::MouseButtonRelease)
    {
        mouseReleaseEvent((QMouseEvent *)event);
    }
    return QWidget::eventFilter(watched, event);
}

void MainWindow::doClose(QCloseEvent *event, bool isFromSigTerm_mac)
{
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    // Check if the window is closed by pressing a keyboard shortcut (Alt+F4 on Windows, Cmd+Q on
    // macOS). We cannot detect the keypress itself, because the system doesn't deliver it as
    // separate keypress messages, but rather as the close event.
    // But we can assume that such event has a specific set of features:
    // 1) it is spontaneous (sent by the operating system);
    // 2) it is sent to active window only (unlike closing via taskbar/task manager);
    // 3) the modifier key is pressed at the time of the event.
    // If all these features are present, switch to the exit window instead of closing immediately.
    if (event && (isSpontaneousCloseEvent_ || event->spontaneous()) && isActiveWindow()) {
        const auto current_modifiers = QApplication::queryKeyboardModifiers();
#if defined(Q_OS_WIN)
        const auto kCheckedModifier = Qt::AltModifier;
#else
        // On macOS, the ControlModifier value corresponds to the Command keys on the keyboard.
        const auto kCheckedModifier = Qt::ControlModifier;
#endif
        if (current_modifiers & kCheckedModifier) {
            isSpontaneousCloseEvent_ = false;
            event->ignore();
            gotoExitWindow();
            return;
        }
    }
#endif  // Q_OS_WIN || Q_OS_MAC

    setEnabled(false);
    isSpontaneousCloseEvent_ = false;

    // for startup fix (when app disabled in task manager)
    LaunchOnStartup::instance().setLaunchOnStartup(backend_->getPreferences()->isLaunchOnStartup());

    backend_->cleanup(WindscribeApplication::instance()->isExitWithRestart(), PersistentState::instance().isFirewallOn(),
                      backend_->getPreferences()->firewalSettings().mode() == ProtoTypes::FIREWALL_MODE_ALWAYS_ON || isExitingAfterUpdate_,
                      backend_->getPreferences()->isLaunchOnStartup());

    // Backend handles setting firewall state after app closes
    // This block handles initializing the firewall state on next run
    if (PersistentState::instance().isFirewallOn()  &&
        backend_->getPreferences()->firewalSettings().mode() == ProtoTypes::FIREWALL_MODE_AUTOMATIC)
    {
        if (WindscribeApplication::instance()->isExitWithRestart())
        {
            if (!backend_->getPreferences()->isLaunchOnStartup() ||
                !backend_->getPreferences()->isAutoConnect())
            {
                qCDebug(LOG_BASIC) << "Setting firewall persistence to false for restart-triggered shutdown";
                PersistentState::instance().setFirewallState(false);
            }
        }
        else // non-restart close
        {
            if (!backend_->getPreferences()->isAutoConnect())
            {
                qCDebug(LOG_BASIC) << "Setting firewall persistence to false for non-restart auto-mode";
                PersistentState::instance().setFirewallState(false);
            }
        }
    }
    qCDebug(LOG_BASIC) << "Firewall on next startup: " << PersistentState::instance().isFirewallOn();

    PersistentState::instance().setWindowPos(this->pos());

    // Shutdown notification controller here, and not in a destructor. Otherwise, sometimes we won't
    // be able to shutdown properly, because the destructor may not be called. On the Windows
    // platform, when the user logs off, the system terminates the process after Qt closes all top
    // level windows. Hence, there is no guarantee that the application will have time to exit its
    // event loop and execute code at the end of the main() function, including destructors of local
    // objects.
    notificationsController_.shutdown();

    // Save favorites here for the reason above.
    backend_->getLocationsModel()->saveFavorites();

    if (WindscribeApplication::instance()->isExitWithRestart() || isFromSigTerm_mac)
    {
        if (!isFromSigTerm_mac)
        {
            qCDebug(LOG_BASIC) << "close main window with restart OS";
        }
        else
        {
            qCDebug(LOG_BASIC) << "close main window with SIGTERM";
        }
        while (!backend_->isAppCanClose())
        {
            QThread::msleep(1);
            qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
        }
        if (event)
        {
            QWidget::closeEvent(event);
        }
        else
        {
            close();
        }
    }
    else
    {
        qCDebug(LOG_BASIC) << "close main window";
        if (event)
        {
            event->ignore();
            QTimer::singleShot(TIME_BEFORE_SHOW_SHUTDOWN_WINDOW, this, SLOT(showShutdownWindow()));
        }
    }
}

void MainWindow::minimizeToTray()
{
    trayIcon_.show();
    QTimer::singleShot(0, this, SLOT(hide()));
    MainWindowState::instance().setActive(false);
}

bool MainWindow::event(QEvent *event)
{
    // qDebug() << "Event Type: " << event->type();

    if (event->type() == QEvent::WindowStateChange) {

        {
            if (this->windowState() == Qt::WindowMinimized)
            {
                MainWindowState::instance().setActive(false);
            }
        }

        deactivationTimer_.stop();
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
        if (backend_ && backend_->getPreferences()->isMinimizeAndCloseToTray()) {
            QWindowStateChangeEvent *e = static_cast<QWindowStateChangeEvent *>(event);
            // make sure we only do this for minimize events
            if ((e->oldState() != Qt::WindowMinimized) && isMinimized())
            {
                minimizeToTray();
                event->ignore();
            }
        }
#endif
    }

#if defined(Q_OS_MAC)
    if (event->type() == QEvent::PaletteChange)
    {
        isRunningInDarkMode_ = InterfaceUtils::isDarkMode();
        qCDebug(LOG_BASIC) << "PaletteChanged, dark mode: " << isRunningInDarkMode_;
        if (!MacUtils::isOsVersionIsBigSur_or_greater())
            updateTrayIconType(currentAppIconType_);
    }
#endif

    if (event->type() == QEvent::WindowActivate)
    {
         MainWindowState::instance().setActive(true);
        // qDebug() << "WindowActivate";
        if (backend_->getPreferences()->isDockedToTray())
            activateAndShow();
        setBackendAppActiveState(true);
        activeState_ = true;
    }
    else if (event->type() == QEvent::WindowDeactivate)
    {
        // qDebug() << "WindowDeactivate";
        setBackendAppActiveState(false);
        activeState_ = false;
    }

    return QWidget::event(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (backend_->isAppCanClose())
    {
        QWidget::closeEvent(event);
        QApplication::closeAllWindows();
        QApplication::quit();
    }
    else
    {
        doClose(event);
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (bMoveEnabled_)
    {
        if (event->buttons() & Qt::LeftButton && bMousePressed_)
        {
            this->move(event->globalPosition().toPoint() - dragPosition_);
            mainWindowController_->hideAllToolTips();
            event->accept();
        }
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (bMoveEnabled_)
    {
        if (event->button() == Qt::LeftButton)
        {
            dragPosition_ = event->globalPosition().toPoint() - this->frameGeometry().topLeft();

            //event->accept();
            bMousePressed_ = true;
        }
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (bMoveEnabled_)
    {
        if (event->button() == Qt::LeftButton && bMousePressed_)
        {
            bMousePressed_ = false;
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
#ifdef QT_DEBUG
    if (event->modifiers() & Qt::ControlModifier)
    {
        if (event->key() == Qt::Key_L)
        {
            gotoLoginWindow();
        }
        else if (event->key() == Qt::Key_E)
        {
            mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_EMERGENCY);
        }
        else if (event->key() == Qt::Key_Q)
        {
            mainWindowController_->expandPreferences();
        }
        else if (event->key() == Qt::Key_W)
        {
            collapsePreferences();
        }
        else if (event->key() == Qt::Key_A)
        {
            mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_LOGGING_IN);
        }
        else if (event->key() == Qt::Key_C)
        {
            mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_CONNECT);
        }
        else if (event->key() == Qt::Key_I)
        {
            mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_INITIALIZATION);
        }
        else if (event->key() == Qt::Key_Z)
        {
            mainWindowController_->expandLocations();
        }
        else if (event->key() == Qt::Key_X)
        {
            mainWindowController_->collapseLocations();
        }
        else if (event->key() == Qt::Key_N)
        {
            mainWindowController_->getNewsFeedWindow()->setMessages(
                notificationsController_.messages(), notificationsController_.shownIds());
            mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_NOTIFICATIONS);
        }
        else if (event->key() == Qt::Key_V)
        {
            mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_EXTERNAL_CONFIG);
        }
        else if (event->key() == Qt::Key_O)
        {
            mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_UPGRADE);
        }
        else if (event->key() == Qt::Key_B)
        {
            mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_UPDATE);
        }
        else if (event->key() == Qt::Key_M)
        {
            mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_GENERAL_MESSAGE);
        }
        else if (event->key() == Qt::Key_D)
        {
            mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_EXIT);
        }
        else if (event->key() == Qt::Key_F)
        {
            mainWindowController_->changeWindow(MainWindowController::WINDOW_CMD_CLOSE_EXIT);
        }
        else if (event->key() == Qt::Key_U)
        {
            mainWindowController_->showUpdateWidget();
        }
        else if (event->key() == Qt::Key_Y)
        {
            mainWindowController_->hideUpdateWidget();
        }
        else if (event->key() == Qt::Key_G)
        {
        }
        else if (event->key() == Qt::Key_H)
        {
            /*ProtoTypes::ShareSecureHotspot ss = backend_->getShareSecureHotspot();
            ss.set_is_enabled(!ss.is_enabled());
            ss.set_ssid("WifiName");
            backend_->setShareSecureHotspot(ss);
            */
            mainWindowController_->getBottomInfoWindow()->setDaysRemaining(-1);
        }
        else if (event->key() == Qt::Key_J)
        {
            /*ProtoTypes::ShareProxyGateway sp = backend_->getShareProxyGateway();
            sp.set_is_enabled(!sp.is_enabled());
            backend_->setShareProxyGateway(sp);
			*/
            mainWindowController_->getBottomInfoWindow()->setDaysRemaining(3);
        }
        else if (event->key() == Qt::Key_P)
        {
            /*ProtoTypes::ShareSecureHotspot ss = backend_->getShareSecureHotspot();
            ss.set_is_enabled(!ss.is_enabled());
            ss.set_ssid("WifiName");
            backend_->setShareSecureHotspot(ss);

            ProtoTypes::ShareProxyGateway sp = backend_->getShareProxyGateway();
            sp.set_is_enabled(!sp.is_enabled());
            backend_->setShareProxyGateway(sp);
			*/
        }
    }
#endif
    // for feeding chars to searchbar when mainwindow has focus
    // qDebug() << "MainWindow::keyPressEvent";
    if (mainWindowController_->isLocationsExpanded())
    {
        if(event->key() != Qt::Key_Escape || event->key() != Qt::Key_Space)
        {
            mainWindowController_->handleKeyPressEvent(event);
        }
    }

    QWidget::keyPressEvent(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    // qDebug() << "MainWindow::keyReleaseEvent";
    if (mainWindowController_->isLocationsExpanded())
    {
        if(event->key() == Qt::Key_Escape || event->key() == Qt::Key_Space)
        {
            // qCDebug(LOG_USER) << "Collapsing Locations [key]";
            mainWindowController_->collapseLocations();
        }
        else
        {
            // qCDebug(LOG_BASIC) << "Pass keyEvent to locations";
            mainWindowController_->handleKeyReleaseEvent(event);
        }
    }
    else if (mainWindowController_->currentWindow() == MainWindowController::WINDOW_ID_CONNECT
             && !mainWindowController_->preferencesVisible())
    {
        if (event->key() == Qt::Key_Down || event->key() == Qt::Key_Space)
        {
            // qCDebug(LOG_USER) << "Expanding Locations [key]";
            mainWindowController_->expandLocations();
        }
        else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
        {
            onConnectWindowConnectClick();
        }
    }

    QWidget::keyReleaseEvent(event);
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    /*{
          QPainter p(this);
          p.fillRect(QRect(0, 0, width(), height()),Qt::cyan);
    }*/

#ifdef Q_OS_MAC
    mainWindowController_->updateNativeShadowIfNeeded();
#endif

    if (!mainWindowController_->isNeedShadow())
    {
        QWidget::paintEvent(event);
    }
    else
    {
        QPainter p(this);
        QPixmap &shadowPixmap = mainWindowController_->getCurrentShadowPixmap();
        p.drawPixmap(0, 0, shadowPixmap);
    }

    if (revealingConnectWindow_)
    {
        QPainter p(this);
        QPixmap connectWindowBackground = mainWindowController_->getConnectWindow()->getShadowPixmap();
        p.drawPixmap(mainWindowController_->getShadowMargin(),
                     mainWindowController_->getShadowMargin(),
                     connectWindowBackground);
    }
}

void MainWindow::setWindowToDpiScaleManager()
{
    DpiScaleManager::instance().setMainWindow(this);
    onScaleChanged();
}

void MainWindow::onMinimizeClick()
{
    showMinimized();
}

void MainWindow::onCloseClick()
{
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    if (backend_->getPreferences()->isMinimizeAndCloseToTray())
    {
        minimizeToTray();
    }
    else
    {
        mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_EXIT);
    }
#elif defined Q_OS_MAC
    mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_EXIT);
#endif
}

void MainWindow::onEscapeClick()
{
    gotoLoginWindow();
}

void MainWindow::onAbortInitialization()
{
    isInitializationAborted_ = true;
    backend_->abortInitialization();
}

void MainWindow::onLoginClick(const QString &username, const QString &password, const QString &code2fa)
{
    mainWindowController_->getTwoFactorAuthWindow()->setCurrentCredentials(username, password);
    mainWindowController_->getLoggingInWindow()->setMessage(QT_TRANSLATE_NOOP("LoginWindow::LoggingInWindowItem", "Logging you in..."));
    mainWindowController_->getLoggingInWindow()->setAdditionalMessage("");
    mainWindowController_->getLoggingInWindow()->startAnimation();
    mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_LOGGING_IN);

    locationsWindow_->setOnlyConfigTabVisible(false);

    backend_->login(username, password, code2fa);
}

void MainWindow::onLoginPreferencesClick()
{
    // the same actions as on connect screen
    onConnectWindowPreferencesClick();
}

void MainWindow::onLoginHaveAccountYesClick()
{
    loginAttemptsController_.reset();
    mainWindowController_->getLoginWindow()->transitionToUsernameScreen();
}

void MainWindow::onLoginBackToWelcomeClick()
{
    mainWindowController_->getLoginWindow()->resetState();
}

void MainWindow::onLoginEmergencyWindowClick()
{
    mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_EMERGENCY);
}
void MainWindow::onLoginExternalConfigWindowClick()
{
    mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_EXTERNAL_CONFIG);
}
void MainWindow::onLoginTwoFactorAuthWindowClick(const QString &username, const QString &password)
{
    mainWindowController_->getTwoFactorAuthWindow()->setErrorMessage(
        ITwoFactorAuthWindow::ERR_MSG_EMPTY);
    mainWindowController_->getTwoFactorAuthWindow()->setLoginMode(false);
    mainWindowController_->getTwoFactorAuthWindow()->setCurrentCredentials(username, password);
    mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_TWO_FACTOR_AUTH);
}

void MainWindow::onConnectWindowConnectClick()
{
    if (backend_->isDisconnected())
    {
        mainWindowController_->collapseLocations();
        backend_->sendConnect(PersistentState::instance().lastLocation());
    }
    else
    {  
        backend_->sendDisconnect();
    }
}

void MainWindow::onConnectWindowFirewallClick()
{
    if (!backend_->isFirewallEnabled())
    {
        backend_->firewallOn();
    }
    else
    {
        backend_->firewallOff();
    }
}

void MainWindow::onLoginFirewallTurnOffClick()
{
    if (backend_->isFirewallEnabled())
        backend_->firewallOff();
}

void MainWindow::onConnectWindowNetworkButtonClick()
{
    mainWindowController_->expandPreferences();
    mainWindowController_->getPreferencesWindow()->setCurrentTab(TAB_CONNECTION, CONNECTION_SCREEN_NETWORK_WHITELIST);
}

void MainWindow::onConnectWindowLocationsClick()
{
    // qCDebug(LOG_USER) << "Locations button clicked";
    if (!mainWindowController_->isLocationsExpanded())
    {
        mainWindowController_->expandLocations();
    }
    else
    {
        mainWindowController_->collapseLocations();
    }
}

void MainWindow::onConnectWindowPreferencesClick()
{
    backend_->getAndUpdateIPv6StateInOS();
    mainWindowController_->expandPreferences();
}

void MainWindow::onConnectWindowNotificationsClick()
{
    mainWindowController_->getNewsFeedWindow()->setMessages(
        notificationsController_.messages(), notificationsController_.shownIds());
    mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_NOTIFICATIONS);
}

void MainWindow::onConnectWindowSplitTunnelingClick()
{
    mainWindowController_->expandPreferences();
    mainWindowController_->getPreferencesWindow()->setCurrentTab(TAB_CONNECTION, CONNECTION_SCREEN_SPLIT_TUNNELING);
}

void MainWindow::onEscapeNotificationsClick()
{
    mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_CONNECT);
}

void MainWindow::onPreferencesEscapeClick()
{
    collapsePreferences();
    backend_->sendEngineSettingsIfChanged();
}

void MainWindow::onPreferencesSignOutClick()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    setEnabled(false);
    signOutReason_ = SIGN_OUT_FROM_MENU;
    backend_->signOut(false);
}

void MainWindow::onPreferencesLoginClick()
{
    collapsePreferences();
    backend_->sendEngineSettingsIfChanged();
}

void MainWindow::onPreferencesHelpClick()
{
    QDesktopServices::openUrl(QUrl( QString("https://%1/help").arg(HardcodedSettings::instance().serverUrl())));
}

void MainWindow::cleanupLogViewerWindow()
{
    if (logViewerWindow_ != nullptr)
    {
        logViewerWindow_->hide();
        logViewerWindow_->deleteLater();
        logViewerWindow_ = nullptr;
    }
}

QRect MainWindow::guessTrayIconLocationOnScreen(QScreen *screen)
{
    const QRect screenGeo = screen->geometry();
    QRect newIconRect = QRect(static_cast<int>(screenGeo.right() - WINDOW_WIDTH *G_SCALE),
                              screenGeo.top(),
                              savedTrayIconRect_.width(),
                              savedTrayIconRect_.height());

    return newIconRect;

}

void MainWindow::onPreferencesViewLogClick()
{
    // must delete every open: bug in qt 5.12.14 will lose parent hierarchy and crash
    cleanupLogViewerWindow();

#ifdef Q_OS_WIN
    if (!MergeLog::canMerge())
    {
        showUserWarning(ProtoTypes::USER_WARNING_VIEW_LOG_FILE_TOO_BIG);
        return;
    }
#endif

    logViewerWindow_ = new LogViewer::LogViewerWindow(this);
    logViewerWindow_->setAttribute( Qt::WA_DeleteOnClose );

    connect(
        logViewerWindow_, &LogViewer::LogViewerWindow::destroyed,
        [=]( QObject* )  {
            logViewerWindow_ = nullptr;
        }
    );

    logViewerWindow_->show();
}

void MainWindow::onPreferencesSendConfirmEmailClick()
{
    backend_->sendConfirmEmail();
}

void MainWindow::onPreferencesSendDebugLogClick()
{
    backend_->sendDebugLog();
}

void MainWindow::onPreferencesEditAccountDetailsClick()
{
    backend_->getWebSessionTokenForEditAccountDetails();
}

void MainWindow::onPreferencesAddEmailButtonClick()
{
    backend_->getWebSessionTokenForAddEmail();
}

void MainWindow::onPreferencesQuitAppClick()
{
    gotoExitWindow();
}

void MainWindow::onPreferencesNoAccountLoginClick()
{
    collapsePreferences();
    mainWindowController_->getLoginWindow()->resetState();
}

void MainWindow::onPreferencesSetIpv6StateInOS(bool bEnabled, bool bRestartNow)
{
    backend_->setIPv6StateInOS(bEnabled);
    if (bRestartNow)
    {
        qCDebug(LOG_BASIC) << "Restart system";
        #ifdef Q_OS_WIN
            WinUtils::reboot();
        #endif
    }
}

void MainWindow::onPreferencesCycleMacAddressClick()
{
    int confirm = QMessageBox::Yes;

    if (internetConnected_)
    {
        QString title = tr("VPN is active");
        QString desc = tr("Rotating your MAC address will result in a disconnect event from the current network. Are you sure?");
        confirm = QMessageBox::question(nullptr, title, desc, QMessageBox::Yes, QMessageBox::No);
    }

    if (confirm == QMessageBox::Yes)
    {
        backend_->cycleMacAddress();
    }
}

void MainWindow::onPreferencesWindowDetectAppropriatePacketSizeButtonClicked()
{
    if (!backend_->isDisconnected())
    {
        const QString title = tr("VPN is active");
        const QString desc = tr("Cannot detect appropriate packet size while connected. Please disconnect first.");
        mainWindowController_->getPreferencesWindow()->showPacketSizeDetectionError(title, desc);
    }
    else if (internetConnected_)
    {
        backend_->sendDetectPacketSize();
    }
    else
    {
        const QString title = tr("No Internet");
        const QString desc = tr("Cannot detect appropriate packet size without internet. Check your connection.");
        mainWindowController_->getPreferencesWindow()->showPacketSizeDetectionError(title, desc);
    }
}

void MainWindow::cleanupAdvParametersWindow()
{
    if (advParametersWindow_ != nullptr)
    {
        disconnect(advParametersWindow_);
        advParametersWindow_->hide();
        advParametersWindow_->deleteLater();
        advParametersWindow_ = nullptr;
    }
}

void MainWindow::onPreferencesAdvancedParametersClicked()
{
    // must delete every open: bug in qt 5.12.14 will lose parent hierarchy and crash
    cleanupAdvParametersWindow();

    advParametersWindow_ = new AdvancedParametersDialog(this);
    advParametersWindow_->setAdvancedParameters(backend_->getPreferences()->debugAdvancedParameters());
    connect(advParametersWindow_, SIGNAL(okClick()), SLOT(onAdvancedParametersOkClick()));
    connect(advParametersWindow_, SIGNAL(cancelClick()), SLOT(onAdvancedParametersCancelClick()));
    advParametersWindow_->show();
}

void MainWindow::onPreferencesCustomConfigsPathChanged(QString path)
{
    locationsWindow_->setCustomConfigsPath(path);
}

void MainWindow::onPreferencesdebugAdvancedParametersChanged(const QString &advParams)
{
    Q_UNUSED(advParams);
    backend_->sendAdvancedParametersChanged();
}

void MainWindow::onPreferencesUpdateChannelChanged(const ProtoTypes::UpdateChannel updateChannel)
{
    Q_UNUSED(updateChannel);

    ignoreUpdateUntilNextRun_ = false;
    // updates engine through engineSettings
}

void MainWindow::onPreferencesReportErrorToUser(const QString &title, const QString &desc)
{
    // The main window controller will assert if we are not on one of these windows, but we may
    // get here when on a different window if Preferences::validateAndUpdateIfNeeded() emits its
    // reportErrorToUser signal.
    if ((mainWindowController_->currentWindow() == MainWindowController::WINDOW_ID_CONNECT ||
         mainWindowController_->currentWindow() == MainWindowController::WINDOW_ID_UPDATE))
    {
        // avoid race condition that allows clicking through the general message overlay
        QTimer::singleShot(0, [this, title, desc](){
            mainWindowController_->getGeneralMessageWindow()->setTitle(title);
            mainWindowController_->getGeneralMessageWindow()->setDescription(desc);
            bGotoUpdateWindowAfterGeneralMessage_ = false;
            mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_GENERAL_MESSAGE);
        });
    }
    else
    {
        QMessageBox::warning(nullptr, title, desc);
    }
}

void MainWindow::onPreferencesCollapsed()
{
    backend_->getPreferences()->validateAndUpdateIfNeeded();
}

void MainWindow::onEmergencyConnectClick()
{
    backend_->emergencyConnectClick();
}

void MainWindow::onEmergencyDisconnectClick()
{
    backend_->emergencyDisconnectClick();
}

void MainWindow::onEmergencyWindscribeLinkClick()
{
    QDesktopServices::openUrl(QUrl( QString("https://%1/help").arg(HardcodedSettings::instance().serverUrl())));
}

void MainWindow::onExternalConfigWindowNextClick()
{
    mainWindowController_->getExternalConfigWindow()->setClickable(false);
    mainWindowController_->getPreferencesWindow()->setLoggedIn(true);
    backend_->getPreferencesHelper()->setIsExternalConfigMode(true);
    locationsWindow_->setOnlyConfigTabVisible(true);
    backend_->gotoCustomOvpnConfigMode();
}

void MainWindow::onTwoFactorAuthWindowButtonAddClick(const QString &code2fa)
{
    mainWindowController_->getLoginWindow()->setCurrent2FACode(code2fa);
    gotoLoginWindow();
}

void MainWindow::onBottomWindowRenewClick()
{
    openUpgradeExternalWindow();
}

void MainWindow::onBottomWindowExternalConfigLoginClick()
{
    /*hideSupplementaryWidgets();
    shadowManager_->setVisible(WINDOW_ID_CONNECT, false);

    loginWindow_->resetState();
    gotoLoginWindow(true);
    loginWindow_->transitionToUsernameScreen();
    */
}

void MainWindow::onBottomWindowSharingFeaturesClick()
{
    onConnectWindowPreferencesClick();
    mainWindowController_->getPreferencesWindow()->setCurrentTab(TAB_SHARE);
}

void MainWindow::onUpdateAppItemClick()
{
    mainWindowController_->getUpdateAppItem()->setMode(IUpdateAppItem::UPDATE_APP_ITEM_MODE_PROGRESS);
    mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_UPDATE);
}

void MainWindow::onUpdateWindowAccept()
{
    downloadRunning_ = true;
    mainWindowController_->getUpdateWindow()->setProgress(0);
    mainWindowController_->getUpdateWindow()->startAnimation();
    mainWindowController_->getUpdateWindow()->changeToDownloadingScreen();
    backend_->sendUpdateVersion(static_cast<qint32>(this->winId()));
}

void MainWindow::onUpdateWindowCancel()
{
    backend_->cancelUpdateVersion();
    mainWindowController_->getUpdateWindow()->changeToPromptScreen();
    downloadRunning_ = false;
}

void MainWindow::onUpdateWindowLater()
{
    mainWindowController_->getUpdateAppItem()->setMode(IUpdateAppItem::UPDATE_APP_ITEM_MODE_PROMPT);
    mainWindowController_->getUpdateAppItem()->setProgress(0);
    mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_CONNECT);

    ignoreUpdateUntilNextRun_ = true;
    mainWindowController_->hideUpdateWidget();
    downloadRunning_ = false;
}

void MainWindow::onUpgradeAccountAccept()
{
    openUpgradeExternalWindow();
    if (mainWindowController_->currentWindow() == MainWindowController::WINDOW_ID_UPGRADE)
        mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_CONNECT);
}

void MainWindow::onUpgradeAccountCancel()
{
    mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_CONNECT);
}

void MainWindow::onGeneralMessageWindowAccept()
{
    if (bGotoUpdateWindowAfterGeneralMessage_)
        mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_UPDATE);
    else
        mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_CONNECT);
}

void MainWindow::onExitWindowAccept()
{
    close();
}

void MainWindow::onExitWindowReject()
{
    mainWindowController_->changeWindow(MainWindowController::WINDOW_CMD_CLOSE_EXIT);
    if (isExitingFromPreferences_) {
        isExitingFromPreferences_ = false;
        mainWindowController_->expandPreferences();
    }
}

void MainWindow::onLocationSelected(LocationID id)
{
    qCDebug(LOG_USER) << "Location selected:" << id.getHashString();

    LocationsModel::LocationInfo li;
    if (backend_->getLocationsModel()->getLocationInfo(id, li))
    {
        mainWindowController_->getConnectWindow()->updateLocationInfo(li.id, li.firstName, li.secondName, li.countryCode, li.pingTime);
        mainWindowController_->collapseLocations();
        PersistentState::instance().setLastLocation(id);
        backend_->sendConnect(id);
    }
}

void MainWindow::onClickedOnPremiumStarCity()
{
    openUpgradeExternalWindow();
}

void MainWindow::onLocationSwitchFavorite(LocationID id, bool isFavorite)
{
    backend_->getLocationsModel()->switchFavorite(id, isFavorite);
    mainWindowController_->getConnectWindow()->updateFavoriteState(id, isFavorite);

    // Also switch favorite flag for the alternate location id; i.e. find the best location with the
    // same city name, if we are currently processing a generic location, and vice versa.
    LocationID altLocationId = id.isBestLocation() ? id.bestLocationToApiLocation() : id.apiLocationToBestLocation();
    LocationsModel::LocationInfo li;
    if (backend_->getLocationsModel()->getLocationInfo(altLocationId, li))
    {
        backend_->getLocationsModel()->switchFavorite(altLocationId, isFavorite);
        mainWindowController_->getConnectWindow()->updateFavoriteState(altLocationId, isFavorite);
    }
}

void MainWindow::onLocationsAddStaticIpClicked()
{
    openStaticIpExternalWindow();
}

void MainWindow::onLocationsClearCustomConfigClicked()
{
    if (!backend_->getPreferences()->customOvpnConfigsPath().isEmpty()) {
        // qCDebug(LOG_BASIC) << "User cleared custom config path";
        backend_->getPreferences()->setCustomOvpnConfigsPath(QString());
        backend_->sendEngineSettingsIfChanged();
    }
}

void MainWindow::onLocationsAddCustomConfigClicked()
{
    ShowingDialogState::instance().setCurrentlyShowingExternalDialog(true);
    QString path = QFileDialog::getExistingDirectory(
        this, tr("Select Custom Config Folder"), "", QFileDialog::ShowDirsOnly);
    ShowingDialogState::instance().setCurrentlyShowingExternalDialog(false);

    if (!path.isEmpty()) {
        // qCDebug(LOG_BASIC) << "User selected custom config path:" << path;

        WriteAccessRightsChecker checker(path);
        if (checker.isWriteable())
        {
            if (!checker.isElevated())
            {
                std::unique_ptr<IAuthChecker> authChecker = AuthCheckerFactory::createAuthChecker();

                AuthCheckerError err = authChecker->authenticate();
                if (err == AuthCheckerError::AUTHENTICATION_ERROR)
                {
                    qCDebug(LOG_BASIC) << "Cannot change path when non-system directory when windscribe is not elevated.";
                    const QString desc = tr(
                        "Cannot select this directory because it is writeable for non-privileged users. "
                        "Custom configs in this directory may pose a potential security risk. "
                        "Please authenticate with an admin user to select this directory.");
                    QMessageBox::warning(g_mainWindow, tr("Windscribe"), desc);
                    return;
                }
                else if (err == AuthCheckerError::HELPER_ERROR)
                {
                    qCDebug(LOG_AUTH_HELPER) << "Failed to verify AuthHelper, binary may be corrupted.";
                    const QString desc = tr(
                        "Failed to verify AuthHelper, binary may be corrupted. "
                        "Please reinstall application to repair.");
                    QMessageBox::warning(g_mainWindow, tr("Windscribe"), desc);
                    return;
                }
            }

            // warn, but still allow path setting
            const QString desc = tr(
                "The selected directory is writeable for non-privileged users. "
                "Custom configs in this directory may pose a potential security risk.");
            QMessageBox::warning(g_mainWindow, tr("Windscribe"), desc);
        }

        // set the path
        backend_->getPreferences()->setCustomOvpnConfigsPath(path);
        backend_->sendEngineSettingsIfChanged();
    }
}

void MainWindow::onBackendInitFinished(ProtoTypes::InitState initState)
{
    setVariablesToInitState();

    if (initState == ProtoTypes::INIT_SUCCESS)
    {
        localIpcServer_->start(); // start local IPC server for receive commands from CLI

        setInitialFirewallState();

        Preferences *p = backend_->getPreferences();
        p->validateAndUpdateIfNeeded();

        backend_->sendSplitTunneling(p->splitTunneling());

        // disable firewall for Mac when split tunneling is active
#ifdef Q_OS_MAC
        if (p->splitTunneling().settings().active())
        {
            backend_->getPreferencesHelper()->setBlockFirewall(true);
            mainWindowController_->getConnectWindow()->setFirewallBlock(true);
        }
#endif

        // enable wifi/proxy sharing, if checked
        if (p->shareSecureHotspot().is_enabled())
        {
            onPreferencesShareSecureHotspotChanged(p->shareSecureHotspot());
        }
        if (p->shareProxyGateway().is_enabled())
        {
            onPreferencesShareProxyGatewayChanged(p->shareProxyGateway());
        }

        if (backend_->isCanLoginWithAuthHash())
        {
            if (!backend_->isSavedApiSettingsExists())
            {
                mainWindowController_->getLoggingInWindow()->setMessage(QT_TRANSLATE_NOOP("LoginWindow::LoggingInWindowItem", "Logging you in..."));
                mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_LOGGING_IN);
            }
            backend_->loginWithAuthHash(backend_->getCurrentAuthHash());
        }
        else
        {
            mainWindowController_->getInitWindow()->startSlideAnimation();
            gotoLoginWindow();
        }

        if (!p->connectionSettings().is_automatic())
        {
            mainWindowController_->getConnectWindow()->setProtocolPort(p->connectionSettings().protocol(), p->connectionSettings().port());
        }
    }
    else if (initState == ProtoTypes::INIT_BFE_SERVICE_NOT_STARTED)
    {
        if (QMessageBox::information(nullptr, QApplication::applicationName(), QObject::tr("Enable \"Base Filtering Engine\" service? This is required for Windscribe to function."),
                                     QMessageBox::Yes, QMessageBox::Close) == QMessageBox::Yes)
        {
            backend_->enableBFE_win();
        }
        else
        {
            QTimer::singleShot(0, this, SLOT(close()));
            return;
        }
    }
    else if (initState == ProtoTypes::INIT_BFE_SERVICE_FAILED_TO_START)
    {
        QMessageBox::information(nullptr, QApplication::applicationName(), QObject::tr("Failed to start \"Base Filtering Engine\" service."),
                                         QMessageBox::Close);
        QTimer::singleShot(0, this, SLOT(close()));
    }
    else if (initState == ProtoTypes::INIT_HELPER_FAILED)
    {
        QMessageBox::information(nullptr, QApplication::applicationName(), tr("Windscribe helper initialize error. Please reinstall the application or contact support."));
        QTimer::singleShot(0, this, SLOT(close()));
    }
    else if (initState == ProtoTypes::INIT_HELPER_USER_CANCELED)
    {
        // close without message box
        QTimer::singleShot(0, this, SLOT(close()));
    }
    else
    {
        if (!isInitializationAborted_)
            QMessageBox::information(nullptr, QApplication::applicationName(), tr("Can't start the engine. Please contact support."));
        QTimer::singleShot(0, this, SLOT(close()));
    }
}

void MainWindow::onBackendInitTooLong()
{
    mainWindowController_->getInitWindow()->setCloseButtonVisible(true);
    mainWindowController_->getInitWindow()->setAdditionalMessage(
        tr("This is taking a while, something could be wrong.\n"
           "If this screen does not disappear,\nplease contact support."), /*useSmallFont =*/ true);
}

void MainWindow::onBackendLoginFinished(bool /*isLoginFromSavedSettings*/)
{
    mainWindowController_->getPreferencesWindow()->setLoggedIn(true);
    mainWindowController_->getTwoFactorAuthWindow()->clearCurrentCredentials();

    if (backend_->getPreferences()->firewalSettings().mode() == ProtoTypes::FIREWALL_MODE_ALWAYS_ON)
    {
        backend_->firewallOn();
        mainWindowController_->getConnectWindow()->setFirewallAlwaysOn(true);
    }

    if (!isLoginOkAndConnectWindowVisible_)
    {
        // choose latest location
        LocationsModel::LocationInfo li;
        if (PersistentState::instance().lastLocation().isValid() && backend_->getLocationsModel()->getLocationInfo(PersistentState::instance().lastLocation(), li))
        {
            mainWindowController_->getConnectWindow()->updateLocationInfo(li.id, li.firstName, li.secondName, li.countryCode, li.pingTime);
        }
        else
        {
            LocationID bestLocation = backend_->getLocationsModel()->getBestLocationId();
            if (backend_->getLocationsModel()->getLocationInfo(bestLocation, li))
            {
                PersistentState::instance().setLastLocation(bestLocation);
                mainWindowController_->getConnectWindow()->updateLocationInfo(li.id, li.firstName, li.secondName, li.countryCode, li.pingTime);
            }
        }

        mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_CONNECT);
        isLoginOkAndConnectWindowVisible_ = true;
    }

    // open new install on first login
    if (PersistentState::instance().isFirstLogin())
    {
        backend_->recordInstall();
        // open first start URL
        QString curUserId = QString::fromStdString(backend_->getSessionStatus().user_id());
        QDesktopServices::openUrl(QUrl( QString("https://%1/installed/desktop?%2").arg(HardcodedSettings::instance().serverUrl()).arg(curUserId)));
    }
    PersistentState::instance().setFirstLogin(false);
}

void MainWindow::onBackendLoginStepMessage(ProtoTypes::LoginMessage msg)
{
    QString additionalMessage;
    if (msg == ProtoTypes::LOGIN_MESSAGE_TRYING_BACKUP1)
    {
        additionalMessage = tr("Trying Backup Endpoints 1/2");
    }
    else if (msg == ProtoTypes::LOGIN_MESSAGE_TRYING_BACKUP2)
    {
        additionalMessage = tr("Trying Backup Endpoints 2/2");
    }
    mainWindowController_->getLoggingInWindow()->setAdditionalMessage(additionalMessage);
}

void MainWindow::onBackendLoginError(ProtoTypes::LoginError loginError, const QString &errorMessage)
{
    if (loginError == ProtoTypes::LOGIN_ERROR_BAD_USERNAME)
    {
        if (backend_->isLastLoginWithAuthHash())
        {
            if (!isLoginOkAndConnectWindowVisible_)
            {
                mainWindowController_->getLoginWindow()->setErrorMessage(ILoginWindow::ERR_MSG_EMPTY, QString());
                mainWindowController_->getLoginWindow()->setEmergencyConnectState(false);
                mainWindowController_->getLoginWindow()->resetState();
                gotoLoginWindow();
            }
            else
            {
                backToLoginWithErrorMessage(ILoginWindow::ERR_MSG_EMPTY, QString());
            }
        }
        else
        {
            loginAttemptsController_.pushIncorrectLogin();
            mainWindowController_->getLoginWindow()->setErrorMessage(loginAttemptsController_.currentMessage(), QString());
            gotoLoginWindow();
        }
    }
    else if (loginError == ProtoTypes::LOGIN_ERROR_BAD_CODE2FA ||
             loginError == ProtoTypes::LOGIN_ERROR_MISSING_CODE2FA)
    {
        const bool is_missing_code2fa = (loginError == ProtoTypes::LOGIN_ERROR_MISSING_CODE2FA);
        mainWindowController_->getTwoFactorAuthWindow()->setErrorMessage(
            is_missing_code2fa ? ITwoFactorAuthWindow::ERR_MSG_NO_CODE
                               : ITwoFactorAuthWindow::ERR_MSG_INVALID_CODE);
        mainWindowController_->getTwoFactorAuthWindow()->setLoginMode(true);
        mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_TWO_FACTOR_AUTH);
    }
    else if (loginError == ProtoTypes::LOGIN_ERROR_NO_CONNECTIVITY)
    {
        if (!isLoginOkAndConnectWindowVisible_)
        {
            //qCDebug(LOG_BASIC) << "Show no connectivity message to user.";
            mainWindowController_->getLoginWindow()->setErrorMessage(ILoginWindow::ERR_MSG_NO_INTERNET_CONNECTIVITY, QString());
            mainWindowController_->getLoginWindow()->setEmergencyConnectState(false);
            gotoLoginWindow();
        }
        else
        {
            //qCDebug(LOG_BASIC) << "No internet connectivity from connected state. Using stale API data from settings.";
            backend_->loginWithLastLoginSettings();
        }
    }
    else if (loginError == ProtoTypes::LOGIN_ERROR_NO_API_CONNECTIVITY)
    {
        if (!isLoginOkAndConnectWindowVisible_)
        {
            //qCDebug(LOG_BASIC) << "Show No API connectivity message to user.";
            mainWindowController_->getLoginWindow()->setErrorMessage(ILoginWindow::ERR_MSG_NO_API_CONNECTIVITY, QString());
            //loginWindow_->setEmergencyConnectState(falseengine_->isEmergencyDisconnected());
            gotoLoginWindow();
        }
    }
    else if (loginError == ProtoTypes::LOGIN_ERROR_INCORRECT_JSON)
    {
        if (!isLoginOkAndConnectWindowVisible_)
        {
            mainWindowController_->getLoginWindow()->setErrorMessage(ILoginWindow::ERR_MSG_INVALID_API_RESPONCE, QString());
            mainWindowController_->getLoginWindow()->setEmergencyConnectState(false);
            gotoLoginWindow();
        }
        else
        {
            backToLoginWithErrorMessage(ILoginWindow::ERR_MSG_INVALID_API_RESPONCE, QString());
        }
    }
    else if (loginError == ProtoTypes::LOGIN_ERROR_PROXY_AUTH_NEED)
    {
        if (!isLoginOkAndConnectWindowVisible_)
        {
            mainWindowController_->getLoginWindow()->setErrorMessage(ILoginWindow::ERR_MSG_PROXY_REQUIRES_AUTH, QString());
            mainWindowController_->getLoginWindow()->setEmergencyConnectState(false);
            gotoLoginWindow();
        }
        else
        {
            backToLoginWithErrorMessage(ILoginWindow::ERR_MSG_PROXY_REQUIRES_AUTH, QString());
        }
    }
    else if (loginError == ProtoTypes::LOGIN_ERROR_SSL_ERROR)
    {
        int res = QMessageBox::information(nullptr, QApplication::applicationName(),
                                           tr("We detected that SSL requests may be intercepted on your network. This could be due to a firewall configured on your computer, or Windscribe being blocking by your network administrator. Ignore SSL errors?"),
                                           QMessageBox::Yes, QMessageBox::No);
        if (res == QMessageBox::Yes)
        {
            backend_->getPreferences()->setIgnoreSslErrors(true);
            mainWindowController_->getLoggingInWindow()->setMessage("");
            backend_->loginWithLastLoginSettings();
        }
        else
        {
            if (!isLoginOkAndConnectWindowVisible_)
            {
                mainWindowController_->getLoginWindow()->setErrorMessage(ILoginWindow::ERR_MSG_INVALID_API_ENDPOINT, QString());
                mainWindowController_->getLoginWindow()->setEmergencyConnectState(false);
                gotoLoginWindow();
            }
            else
            {
                backToLoginWithErrorMessage(ILoginWindow::ERR_MSG_INVALID_API_ENDPOINT, QString());
                return;
            }
        }
    }
    else if (loginError == ProtoTypes::LOGIN_ERROR_ACCOUNT_DISABLED)
    {
        if (!isLoginOkAndConnectWindowVisible_)
        {
            mainWindowController_->getLoginWindow()->setErrorMessage(ILoginWindow::ERR_MSG_ACCOUNT_DISABLED, errorMessage);
            mainWindowController_->getLoginWindow()->setEmergencyConnectState(false);
            gotoLoginWindow();
        }
        else
        {
            backToLoginWithErrorMessage(ILoginWindow::ERR_MSG_ACCOUNT_DISABLED, errorMessage);
        }
    }
    else if (loginError == ProtoTypes::LOGIN_ERROR_SESSION_INVALID)
    {
        if (!isLoginOkAndConnectWindowVisible_)
        {
            mainWindowController_->getLoginWindow()->setErrorMessage(ILoginWindow::ERR_MSG_SESSION_EXPIRED, QString());
            mainWindowController_->getLoginWindow()->setEmergencyConnectState(false);
            gotoLoginWindow();
        }
        else
        {
            backToLoginWithErrorMessage(ILoginWindow::ERR_MSG_SESSION_EXPIRED, QString());
        }
    }
}

void MainWindow::onBackendSessionStatusChanged(const ProtoTypes::SessionStatus &sessionStatus)
{
    //bFreeSessionStatus_ = sessionStatus->isPremium == 0;
    blockConnect_.setNotBlocking();
    int status = sessionStatus.status();
    // multiple account abuse detection
    QString entryUsername;
    bool bEntryIsPresent = multipleAccountDetection_->entryIsPresent(entryUsername);
    if (bEntryIsPresent && (!sessionStatus.is_premium()) && sessionStatus.alc_size() == 0 && sessionStatus.status() == 1 && entryUsername != QString::fromStdString(sessionStatus.username()))
    {
        status = 2;
        //qCDebug(LOG_BASIC) << "Abuse detection: User session status was changed to 2. Original username:" << entryUsername;
        blockConnect_.setBlockedMultiAccount(entryUsername);
    }
    else if (bEntryIsPresent && entryUsername == QString::fromStdString(sessionStatus.username()) && sessionStatus.status() == 1)
    {
        multipleAccountDetection_->removeEntry();
    }

    // free account
    if (!sessionStatus.is_premium())
    {
        if (status == 2)
        {
            // write entry into registry expired_user = username
            multipleAccountDetection_->userBecomeExpired(QString::fromStdString(sessionStatus.username()));

            if ((!PersistentState::instance().lastLocation().isCustomConfigsLocation()) &&
                (backend_->currentConnectState() == ProtoTypes::CONNECTED || backend_->currentConnectState() == ProtoTypes::CONNECTING))
            {
                bDisconnectFromTrafficExceed_ = true;
                backend_->sendDisconnect();
                mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_UPGRADE);
            }

            mainWindowController_->getBottomInfoWindow()->setDataRemaining(0, 0);
            if (!blockConnect_.isBlocked())
            {
                blockConnect_.setBlockedExceedTraffic();
            }
        }
        else
        {
            if (sessionStatus.traffic_max() == -1)
            {
                mainWindowController_->getBottomInfoWindow()->setDataRemaining(-1, -1);
            }
            else
            {
                if (backend_->getPreferences()->isShowNotifications())
                {
                    freeTrafficNotificationController_->updateTrafficInfo(sessionStatus.traffic_used(), sessionStatus.traffic_max());
                }

                mainWindowController_->getBottomInfoWindow()->setDataRemaining(sessionStatus.traffic_used(), sessionStatus.traffic_max());
            }
        }
    }
    // premium account
    else
    {
        if (sessionStatus.rebill() == 0)
        {
            QDate curDate = QDateTime::currentDateTimeUtc().date();
            QDate expireDate = QDate::fromString(QString::fromStdString(sessionStatus.premium_expire_date()), "yyyy-MM-dd");

            int days = curDate.daysTo(expireDate);
            if (days >= 0 && days <= 5)
            {
                mainWindowController_->getBottomInfoWindow()->setDaysRemaining(days);
            }
            else
            {
                mainWindowController_->getBottomInfoWindow()->setDaysRemaining(-1);
            }
        }
        else
        {
            mainWindowController_->getBottomInfoWindow()->setDaysRemaining(-1);
        }
    }

    if (status == 3)
    {
        blockConnect_.setBlockedBannedUser();
        if ((!PersistentState::instance().lastLocation().isCustomConfigsLocation()) &&
            (backend_->currentConnectState() == ProtoTypes::CONNECTED || backend_->currentConnectState() == ProtoTypes::CONNECTING))
        {
            backend_->sendDisconnect();
        }
    }
    backend_->setBlockConnect(blockConnect_.isBlocked());

    if (isPrevSessionStatusInitialized_)
    {
        if (prevSessionStatus_ == 2 && status == 1)
        {
            backend_->clearCredentials();
        }
    }

    prevSessionStatus_ = status;
    isPrevSessionStatusInitialized_ = true;
}

void MainWindow::onBackendCheckUpdateChanged(const ProtoTypes::CheckUpdateInfo &checkUpdateInfo)
{
    if (checkUpdateInfo.is_available())
    {
        qCDebug(LOG_BASIC) << "Update available";
        if (!checkUpdateInfo.is_supported())
        {
            blockConnect_.setNeedUpgrade();
        }

        QString betaStr;
        betaStr = "-" + QString::number(checkUpdateInfo.latest_build());
        if (checkUpdateInfo.update_channel() == ProtoTypes::UPDATE_CHANNEL_BETA)
        {
            betaStr += "b";
        }
        else if (checkUpdateInfo.update_channel() == ProtoTypes::UPDATE_CHANNEL_GUINEA_PIG)
        {
            betaStr += "g";
        }

        //updateWidget_->setText(tr("Update available - v") + version + betaStr);

        mainWindowController_->getUpdateAppItem()->setVersionAvailable(QString::fromStdString(checkUpdateInfo.version()), checkUpdateInfo.latest_build());
        mainWindowController_->getUpdateWindow()->setVersion(QString::fromStdString(checkUpdateInfo.version()), checkUpdateInfo.latest_build());

        if (!ignoreUpdateUntilNextRun_)
        {
            mainWindowController_->showUpdateWidget();
        }
    }
    else
    {
        qCDebug(LOG_BASIC) << "No available update";
        mainWindowController_->hideUpdateWidget();
    }
}

void MainWindow::onBackendMyIpChanged(QString ip, bool isFromDisconnectedState)
{
    mainWindowController_->getConnectWindow()->updateMyIp(ip);
    if (isFromDisconnectedState)
    {
        PersistentState::instance().setLastExternalIp(ip);
        updateTrayTooltip(tr("Disconnected") + "\n" + ip);
    }
    else
    {
        LocationsModel::LocationInfo li;
        if (backend_->getLocationsModel()->getLocationInfo(PersistentState::instance().lastLocation(), li))
        {
            updateTrayTooltip(tr("Connected to ") + li.firstName + "-" + li.secondName + "\n" + ip);
        }
    }
}

void MainWindow::onBackendConnectStateChanged(const ProtoTypes::ConnectState &connectState)
{  
    mainWindowController_->getConnectWindow()->updateConnectState(connectState);

    if (connectState.has_location())
    {
        // if connecting/connected location not equal current selected location, then change current selected location and update in GUI
        LocationID connectStateLocationId = LocationID::createFromProtoBuf(connectState.location());
        if (PersistentState::instance().lastLocation() != connectStateLocationId)
        {
            PersistentState::instance().setLastLocation( connectStateLocationId );
            LocationsModel::LocationInfo li;
            if (backend_->getLocationsModel()->getLocationInfo(PersistentState::instance().lastLocation(), li))
            {
                mainWindowController_->getConnectWindow()->updateLocationInfo(li.id, li.firstName, li.secondName, li.countryCode, li.pingTime);
            }
        }
    }

    if (connectState.connect_state_type() == ProtoTypes::DISCONNECTED)
    {
        updateConnectWindowStateProtocolPortDisplay(backend_->getPreferences()->connectionSettings());
    }

    if (connectState.connect_state_type() == ProtoTypes::CONNECTED)
    {
        bytesTransferred_ = 0;
        connectionElapsedTimer_.start();

        // Ensure the icon has been updated, as QSystemTrayIcon::showMessage displays this icon
        // in the notification window on Windows.
        updateAppIconType(AppIconType::CONNECTED);
        updateTrayIconType(AppIconType::CONNECTED);

        if (backend_->getPreferences()->isShowNotifications())
        {
            if (!bNotificationConnectedShowed_)
            {
                LocationsModel::LocationInfo li;
                if (backend_->getLocationsModel()->getLocationInfo(PersistentState::instance().lastLocation(), li))
                {
                    trayIcon_.showMessage("Windscribe", tr("You are now connected to Windscribe (%1).").arg(li.firstName + "-" + li.secondName));
                    bNotificationConnectedShowed_ = true;
                }
            }
        }
    }
    else if (connectState.connect_state_type() == ProtoTypes::CONNECTING || connectState.connect_state_type() == ProtoTypes::DISCONNECTING)
    {
        updateAppIconType(AppIconType::CONNECTING);
        updateTrayIconType(AppIconType::CONNECTING);
        mainWindowController_->clearServerRatingsTooltipState();
    }
    else if (connectState.connect_state_type() == ProtoTypes::DISCONNECTED)
    {
        // Ensure the icon has been updated, as QSystemTrayIcon::showMessage displays this icon
        // in the notification window on Windows.
        updateAppIconType(AppIconType::DISCONNECTED);
        updateTrayIconType(AppIconType::DISCONNECTED);

        if (bNotificationConnectedShowed_)
        {
            if (backend_->getPreferences()->isShowNotifications())
            {
                trayIcon_.showMessage("Windscribe", tr("Connection to Windscribe has been terminated.\n%1 transferred in %2").arg(getConnectionTransferred()).arg(getConnectionTime()));
            }
            bNotificationConnectedShowed_ = false;
        }

        if (connectState.disconnect_reason() == ProtoTypes::DISCONNECTED_WITH_ERROR)
        {
            handleDisconnectWithError(connectState);
        }
    }
}

void MainWindow::onBackendEmergencyConnectStateChanged(const ProtoTypes::ConnectState &connectState)
{
    mainWindowController_->getEmergencyConnectWindow()->setState(connectState);
    mainWindowController_->getLoginWindow()->setEmergencyConnectState(connectState.connect_state_type() == ProtoTypes::CONNECTED);
}

void MainWindow::onBackendFirewallStateChanged(bool isEnabled)
{
    mainWindowController_->getConnectWindow()->updateFirewallState(isEnabled);
    PersistentState::instance().setFirewallState(isEnabled);
}

void MainWindow::onNetworkChanged(ProtoTypes::NetworkInterface network)
{
    qCDebug(LOG_BASIC) << "Network Changed: "
                       << "Index: " << network.interface_index()
                       << ", Network/SSID: " << QString::fromStdString(network.network_or_ssid())
                       << ", MAC: " << QString::fromStdString(network.physical_address())
                       << ", device name: " << QString::fromStdString(network.device_name())
                       << " friendly: " << QString::fromStdString(network.friendly_name());

    mainWindowController_->getConnectWindow()->updateNetworkState(network);
    mainWindowController_->getPreferencesWindow()->updateNetworkState(network);
}

void MainWindow::onSplitTunnelingStateChanged(bool isActive)
{
    mainWindowController_->getConnectWindow()->setSplitTunnelingState(isActive);
}

void MainWindow::onBackendSignOutFinished()
{
    loginAttemptsController_.reset();
    isPrevSessionStatusInitialized_ = false;
    mainWindowController_->getPreferencesWindow()->setLoggedIn(false);
    isLoginOkAndConnectWindowVisible_ = false;
    backend_->getPreferencesHelper()->setIsExternalConfigMode(false);

    //hideSupplementaryWidgets();

    //shadowManager_->setVisible(ShadowManager::SHAPE_ID_PREFERENCES, false);
    //shadowManager_->setVisible(ShadowManager::SHAPE_ID_CONNECT_WINDOW, false);
    //preferencesWindow_->getGraphicsObject()->hide();
    //curWindowState_.setPreferencesState(PREFERENCES_STATE_COLLAPSED);

    if (signOutReason_ == SIGN_OUT_FROM_MENU)
    {
        mainWindowController_->getLoginWindow()->resetState();
        mainWindowController_->getLoginWindow()->setErrorMessage(ILoginWindow::ERR_MSG_EMPTY, QString());
    }
    else if (signOutReason_ == SIGN_OUT_SESSION_EXPIRED)
    {
        mainWindowController_->getLoginWindow()->transitionToUsernameScreen();
        mainWindowController_->getLoginWindow()->setErrorMessage(ILoginWindow::ERR_MSG_SESSION_EXPIRED, QString());
    }
    else if (signOutReason_ == SIGN_OUT_WITH_MESSAGE)
    {
        mainWindowController_->getLoginWindow()->transitionToUsernameScreen();
        mainWindowController_->getLoginWindow()->setErrorMessage(signOutMessageType_, signOutErrorMessage_);
    }
    else
    {
        Q_ASSERT(false);
        mainWindowController_->getLoginWindow()->resetState();
        mainWindowController_->getLoginWindow()->setErrorMessage(ILoginWindow::ERR_MSG_EMPTY, QString());
    }

    mainWindowController_->getLoginWindow()->setEmergencyConnectState(false);
    gotoLoginWindow();

    mainWindowController_->hideUpdateWidget();
    setEnabled(true);
    QApplication::restoreOverrideCursor();
}

void MainWindow::onBackendCleanupFinished()
{
    qCDebug(LOG_BASIC) << "Backend Cleanup Finished";
    close();
}

void MainWindow::onBackendGotoCustomOvpnConfigModeFinished()
{
    if (backend_->getPreferences()->firewalSettings().mode() == ProtoTypes::FIREWALL_MODE_ALWAYS_ON)
    {
        backend_->firewallOn();
        mainWindowController_->getConnectWindow()->setFirewallAlwaysOn(true);
    }

    if (!isLoginOkAndConnectWindowVisible_)
    {
        // Choose latest location if it's a custom config location; first valid custom config
        // location otherwise.
        LocationsModel::LocationInfo li;
        const LocationID lastLocation{PersistentState::instance().lastLocation()};
        if (lastLocation.isCustomConfigsLocation() &&
            backend_->getLocationsModel()->getLocationInfo(lastLocation, li))
        {
            mainWindowController_->getConnectWindow()->updateLocationInfo(
                li.id, li.firstName, li.secondName, li.countryCode, li.pingTime);
        }
        else
        {
            const LocationID bestLocation{
                backend_->getLocationsModel()->getFirstValidCustomConfigLocationId()};
            if (bestLocation.isValid() &&
                backend_->getLocationsModel()->getLocationInfo(bestLocation, li))
            {
                PersistentState::instance().setLastLocation(bestLocation);
            }
            // |li| can be empty here, so this will reset current location.
            mainWindowController_->getConnectWindow()->updateLocationInfo(
                li.id, li.firstName, li.secondName, li.countryCode, li.pingTime);
        }

        mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_CONNECT);
        isLoginOkAndConnectWindowVisible_ = true;
    }
}

void MainWindow::onBackendConfirmEmailResult(bool bSuccess)
{
    mainWindowController_->getPreferencesWindow()->setConfirmEmailResult(bSuccess);
}

void MainWindow::onBackendDebugLogResult(bool bSuccess)
{
    mainWindowController_->getPreferencesWindow()->setDebugLogResult(bSuccess);
}

void MainWindow::onBackendStatisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes)
{
    if (isTotalBytes)
    {
        bytesTransferred_ = bytesIn + bytesOut;
    }
    else
    {
        bytesTransferred_ += bytesIn + bytesOut;
    }

    mainWindowController_->getConnectWindow()->setConnectionTimeAndData(getConnectionTime(), getConnectionTransferred());
}

void MainWindow::onBackendProxySharingInfoChanged(const ProtoTypes::ProxySharingInfo &psi)
{
    if (psi.has_address())
    {
        backend_->getPreferencesHelper()->setProxyGatewayAddress(QString::fromStdString(psi.address()));
    }

    if (psi.has_is_enabled())
    {
        if (psi.is_enabled())
        {
            mainWindowController_->getBottomInfoWindow()->setProxyGatewayFeatures(true, psi.mode());
        }
        else
        {
            mainWindowController_->getBottomInfoWindow()->setProxyGatewayFeatures(false, ProtoTypes::PROXY_SHARING_HTTP);
        }
    }

    if (psi.has_users_count())
    {
        mainWindowController_->getBottomInfoWindow()->setProxyGatewayUsersCount(psi.users_count());
    }
}

void MainWindow::onBackendWifiSharingInfoChanged(const ProtoTypes::WifiSharingInfo &wsi)
{
    if (wsi.has_is_enabled())
    {
        if (wsi.is_enabled())
        {
            mainWindowController_->getBottomInfoWindow()->setSecureHotspotFeatures(true, QString::fromStdString(wsi.ssid()));
        }
        else
        {
            mainWindowController_->getBottomInfoWindow()->setSecureHotspotFeatures(false, "");
        }
    }

    if (wsi.has_users_count())
    {
        mainWindowController_->getBottomInfoWindow()->setSecureHotspotUsersCount(wsi.users_count());
    }
}

void MainWindow::onBackendRequestCustomOvpnConfigCredentials()
{
    DialogGetUsernamePassword dlg(this, true);
    if (dlg.exec() == QDialog::Accepted)
    {
        backend_->continueWithCredentialsForOvpnConfig(dlg.username(), dlg.password(), dlg.isNeedSave());
    }
    else
    {
        backend_->continueWithCredentialsForOvpnConfig("", "", false);
    }
}

void MainWindow::onBackendSessionDeleted()
{
    qCDebug(LOG_BASIC) << "Handle deleted session";

    QApplication::setOverrideCursor(Qt::WaitCursor);
    setEnabled(false);
    signOutReason_ = SIGN_OUT_SESSION_EXPIRED;
    backend_->signOut(true);
}

void MainWindow::onBackendTestTunnelResult(bool success)
{
    if (!success)
    {
        QMessageBox::information(nullptr, QApplication::applicationName(),
            tr("We've detected that your network settings may interfere with Windscribe. "
               "Please disconnect and send us a Debug Log, by going into Preferences and clicking the \"Send Log\" button."));
    }

    mainWindowController_->getConnectWindow()->setTestTunnelResult(success);
}

void MainWindow::onBackendLostConnectionToHelper()
{
    qCDebug(LOG_BASIC) << "Helper connection was lost";
    QMessageBox::information(nullptr, QApplication::applicationName(), tr("Couldn't connect to Windscribe helper, please restart the application"));
}

void MainWindow::onBackendHighCpuUsage(const QStringList &processesList)
{
    if (!PersistentState::instance().isIgnoreCpuUsageWarnings())
    {
        QString processesListString;
        for (const QString &processName : processesList)
        {
            if (!processesListString.isEmpty())
            {
                processesListString += ", ";
            }
            processesListString += processName;
        }

        qCDebug(LOG_BASIC) << "Detected high CPU usage in processes:" << processesListString;

        QString msg = QString(tr("Windscribe has detected that %1 is using a high amount of CPU due to a potential conflict with the VPN connection. Do you want to disable the Windscribe TCP socket termination feature that may be causing this issue?").arg(processesListString));

        DialogMessageCpuUsage msgBox(this, msg);
        msgBox.exec();
        if (msgBox.retCode() == DialogMessageCpuUsage::RET_YES)
        {
            PersistentState::instance().setIgnoreCpuUsageWarnings(false);
        }
        if (msgBox.isIgnoreWarnings()) // ignore future warnings
        {
            PersistentState::instance().setIgnoreCpuUsageWarnings(true);
        }
    }
}

void MainWindow::showUserWarning(ProtoTypes::UserWarningType userWarningType)
{
    QString titleText = "";
    QString descText = "";
    if (userWarningType == ProtoTypes::USER_WARNING_MAC_SPOOFING_FAILURE_HARD)
    {
        titleText = tr("MAC Spoofing Failed");
        descText = tr("Your network adapter does not support MAC spoofing. Try a different adapter.");
    }
    else if (userWarningType == ProtoTypes::USER_WARNING_MAC_SPOOFING_FAILURE_SOFT)
    {
        titleText = tr("MAC Spoofing Failed");
        descText = tr("Could not spoof MAC address, try updating your OS to the latest version.");
    }
    else if (userWarningType == ProtoTypes::USER_WARNING_SEND_LOG_FILE_TOO_BIG)
    {
        titleText = tr("Logs too large to send");
        descText = tr("Could not send logs to Windscribe, they are too big. Either re-send after replicating the issue or manually compressing and sending to support.");
    }
    else if (userWarningType == ProtoTypes::USER_WARNING_VIEW_LOG_FILE_TOO_BIG)
    {
        titleText = tr("Logs too large to view");
        descText = tr("Could not view the logs because they are too big. You may want to try viewing manually.");
    }
    else if (userWarningType == ProtoTypes::USER_WARNING_CHECK_UPDATE_INVALID_PLATFORM)
    {
        if (!alreadyShownWarnings_.contains(userWarningType)) // only show this once per run
        {
            alreadyShownWarnings_.insert(userWarningType);
            titleText = tr("Check for update failed");
            descText = tr("Windscribe could not check for update due to an invalid platform config. You may want to try manually updating your installation.");
        }
    }

    if (titleText != "" || descText != "")
    {
        if (!currentlyShowingUserWarningMessage_)
        {
            currentlyShowingUserWarningMessage_ = true;
            QMessageBox::warning(nullptr, titleText, descText, QMessageBox::Ok);
            currentlyShowingUserWarningMessage_ = false;
        }
    }
}

void MainWindow::onBackendUserWarning(ProtoTypes::UserWarningType userWarningType)
{
    showUserWarning(userWarningType);
}

void MainWindow::onBackendInternetConnectivityChanged(bool connectivity)
{
    mainWindowController_->getConnectWindow()->setInternetConnectivity(connectivity);
    internetConnected_ = connectivity;
}

void MainWindow::onBackendProtocolPortChanged(const ProtoTypes::Protocol &protocol, const uint port)
{
    mainWindowController_->getConnectWindow()->setProtocolPort(protocol, port);
}

void MainWindow::onBackendPacketSizeDetectionStateChanged(bool on, bool isError)
{
    mainWindowController_->getPreferencesWindow()->setPacketSizeDetectionState(on);

    if (!on && isError) {
        const QString title = tr("Detection Error");
        const QString desc = tr("Cannot detect appropriate packet size due to an error. Please try again.");
        mainWindowController_->getPreferencesWindow()->showPacketSizeDetectionError(title, desc);
    }
}

void MainWindow::onBackendUpdateVersionChanged(uint progressPercent, ProtoTypes::UpdateVersionState state, ProtoTypes::UpdateVersionError error)
{
    // qDebug() << "Mainwindow::onBackendUpdateVersionChanged: " << progressPercent << ", " << state;

    if (state == ProtoTypes::UPDATE_VERSION_STATE_DONE)
    {

        if (downloadRunning_) // not cancelled by user
        {
            if (error == ProtoTypes::UPDATE_VERSION_ERROR_NO_ERROR)
            {
                isExitingAfterUpdate_ = true; // the flag for prevent firewall off for some states

                // nothing todo, because installer will close app here
#ifdef Q_OS_LINUX
                // restart the application after update
                doClose(nullptr, false);
                qApp->quit();
                QProcess::startDetached(QApplication::applicationFilePath());
#endif
            }
            else // Error
            {
                mainWindowController_->getUpdateAppItem()->setProgress(0);
                mainWindowController_->getUpdateWindow()->stopAnimation();
                mainWindowController_->getUpdateWindow()->changeToPromptScreen();

                QString titleText = tr("Auto-Update Failed");
                QString descText = tr("Please contact support");
                if (error == ProtoTypes::UPDATE_VERSION_ERROR_DL_FAIL)
                {
                    descText = tr("Please try again using a different network connection.");
                }
                else if (error == ProtoTypes::UPDATE_VERSION_ERROR_SIGN_FAIL)
                {
                    descText = tr("Can't run the downloaded installer. It does not have the correct signature.");
                }
                else if (error == ProtoTypes::UPDATE_VERSION_ERROR_MOUNT_FAIL)
                {
                    descText = tr("Cannot access the installer. Image mounting has failed.");
                }
                else if (error == ProtoTypes::UPDATE_VERSION_ERROR_DMG_HAS_NO_INSTALLER_FAIL)
                {
                    descText = tr("Downloaded image does not contain installer.");
                }
                else if (error == ProtoTypes::UPDATE_VERSION_ERROR_CANNOT_REMOVE_EXISTING_TEMP_INSTALLER_FAIL)
                {
                    descText = tr("Cannot overwrite a pre-existing temporary installer.");
                }
                else if (error == ProtoTypes::UPDATE_VERSION_ERROR_COPY_FAIL)
                {
                    descText = tr("Failed to copy installer to temp location.");
                }
                else if (error == ProtoTypes::UPDATE_VERSION_ERROR_START_INSTALLER_FAIL)
                {
                    descText = tr("Auto-Updater has failed to run installer.");
                }
                else if (error == ProtoTypes::UPDATE_VERSION_ERROR_COMPARE_HASH_FAIL)
                {
                    descText = tr("Cannot run the downloaded installer. It does not have the expected hash.");
                }
                else if (error == ProtoTypes::UPDATE_VERSION_ERROR_API_HASH_INVALID)
                {
                    descText = tr("Windscribe API has returned an invalid hash for downloaded installer. Please contact support.");
                }
                mainWindowController_->getGeneralMessageWindow()->setErrorMode(true);
                mainWindowController_->getGeneralMessageWindow()->setTitle(titleText);
                mainWindowController_->getGeneralMessageWindow()->setDescription(descText);
                bGotoUpdateWindowAfterGeneralMessage_ = true;
                mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_GENERAL_MESSAGE);
            }
        }
        else
        {
            mainWindowController_->getUpdateAppItem()->setProgress(0);
            mainWindowController_->getUpdateWindow()->stopAnimation();
            mainWindowController_->getUpdateWindow()->changeToPromptScreen();
        }
        downloadRunning_ = false;
    }
    else if (state == ProtoTypes::UPDATE_VERSION_STATE_DOWNLOADING)
    {
        // qDebug() << "Running -- updating progress";
        mainWindowController_->getUpdateAppItem()->setProgress(progressPercent);
        mainWindowController_->getUpdateWindow()->setProgress(progressPercent);
    }
    else if (state == ProtoTypes::UPDATE_VERSION_STATE_RUNNING)
    {
        // Send main window center coordinates from the GUI, to position the installer properly.
        const bool is_visible = isVisible() && !isMinimized();
        qint32 center_x = INT_MAX;
        qint32 center_y = INT_MAX;

        if (is_visible)
        {
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
            center_x = geometry().x() + geometry().width() / 2;
            center_y = geometry().y() + geometry().height() / 2;
#elif defined Q_OS_MAC
            MacUtils::getNSWindowCenter((void *)this->winId(), center_x, center_y);
#endif
        }
        backend_->sendUpdateWindowInfo(center_x, center_y);
    }
}

void MainWindow::openBrowserToMyAccountWithToken(const QString &tempSessionToken)
{
    QString getUrl = QString("https://%1/myaccount?temp_session=%2")
                        .arg(HardcodedSettings::instance().serverUrl())
                        .arg(tempSessionToken);
    // qCDebug(LOG_BASIC) << "Opening external link: " << getUrl;
    QDesktopServices::openUrl(QUrl(getUrl));
}

void MainWindow::onBackendWebSessionTokenForEditAccountDetails(const QString &tempSessionToken)
{
    openBrowserToMyAccountWithToken(tempSessionToken);
}

void MainWindow::onBackendWebSessionTokenForAddEmail(const QString &tempSessionToken)
{
    openBrowserToMyAccountWithToken(tempSessionToken);
}

void MainWindow::onBackendEngineCrash()
{
    mainWindowController_->getInitWindow()->startWaitingAnimation();
    mainWindowController_->getInitWindow()->setAdditionalMessage(
        tr("Lost connection to the backend process.\nRecovering..."), /*useSmallFont =*/ false);
    mainWindowController_->getInitWindow()->setCropHeight(0); // Needed so that Init screen is correct height when engine fails from connect window
    mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_INITIALIZATION);
}

void MainWindow::onBackendLocationsUpdated()
{
    const auto currentLocation{ PersistentState::instance().lastLocation() };
    if (!currentLocation.isCustomConfigsLocation())
        return;

    // Update custom config location info, because user could have selected another config path.
    LocationsModel::LocationInfo li;
    backend_->getLocationsModel()->getLocationInfo(currentLocation, li);
    mainWindowController_->getConnectWindow()->updateLocationInfo(
        li.id, li.firstName, li.secondName, li.countryCode, li.pingTime);
}

void MainWindow::onNotificationControllerNewPopupMessage(int messageId)
{
    mainWindowController_->getNewsFeedWindow()->setMessagesWithCurrentOverride(
        notificationsController_.messages(), notificationsController_.shownIds(), messageId);
    mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_NOTIFICATIONS);
}

void MainWindow::onBestLocationChanged(const LocationID &bestLocation)
{
    Q_UNUSED(bestLocation);
    if (PersistentState::instance().lastLocation().isValid() &&  PersistentState::instance().lastLocation().isBestLocation())
    {
        if (backend_->isDisconnected())
        {
            PersistentState::instance().setLastLocation(bestLocation);
        }
        else
        {
            PersistentState::instance().setLastLocation(PersistentState::instance().lastLocation().bestLocationToApiLocation());
        }
        LocationsModel::LocationInfo li;
        if (backend_->getLocationsModel()->getLocationInfo(PersistentState::instance().lastLocation(), li))
        {
            mainWindowController_->getConnectWindow()->updateLocationInfo(li.id, li.firstName, li.secondName, li.countryCode, li.pingTime);
        }
    }
}

void MainWindow::onPreferencesFirewallSettingsChanged(const ProtoTypes::FirewallSettings &fm)
{
    if (fm.mode() == ProtoTypes::FIREWALL_MODE_ALWAYS_ON)
    {
        mainWindowController_->getConnectWindow()->setFirewallAlwaysOn(true);
        if (!PersistentState::instance().isFirewallOn())
        {
            backend_->firewallOn();
        }
    }
    else
    {
        mainWindowController_->getConnectWindow()->setFirewallAlwaysOn(false);
    }
}

void MainWindow::onPreferencesShareProxyGatewayChanged(const ProtoTypes::ShareProxyGateway &sp)
{
    if (sp.is_enabled())
    {
        backend_->startProxySharing(sp.proxy_sharing_mode());
    }
    else
    {
        backend_->stopProxySharing();
    }
}

void MainWindow::onPreferencesShareSecureHotspotChanged(const ProtoTypes::ShareSecureHotspot &ss)
{
    if (ss.is_enabled() && !ss.ssid().empty() && ss.password().length() >= 8)
    {
        mainWindowController_->getBottomInfoWindow()->setSecureHotspotFeatures(true, QString::fromStdString(ss.ssid()));
        backend_->startWifiSharing(QString::fromStdString(ss.ssid()), QString::fromStdString(ss.password()));
    }
    else
    {
        mainWindowController_->getBottomInfoWindow()->setSecureHotspotFeatures(false, QString());
        backend_->stopWifiSharing();
    }
}

void MainWindow::onPreferencesLocationOrderChanged(ProtoTypes::OrderLocationType o)
{
    backend_->getLocationsModel()->setOrderLocationsType(o);
}

void MainWindow::onPreferencesSplitTunnelingChanged(ProtoTypes::SplitTunneling st)
{
    // turn off and disable firewall for Mac when split tunneling is active
#ifdef Q_OS_MAC
    if (st.settings().active())
    {
        if (backend_->isFirewallEnabled())
        {
            backend_->firewallOff();
        }
        ProtoTypes::FirewallSettings firewallSettings;
        firewallSettings.set_mode(ProtoTypes::FIREWALL_MODE_MANUAL);
        backend_->getPreferences()->setFirewallSettings(firewallSettings);
        backend_->getPreferencesHelper()->setBlockFirewall(true);
        mainWindowController_->getConnectWindow()->setFirewallBlock(true);
    }
    else
    {
        backend_->getPreferencesHelper()->setBlockFirewall(false);
        mainWindowController_->getConnectWindow()->setFirewallBlock(false);
    }
#endif
    backend_->sendSplitTunneling(st);
}

// for aggressive (dynamic) signalling of EngineSettings save
void MainWindow::onPreferencesUpdateEngineSettings()
{
	// prevent SetSettings while we are currently receiving from new settigns from engine
	// Issues with initializing certain preferences state (See ApiResolution and App Internal DNS)
    if (!backend_->getPreferences()->isReceivingEngineSettings()) 
    {
        backend_->sendEngineSettingsIfChanged();
    }
}

void MainWindow::onPreferencesLaunchOnStartupChanged(bool bEnabled)
{
    LaunchOnStartup::instance().setLaunchOnStartup(bEnabled);
}

void MainWindow::updateConnectWindowStateProtocolPortDisplay(ProtoTypes::ConnectionSettings connectionSettings)
{
    if (connectionSettings.is_automatic())
    {
#if defined(Q_OS_LINUX)
        mainWindowController_->getConnectWindow()->setProtocolPort(ProtoTypes::PROTOCOL_UDP, 443);
#else
        mainWindowController_->getConnectWindow()->setProtocolPort(ProtoTypes::PROTOCOL_IKEV2, 500);
#endif
    }
    else
    {
        mainWindowController_->getConnectWindow()->setProtocolPort(connectionSettings.protocol(), connectionSettings.port());
    }
}

void MainWindow::onPreferencesConnectionSettingsChanged(ProtoTypes::ConnectionSettings connectionSettings)
{
    if (backend_->isDisconnected())
    {
        updateConnectWindowStateProtocolPortDisplay(connectionSettings);
    }
}

void MainWindow::onPreferencesIsDockedToTrayChanged(bool isDocked)
{
    mainWindowController_->setIsDockedToTray(isDocked);
    bMoveEnabled_ = !isDocked;
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
}

#ifdef Q_OS_MAC
void MainWindow::hideShowDockIcon(bool hideFromDock)
{
    desiredDockIconVisibility_ = !hideFromDock;
    hideShowDockIconTimer_.start(300);
}

const QRect MainWindow::bestGuessForTrayIconRectFromLastScreen(const QPoint &pt)
{
    QRect lastScreenTrayRect = trayIconRectForLastScreen();
    if (lastScreenTrayRect.isValid())
    {
        return lastScreenTrayRect;
    }
    // qDebug() << "No valid history of last screen";
    return trayIconRectForScreenContainingPt(pt);
}

const QRect MainWindow::trayIconRectForLastScreen()
{
    if (lastScreenName_ != "")
    {
        QRect rect = generateTrayIconRectFromHistory(lastScreenName_);
        if (rect.isValid())
        {
            return rect;
        }
    }
    // qDebug() << "No valid last screen";
    return QRect(0,0,0,0); // invalid
}

const QRect MainWindow::trayIconRectForScreenContainingPt(const QPoint &pt)
{
    QScreen *screen = WidgetUtils::slightlySaferScreenAt(pt);
    if (!screen)
    {
        // qCDebug(LOG_BASIC) << "Cannot find screen while determining tray icon";
        return QRect(0,0,0,0);
    }
    return guessTrayIconLocationOnScreen(screen);
}

const QRect MainWindow::generateTrayIconRectFromHistory(const QString &screenName)
{
    if (systemTrayIconRelativeGeoScreenHistory_.contains(screenName))
    {
        // ensure is in current list
        QScreen *screen = WidgetUtils::screenByName(screenName);

        if (screen)
        {

            // const QRect screenGeo = WidgetUtils::smartScreenGeometry(screen);
            const QRect screenGeo = screen->geometry();

            TrayIconRelativeGeometry &rect = systemTrayIconRelativeGeoScreenHistory_[lastScreenName_];
            QRect newIconRect = QRect(screenGeo.x() + rect.x(),
                                      screenGeo.y() + rect.y(),
                                         rect.width(), rect.height());
            return newIconRect;
        }
        //qDebug() << "   No screen by name: " << screenName;
        return QRect(0,0,0,0);
    }
    //qDebug() << "   No history for screen: " << screenName;
    return QRect(0,0,0,0);
}

void MainWindow::onPreferencesHideFromDockChanged(bool hideFromDock)
{
    hideShowDockIcon(hideFromDock);
}

void MainWindow::hideShowDockIconImpl(bool bAllowActivateAndShow)
{
    if (currentDockIconVisibility_ != desiredDockIconVisibility_) {
        currentDockIconVisibility_ = desiredDockIconVisibility_;
        if (currentDockIconVisibility_) {
            MacUtils::showDockIcon();
        }
        else {
            // A call to |hideDockIcon| will hide the window, this is annoying but that's how
            // one hides the dock icon on Mac. If there are any GUI events queued, especially
            // those that are going to show some widgets, it may result in a crash. To avoid it, we
            // pump the message loop here, including user input events.
            qApp->processEvents();
            MacUtils::hideDockIcon();

            if (bAllowActivateAndShow) {
                // Do not attempt to show the window immediately, it may take some time to transform
                // process type in |hideDockIcon|.
                QTimer::singleShot(1, this, [this]() {
                    activateAndShow();
                    setBackendAppActiveState(true);
                });
            }
        }
    }
}
#endif

void MainWindow::activateAndShow()
{
    // qDebug() << "ActivateAndShow()";
#ifdef Q_OS_MAC
    const bool kAllowMoveBetweenSpaces = backend_->getPreferences()->isHideFromDock();
    WidgetUtils_mac::allowMoveBetweenSpacesForWindow(this, kAllowMoveBetweenSpaces);
#endif
    mainWindowController_->updateMainAndViewGeometry(true);
    if (!isVisible() || isMinimized())
        showNormal();
    if (!isActiveWindow())
        activateWindow();
#ifdef Q_OS_MAC
    MacUtils::activateApp();
#endif

    lastWindowStateChange_ = QDateTime::currentMSecsSinceEpoch();
}

void MainWindow::deactivateAndHide()
{
    MainWindowState::instance().setActive(false);
#if defined(Q_OS_MAC)
    hide();
#elif defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    if (backend_->getPreferences()->isDockedToTray())
    {
        setWindowState(Qt::WindowMinimized);
    }
#endif
    cleanupAdvParametersWindow();
    cleanupLogViewerWindow();
    lastWindowStateChange_ = QDateTime::currentMSecsSinceEpoch();
}

void MainWindow::setBackendAppActiveState(bool state)
{
    TooltipController::instance().hideAllTooltips();

    if (backendAppActiveState_ != state) {
        backendAppActiveState_ = state;
        state ? backend_->applicationActivated() : backend_->applicationDeactivated();
    }
}

void MainWindow::toggleVisibilityIfDocked()
{
    // qDebug() << "on dock click Application State: " << qApp->applicationState();

    if (backend_->getPreferences()->isDockedToTray())
    {
        if (isVisible() && activeState_)
        {
            // qDebug() << "dock click deactivate";
            deactivateAndHide();
            setBackendAppActiveState(false);
        }
        else
        {
            // qDebug() << "dock click activate";
            activateAndShow();
            setBackendAppActiveState(true);
        }
    }
}

void MainWindow::onAppActivateFromAnotherInstance()
{
    //qDebug() << "on App activate from another instance";
    activateAndShow();
}

void MainWindow::onAppShouldTerminate_mac()
{
    qCDebug(LOG_BASIC) << "onShouldTerminate_mac signal in MainWindow";
    isSpontaneousCloseEvent_ = true;
    close();
}

void MainWindow::onReceivedOpenLocationsMessage()
{
    activateAndShow();

#ifdef Q_OS_MAC
    // Strange bug on Mac that causes flicker when activateAndShow() is called from a minimized state
    // calling hide() first seems to fix
    hide();
    activateAndShow();
#endif

    if (mainWindowController_->preferencesVisible())
    {
        collapsePreferences();
    }
    else if (mainWindowController_->currentWindow() != MainWindowController::WINDOW_ID_CONNECT)
    {
        mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_CONNECT);
    }

    // TODO: replace this delay with something less hacky
    // There is a race condition when CLI tries to expand the locations
    // from a CLI-spawned-GUI (Win): the location foreground doesn't appear, only the location's shadow
    // from a CLI-spawned-GUI (Mac): could fail Q_ASSERT(curWindow_ == WINDOW_ID_CONNECT) in expandLocations
    QTimer::singleShot(500, [this](){
        mainWindowController_->expandLocations();
        localIpcServer_->sendLocationsShown();
    });
}

void MainWindow::onConnectToLocation(const LocationID &id)
{
    onLocationSelected(id);
}

void MainWindow::onAppCloseRequest()
{
    // The main window could be hidden, e.g. deactivated in docked mode. In this case, trying to
    // close the app using a Dock Icon, will result in a fail, because there is no window to send
    // a closing signal to. Even worse, if the system attempts to close such app during the
    // shutdown, it will block the shutdown entirely. See issue #154 for the details.
    // To deal with the issue, restore main window visibility before further close event
    // propagation. Please note that we shouldn't close the app from this handler, but we'd rather
    // wait for a "spontaneous" system event of other Qt event handlers.
    // Also, there is no need to restore inactive, but visible (e.g. minimized) windows -- they are
    // handled and closed by Qt correctly.
    if (!isVisible())
        activateAndShow();
}

#if defined(Q_OS_WIN)
void MainWindow::onAppWinIniChanged()
{
    bool newDarkMode = InterfaceUtils::isDarkMode();
    if (newDarkMode != isRunningInDarkMode_)
    {
        isRunningInDarkMode_ = newDarkMode;
        qCDebug(LOG_BASIC) << "updating dark mode: " << isRunningInDarkMode_;
        updateTrayIconType(currentAppIconType_);
    }
}
#endif

void MainWindow::showShutdownWindow()
{
    setEnabled(true);
    mainWindowController_->getExitWindow()->setShutdownAnimationMode(true);
}

void MainWindow::onCurrentNetworkUpdated(ProtoTypes::NetworkInterface networkInterface)
{
    mainWindowController_->getConnectWindow()->updateNetworkState(networkInterface);
}

QRect MainWindow::trayIconRect()
{
#if defined(Q_OS_MAC)
    if (trayIcon_.isVisible())
    {
        const QRect rc = trayIcon_.geometry();
        // qDebug() << "System-reported tray icon rect: " << rc;

        // check for valid tray icon
        if (!rc.isValid())
        {
            // qCDebug(LOG_BASIC) << "   Tray icon invalid - check last screen " << rc;
            QRect lastGuess = bestGuessForTrayIconRectFromLastScreen(rc.topLeft());
            if (lastGuess.isValid()) return lastGuess;
            //qDebug() << "Using cached rect: " << savedTrayIconRect_;
            return savedTrayIconRect_;
        }

        // check for valid screen
        QScreen *screen = QGuiApplication::screenAt(rc.center());
        if (!screen)
        {
            // qCDebug(LOG_BASIC) << "No screen at point -- make best guess " << rc;
            QRect bestGuess = trayIconRectForScreenContainingPt(rc.topLeft());
            if (bestGuess.isValid())
            {
                //qDebug() << "Using best guess rect << " << bestGuess;
                return bestGuess;
            }
            //qDebug() << "Using cached rect: " << savedTrayIconRect_;
            return savedTrayIconRect_;
        }

        QRect screenGeo = screen->geometry();

        // valid screen and tray icon -- update the cache
        systemTrayIconRelativeGeoScreenHistory_[screen->name()] = QRect(abs(rc.x() - screenGeo.x()), abs(rc.y() - screenGeo.y()), rc.width(), rc.height());
        lastScreenName_ = screen->name();
        savedTrayIconRect_ = rc;
        // qCDebug(LOG_BASIC) << "Caching: Screen with system-reported tray icon relative geo : " << screen->name() << " " << screenGeo << "; Tray Icon Rect: " << sii.iconRelativeGeo;
        return savedTrayIconRect_;
    }

    // qCDebug(LOG_BASIC) << "Tray Icon not visible -- using last saved TrayIconRect";

#else
    if (trayIcon_.isVisible()) {
        QRect trayIconRect = trayIcon_.geometry();
        if (trayIconRect.isValid()) {
            savedTrayIconRect_ = trayIconRect;
        }
    }
#endif
    return savedTrayIconRect_;
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    // qDebug() << "Tray Activated: " << reason;

    switch (reason)
    {
        case QSystemTrayIcon::Trigger:
        case QSystemTrayIcon::DoubleClick:
        {
            // qDebug() << "Tray triggered";
            deactivationTimer_.stop();
#if defined(Q_OS_WIN)
            if (isMinimized() || !backend_->getPreferences()->isDockedToTray()) {
                activateAndShow();
                setBackendAppActiveState(true);
            } else {
                deactivateAndHide();
                setBackendAppActiveState(false);
            }
            // Fix a nasty tray icon double-click bug in Qt.
            if (reason == QSystemTrayIcon::DoubleClick)
                WidgetUtils_win::fixSystemTrayIconDblClick();
#elif defined(Q_OS_MAC)
            if (backend_->getPreferences()->isDockedToTray())
            {
                toggleVisibilityIfDocked();
            }
#elif defined(Q_OS_LINUX)
            if (backend_->getPreferences()->isDockedToTray())
            {
                toggleVisibilityIfDocked();
            }
            else if (!isVisible()) // closed to tray
            {
                activateAndShow();
                setBackendAppActiveState(true);
            }
#endif
        }
        break;
        default:
        break;
    }
}

void MainWindow::onTrayMenuConnect()
{
    onConnectWindowConnectClick();
}

void MainWindow::onTrayMenuDisconnect()
{
    onConnectWindowConnectClick();
}

void MainWindow::onTrayMenuPreferences()
{
    activateAndShow();
    setBackendAppActiveState(true);
    mainWindowController_->expandPreferences();
}

void MainWindow::onTrayMenuShowHide()
{
    if (isMinimized() || !isVisible())
    {
        activateAndShow();
        setBackendAppActiveState(true);
    }
    else
    {
        deactivateAndHide();
        setBackendAppActiveState(false);
    }
}

void MainWindow::onTrayMenuHelpMe()
{
    QDesktopServices::openUrl(QUrl( QString("https://%1/help").arg(HardcodedSettings::instance().serverUrl())));
}

void MainWindow::onTrayMenuQuit()
{
    doClose();
}

void MainWindow::onFreeTrafficNotification(const QString &message)
{
    trayIcon_.showMessage("Windscribe", message);
}

void MainWindow::onNativeInfoErrorMessage(QString title, QString desc)
{
    QMessageBox::information(nullptr, title, desc, QMessageBox::Ok);
}

void MainWindow::onSplitTunnelingAppsAddButtonClick()
{
    QString filename;
    ShowingDialogState::instance().setCurrentlyShowingExternalDialog(true);
#if defined(Q_OS_WIN)
    QProcess getOpenFileNameProcess;
    QString changeIcsExePath = QCoreApplication::applicationDirPath() + "/ChangeIcs.exe";
    getOpenFileNameProcess.start(changeIcsExePath, { "-browseforapp" }, QIODeviceBase::ReadOnly);
    if (getOpenFileNameProcess.waitForStarted(-1)) {
        const int kRefreshGuiMs = 10;
        do {
            QApplication::processEvents();
            if (getOpenFileNameProcess.waitForFinished(kRefreshGuiMs)) {
                filename = getOpenFileNameProcess.readAll().trimmed();
                if (filename.isEmpty()) {
                    ShowingDialogState::instance().setCurrentlyShowingExternalDialog(false);
                    return;
                }
            }
        } while (getOpenFileNameProcess.state() == QProcess::Running);
    }
#endif  // Q_OS_WIN
    if (filename.isEmpty())
        filename = QFileDialog::getOpenFileName(this, tr("Select an application"), "C:\\");
    ShowingDialogState::instance().setCurrentlyShowingExternalDialog(false);

    if (!filename.isEmpty()) // TODO: validation
    {
        mainWindowController_->getPreferencesWindow()->addApplicationManually(filename);
    }
}

void MainWindow::onRevealConnectStateChanged(bool revealingConnect)
{
    revealingConnectWindow_ = revealingConnect;
    update();
}

void MainWindow::onMainWindowControllerSendServerRatingUp()
{
    backend_->speedRating(1, PersistentState::instance().lastExternalIp());
}

void MainWindow::onMainWindowControllerSendServerRatingDown()
{
    backend_->speedRating(0, PersistentState::instance().lastExternalIp());
}

void MainWindow::loadTrayMenuItems()
{
    if (mainWindowController_->currentWindow() == MainWindowController::WINDOW_ID_CONNECT) // logged in
    {
        if (backend_->currentConnectState() == ProtoTypes::DISCONNECTED)
        {
            trayMenu_.addAction(tr("Connect"), this, SLOT(onTrayMenuConnect()));
        }
        else
        {
            trayMenu_.addAction(tr("Disconnect"), this, SLOT(onTrayMenuDisconnect()));
        }
        trayMenu_.addSeparator();

#ifndef Q_OS_LINUX
        const auto *lm = backend_->getLocationsModel();
        if (lm->getNumGenericLocations() > 0)
            trayMenu_.addMenu(&locationsMenu_[LOCATIONS_TRAY_MENU_TYPE_GENERIC]);
        if (lm->getNumFavoriteLocations() > 0)
            trayMenu_.addMenu(&locationsMenu_[LOCATIONS_TRAY_MENU_TYPE_FAVORITES]);
        if (lm->getNumStaticIPLocations() > 0)
            trayMenu_.addMenu(&locationsMenu_[LOCATIONS_TRAY_MENU_TYPE_STATIC_IPS]);
        if (lm->getNumCustomConfigLocations() > 0)
            trayMenu_.addMenu(&locationsMenu_[LOCATIONS_TRAY_MENU_TYPE_CUSTOM_CONFIGS]);

        trayMenu_.addSeparator();
#endif
    }

#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
    trayMenu_.addAction(tr("Show/Hide"), this, SLOT(onTrayMenuShowHide()));
#endif

    if (!mainWindowController_->preferencesVisible())
    {
        trayMenu_.addAction(tr("Preferences"), this, SLOT(onTrayMenuPreferences()));
    }

    trayMenu_.addAction(tr("Help"), this, SLOT(onTrayMenuHelpMe()));
    trayMenu_.addAction(tr("Exit"), this, SLOT(onTrayMenuQuit()));

#ifndef Q_OS_LINUX
#if !defined(USE_LOCATIONS_TRAY_MENU_NATIVE)
    LocationsTrayMenuScaleManager::instance().setTrayIconGeometry(trayIcon_.geometry());
    for (int i = 0; i < LOCATIONS_TRAY_MENU_NUM_TYPES; ++i) {
        locationsTrayMenuWidget_[i]->setFontForItems(trayMenu_.font());
        // Force geometry update of the menu, because widget size could have been changed.
        // Send the resize event, so it will make the menu to rebuild its size based on items.
        QResizeEvent resizeEvent(locationsTrayMenuWidget_[i]->size(), locationsMenu_[i].size());
        qApp->sendEvent(&locationsMenu_[i], &resizeEvent);
    }
#endif
#endif
}

void MainWindow::onTrayMenuAboutToShow()
{
    // qDebug() << "Tray menu about to show";

    trayMenu_.clear();
#ifdef Q_OS_MAC
    if (!backend_->getPreferences()->isDockedToTray())
    {
        loadTrayMenuItems();
    }
#else
    loadTrayMenuItems();
#endif
}

void MainWindow::onLocationsTrayMenuLocationSelected(int type, QString locationTitle, int cityIndex)
{
    Q_ASSERT(type >= 0 && type < LOCATIONS_TRAY_MENU_NUM_TYPES);

   // close menu
#ifdef Q_OS_WIN
    trayMenu_.close();
#elif !defined(USE_LOCATIONS_TRAY_MENU_NATIVE)
    #ifndef Q_OS_LINUX
        listWidgetAction_[type]->trigger(); // close doesn't work by default on mac
    #endif
#endif

    const LocationsModel *lm = backend_->getLocationsModel();
    if (type != LOCATIONS_TRAY_MENU_TYPE_GENERIC) {
        auto id = (type == LOCATIONS_TRAY_MENU_TYPE_CUSTOM_CONFIGS)
            ? lm->findCustomConfigLocationByTitle(locationTitle)
            : lm->findGenericLocationByTitle(locationTitle);
        if (id.isValid())
            onLocationSelected(id);
        return;
    }

    const auto lmi = lm->getLocationModelItemByTitle(locationTitle);
    if (!lmi) {
        qCDebug(LOG_BASIC) << "Couldn't find city by that region (" << locationTitle << ")";
        return;
    }
    if (lmi->id.isStaticIpsLocation() || lmi->id.isCustomConfigsLocation()) {
        qCDebug(LOG_BASIC) << "StaticIPs/CustomConfig region (" << locationTitle << ")";
        return;
    }

    const bool is_premium = backend_->getSessionStatus().is_premium();
    if (cityIndex >= 0) {
        for (const auto &city : lmi->cities) {
            if (cityIndex--)
                continue;
            if (is_premium || !city.bShowPremiumStarOnly)
                onLocationSelected(city.id);
            break;
        }
        return;
    }

    // Count valid cities.
    int city_count = 0;
    if (is_premium) {
        city_count = lmi->cities.count();
    } else for (const auto &city : lmi->cities) {
        if (!city.bShowPremiumStarOnly)
            ++city_count;
    }
    if (!city_count) {
        qCDebug(LOG_BASIC) << "No valid cities in the region (" << locationTitle << ")";
        return;
    }

    // connect to random node in region
    int number = Utils::generateIntegerRandom(1, city_count);
    qCDebug(LOG_BASIC) << "Connecting to city " << number << "/" << city_count
                       << " in the region (" << locationTitle << ")";
    for (const auto &city : lmi->cities) {
        if (!is_premium && city.bShowPremiumStarOnly)
            continue;
        if (!--number) {
            onLocationSelected(city.id);
            break;
        }
    }
}

void MainWindow::onScaleChanged()
{
    ImageResourcesSvg::instance().clearHashAndStartPreloading();
    ImageResourcesJpg::instance().clearHash();
    FontManager::instance().clearCache();
    mainWindowController_->updateScaling();
    updateTrayIconType(currentAppIconType_);
}

void MainWindow::onDpiScaleManagerNewScreen(QScreen *screen)
{
    Q_UNUSED(screen)
#ifdef Q_OS_MAC
    // There is a bug that causes the app to be drawn in strange locations under the following scenario:
    // On Mac: when laptop lid is closed/opened and app is docked
    // Instead we hide the app because an explicit click will redraw the click correctly and this should be relatively rare
    // Any attempt to remove the bug resulted in only pushing the bug around and this is extremely tedious to test. Fair warning.
    if (backend_->getPreferences()->isDockedToTray())
    {
        // qDebug() << "New scale from DpiScaleManager (docked app) - hide app";
        deactivateAndHide();
    }
#endif
}

void MainWindow::onFocusWindowChanged(QWindow *focusWindow)
{
    // On Windows, there are more top-level windows rather than one, main window. E.g. all the
    // combobox widgets are separate windows. As a result, opening a combobox menu will result in
    // main window having lost the focus. To work around the problem, on Windows, we catch the
    // focus change event. If the |focusWindow| is not null, we're still displaying the application;
    // otherwise, a window of some other application has been activated, and we can hide.
    // On Mac, we apply the fix as well, so that MessageBox/Log Window/etc. won't hide the app
    // window in docked mode. Otherwise, closing the MessageBox/Log Window/etc. will lead to an
    // unwanted app termination.
    const bool kIsTrayIconClicked = trayIconRect().contains(QCursor::pos());
    if (!focusWindow && !kIsTrayIconClicked && !ShowingDialogState::instance().isCurrentlyShowingExternalDialog() && !logViewerWindow_) {
        if (backend_->getPreferences()->isDockedToTray()) {
            const int kDeactivationDelayMs = 100;
            deactivationTimer_.start(kDeactivationDelayMs);
        }
    } else {
        deactivationTimer_.stop();
    }
}

void MainWindow::onWindowDeactivateAndHideImpl()
{
    deactivateAndHide();
}

void MainWindow::onAdvancedParametersOkClick()
{
    const QString text = advParametersWindow_->advancedParametersText();
    backend_->getPreferences()->setDebugAdvancedParameters(text);
    cleanupAdvParametersWindow();
}

void MainWindow::onAdvancedParametersCancelClick()
{
    cleanupAdvParametersWindow();
}

void MainWindow::onLanguageChanged()
{
    /*exitWindow_->setTitle(tr(CLOSING_WINDSCRIBE.toStdString().c_str()));
    exitWindow_->setAcceptText(tr(CLOSE_ACCEPT.toStdString().c_str()));
    exitWindow_->setRejectText(tr(CLOSE_REJECT.toStdString().c_str()));*/
}

void MainWindow::hideSupplementaryWidgets()
{
    /*ShareSecureHotspot ss = backend_.getShareSecureHotspot();
    ss.isEnabled = false;
    backend_.setShareSecureHotspot(ss);

    ShareProxyGateway sp = backend_.getShareProxyGateway();
    sp.isEnabled = false;
    backend_.setShareProxyGateway(sp);

    UpgradeModeType um = backend_.getUpgradeMode();
    um.isNeedUpgrade = false;
    um.gbLeft = 5;
    um.gbMax = 10;
    backend_.setUpgradeMode(um);

    bottomInfoWindow_->setExtConfigMode(false);
    updateBottomInfoWindowVisibilityAndPos();
    updateExpandAnimationParameters();

    updateAppItem_->getGraphicsObject()->hide();
    shadowManager_->removeObject(ShadowManager::SHAPE_ID_UPDATE_WIDGET);

    invalidateShadow();*/
}

void MainWindow::backToLoginWithErrorMessage(ILoginWindow::ERROR_MESSAGE_TYPE errorMessageType, const QString &errorMessage)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    setEnabled(false);
    signOutMessageType_ = errorMessageType;
    signOutReason_ = SIGN_OUT_WITH_MESSAGE;
    signOutErrorMessage_ = errorMessage;
    backend_->signOut(false);
}

void MainWindow::setupTrayIcon()
{
    updateTrayTooltip(tr("Disconnected") + "\n" + PersistentState::instance().lastExternalIp());

    trayIcon_.setContextMenu(&trayMenu_);
    connect(&trayMenu_, SIGNAL(aboutToShow()), SLOT(onTrayMenuAboutToShow()));

#ifndef Q_OS_LINUX
    const QString kLocationTrayMenuNames[] = {
      tr("Locations"),   // LOCATIONS_TRAY_MENU_TYPE_GENERIC
      tr("Favourites"),  // LOCATIONS_TRAY_MENU_TYPE_FAVORITES
      tr("Static IPs"),  // LOCATIONS_TRAY_MENU_TYPE_STATIC_IPS
      tr("Configured"),  // LOCATIONS_TRAY_MENU_TYPE_CUSTOM_CONFIGS
    };

    for (int i = 0; i < LOCATIONS_TRAY_MENU_NUM_TYPES; ++i) {
        locationsMenu_[i].setTitle(kLocationTrayMenuNames[i]);
#if defined(USE_LOCATIONS_TRAY_MENU_NATIVE)
        locationsMenu_[i].setMenuType(static_cast<LocationsTrayMenuType>(i));
        locationsMenu_[i].setLocationsModel(backend_->getLocationsModel());
        connect(&locationsMenu_[i], SIGNAL(locationSelected(int, QString, int)),
            SLOT(onLocationsTrayMenuLocationSelected(int, QString, int)));
#else  // USE_LOCATIONS_TRAY_MENU_NATIVE
        locationsTrayMenuWidget_[i] = new LocationsTrayMenuWidget(
            static_cast<LocationsTrayMenuType>(i), &locationsMenu_[i]);
        locationsTrayMenuWidget_[i]->setLocationsModel(backend_->getLocationsModel());
        listWidgetAction_[i] = new QWidgetAction(&locationsMenu_[i]);
        listWidgetAction_[i]->setDefaultWidget(locationsTrayMenuWidget_[i]);
        locationsMenu_[i].addAction(listWidgetAction_[i]);
        connect(locationsTrayMenuWidget_[i], SIGNAL(locationSelected(int, QString, int)),
            SLOT(onLocationsTrayMenuLocationSelected(int, QString, int)));
#endif  // USE_LOCATIONS_TRAY_MENU_NATIVE
    }

#endif

    updateAppIconType(AppIconType::DISCONNECTED);
    updateTrayIconType(AppIconType::DISCONNECTED);
    trayIcon_.show();

    connect(&trayIcon_, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            SLOT( onTrayActivated(QSystemTrayIcon::ActivationReason) ));
}

QString MainWindow::getConnectionTime()
{
    if (connectionElapsedTimer_.isValid())
    {
        const auto totalSeconds = connectionElapsedTimer_.elapsed() / 1000;
        const auto hours = totalSeconds / 3600;
        const auto minutes = (totalSeconds - hours * 3600) / 60;
        const auto seconds = (totalSeconds - hours * 3600) % 60;

        return QString("%1:%2:%3")
                .arg(hours   < 10 ? QString("0%1").arg(hours)   : QString::number(hours))
                .arg(minutes < 10 ? QString("0%1").arg(minutes) : QString::number(minutes))
                .arg(seconds < 10 ? QString("0%1").arg(seconds) : QString::number(seconds));
    }
    else
    {
        return "";
    }
}

QString MainWindow::getConnectionTransferred()
{
    return Utils::humanReadableByteCount(bytesTransferred_, true);
}

void MainWindow::setInitialFirewallState()
{
    bool bFirewallStateOn = PersistentState::instance().isFirewallOn();
    qCDebug(LOG_BASIC) << "Firewall state from last app start:" << bFirewallStateOn;

    if (bFirewallStateOn)
    {
        backend_->firewallOn();
        if (backend_->getPreferences()->firewalSettings().mode() == ProtoTypes::FIREWALL_MODE_ALWAYS_ON)
        {
            mainWindowController_->getConnectWindow()->setFirewallAlwaysOn(true);
        }
    }
    else
    {
        if (backend_->getPreferences()->firewalSettings().mode() == ProtoTypes::FIREWALL_MODE_ALWAYS_ON)
        {
            backend_->firewallOn();
            mainWindowController_->getConnectWindow()->setFirewallAlwaysOn(true);
        }
        else
        {
            backend_->firewallOff();
        }
    }
}

void MainWindow::handleDisconnectWithError(const ProtoTypes::ConnectState &connectState)
{
    Q_ASSERT(connectState.disconnect_reason() == ProtoTypes::DISCONNECTED_WITH_ERROR);

    QString msg;
    if (connectState.connect_error() == ProtoTypes::NO_OPENVPN_SOCKET)
    {
        msg = tr("Can't connect to openvpn process.");
    }
    else if (connectState.connect_error() == ProtoTypes::CANT_RUN_OPENVPN)
    {
        msg = tr("Can't start openvpn process.");
    }
    else if (connectState.connect_error() == ProtoTypes::COULD_NOT_FETCH_CREDENTAILS)
    {
         msg = tr("Couldn't fetch server credentials. Please try again later.");
    }
    else if (connectState.connect_error() == ProtoTypes::LOCATION_NOT_EXIST || connectState.connect_error() == ProtoTypes::LOCATION_NO_ACTIVE_NODES)
    {
        if (!PersistentState::instance().lastLocation().isBestLocation())
        {
            qCDebug(LOG_BASIC) << "Location not exist or no active nodes, try connect to best location";
            const LocationID bestLocation{backend_->getLocationsModel()->getBestLocationId()};
            if (bestLocation.isValid()) {
                PersistentState::instance().setLastLocation(bestLocation);
                LocationsModel::LocationInfo li;
                if (backend_->getLocationsModel()->getLocationInfo(PersistentState::instance().lastLocation(), li))
                {
                    mainWindowController_->getConnectWindow()->updateLocationInfo(li.id, li.firstName, li.secondName, li.countryCode, li.pingTime);
                }
                onConnectWindowConnectClick();
            }
        }
        else
        {
            qCDebug(LOG_BASIC) << "Best Location not exist or no active nodes, goto disconnected mode";
        }
        return;
    }
    else if (connectState.connect_error() == ProtoTypes::ALL_TAP_IN_USE)
    {
        msg = tr("All TAP-Windows adapters on this system are currently in use.");
    }
    else if (connectState.connect_error() == ProtoTypes::IKEV_FAILED_SET_ENTRY_WIN || connectState.connect_error() == ProtoTypes::IKEV_NOT_FOUND_WIN)
    {
        msg = tr("IKEv2 connection failed. Please send a debug log and open a support ticket. You can switch to UDP or TCP connection modes in the mean time.");
    }
    else if (connectState.connect_error() == ProtoTypes::IKEV_FAILED_MODIFY_HOSTS_WIN)
    {
        QMessageBox msgBox;
        const auto* yesButton = msgBox.addButton(tr("Fix Issue"), QMessageBox::YesRole);
        msgBox.addButton(tr("Cancel"), QMessageBox::NoRole);
        msgBox.setWindowTitle(QApplication::applicationName());
        msgBox.setText(tr("Your hosts file is read-only. IKEv2 connectivity requires for it to be writable. Fix the issue automatically?"));
        msgBox.exec();
        if(msgBox.clickedButton() == yesButton) {
            if(backend_) {
                backend_->sendMakeHostsFilesWritableWin();
            }
        }
        return;
    }
    else if (connectState.connect_error() == ProtoTypes::IKEV_NETWORK_EXTENSION_NOT_FOUND_MAC)
    {
        msg = tr("Failed to load the network extension framework.");
    }
    else if (connectState.connect_error() == ProtoTypes::IKEV_FAILED_SET_KEYCHAIN_MAC)
    {
        msg = tr("Failed set password to keychain.");
    }
    else if (connectState.connect_error() == ProtoTypes::IKEV_FAILED_START_MAC)
    {
        msg = tr("Failed to start IKEv2 connection.");
    }
    else if (connectState.connect_error() == ProtoTypes::IKEV_FAILED_LOAD_PREFERENCES_MAC)
    {
        msg = tr("Failed to load IKEv2 preferences.");
    }
    else if (connectState.connect_error() == ProtoTypes::IKEV_FAILED_SAVE_PREFERENCES_MAC)
    {
        msg = tr("Failed to create IKEv2 Profile. Please connect again and select \"Allow\".");
    }
    else if (connectState.connect_error() == ProtoTypes::WIREGUARD_CONNECTION_ERROR)
    {
        msg = tr("Failed to setup WireGuard connection.");
    }
#ifdef Q_OS_WIN
    else if (connectState.connect_error() == ProtoTypes::NO_INSTALLED_TUN_TAP)
    {
       /* msg = tr("There are no TAP adapters installed. Attempt to install now?");
        //activateAndSetSkipFocus();
        if (QMessageBox::information(nullptr, QApplication::applicationName(), msg, QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
        {
            qCDebug(LOG_USER) << "User selected install TAP-drivers now";
            qCDebug(LOG_BASIC) << "Trying install new TAP-driver";
            backend_->installTapAdapter(ProtoTypes::TAP_ADAPTER_6, true);
        }
        else
        {
            qCDebug(LOG_USER) << "User cancel attempt to install TAP-drivers";
        }*/
        return;
    }
#endif
    else if (connectState.connect_error() == ProtoTypes::CONNECTION_BLOCKED)
    {
        if (blockConnect_.isBlockedExceedTraffic()) {
            mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_UPGRADE);
            return;
        }
        msg = blockConnect_.message();
    }
    else if (connectState.connect_error() == ProtoTypes::CANNOT_OPEN_CUSTOM_CONFIG)
    {
        const LocationID bestLocation{backend_->getLocationsModel()->getBestLocationId()};
        if (bestLocation.isValid()) {
            PersistentState::instance().setLastLocation(bestLocation);
            LocationsModel::LocationInfo li;
            if (backend_->getLocationsModel()->getLocationInfo(
                PersistentState::instance().lastLocation(), li)) {
                mainWindowController_->getConnectWindow()->updateLocationInfo(
                    li.id, li.firstName, li.secondName, li.countryCode, li.pingTime);
            }
        }

        msg = tr("Failed to setup custom openvpn configuration.");
    }
    else if(connectState.connect_error() == ProtoTypes::WINTUN_DRIVER_REINSTALLATION_ERROR)
    {
        msg = tr("Wintun driver fatal error. Failed to reinstall it automatically. Please try to reinstall it manually.");
    }
    else if(connectState.connect_error() == ProtoTypes::TAP_DRIVER_REINSTALLATION_ERROR)
    {
        msg = tr("Tap driver Fatal error. Failed to reinstall it automatically. Please try to reinstall it manually.");
    }
    else if (connectState.connect_error() == ProtoTypes::EXE_VERIFY_WSTUNNEL_ERROR)
    {
        msg = tr("WSTunnel binary failed verification. Please re-install windscribe from trusted source.");
    }
    else if (connectState.connect_error() == ProtoTypes::EXE_VERIFY_STUNNEL_ERROR)
    {
        msg = tr("STunnel binary failed verification. Please re-install windscribe from trusted source.");
    }
    else if (connectState.connect_error() == ProtoTypes::EXE_VERIFY_WIREGUARD_ERROR)
    {
        msg = tr("Wireguard binary failed verification. Please re-install windscribe from trusted source.");
    }
    else if (connectState.connect_error() == ProtoTypes::EXE_VERIFY_OPENVPN_ERROR)
    {
        msg = tr("OpenVPN binary failed verification. Please re-install windscribe from trusted source.");
    }
    else
    {
         msg = tr("Error during connection (%1)").arg(QString::number(connectState.connect_error()));
    }

    QMessageBox::information(nullptr, QApplication::applicationName(), msg);
}

void MainWindow::setVariablesToInitState()
{
    signOutReason_ = SIGN_OUT_UNDEFINED;
    isLoginOkAndConnectWindowVisible_ = false;
    bNotificationConnectedShowed_ = false;
    bytesTransferred_ = 0;
    bDisconnectFromTrafficExceed_ = false;
    isPrevSessionStatusInitialized_ = false;
    backend_->getPreferencesHelper()->setIsExternalConfigMode(false);
}

void MainWindow::openStaticIpExternalWindow()
{
    QDesktopServices::openUrl(QUrl( QString("https://%1/staticips?cpid=app_windows").arg(HardcodedSettings::instance().serverUrl())));
}

void MainWindow::openUpgradeExternalWindow()
{
    QDesktopServices::openUrl(QUrl( QString("https://%1/upgrade?pcpid=desktop_upgrade").arg(HardcodedSettings::instance().serverUrl())));
}

void MainWindow::gotoLoginWindow()
{
    mainWindowController_->getLoginWindow()->setFirewallTurnOffButtonVisibility(backend_->isFirewallEnabled());
    mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_LOGIN);
}

void MainWindow::gotoExitWindow()
{
    if (mainWindowController_->currentWindow() == MainWindowController::WINDOW_ID_EXIT)
        return;
    isExitingFromPreferences_ = mainWindowController_->preferencesVisible();
    if (isExitingFromPreferences_)
        collapsePreferences();
    mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_EXIT);
}

void MainWindow::collapsePreferences()
{
    mainWindowController_->getLoginWindow()->setFirewallTurnOffButtonVisibility(
        backend_->isFirewallEnabled());
    mainWindowController_->collapsePreferences();
}

void MainWindow::updateAppIconType(AppIconType type)
{
    if (currentAppIconType_ == type)
        return;

    const QIcon *icon = nullptr;
    switch (type) {
    case AppIconType::DISCONNECTED:
        icon = IconManager::instance().getDisconnectedIcon();
        break;
    case AppIconType::CONNECTING:
        icon = IconManager::instance().getConnectingIcon();
        break;
    case AppIconType::CONNECTED:
        icon = IconManager::instance().getConnectedIcon();
        break;
    default:
        break;
    }
    if (icon)
        qApp->setWindowIcon(*icon);
    currentAppIconType_ = type;
}

void MainWindow::updateTrayIconType(AppIconType type)
{
    const QIcon *icon = nullptr;
    switch (type) {
    case AppIconType::DISCONNECTED:
        icon = IconManager::instance().getDisconnectedTrayIcon(isRunningInDarkMode_);
        break;
    case AppIconType::CONNECTING:
        icon = IconManager::instance().getConnectingTrayIcon(isRunningInDarkMode_);
        break;
    case AppIconType::CONNECTED:
        icon = IconManager::instance().getConnectedTrayIcon(isRunningInDarkMode_);
        break;
    default:
        break;
    }

     if (icon) {
         // We must call setIcon so calls to QSystemTrayIcon::showMessage will use the
         // correct icon.  Otherwise, the singleShot call below may cause showMessage
         // to pick up the old icon.
         trayIcon_.setIcon(*icon);
#if defined(Q_OS_WIN)
         const QPixmap pm = icon->pixmap(QSize(16, 16) * G_SCALE);
         if (!pm.isNull()) {
             QTimer::singleShot(1, [pm]() {
                 WidgetUtils_win::updateSystemTrayIcon(pm, QString());
             });
         }
#endif
    }
}

void MainWindow::updateTrayTooltip(QString tooltip)
{
#if defined(Q_OS_WIN)
    WidgetUtils_win::updateSystemTrayIcon(QPixmap(), std::move(tooltip));
#else
    trayIcon_.setToolTip(tooltip);
#endif
}

void MainWindow::onWireGuardAtKeyLimit()
{
    int result = QMessageBox::warning(g_mainWindow, tr("Windscribe"),
        tr("You have reached your limit of WireGuard public keys. Do you want to delete your oldest key?"),
        QMessageBox::Ok | QMessageBox::Cancel);

    Q_EMIT wireGuardKeyLimitUserResponse(result == QMessageBox::Ok);
}
