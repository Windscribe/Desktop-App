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

#if defined(Q_OS_LINUX)
#include <QtDBus/QtDBus>
#endif

#include "application/windscribeapplication.h"
#include "commongraphics/commongraphics.h"
#include "backend/persistentstate.h"

#include "utils/ws_assert.h"
#include "utils/extraconfig.h"
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

#if defined(Q_OS_WIN)
    #include "utils/winutils.h"
    #include "widgetutils/widgetutils_win.h"
    #include <windows.h>
#elif defined(Q_OS_LINUX)
    #include "utils/authchecker_linux.h"
#else
    #include "utils/macutils.h"
    #include "widgetutils/widgetutils_mac.h"
    #include "utils/authchecker_mac.h"
#endif
#include "widgetutils/widgetutils.h"

QWidget *g_mainWindow = NULL;

MainWindow::MainWindow() :
    QWidget(nullptr),
    backend_(NULL),
    logViewerWindow_(nullptr),
    advParametersWindow_(nullptr),
    currentAppIconType_(AppIconType::DISCONNECTED),
    trayIcon_(),
    bNotificationConnectedShowed_(false),
    bytesTransferred_(0),
    bMousePressed_(false),
    bMoveEnabled_(true),
    signOutReason_(SIGN_OUT_UNDEFINED),
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
    ignoreUpdateUntilNextRun_(false),
    userProtocolOverride_(false),
    tunnelTestMsgBox_(nullptr)
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
#ifdef Q_OS_MAC
    // Work around https://bugreports.qt.io/browse/QTBUG-107008 by delaying showing trayIcon_.
    QTimer::singleShot(0, this, [this] {
        trayIcon_.show();
    });
    const QRect desktopScreenRc = screen->geometry();
    if (desktopScreenRc.top() != desktopAvailableRc.top()) {
        while (trayIcon_.geometry().isEmpty())
            qApp->processEvents();
    }
#else
    trayIcon_.show();
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
    connect(freeTrafficNotificationController_, &FreeTrafficNotificationController::freeTrafficNotification, this, &MainWindow::onFreeTrafficNotification);

    unsigned long guiPid = Utils::getCurrentPid();
    qCDebug(LOG_BASIC) << "GUI pid: " << guiPid;
    backend_ = new Backend(this);

#ifdef Q_OS_MAC
    WidgetUtils_mac::allowMinimizeForFramelessWindow(this);
#endif

    connect(backend_, &Backend::initFinished, this, &MainWindow::onBackendInitFinished);
    connect(backend_, &Backend::initTooLong, this, &MainWindow::onBackendInitTooLong);
    connect(backend_, &Backend::loginFinished, this, &MainWindow::onBackendLoginFinished);
    connect(backend_, &Backend::tryingBackupEndpoint, this, &MainWindow::onBackendTryingBackupEndpoint);
    connect(backend_, &Backend::loginError, this, &MainWindow::onBackendLoginError);
    connect(backend_, &Backend::signOutFinished, this, &MainWindow::onBackendSignOutFinished);
    connect(backend_, &Backend::sessionStatusChanged, this, &MainWindow::onBackendSessionStatusChanged);
    connect(backend_, &Backend::checkUpdateChanged, this, &MainWindow::onBackendCheckUpdateChanged);
    connect(backend_, &Backend::myIpChanged, this, &MainWindow::onBackendMyIpChanged);
    connect(backend_, &Backend::connectStateChanged, this, &MainWindow::onBackendConnectStateChanged);
    connect(backend_, &Backend::emergencyConnectStateChanged, this, &MainWindow::onBackendEmergencyConnectStateChanged);
    connect(backend_, &Backend::firewallStateChanged, this, &MainWindow::onBackendFirewallStateChanged);
    connect(backend_, &Backend::confirmEmailResult, this, &MainWindow::onBackendConfirmEmailResult);
    connect(backend_, &Backend::debugLogResult, this, &MainWindow::onBackendDebugLogResult);
    connect(backend_, &Backend::networkChanged, this, &MainWindow::onNetworkChanged);
    connect(backend_, &Backend::splitTunnelingStateChanged, this, &MainWindow::onSplitTunnelingStateChanged);
    connect(backend_, &Backend::statisticsUpdated, this, &MainWindow::onBackendStatisticsUpdated);
    connect(backend_, &Backend::requestCustomOvpnConfigCredentials, this, &MainWindow::onBackendRequestCustomOvpnConfigCredentials);
    connect(backend_, &Backend::proxySharingInfoChanged, this, &MainWindow::onBackendProxySharingInfoChanged);
    connect(backend_, &Backend::wifiSharingInfoChanged, this, &MainWindow::onBackendWifiSharingInfoChanged);
    connect(backend_, &Backend::cleanupFinished, this, &MainWindow::onBackendCleanupFinished);
    connect(backend_, &Backend::gotoCustomOvpnConfigModeFinished, this, &MainWindow::onBackendGotoCustomOvpnConfigModeFinished);
    connect(backend_, &Backend::sessionDeleted, this, &MainWindow::onBackendSessionDeleted);
    connect(backend_, &Backend::testTunnelResult, this, &MainWindow::onBackendTestTunnelResult);
    connect(backend_, &Backend::lostConnectionToHelper, this, &MainWindow::onBackendLostConnectionToHelper);
    connect(backend_, &Backend::highCpuUsage, this, &MainWindow::onBackendHighCpuUsage);
    connect(backend_, &Backend::userWarning, this, &MainWindow::onBackendUserWarning);
    connect(backend_, &Backend::internetConnectivityChanged, this, &MainWindow::onBackendInternetConnectivityChanged);
    connect(backend_, &Backend::protocolPortChanged, this, &MainWindow::onBackendProtocolPortChanged);
    connect(backend_, &Backend::packetSizeDetectionStateChanged, this, &MainWindow::onBackendPacketSizeDetectionStateChanged);
    connect(backend_, &Backend::updateVersionChanged, this, &MainWindow::onBackendUpdateVersionChanged);
    connect(backend_, &Backend::webSessionTokenForManageAccount, this, &MainWindow::onBackendWebSessionTokenForManageAccount);
    connect(backend_, &Backend::webSessionTokenForAddEmail, this, &MainWindow::onBackendWebSessionTokenForAddEmail);
    connect(backend_, &Backend::webSessionTokenForManageRobertRules, this, &MainWindow::onBackendWebSessionTokenForManageRobertRules);
    connect(backend_, &Backend::engineCrash, this, &MainWindow::onBackendEngineCrash);
    connect(backend_, &Backend::wireGuardAtKeyLimit, this, &MainWindow::onWireGuardAtKeyLimit);
    connect(backend_, &Backend::robertFiltersChanged, this, &MainWindow::onBackendRobertFiltersChanged);
    connect(backend_, &Backend::setRobertFilterResult, this, &MainWindow::onBackendSetRobertFilterResult);
    connect(backend_, &Backend::protocolStatusChanged, this, &MainWindow::onBackendProtocolStatusChanged);
    connect(backend_, &Backend::helperSplitTunnelingStartFailed, this, &MainWindow::onHelperSplitTunnelingStartFailed);
    notificationsController_.connect(backend_, &Backend::notificationsChanged, &notificationsController_, &NotificationsController::updateNotifications);
    connect(this, &MainWindow::wireGuardKeyLimitUserResponse, backend_, &Backend::wireGuardKeyLimitUserResponse);

    locationsWindow_ = new LocationsWindow(this, backend_->getPreferences(), backend_->locationsModelManager());
    connect(locationsWindow_, &LocationsWindow::selected, this, &MainWindow::onLocationSelected);
    connect(locationsWindow_, &LocationsWindow::clickedOnPremiumStarCity, this, &MainWindow::onClickedOnPremiumStarCity);
    connect(locationsWindow_, &LocationsWindow::addStaticIpClicked, this, &MainWindow::onLocationsAddStaticIpClicked);
    connect(locationsWindow_, &LocationsWindow::clearCustomConfigClicked, this, &MainWindow::onLocationsClearCustomConfigClicked);
    connect(locationsWindow_, &LocationsWindow::addCustomConfigClicked, this, &MainWindow::onLocationsAddCustomConfigClicked);

    locationsWindow_->setLatencyDisplay(backend_->getPreferences()->latencyDisplay());
    locationsWindow_->connect(backend_->getPreferences(), &Preferences::latencyDisplayChanged, locationsWindow_, &LocationsWindow::setLatencyDisplay);
    locationsWindow_->setShowLocationLoad(backend_->getPreferences()->isShowLocationLoad());
    connect(backend_->getPreferences(), &Preferences::showLocationLoadChanged, locationsWindow_, &LocationsWindow::setShowLocationLoad);
    connect(backend_->getPreferences(), &Preferences::isAutoConnectChanged, this, &MainWindow::onAutoConnectUpdated);

    localIpcServer_ = new LocalIPCServer(backend_, this);
    connect(localIpcServer_, &LocalIPCServer::showLocations, this, &MainWindow::onReceivedOpenLocationsMessage);
    connect(localIpcServer_, &LocalIPCServer::connectToLocation, this, &MainWindow::onConnectToLocation);
    connect(localIpcServer_, &LocalIPCServer::attemptLogin, this, &MainWindow::onLoginClick);

    mainWindowController_ = new MainWindowController(this, locationsWindow_, backend_->getPreferencesHelper(), backend_->getPreferences(), backend_->getAccountInfo());

    mainWindowController_->getConnectWindow()->updateMyIp(PersistentState::instance().lastExternalIp());
    mainWindowController_->getConnectWindow()->updateNotificationsState(notificationsController_.totalMessages(), notificationsController_.unreadMessages());
    dynamic_cast<QObject*>(mainWindowController_->getConnectWindow())->connect(&notificationsController_, SIGNAL(stateChanged(int, int)), SLOT(updateNotificationsState(int, int)));

    connect(&notificationsController_, &NotificationsController::newPopupMessage, this, &MainWindow::onNotificationControllerNewPopupMessage);

    connect(dynamic_cast<QObject*>(mainWindowController_->getNewsFeedWindow()), SIGNAL(messageRead(qint64)),
            &notificationsController_, SLOT(setNotificationRead(qint64)));
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
    connect(dynamic_cast<QObject*>(mainWindowController_->getConnectWindow()), SIGNAL(protocolsClick()), SLOT(onConnectWindowProtocolsClick()));

    dynamic_cast<QObject*>(mainWindowController_->getConnectWindow())->connect(dynamic_cast<QObject*>(backend_), SIGNAL(firewallStateChanged(bool)), SLOT(updateFirewallState(bool)));

    // news feed window signals
    connect(dynamic_cast<QObject*>(mainWindowController_->getNewsFeedWindow()), SIGNAL(escape()), SLOT(onEscapeNotificationsClick()));
    // protocols window signals
    connect(dynamic_cast<QObject*>(mainWindowController_->getProtocolWindow()), SIGNAL(escape()), SLOT(onEscapeProtocolsClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getProtocolWindow()), SIGNAL(protocolClicked(types::Protocol, uint)), SLOT(onProtocolWindowProtocolClick(types::Protocol, uint)));
    connect(dynamic_cast<QObject*>(mainWindowController_->getProtocolWindow()), SIGNAL(setAsPreferredProtocol(types::ConnectionSettings)), SLOT(onProtocolWindowSetAsPreferred(types::ConnectionSettings)));
    connect(dynamic_cast<QObject*>(mainWindowController_->getProtocolWindow()), SIGNAL(sendDebugLog()), SLOT(onSendDebugLogClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getProtocolWindow()), SIGNAL(stopConnection()), SLOT(onProtocolWindowDisconnect()));

    // preferences window signals
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(quitAppClick()), SLOT(onPreferencesQuitAppClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(escape()), SLOT(onPreferencesEscapeClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(signOutClick()), SLOT(onPreferencesSignOutClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(loginClick()), SLOT(onPreferencesLoginClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(viewLogClick()), SLOT(onPreferencesViewLogClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(advancedParametersClicked()), SLOT(onPreferencesAdvancedParametersClicked()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(currentNetworkUpdated(types::NetworkInterface)), SLOT(onCurrentNetworkUpdated(types::NetworkInterface)));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(sendConfirmEmailClick()), SLOT(onPreferencesSendConfirmEmailClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(sendDebugLogClick()), SLOT(onSendDebugLogClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(manageAccountClick()), SLOT(onPreferencesManageAccountClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(addEmailButtonClick()), SLOT(onPreferencesAddEmailButtonClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(manageRobertRulesClick()), SLOT(onPreferencesManageRobertRulesClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(accountLoginClick()), SLOT(onPreferencesAccountLoginClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(cycleMacAddressClick()), SLOT(onPreferencesCycleMacAddressClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(detectPacketSizeClick()), SLOT(onPreferencesWindowDetectPacketSizeClick()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(getRobertFilters()), SLOT(onPreferencesGetRobertFilters()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(setRobertFilter(types::RobertFilter)), SLOT(onPreferencesSetRobertFilter(types::RobertFilter)));
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
    connect(dynamic_cast<QObject*>(mainWindowController_->getGeneralMessageWindow()), SIGNAL(rejectClick()), SLOT(onGeneralMessageWindowAccept()));

    // logout & exit window signals
    connect(dynamic_cast<QObject*>(mainWindowController_->getLogoutWindow()), SIGNAL(acceptClick()), SLOT(onLogoutWindowAccept()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getLogoutWindow()), SIGNAL(rejectClick()), SLOT(onExitWindowReject()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getExitWindow()), SIGNAL(acceptClick()), SLOT(onExitWindowAccept()));
    connect(dynamic_cast<QObject*>(mainWindowController_->getExitWindow()), SIGNAL(rejectClick()), SLOT(onExitWindowReject()));

    connect(dynamic_cast<QObject*>(mainWindowController_->getPreferencesWindow()), SIGNAL(splitTunnelingAppsAddButtonClick()), SLOT(onSplitTunnelingAppsAddButtonClick()));

    connect(mainWindowController_, &MainWindowController::sendServerRatingUp, this, &MainWindow::onMainWindowControllerSendServerRatingUp);
    connect(mainWindowController_, &MainWindowController::sendServerRatingDown, this, &MainWindow::onMainWindowControllerSendServerRatingDown);
    connect(mainWindowController_, &MainWindowController::preferencesCollapsed, this, &MainWindow::onPreferencesCollapsed);

    // preferences changes signals
    connect(backend_->getPreferences(), &Preferences::firewallSettingsChanged, this, &MainWindow::onPreferencesFirewallSettingsChanged);
    connect(backend_->getPreferences(), &Preferences::shareProxyGatewayChanged, this, &MainWindow::onPreferencesShareProxyGatewayChanged);
    connect(backend_->getPreferences(), &Preferences::shareSecureHotspotChanged, this, &MainWindow::onPreferencesShareSecureHotspotChanged);
    connect(backend_->getPreferences(), &Preferences::locationOrderChanged, this, &MainWindow::onPreferencesLocationOrderChanged);
    connect(backend_->getPreferences(), &Preferences::splitTunnelingChanged, this, &MainWindow::onPreferencesSplitTunnelingChanged);
    connect(backend_->getPreferences(), &Preferences::isAllowLanTrafficChanged, this, &MainWindow::onPreferencesAllowLanTrafficChanged);
    connect(backend_->getPreferences(), &Preferences::isLaunchOnStartupChanged, this, &MainWindow::onPreferencesLaunchOnStartupChanged);
    connect(backend_->getPreferences(), &Preferences::connectionSettingsChanged, this, &MainWindow::onPreferencesConnectionSettingsChanged);
    connect(backend_->getPreferences(), &Preferences::networkPreferredProtocolsChanged, this, &MainWindow::onPreferencesNetworkPreferredProtocolsChanged);
    connect(backend_->getPreferences(), &Preferences::isDockedToTrayChanged, this, &MainWindow::onPreferencesIsDockedToTrayChanged);
    connect(backend_->getPreferences(), &Preferences::updateChannelChanged, this, &MainWindow::onPreferencesUpdateChannelChanged);
    connect(backend_->getPreferences(), &Preferences::customConfigsPathChanged, this, &MainWindow::onPreferencesCustomConfigsPathChanged);
    connect(backend_->getPreferences(), &Preferences::debugAdvancedParametersChanged, this, &MainWindow::onPreferencesAdvancedParametersChanged);
    connect(backend_->getPreferences(), &Preferences::reportErrorToUser, this, &MainWindow::onPreferencesReportErrorToUser);
#ifdef Q_OS_MAC
    connect(backend_->getPreferences(), &Preferences::hideFromDockChanged, this, &MainWindow::onPreferencesHideFromDockChanged);
#endif
    connect(backend_->getPreferences(), &Preferences::networkLastKnownGoodProtocolPortChanged, this, &MainWindow::onPreferencesLastKnownGoodProtocolChanged);

    // WindscribeApplication signals
    WindscribeApplication * app = WindscribeApplication::instance();
    connect(app, &WindscribeApplication::clickOnDock, this, &MainWindow::toggleVisibilityIfDocked);
    connect(app, &WindscribeApplication::activateFromAnotherInstance, this, &MainWindow::onAppActivateFromAnotherInstance);
    connect(app, &WindscribeApplication::shouldTerminate_mac, this, &MainWindow::onAppShouldTerminate_mac);
    connect(app, &WindscribeApplication::focusWindowChanged, this, &MainWindow::onFocusWindowChanged);
    connect(app, &WindscribeApplication::applicationCloseRequest, this, &MainWindow::onAppCloseRequest);
#if defined(Q_OS_WIN)
    connect(app, &WindscribeApplication::winIniChanged, this, &MainWindow::onAppWinIniChanged);
#endif
    /*connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &MainWindow::onLanguageChanged); */

    mainWindowController_->getViewport()->installEventFilter(this);
    connect(mainWindowController_, SIGNAL(shadowUpdated()), SLOT(update()));
    connect(mainWindowController_, SIGNAL(revealConnectWindowStateChanged(bool)), this, SLOT(onRevealConnectStateChanged(bool)));
    connect(mainWindowController_, &MainWindowController::revealConnectWindowStateChanged, this, &MainWindow::onRevealConnectStateChanged);

    setupTrayIcon();

    backend_->locationsModelManager()->setLocationOrder(backend_->getPreferences()->locationOrder());
    selectedLocation_.reset(new gui_locations::SelectedLocation(backend_->locationsModelManager()->locationsModel()));
    connect(selectedLocation_.get(), &gui_locations::SelectedLocation::changed, this, &MainWindow::onSelectedLocationChanged);
    connect(selectedLocation_.get(), &gui_locations::SelectedLocation::removed, this, &MainWindow::onSelectedLocationRemoved);

    connect(&DpiScaleManager::instance(), &DpiScaleManager::scaleChanged, this, &MainWindow::onScaleChanged);
    connect(&DpiScaleManager::instance(), &DpiScaleManager::newScreen, this, &MainWindow::onDpiScaleManagerNewScreen);

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
    connect(&deactivationTimer_, &QTimer::timeout, this, &MainWindow::onWindowDeactivateAndHideImpl);

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

    bool showAppMinimized = false;

    if (backend_ && backend_->getPreferences()->isStartMinimized()) {
        showAppMinimized = true;
    }
    #if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    else if (backend_ && backend_->getPreferences()->isMinimizeAndCloseToTray()) {
        QCommandLineParser cmdParser;
        cmdParser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
        QCommandLineOption osRestartOption("os_restart");
        cmdParser.addOption(osRestartOption);
        cmdParser.process(*WindscribeApplication::instance());
        if (cmdParser.isSet(osRestartOption)) {
            showAppMinimized = true;
        }
    }
    #endif

    if (showAppMinimized) {
        #ifdef Q_OS_WIN
        // showMinimized, as of Qt 6.3.1, on Windows does not work.  The app is displayed (shown)
        // and all user input is disabled until ones clicks on the taskbar icon or alt-tabs back
        // to the app.
        show();
        QTimer::singleShot(50, this, &MainWindow::onMinimizeClick);
        #else
        showMinimized();
        #endif
    }
    else {
        show();
    }
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
                      backend_->getPreferences()->firewallSettings().mode == FIREWALL_MODE_ALWAYS_ON || isExitingAfterUpdate_,
                      backend_->getPreferences()->isLaunchOnStartup());

    // Backend handles setting firewall state after app closes
    // This block handles initializing the firewall state on next run
    if (PersistentState::instance().isFirewallOn() &&
        backend_->getPreferences()->firewallSettings().mode == FIREWALL_MODE_AUTOMATIC)
    {
        if (WindscribeApplication::instance()->isExitWithRestart()) {
            if (!backend_->getPreferences()->isLaunchOnStartup() || !backend_->getPreferences()->isAutoConnect()) {
                qCDebug(LOG_BASIC) << "Setting firewall persistence to false for restart-triggered shutdown";
                PersistentState::instance().setFirewallState(false);
            }
        }
        else { // non-restart close
            if (!backend_->getPreferences()->isAutoConnect()) {
                qCDebug(LOG_BASIC) << "Setting firewall persistence to false for non-restart auto-mode";
                PersistentState::instance().setFirewallState(false);
            }
        }
    }

    qCDebug(LOG_BASIC) << "Firewall on next startup: " << PersistentState::instance().isFirewallOn();

    PersistentState::instance().setAppGeometry(this->saveGeometry());


    // Shutdown notification controller here, and not in a destructor. Otherwise, sometimes we won't
    // be able to shutdown properly, because the destructor may not be called. On the Windows
    // platform, when the user logs off, the system terminates the process after Qt closes all top
    // level windows. Hence, there is no guarantee that the application will have time to exit its
    // event loop and execute code at the end of the main() function, including destructors of local
    // objects.
    notificationsController_.shutdown();

    // Save favorites and persistent state here for the reason above.
    PersistentState::instance().save();
    backend_->locationsModelManager()->saveFavoriteLocations();

    if (WindscribeApplication::instance()->isExitWithRestart() || isFromSigTerm_mac) {
        // Since we may process events below, disable UI updates and prevent the slot for this signal
        // from attempting to close this widget. We have encountered instances of that occurring during
        // the app update process on macOS.
        setUpdatesEnabled(false);
        disconnect(WindscribeApplication::instance(), &WindscribeApplication::shouldTerminate_mac, this, nullptr);

        qCDebug(LOG_BASIC) << "close main window with" << (isFromSigTerm_mac ? "SIGTERM" : "restart OS");

        while (!backend_->isAppCanClose()) {
            QThread::msleep(1);
            qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
        }

        if (event) {
            QWidget::closeEvent(event);
        }
        else {
            close();
        }
    }
    else {
        qCDebug(LOG_BASIC) << "close main window";
        if (event) {
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

            if (QGuiApplication::platformName() == "wayland")
            {
                this->window()->windowHandle()->startSystemMove();
            }
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

bool MainWindow::handleKeyPressEvent(QKeyEvent *event)
{
    if (mainWindowController_->isLocationsExpanded())
    {
        if(event->key() == Qt::Key_Escape || event->key() == Qt::Key_Space)
        {
            mainWindowController_->collapseLocations();
            return true;
        }
    }
    else if (mainWindowController_->currentWindow() == MainWindowController::WINDOW_ID_CONNECT
             && !mainWindowController_->isPreferencesVisible() && !mainWindowController_->isNewsFeedVisible())
    {
        if (event->key() == Qt::Key_Down || event->key() == Qt::Key_Space)
        {
            mainWindowController_->expandLocations();
            return true;
        }
        else if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
        {
            onConnectWindowConnectClick();
            return true;
        }
    }

    return false;
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
    if (username.contains("@")) {
        mainWindowController_->getLoginWindow()->setErrorMessage(ILoginWindow::ERR_MSG_USERNAME_IS_EMAIL, QString());
        return;
    }
    mainWindowController_->getTwoFactorAuthWindow()->setCurrentCredentials(username, password);
    mainWindowController_->getLoggingInWindow()->setMessage(tr("Logging you in..."));
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
        if (!selectedLocation_->isValid())
        {
            LocationID bestLocation = backend_->locationsModelManager()->getBestLocationId();
            WS_ASSERT(bestLocation.isValid());
            selectedLocation_->set(bestLocation);
            PersistentState::instance().setLastLocation(selectedLocation_->locationdId());
            WS_ASSERT(selectedLocation_->isValid());
        }
        backend_->sendConnect(selectedLocation_->locationdId());
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
    mainWindowController_->getPreferencesWindow()->setCurrentTab(TAB_CONNECTION, CONNECTION_SCREEN_NETWORK_OPTIONS);
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

void MainWindow::onConnectWindowProtocolsClick()
{
    mainWindowController_->expandProtocols(ProtocolWindowMode::kChangeProtocol);
}

void MainWindow::onConnectWindowNotificationsClick()
{
    mainWindowController_->getNewsFeedWindow()->setMessages(
        notificationsController_.messages(), notificationsController_.shownIds());
    mainWindowController_->expandNewsFeed();
}

void MainWindow::onConnectWindowSplitTunnelingClick()
{
    mainWindowController_->expandPreferences();
    mainWindowController_->getPreferencesWindow()->setCurrentTab(TAB_CONNECTION, CONNECTION_SCREEN_SPLIT_TUNNELING);
}

void MainWindow::onEscapeNotificationsClick()
{
    mainWindowController_->collapseNewsFeed();
}

void MainWindow::onEscapeProtocolsClick()
{
    mainWindowController_->collapseProtocols();
}

void MainWindow::onProtocolWindowProtocolClick(const types::Protocol &protocol, uint port)
{
    // If chosen protocol is different from preferred, turn off the badge
    types::ProtocolStatus ps = mainWindowController_->getConnectWindow()->getProtocolStatus();
    if (protocol != ps.protocol || port != ps.port) {
        mainWindowController_->getConnectWindow()->setIsPreferredProtocol(false);
    }

    mainWindowController_->getConnectWindow()->setProtocolPort(protocol, port);
    backend_->sendConnect(PersistentState::instance().lastLocation(), types::ConnectionSettings(protocol, port, false));
    userProtocolOverride_ = true;
}

void MainWindow::onProtocolWindowSetAsPreferred(const types::ConnectionSettings &settings)
{
    if (curNetwork_.isValid()) {
        backend_->getPreferences()->setNetworkPreferredProtocol(curNetwork_.networkOrSsid, settings);
    }
}

void MainWindow::onProtocolWindowDisconnect()
{
    backend_->sendDisconnect();
}

void MainWindow::onPreferencesEscapeClick()
{
    collapsePreferences();
}

void MainWindow::onPreferencesSignOutClick()
{
    gotoLogoutWindow();
}

void MainWindow::onPreferencesLoginClick()
{
    collapsePreferences();
    if (backend_->getPreferencesHelper()->isExternalConfigMode()) {
        // We're not really logged in, but trigger events as if user has confirmed logout
        onLogoutWindowAccept();
    }
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
        showUserWarning(USER_WARNING_VIEW_LOG_FILE_TOO_BIG);
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

void MainWindow::onSendDebugLogClick()
{
    backend_->sendDebugLog();
}

void MainWindow::onPreferencesManageAccountClick()
{
    backend_->getWebSessionTokenForManageAccount();
}

void MainWindow::onPreferencesAddEmailButtonClick()
{
    backend_->getWebSessionTokenForAddEmail();
}

void MainWindow::onPreferencesManageRobertRulesClick()
{
    backend_->getWebSessionTokenForManageRobertRules();
}

void MainWindow::onPreferencesQuitAppClick()
{
    gotoExitWindow();
}

void MainWindow::onPreferencesAccountLoginClick()
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

void MainWindow::onPreferencesWindowDetectPacketSizeClick()
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
    connect(advParametersWindow_, &AdvancedParametersDialog::okClick, this, &MainWindow::onAdvancedParametersOkClick);
    connect(advParametersWindow_, &AdvancedParametersDialog::cancelClick, this, &MainWindow::onAdvancedParametersCancelClick);
    advParametersWindow_->show();
}

void MainWindow::onPreferencesCustomConfigsPathChanged(QString path)
{
    locationsWindow_->setCustomConfigsPath(path);
}

void MainWindow::onPreferencesAdvancedParametersChanged(const QString &advParams)
{
    Q_UNUSED(advParams);
    backend_->sendAdvancedParametersChanged();
}

void MainWindow::onPreferencesUpdateChannelChanged(UPDATE_CHANNEL updateChannel)
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
    mainWindowController_->getPreferencesWindow()->setCurrentTab(TAB_CONNECTION);
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
    backend_->sendUpdateVersion(static_cast<qint64>(this->winId()));
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

void MainWindow::onLogoutWindowAccept()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    setEnabled(false);
    signOutReason_ = SIGN_OUT_FROM_MENU;
    isExitingFromPreferences_ = false;
    selectedLocation_->clear();
    backend_->signOut(false);
}

void MainWindow::onExitWindowAccept()
{
    close();
}

void MainWindow::onExitWindowReject()
{
    if (isExitingFromPreferences_) {
        isExitingFromPreferences_ = false;
        mainWindowController_->changeWindow(MainWindowController::WINDOW_CMD_CLOSE_EXIT_FROM_PREFS);
    } else {
        mainWindowController_->changeWindow(MainWindowController::WINDOW_CMD_CLOSE_EXIT);
    }
}

void MainWindow::onLocationSelected(const LocationID &lid)
{
    qCDebug(LOG_USER) << "Location selected:" << lid.getHashString();

    selectedLocation_->set(lid);
    PersistentState::instance().setLastLocation(selectedLocation_->locationdId());
    if (selectedLocation_->isValid())
    {
        mainWindowController_->getConnectWindow()->updateLocationInfo(selectedLocation_->firstName(), selectedLocation_->secondName(),
                                                                      selectedLocation_->countryCode(), selectedLocation_->pingTime(),
                                                                      selectedLocation_->locationdId().isCustomConfigsLocation());
        mainWindowController_->collapseLocations();
        backend_->sendConnect(lid);
    }
    else
    {
        WS_ASSERT(false);
    }
}

void MainWindow::onClickedOnPremiumStarCity()
{
    openUpgradeExternalWindow();
}

void MainWindow::onLocationsAddStaticIpClicked()
{
    openStaticIpExternalWindow();
}

void MainWindow::onLocationsClearCustomConfigClicked()
{
    backend_->getPreferences()->setCustomOvpnConfigsPath(QString());
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
                if (err == AuthCheckerError::AUTH_AUTHENTICATION_ERROR)
                {
                    qCDebug(LOG_BASIC) << "Cannot change path when non-system directory when windscribe is not elevated.";
                    const QString desc = tr(
                        "Cannot select this directory because it is writeable for non-privileged users. "
                        "Custom configs in this directory may pose a potential security risk. "
                        "Please authenticate with an admin user to select this directory.");
                    QMessageBox::warning(g_mainWindow, tr("Windscribe"), desc);
                    return;
                }
                else if (err == AuthCheckerError::AUTH_HELPER_ERROR)
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
    }
}

void MainWindow::onBackendInitFinished(INIT_STATE initState)
{
    setVariablesToInitState();

    if (initState == INIT_STATE_SUCCESS)
    {
        setInitialFirewallState();

        Preferences *p = backend_->getPreferences();
        p->validateAndUpdateIfNeeded();

        backend_->sendSplitTunneling(p->splitTunneling());

#ifdef Q_OS_MAC
        // disable firewall for Mac when split tunneling is active
        if (p->splitTunneling().settings.active)
        {
            backend_->getPreferencesHelper()->setBlockFirewall(true);
            mainWindowController_->getConnectWindow()->setFirewallBlock(true);
        }
        // on Mac, remove apps from the split tunnel config since we don't support them
        p->setSplitTunnelingApps(QList<types::SplitTunnelingApp>());
#endif

        // enable wifi/proxy sharing, if checked
        if (p->shareSecureHotspot().isEnabled)
        {
            onPreferencesShareSecureHotspotChanged(p->shareSecureHotspot());
        }
        if (p->shareProxyGateway().isEnabled)
        {
            onPreferencesShareProxyGatewayChanged(p->shareProxyGateway());
        }

        QString autoLoginUsername;
        QString autoLoginPassword;

        if (backend_->haveAutoLoginCredentials(autoLoginUsername, autoLoginPassword)) {
            mainWindowController_->getInitWindow()->startSlideAnimation();
            gotoLoginWindow();
            // Give the UI time to finish transitioning (adjusting window height, etc.) in the
            // gotoLoginWindow() call before we attempt login.  Otherwise, the screen height
            // may be incorrect if the login fails (e.g. due to invalid credentials).
            QTimer::singleShot(150, this, [this, autoLoginUsername, autoLoginPassword]() {
                onLoginClick(autoLoginUsername, autoLoginPassword, QString());
            });
        }
        else if (backend_->isCanLoginWithAuthHash())
        {
            if (!backend_->isSavedApiSettingsExists())
            {
                mainWindowController_->getLoggingInWindow()->setMessage(tr("Logging you in..."));
                mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_LOGGING_IN);
            }
            backend_->loginWithAuthHash();
        }
        else
        {
            mainWindowController_->getInitWindow()->startSlideAnimation();
            gotoLoginWindow();
        }

        // Reset last known good protocol/port
        backend_->getPreferences()->clearLastKnownGoodProtocols();

        updateConnectWindowStateProtocolPortDisplay();

        // Start the IPC server last to give the above commands time to finish before we
        // start accepting commands from the CLI.
        localIpcServer_->start();
    }
    else if (initState == INIT_STATE_BFE_SERVICE_NOT_STARTED)
    {
        if (QMessageBox::information(nullptr, QApplication::applicationName(), tr("Enable \"Base Filtering Engine\" service? This is required for Windscribe to function."),
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
    else if (initState == INIT_STATE_BFE_SERVICE_FAILED_TO_START)
    {
        QMessageBox::information(nullptr, QApplication::applicationName(), QObject::tr("Failed to start \"Base Filtering Engine\" service."),
                                         QMessageBox::Close);
        QTimer::singleShot(0, this, SLOT(close()));
    }
    else if (initState == INIT_STATE_HELPER_FAILED)
    {
        QMessageBox::information(nullptr, QApplication::applicationName(), tr("Windscribe helper initialize error. Please reinstall the application or contact support."));
        QTimer::singleShot(0, this, SLOT(close()));
    }
    else if (initState == INIT_STATE_HELPER_USER_CANCELED)
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

    if (backend_->getPreferences()->firewallSettings().mode == FIREWALL_MODE_ALWAYS_ON)
    {
        backend_->firewallOn();
        mainWindowController_->getConnectWindow()->setFirewallAlwaysOn(true);
    }

    if (!isLoginOkAndConnectWindowVisible_)
    {
        // choose latest saved location
        selectedLocation_->set(PersistentState::instance().lastLocation());
        if (!selectedLocation_->isValid())
        {
            LocationID bestLocation = backend_->locationsModelManager()->getBestLocationId();
            WS_ASSERT(bestLocation.isValid());
            if (!bestLocation.isValid())
            {
                qCDebug(LOG_BASIC) << "Fatal error: MainWindow::onBackendLoginFinished, WS_ASSERT(bestLocation.isValid());";
            }
            selectedLocation_->set(bestLocation);
            PersistentState::instance().setLastLocation(selectedLocation_->locationdId());
        }
        mainWindowController_->getConnectWindow()->updateLocationInfo(selectedLocation_->firstName(), selectedLocation_->secondName(),
                                                                      selectedLocation_->countryCode(), selectedLocation_->pingTime(),
                                                                      selectedLocation_->locationdId().isCustomConfigsLocation());
        mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_CONNECT);
        isLoginOkAndConnectWindowVisible_ = true;
    }

    // open new install on first login
    if (PersistentState::instance().isFirstLogin())
    {
        backend_->recordInstall();
        // open first start URL
        QString curUserId = backend_->getSessionStatus().getUserId();
        QDesktopServices::openUrl(QUrl( QString("https://%1/installed/desktop?%2").arg(HardcodedSettings::instance().serverUrl()).arg(curUserId)));
    }
    PersistentState::instance().setFirstLogin(false);
}

void MainWindow::onBackendTryingBackupEndpoint(int num, int cnt)
{
    if (!isLoginOkAndConnectWindowVisible_)
    {
        QString additionalMessage = tr("Trying Backup Endpoints %1/%2").arg(num).arg(cnt);
        mainWindowController_->getLoggingInWindow()->setAdditionalMessage(additionalMessage);
    }
}

void MainWindow::onBackendLoginError(LOGIN_RET loginError, const QString &errorMessage)
{
    if (loginError == LOGIN_RET_BAD_USERNAME)
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
            // It's possible we were passed invalid credentials by the CLI or installer auto-login.  Ensure we transition
            // the user to the username/password entry screen so they can see the error message and attempt a manual login.
            mainWindowController_->getLoginWindow()->transitionToUsernameScreen();
            gotoLoginWindow();
        }
    }
    else if (loginError == LOGIN_RET_BAD_CODE2FA ||
             loginError == LOGIN_RET_MISSING_CODE2FA)
    {
        const bool is_missing_code2fa = (loginError == LOGIN_RET_MISSING_CODE2FA);
        mainWindowController_->getTwoFactorAuthWindow()->setErrorMessage(
            is_missing_code2fa ? ITwoFactorAuthWindow::ERR_MSG_NO_CODE
                               : ITwoFactorAuthWindow::ERR_MSG_INVALID_CODE);
        mainWindowController_->getTwoFactorAuthWindow()->setLoginMode(true);
        mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_TWO_FACTOR_AUTH);
    }
    else if (loginError == LOGIN_RET_NO_CONNECTIVITY)
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
            backend_->loginWithLastLoginSettings();
        }
    }
    else if (loginError == LOGIN_RET_NO_API_CONNECTIVITY)
    {
        if (!isLoginOkAndConnectWindowVisible_)
        {
            //qCDebug(LOG_BASIC) << "Show No API connectivity message to user.";
            mainWindowController_->getLoginWindow()->setErrorMessage(ILoginWindow::ERR_MSG_NO_API_CONNECTIVITY, QString());
            //loginWindow_->setEmergencyConnectState(falseengine_->isEmergencyDisconnected());
            gotoLoginWindow();
        }
    }
    else if (loginError == LOGIN_RET_INCORRECT_JSON)
    {
        if (!isLoginOkAndConnectWindowVisible_)
        {
            mainWindowController_->getLoginWindow()->setErrorMessage(ILoginWindow::ERR_MSG_INVALID_API_RESPONSE, QString());
            mainWindowController_->getLoginWindow()->setEmergencyConnectState(false);
            gotoLoginWindow();
        }
        else
        {
            backToLoginWithErrorMessage(ILoginWindow::ERR_MSG_INVALID_API_RESPONSE, QString());
        }
    }
    else if (loginError == LOGIN_RET_PROXY_AUTH_NEED)
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
    else if (loginError == LOGIN_RET_SSL_ERROR)
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
    else if (loginError == LOGIN_RET_ACCOUNT_DISABLED)
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
    else if (loginError == LOGIN_RET_SESSION_INVALID)
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
    else if (loginError == LOGIN_RET_RATE_LIMITED)
    {
        if (!isLoginOkAndConnectWindowVisible_)
        {
            mainWindowController_->getLoginWindow()->setErrorMessage(ILoginWindow::ERR_MSG_RATE_LIMITED, QString());
            mainWindowController_->getLoginWindow()->setEmergencyConnectState(false);
            gotoLoginWindow();
        }
        else
        {
            backToLoginWithErrorMessage(ILoginWindow::ERR_MSG_RATE_LIMITED, QString());
        }

    }
}

void MainWindow::onBackendSessionStatusChanged(const types::SessionStatus &sessionStatus)
{
    //bFreeSessionStatus_ = sessionStatus->isPremium == 0;
    blockConnect_.setNotBlocking();
    qint32 status = sessionStatus.getStatus();
    // multiple account abuse detection
    QString entryUsername;
    bool bEntryIsPresent = multipleAccountDetection_->entryIsPresent(entryUsername);
    if (bEntryIsPresent && (!sessionStatus.isPremium()) && sessionStatus.getAlc().size() == 0 && sessionStatus.getStatus() == 1 && entryUsername != sessionStatus.getUsername())
    {
        status = 2;
        //qCDebug(LOG_BASIC) << "Abuse detection: User session status was changed to 2. Original username:" << entryUsername;
        blockConnect_.setBlockedMultiAccount(entryUsername);
    }
    else if (bEntryIsPresent && entryUsername == sessionStatus.getUsername() && sessionStatus.getStatus() == 1)
    {
        multipleAccountDetection_->removeEntry();
    }

    // free account
    if (!sessionStatus.isPremium())
    {
        if (status == 2)
        {
            // write entry into registry expired_user = username
            multipleAccountDetection_->userBecomeExpired(sessionStatus.getUsername());

            if ((!selectedLocation_->locationdId().isCustomConfigsLocation()) &&
                (backend_->currentConnectState() == CONNECT_STATE_CONNECTED || backend_->currentConnectState() == CONNECT_STATE_CONNECTING))
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
            if (sessionStatus.getTrafficMax() == -1)
            {
                mainWindowController_->getBottomInfoWindow()->setDataRemaining(-1, -1);
            }
            else
            {
                if (backend_->getPreferences()->isShowNotifications())
                {
                    freeTrafficNotificationController_->updateTrafficInfo(sessionStatus.getTrafficUsed(), sessionStatus.getTrafficMax());
                }

                mainWindowController_->getBottomInfoWindow()->setDataRemaining(sessionStatus.getTrafficUsed(), sessionStatus.getTrafficMax());
            }
        }
    }
    // premium account
    else
    {
        if (sessionStatus.getRebill() == 0)
        {
            QDate curDate = QDateTime::currentDateTimeUtc().date();
            QDate expireDate = QDate::fromString(sessionStatus.getPremiumExpireDate(), "yyyy-MM-dd");

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
        if ((!selectedLocation_->locationdId().isCustomConfigsLocation()) &&
            (backend_->currentConnectState() == CONNECT_STATE_CONNECTED || backend_->currentConnectState() == CONNECT_STATE_CONNECTING))
        {
            backend_->sendDisconnect();
        }
    }
    backend_->setBlockConnect(blockConnect_.isBlocked());
}

void MainWindow::onBackendCheckUpdateChanged(const types::CheckUpdate &checkUpdateInfo)
{
    if (checkUpdateInfo.isAvailable)
    {
        qCDebug(LOG_BASIC) << "Update available";
        if (!checkUpdateInfo.isSupported)
        {
            blockConnect_.setNeedUpgrade();
        }

        QString betaStr;
        betaStr = "-" + QString::number(checkUpdateInfo.latestBuild);
        if (checkUpdateInfo.updateChannel == UPDATE_CHANNEL_BETA)
        {
            betaStr += "b";
        }
        else if (checkUpdateInfo.updateChannel == UPDATE_CHANNEL_GUINEA_PIG)
        {
            betaStr += "g";
        }

        //updateWidget_->setText(tr("Update available - v") + version + betaStr);

        mainWindowController_->getUpdateAppItem()->setVersionAvailable(checkUpdateInfo.version, checkUpdateInfo.latestBuild);
        mainWindowController_->getUpdateWindow()->setVersion(checkUpdateInfo.version, checkUpdateInfo.latestBuild);

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
        if (selectedLocation_->isValid())
        {
            updateTrayTooltip(tr("Connected to ") + selectedLocation_->firstName() + "-" + selectedLocation_->secondName() + "\n" + ip);
        }
    }
}

void MainWindow::onBackendConnectStateChanged(const types::ConnectState &connectState)
{
    mainWindowController_->getConnectWindow()->updateConnectState(connectState);

    if (connectState.location.isValid())
    {
        // if connecting/connected location not equal current selected location, then change current selected location and update in GUI
        if (selectedLocation_->locationdId() != connectState.location)
        {
            selectedLocation_->set(connectState.location);
            PersistentState::instance().setLastLocation(selectedLocation_->locationdId());
            if (selectedLocation_->isValid())
            {
                mainWindowController_->getConnectWindow()->updateLocationInfo(selectedLocation_->firstName(), selectedLocation_->secondName(),
                                                                              selectedLocation_->countryCode(), selectedLocation_->pingTime(),
                                                                              selectedLocation_->locationdId().isCustomConfigsLocation());
            }
            else {
                qCDebug(LOG_BASIC) << "Fatal error: MainWindow::onBackendConnectStateChanged, WS_ASSERT(selectedLocation_.isValid());";
                WS_ASSERT(false);
            }
        }
    }

    if (connectState.connectState == CONNECT_STATE_CONNECTED)
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
                if (selectedLocation_->isValid())
                {
                    showTrayMessage(tr("You are now connected to Windscribe (%1).").arg(selectedLocation_->firstName() + "-" + selectedLocation_->secondName()));
                    bNotificationConnectedShowed_ = true;
                }
            }
        }
    }
    else if (connectState.connectState == CONNECT_STATE_CONNECTING || connectState.connectState == CONNECT_STATE_DISCONNECTING)
    {
        mainWindowController_->getProtocolWindow()->resetProtocolStatus();

        updateAppIconType(AppIconType::CONNECTING);
        updateTrayIconType(AppIconType::CONNECTING);
        mainWindowController_->clearServerRatingsTooltipState();

    }
    else if (connectState.connectState == CONNECT_STATE_DISCONNECTED)
    {
        updateConnectWindowStateProtocolPortDisplay();

        if (connectState.disconnectReason == DISCONNECTED_WITH_ERROR) {
            updateAppIconType(AppIconType::DISCONNECTED_WITH_ERROR);
        } else {
            updateAppIconType(AppIconType::DISCONNECTED);
        }

        // Ensure the icon has been updated, as QSystemTrayIcon::showMessage displays this icon
        // in the notification window on Windows.
        updateTrayIconType(AppIconType::DISCONNECTED);

        if (bNotificationConnectedShowed_)
        {
            if (backend_->getPreferences()->isShowNotifications()) {
                showTrayMessage(tr("Connection to Windscribe has been terminated.\n%1 transferred in %2").arg(getConnectionTransferred(), getConnectionTime()));
            }
            bNotificationConnectedShowed_ = false;
        }

        if (connectState.disconnectReason == DISCONNECTED_WITH_ERROR)
        {
            handleDisconnectWithError(connectState);
        }

        mainWindowController_->getProtocolWindow()->resetProtocolStatus();
        userProtocolOverride_ = false;
    }
}

void MainWindow::onBackendEmergencyConnectStateChanged(const types::ConnectState &connectState)
{
    mainWindowController_->getEmergencyConnectWindow()->setState(connectState);
    mainWindowController_->getLoginWindow()->setEmergencyConnectState(connectState.connectState == CONNECT_STATE_CONNECTED);
}

void MainWindow::onBackendFirewallStateChanged(bool isEnabled)
{
    mainWindowController_->getConnectWindow()->updateFirewallState(isEnabled);
    PersistentState::instance().setFirewallState(isEnabled);
}

void MainWindow::onNetworkChanged(types::NetworkInterface network)
{
    qCDebug(LOG_BASIC) << "Network Changed: "
                       << "Index: " << network.interfaceIndex
                       << ", Network/SSID: " << network.networkOrSsid
                       << ", MAC: " << network.physicalAddress
                       << ", device name: " << network.deviceName
                       << " friendly: " << network.friendlyName;

    mainWindowController_->getConnectWindow()->updateNetworkState(network);
    mainWindowController_->getPreferencesWindow()->updateNetworkState(network);
    curNetwork_ = network;
    if (backend_->isDisconnected()) {
        updateConnectWindowStateProtocolPortDisplay();
    }
}

void MainWindow::onSplitTunnelingStateChanged(bool isActive)
{
    mainWindowController_->getConnectWindow()->setSplitTunnelingState(isActive);
}

void MainWindow::onBackendSignOutFinished()
{
    loginAttemptsController_.reset();
    mainWindowController_->getPreferencesWindow()->setLoggedIn(false);
    isLoginOkAndConnectWindowVisible_ = false;
    backend_->getPreferencesHelper()->setIsExternalConfigMode(false);
    mainWindowController_->getBottomInfoWindow()->setDataRemaining(-1, -1);

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
        WS_ASSERT(false);
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
    if (backend_->getPreferences()->firewallSettings().mode == FIREWALL_MODE_ALWAYS_ON)
    {
        backend_->firewallOn();
        mainWindowController_->getConnectWindow()->setFirewallAlwaysOn(true);
    }

    if (!isLoginOkAndConnectWindowVisible_)
    {
        // Choose latest location if it's a custom config location; first valid custom config
        // location otherwise.
        selectedLocation_->set(PersistentState::instance().lastLocation());
        if (selectedLocation_->isValid() && selectedLocation_->locationdId().isCustomConfigsLocation())
        {
            mainWindowController_->getConnectWindow()->updateLocationInfo(selectedLocation_->firstName(), selectedLocation_->secondName(),
                                                                          selectedLocation_->countryCode(), selectedLocation_->pingTime(),
                                                                          selectedLocation_->locationdId().isCustomConfigsLocation());
        }
        else
        {
            LocationID firstValidCustomLocation = backend_->locationsModelManager()->getFirstValidCustomConfigLocationId();
            if (firstValidCustomLocation.isValid()) {
                selectedLocation_->set(firstValidCustomLocation);
                PersistentState::instance().setLastLocation(selectedLocation_->locationdId());
                // |selectedLocation_| can be empty (nopt valid) here, so this will reset current location.
                mainWindowController_->getConnectWindow()->updateLocationInfo(selectedLocation_->firstName(), selectedLocation_->secondName(),
                                                                              selectedLocation_->countryCode(), selectedLocation_->pingTime(),
                                                                              selectedLocation_->locationdId().isCustomConfigsLocation());
            }
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
    mainWindowController_->getPreferencesWindow()->setSendLogResult(bSuccess);
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

void MainWindow::onBackendProxySharingInfoChanged(const types::ProxySharingInfo &psi)
{
    backend_->getPreferencesHelper()->setProxyGatewayAddress(psi.address);

    if (psi.isEnabled)
    {
        mainWindowController_->getBottomInfoWindow()->setProxyGatewayFeatures(true, psi.mode);
    }
    else
    {
        mainWindowController_->getBottomInfoWindow()->setProxyGatewayFeatures(false, PROXY_SHARING_HTTP);
    }

    mainWindowController_->getBottomInfoWindow()->setProxyGatewayUsersCount(psi.usersCount);
}

void MainWindow::onBackendWifiSharingInfoChanged(const types::WifiSharingInfo &wsi)
{
    if (wsi.isEnabled)
    {
        mainWindowController_->getBottomInfoWindow()->setSecureHotspotFeatures(true, wsi.ssid);
    }
    else
    {
        mainWindowController_->getBottomInfoWindow()->setSecureHotspotFeatures(false, "");
    }

    mainWindowController_->getBottomInfoWindow()->setSecureHotspotUsersCount(wsi.usersCount);
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
    selectedLocation_->clear();
    backend_->signOut(true);
}

void MainWindow::onBackendTestTunnelResult(bool success)
{
    if (!ExtraConfig::instance().getIsTunnelTestNoError() && !success)
    {
        if (tunnelTestMsgBox_ == nullptr) {
            tunnelTestMsgBox_ = new QMessageBox(
                QMessageBox::Information,
                QApplication::applicationName(),
                tr("We've detected that your network settings may interfere with Windscribe. "
                   "Please disconnect and send us a Debug Log, by going into Preferences and clicking the \"Send Log\" button."),
                QMessageBox::Ok);
            tunnelTestMsgBox_->setWindowModality(Qt::WindowModal);
            // Connect manually instead of passing slot into open(), since that seems to throw some warnings
            tunnelTestMsgBox_->open(nullptr, nullptr);
            connect(tunnelTestMsgBox_, &QMessageBox::buttonClicked, this, &MainWindow::onMsgBoxClicked);
        }
    }

    if (success && selectedLocation_->isValid() && !selectedLocation_->locationdId().isCustomConfigsLocation()) {
        types::ProtocolStatus ps = mainWindowController_->getConnectWindow()->getProtocolStatus();

        mainWindowController_->getProtocolWindow()->setProtocolStatus(
            types::ProtocolStatus(ps.protocol, ps.port, types::ProtocolStatus::Status::kConnected));

        types::Protocol defaultProtocol = getDefaultProtocolForNetwork(curNetwork_.networkOrSsid);

        if (backend_->getPreferences()->networkLastKnownGoodProtocol(curNetwork_.networkOrSsid) != ps.protocol ||
            backend_->getPreferences()->networkLastKnownGoodPort(curNetwork_.networkOrSsid) != ps.port)
        {
            backend_->getPreferences()->setNetworkLastKnownGoodProtocolPort(curNetwork_.networkOrSsid, ps.protocol, ps.port);

            // User manually selected a network or we failed over to a different protocol for the first time.
            // Ask if they want to save
            if (userProtocolOverride_ || defaultProtocol != ps.protocol) {
                if (!backend_->getPreferences()->hasNetworkPreferredProtocol(curNetwork_.networkOrSsid) ||
                    backend_->getPreferences()->networkPreferredProtocol(curNetwork_.networkOrSsid).protocol() != ps.protocol ||
                    backend_->getPreferences()->networkPreferredProtocol(curNetwork_.networkOrSsid).port() != ps.port)
                {
                    mainWindowController_->expandProtocols(ProtocolWindowMode::kSavePreferredProtocol);
                }
                userProtocolOverride_ = false;
            }
        }
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

void MainWindow::showUserWarning(USER_WARNING_TYPE userWarningType)
{
    QString titleText;
    QString descText;
    if (userWarningType == USER_WARNING_MAC_SPOOFING_FAILURE_HARD)
    {
        titleText = tr("MAC Spoofing Failed");
        descText = tr("Your network adapter does not support MAC spoofing. Try a different adapter.");
    }
    else if (userWarningType == USER_WARNING_MAC_SPOOFING_FAILURE_SOFT)
    {
        titleText = tr("MAC Spoofing Failed");
        descText = tr("Could not spoof MAC address, try updating your OS to the latest version.");
    }
    else if (userWarningType == USER_WARNING_SEND_LOG_FILE_TOO_BIG)
    {
        titleText = tr("Logs too large to send");
        descText = tr("Could not send logs to Windscribe, they are too big. Either re-send after replicating the issue or manually compressing and sending to support.");
    }
    else if (userWarningType == USER_WARNING_VIEW_LOG_FILE_TOO_BIG)
    {
        titleText = tr("Logs too large to view");
        descText = tr("Could not view the logs because they are too big. You may want to try viewing manually.");
    }

    if (!titleText.isEmpty() && !descText.isEmpty())
    {
        if (!currentlyShowingUserWarningMessage_)
        {
            currentlyShowingUserWarningMessage_ = true;
            QMessageBox::warning(nullptr, titleText, descText, QMessageBox::Ok);
            currentlyShowingUserWarningMessage_ = false;
        }
    }
}

void MainWindow::onBackendUserWarning(USER_WARNING_TYPE userWarningType)
{
    showUserWarning(userWarningType);
}

void MainWindow::onBackendInternetConnectivityChanged(bool connectivity)
{
    mainWindowController_->getConnectWindow()->setInternetConnectivity(connectivity);
    internetConnected_ = connectivity;
}

void MainWindow::onBackendProtocolPortChanged(const types::Protocol &protocol, const uint port)
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

void MainWindow::onPreferencesGetRobertFilters()
{
    qCDebug(LOG_USER) << "Requesting ROBERT filters from server";
    backend_->getRobertFilters();
}

void MainWindow::onBackendRobertFiltersChanged(bool success, const QVector<types::RobertFilter> &filters)
{
    qCDebug(LOG_USER) << "Get ROBERT filters response: " << success;
    if (success)
    {
        mainWindowController_->getPreferencesWindow()->setRobertFilters(filters);
    }
    else
    {
        mainWindowController_->getPreferencesWindow()->setRobertFiltersError();
    }
}

void MainWindow::onPreferencesSetRobertFilter(const types::RobertFilter &filter)
{
    qCDebug(LOG_USER) << "Set ROBERT filter: " << filter.id << ", " << filter.status;
    backend_->setRobertFilter(filter);
}

void MainWindow::onBackendSetRobertFilterResult(bool success)
{
    qCDebug(LOG_BASIC) << "Set ROBERT filter response:" << success;
    if (success)
    {
        backend_->syncRobert();
    }
}

void MainWindow::onBackendSyncRobertResult(bool success)
{
    qCDebug(LOG_BASIC) << "Sync ROBERT response:" << success;
}

void MainWindow::onBackendProtocolStatusChanged(const QVector<types::ProtocolStatus> &status)
{
    mainWindowController_->getProtocolWindow()->setProtocolStatus(status);
    mainWindowController_->expandProtocols();
}

void MainWindow::onBackendUpdateVersionChanged(uint progressPercent, UPDATE_VERSION_STATE state, UPDATE_VERSION_ERROR error)
{
    // qDebug() << "Mainwindow::onBackendUpdateVersionChanged: " << progressPercent << ", " << state;

    if (state == UPDATE_VERSION_STATE_DONE)
    {

        if (downloadRunning_) // not cancelled by user
        {
            if (error == UPDATE_VERSION_ERROR_NO_ERROR)
            {
                isExitingAfterUpdate_ = true; // the flag for prevent firewall off for some states

                // nothing todo, because installer will close app here
#ifdef Q_OS_LINUX
                // restart the application after update
                doClose(nullptr, false);
                qApp->quit();
                qCDebug(LOG_BASIC) << "Restarting app";
                QProcess process;
                process.setProgram(QApplication::applicationFilePath());
                process.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
                bool ret = process.startDetached();
                if (!ret) {
                    qCDebug(LOG_BASIC) << "Could not launch app";
                }
#endif
            }
            else // Error
            {
                mainWindowController_->getUpdateAppItem()->setProgress(0);
                mainWindowController_->getUpdateWindow()->stopAnimation();
                mainWindowController_->getUpdateWindow()->changeToPromptScreen();

                QString titleText = tr("Auto-Update Failed");
                QString descText = tr("Please contact support");
                if (error == UPDATE_VERSION_ERROR_DL_FAIL)
                {
                    descText = tr("Please try again using a different network connection.");
                }
                else if (error == UPDATE_VERSION_ERROR_SIGN_FAIL)
                {
                    descText = tr("Can't run the downloaded installer. It does not have the correct signature.");
                }
                else if (error == UPDATE_VERSION_ERROR_OTHER_FAIL)
                {
                    descText = tr("An unexpected error occurred. Please contact support.");
                }
                else if (error == UPDATE_VERSION_ERROR_MOUNT_FAIL)
                {
                    descText = tr("Cannot access the installer. Image mounting has failed.");
                }
                else if (error == UPDATE_VERSION_ERROR_DMG_HAS_NO_INSTALLER_FAIL)
                {
                    descText = tr("Downloaded image does not contain installer.");
                }
                else if (error == UPDATE_VERSION_ERROR_CANNOT_REMOVE_EXISTING_TEMP_INSTALLER_FAIL)
                {
                    descText = tr("Cannot overwrite a pre-existing temporary installer.");
                }
                else if (error == UPDATE_VERSION_ERROR_COPY_FAIL)
                {
                    descText = tr("Failed to copy installer to temp location.");
                }
                else if (error == UPDATE_VERSION_ERROR_START_INSTALLER_FAIL)
                {
                    descText = tr("Auto-Updater has failed to run installer. Please relaunch Windscribe and try again.");
                }
                else if (error == UPDATE_VERSION_ERROR_COMPARE_HASH_FAIL)
                {
                    descText = tr("Cannot run the downloaded installer. It does not have the expected hash.");
                }
                else if (error == UPDATE_VERSION_ERROR_API_HASH_INVALID)
                {
                    descText = tr("Windscribe API has returned an invalid hash for downloaded installer. Please contact support.");
                }
                mainWindowController_->getGeneralMessageWindow()->setIcon("ERROR_ICON");
                mainWindowController_->getGeneralMessageWindow()->setTitle(titleText);
                mainWindowController_->getGeneralMessageWindow()->setDescription(descText);
                mainWindowController_->getGeneralMessageWindow()->setAcceptText(tr("Ok"));
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
    else if (state == UPDATE_VERSION_STATE_DOWNLOADING)
    {
        // qDebug() << "Running -- updating progress";
        mainWindowController_->getUpdateAppItem()->setProgress(progressPercent);
        mainWindowController_->getUpdateWindow()->setProgress(progressPercent);
    }
    else if (state == UPDATE_VERSION_STATE_RUNNING)
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
    QDesktopServices::openUrl(QUrl(getUrl));
}

void MainWindow::onBackendWebSessionTokenForManageAccount(const QString &tempSessionToken)
{
    openBrowserToMyAccountWithToken(tempSessionToken);
}

void MainWindow::onBackendWebSessionTokenForAddEmail(const QString &tempSessionToken)
{
    openBrowserToMyAccountWithToken(tempSessionToken);
}

void MainWindow::onBackendWebSessionTokenForManageRobertRules(const QString &tempSessionToken)
{
    QString getUrl = QString("https://%1/myaccount?temp_session=%2#robertrules")
                        .arg(HardcodedSettings::instance().serverUrl())
                        .arg(tempSessionToken);
    QDesktopServices::openUrl(QUrl(getUrl));
}

void MainWindow::onBackendEngineCrash()
{
    mainWindowController_->getInitWindow()->startWaitingAnimation();
    mainWindowController_->getInitWindow()->setAdditionalMessage(
        tr("Lost connection to the backend process.\nRecovering..."), /*useSmallFont =*/ false);
    mainWindowController_->getInitWindow()->setCropHeight(0); // Needed so that Init screen is correct height when engine fails from connect window
    mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_INITIALIZATION);
}

void MainWindow::onNotificationControllerNewPopupMessage(int messageId)
{
    mainWindowController_->getNewsFeedWindow()->setMessagesWithCurrentOverride(
        notificationsController_.messages(), notificationsController_.shownIds(), messageId);
    mainWindowController_->expandNewsFeed();
}

void MainWindow::onPreferencesFirewallSettingsChanged(const types::FirewallSettings &fm)
{
    if (fm.mode == FIREWALL_MODE_ALWAYS_ON)
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

void MainWindow::onPreferencesShareProxyGatewayChanged(const types::ShareProxyGateway &sp)
{
    if (sp.isEnabled)
    {
        backend_->startProxySharing((PROXY_SHARING_TYPE)sp.proxySharingMode);
    }
    else
    {
        backend_->stopProxySharing();
    }
}

void MainWindow::onPreferencesShareSecureHotspotChanged(const types::ShareSecureHotspot &ss)
{
    if (ss.isEnabled && !ss.ssid.isEmpty() && ss.password.length() >= 8)
    {
        mainWindowController_->getBottomInfoWindow()->setSecureHotspotFeatures(true, ss.ssid);
        backend_->startWifiSharing(ss.ssid, ss.password);
    }
    else
    {
        mainWindowController_->getBottomInfoWindow()->setSecureHotspotFeatures(false, QString());
        backend_->stopWifiSharing();
    }
}

void MainWindow::onPreferencesLocationOrderChanged(ORDER_LOCATION_TYPE o)
{
    backend_->locationsModelManager()->setLocationOrder(o);
}

void MainWindow::onPreferencesSplitTunnelingChanged(types::SplitTunneling st)
{
    // turn off and disable firewall for Mac when split tunneling is active
#ifdef Q_OS_MAC
    if (st.settings.active)
    {
        if (backend_->isFirewallEnabled())
        {
            backend_->firewallOff();
        }
        types::FirewallSettings firewallSettings;
        firewallSettings.mode = FIREWALL_MODE_MANUAL;
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

void MainWindow::onPreferencesAllowLanTrafficChanged(bool /*allowLanTraffic*/)
{
    // Changing Allow Lan Traffic may affect split tunnel behavior
    onPreferencesSplitTunnelingChanged(backend_->getPreferences()->splitTunneling());
}


void MainWindow::onPreferencesLaunchOnStartupChanged(bool bEnabled)
{
    LaunchOnStartup::instance().setLaunchOnStartup(bEnabled);
}

void MainWindow::updateConnectWindowStateProtocolPortDisplay()
{
    types::Protocol lastKnownGoodProtocol = backend_->getPreferences()->networkLastKnownGoodProtocol(curNetwork_.networkOrSsid);
    uint lastKnownGoodPort = backend_->getPreferences()->networkLastKnownGoodPort(curNetwork_.networkOrSsid);

    if (!backend_->getPreferences()->networkPreferredProtocol(curNetwork_.networkOrSsid).isAutomatic()) {
        mainWindowController_->getConnectWindow()->setProtocolPort(backend_->getPreferences()->networkPreferredProtocol(curNetwork_.networkOrSsid).protocol(),
                                                                   backend_->getPreferences()->networkPreferredProtocol(curNetwork_.networkOrSsid).port());
        mainWindowController_->getConnectWindow()->setIsPreferredProtocol(true);
    } else if (backend_->getPreferences()->connectionSettings().isAutomatic()) {
        if (lastKnownGoodProtocol.isValid()) {
            mainWindowController_->getConnectWindow()->setProtocolPort(lastKnownGoodProtocol, lastKnownGoodPort);
        } else {
            mainWindowController_->getConnectWindow()->setProtocolPort(types::Protocol::WIREGUARD, 443);
        }
        mainWindowController_->getConnectWindow()->setIsPreferredProtocol(false);
    } else {
        mainWindowController_->getConnectWindow()->setProtocolPort(backend_->getPreferences()->connectionSettings().protocol(),
                                                                   backend_->getPreferences()->connectionSettings().port());
        mainWindowController_->getConnectWindow()->setIsPreferredProtocol(false);
    }
}

void MainWindow::onPreferencesConnectionSettingsChanged(types::ConnectionSettings connectionSettings)
{
    Q_UNUSED(connectionSettings);

    if (backend_->isDisconnected())
    {
        updateConnectWindowStateProtocolPortDisplay();
    }
}

void MainWindow::onPreferencesNetworkPreferredProtocolsChanged(QMap<QString, types::ConnectionSettings> p)
{
    if (backend_->isDisconnected()) {
        updateConnectWindowStateProtocolPortDisplay();
    } else {
        // If everything is the same but we set/unset the current connection as preferred, update the icon
        types::ProtocolStatus ps = mainWindowController_->getConnectWindow()->getProtocolStatus();
        if (p.contains(curNetwork_.networkOrSsid) &&
            p[curNetwork_.networkOrSsid].isAutomatic() == false &&
            p[curNetwork_.networkOrSsid].protocol() == ps.protocol &&
            p[curNetwork_.networkOrSsid].port() == ps.port)
        {
            mainWindowController_->getConnectWindow()->setIsPreferredProtocol(true);
        } else {
            mainWindowController_->getConnectWindow()->setIsPreferredProtocol(false);
        }
    }
}

void MainWindow::onPreferencesIsDockedToTrayChanged(bool isDocked)
{
    mainWindowController_->setIsDockedToTray(isDocked);
    bMoveEnabled_ = !isDocked;
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
}

void MainWindow::onPreferencesLastKnownGoodProtocolChanged(const QString &network, const types::Protocol &protocol, uint port)
{
    Q_UNUSED(protocol);
    Q_UNUSED(port);

    if (network == curNetwork_.networkOrSsid && backend_->isDisconnected()) {
        updateConnectWindowStateProtocolPortDisplay();
    }
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

    if (!isVisible()) {
        showNormal();
#ifdef Q_OS_WIN
        // Windows will have removed our icon overlay if the app was hidden.
        updateAppIconType(currentAppIconType_);
#endif
    }
    else if (isMinimized())
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

    if (mainWindowController_->isPreferencesVisible())
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
    // from a CLI-spawned-GUI (Mac): could fail WS_ASSERT(curWindow_ == WINDOW_ID_CONNECT) in expandLocations
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
    mainWindowController_->getExitWindow()->setSpinnerMode(true);
}

void MainWindow::onCurrentNetworkUpdated(types::NetworkInterface networkInterface)
{
    mainWindowController_->getConnectWindow()->updateNetworkState(networkInterface);
    backend_->handleNetworkChange(networkInterface, true);
}

void MainWindow::onAutoConnectUpdated(bool on)
{
    Q_UNUSED(on);
    backend_->handleNetworkChange(backend_->getCurrentNetworkInterface(), true);
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
    showTrayMessage(message);
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
    getOpenFileNameProcess.start(changeIcsExePath, { "-browseforapp" }, QIODevice::ReadOnly);
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

void MainWindow::createTrayMenuItems()
{
    if (mainWindowController_->currentWindow() == MainWindowController::WINDOW_ID_CONNECT) // logged in
    {
        if (backend_->currentConnectState() == CONNECT_STATE_DISCONNECTED)
        {
            trayMenu_.addAction(tr("Connect"), this, SLOT(onTrayMenuConnect()));
        }
        else
        {
            trayMenu_.addAction(tr("Disconnect"), this, SLOT(onTrayMenuDisconnect()));
        }
        trayMenu_.addSeparator();

#ifndef Q_OS_LINUX

#ifdef USE_LOCATIONS_TRAY_MENU_NATIVE
        if (backend_->locationsModelManager()->sortedLocationsProxyModel()->rowCount() > 0) {
            QSharedPointer<LocationsTrayMenuNative> menu(new LocationsTrayMenuNative(nullptr, backend_->locationsModelManager()->sortedLocationsProxyModel()), &QObject::deleteLater);
            menu->setTitle(tr("Locations"));
            trayMenu_.addMenu(menu.get());
            connect(menu.get(), &LocationsTrayMenuNative::locationSelected, this, &MainWindow::onLocationsTrayMenuLocationSelected);
            locationsMenu_.append(menu);
        }
        if (backend_->locationsModelManager()->favoriteCitiesProxyModel()->rowCount() > 0) {
            QSharedPointer<LocationsTrayMenuNative> menu(new LocationsTrayMenuNative(nullptr, backend_->locationsModelManager()->favoriteCitiesProxyModel()), &QObject::deleteLater);
            menu->setTitle(tr("Favourites"));
            trayMenu_.addMenu(menu.get());
            connect(menu.get(), &LocationsTrayMenuNative::locationSelected, this, &MainWindow::onLocationsTrayMenuLocationSelected);
            locationsMenu_.append(menu);
        }
        if (backend_->locationsModelManager()->staticIpsProxyModel()->rowCount() > 0) {
            QSharedPointer<LocationsTrayMenuNative> menu(new LocationsTrayMenuNative(nullptr, backend_->locationsModelManager()->staticIpsProxyModel()), &QObject::deleteLater);
            menu->setTitle(tr("Static IPs"));
            trayMenu_.addMenu(menu.get());
            connect(menu.get(), &LocationsTrayMenuNative::locationSelected, this, &MainWindow::onLocationsTrayMenuLocationSelected);
            locationsMenu_.append(menu);
        }
        if (backend_->locationsModelManager()->customConfigsProxyModel()->rowCount() > 0) {
            QSharedPointer<LocationsTrayMenuNative> menu(new LocationsTrayMenuNative(nullptr, backend_->locationsModelManager()->customConfigsProxyModel()), &QObject::deleteLater);
            menu->setTitle(tr("Configured"));
            trayMenu_.addMenu(menu.get());
            connect(menu.get(), &LocationsTrayMenuNative::locationSelected, this, &MainWindow::onLocationsTrayMenuLocationSelected);
            locationsMenu_.append(menu);
        }
#else
        if (backend_->locationsModelManager()->sortedLocationsProxyModel()->rowCount() > 0) {
            QSharedPointer<LocationsTrayMenu> menu(new LocationsTrayMenu(backend_->locationsModelManager()->sortedLocationsProxyModel(), trayMenu_.font(), trayIcon_.geometry()), &QObject::deleteLater);
            menu->setTitle(tr("Locations"));
            trayMenu_.addMenu(menu.get());
            connect(menu.get(), &LocationsTrayMenu::locationSelected, this, &MainWindow::onLocationsTrayMenuLocationSelected);
            locationsMenu_.append(menu);
        }
        if (backend_->locationsModelManager()->favoriteCitiesProxyModel()->rowCount() > 0) {
            QSharedPointer<LocationsTrayMenu> menu(new LocationsTrayMenu(backend_->locationsModelManager()->favoriteCitiesProxyModel(), trayMenu_.font(), trayIcon_.geometry()), &QObject::deleteLater);
            menu->setTitle(tr("Favourites"));
            trayMenu_.addMenu(menu.get());
            connect(menu.get(), &LocationsTrayMenu::locationSelected, this, &MainWindow::onLocationsTrayMenuLocationSelected);
            locationsMenu_.append(menu);
        }
        if (backend_->locationsModelManager()->staticIpsProxyModel()->rowCount() > 0) {
            QSharedPointer<LocationsTrayMenu> menu(new LocationsTrayMenu(backend_->locationsModelManager()->staticIpsProxyModel(), trayMenu_.font(), trayIcon_.geometry()), &QObject::deleteLater);
            menu->setTitle(tr("Static IPs"));
            trayMenu_.addMenu(menu.get());
            connect(menu.get(), &LocationsTrayMenu::locationSelected, this, &MainWindow::onLocationsTrayMenuLocationSelected);
            locationsMenu_.append(menu);
        }
        if (backend_->locationsModelManager()->customConfigsProxyModel()->rowCount() > 0) {
            QSharedPointer<LocationsTrayMenu> menu(new LocationsTrayMenu(backend_->locationsModelManager()->customConfigsProxyModel(), trayMenu_.font(), trayIcon_.geometry()), &QObject::deleteLater);
            menu->setTitle(tr("Configured"));
            trayMenu_.addMenu(menu.get());
            connect(menu.get(), &LocationsTrayMenu::locationSelected, this, &MainWindow::onLocationsTrayMenuLocationSelected);
            locationsMenu_.append(menu);
        }
#endif
        trayMenu_.addSeparator();
#endif
    }

#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
    trayMenu_.addAction(tr("Show/Hide"), this, SLOT(onTrayMenuShowHide()));
#endif

    if (!mainWindowController_->isPreferencesVisible())
    {
        trayMenu_.addAction(tr("Preferences"), this, SLOT(onTrayMenuPreferences()));
    }

    trayMenu_.addAction(tr("Help"), this, SLOT(onTrayMenuHelpMe()));
    trayMenu_.addAction(tr("Exit"), this, SLOT(onTrayMenuQuit()));
}

void MainWindow::onTrayMenuAboutToShow()
{
    trayMenu_.clear();
#ifndef Q_OS_LINUX
    locationsMenu_.clear();
#endif
#ifdef Q_OS_MAC
    if (!backend_->getPreferences()->isDockedToTray())
    {
        createTrayMenuItems();
    }
#else
    createTrayMenuItems();
#endif
}

void MainWindow::onTrayMenuAboutToHide()
{
#ifdef Q_OS_WIN
    locationsMenu_.clear();
#endif
}

void MainWindow::onLocationsTrayMenuLocationSelected(const LocationID &lid)
{
   // close menu
#ifdef Q_OS_WIN
    trayMenu_.close();
#elif !defined(USE_LOCATIONS_TRAY_MENU_NATIVE)
    #ifndef Q_OS_LINUX
        listWidgetAction_[type]->trigger(); // close doesn't work by default on mac
    #endif
#endif
    onLocationSelected(lid);
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
#if defined(Q_OS_MAC)
    // There is a bug that causes the app to be drawn in strange locations under the following scenario:
    // On Mac: when laptop lid is closed/opened and app is docked
    // Instead we hide the app because an explicit click will redraw the click correctly and this should be relatively rare
    // Any attempt to remove the bug resulted in only pushing the bug around and this is extremely tedious to test. Fair warning.
    if (backend_->getPreferences()->isDockedToTray())
    {
        // qDebug() << "New scale from DpiScaleManager (docked app) - hide app";
        deactivateAndHide();
    }
#elif defined(Q_OS_WIN)
    // When dragging the app between displays, it occasionally does not resize itself correctly, causing portions
    // of the app to be cut off.  Debugging showed that MainWindowController::updateMainAndViewGeometry is setting
    // the correct app geometry, but Qt/Windows appear to be losing the resize event.  I tried calling QWidget's
    // update(), repaint(), unpdateGeometry(), resize(), etc. here and none worked.
    // Disabling our use of QGuiApplication::setHighDpiScaleFactorRoundingPolicy() fixes the issue.  The downside
    // of disabling this API is that fonts and icons will be a bit blurry/jaggy on displays set to a non-integer
    // scale factor (e.g. 150% = 1.5 scale factor).
    QTimer::singleShot(500, this, [this]() {
        mainWindowController_->updateScaling();
    });
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
    selectedLocation_->clear();
    backend_->signOut(false);
}

void MainWindow::setupTrayIcon()
{
    updateTrayTooltip(tr("Disconnected") + "\n" + PersistentState::instance().lastExternalIp());

    trayIcon_.setContextMenu(&trayMenu_);
    connect(&trayMenu_, &QMenu::aboutToShow, this, &MainWindow::onTrayMenuAboutToShow);
    connect(&trayMenu_, &QMenu::aboutToHide, this, &MainWindow::onTrayMenuAboutToHide);

    updateAppIconType(AppIconType::DISCONNECTED);
    updateTrayIconType(AppIconType::DISCONNECTED);
    trayIcon_.show();

    connect(&trayIcon_, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);
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
    QLocale locale(LanguageController::instance().getLanguage());
    return locale.formattedDataSize(bytesTransferred_, 1, QLocale::DataSizeTraditionalFormat);
}

void MainWindow::setInitialFirewallState()
{
    bool bFirewallStateOn = PersistentState::instance().isFirewallOn();
    qCDebug(LOG_BASIC) << "Firewall state from last app start:" << bFirewallStateOn;

    if (bFirewallStateOn)
    {
        backend_->firewallOn();
        if (backend_->getPreferences()->firewallSettings().mode == FIREWALL_MODE_ALWAYS_ON)
        {
            mainWindowController_->getConnectWindow()->setFirewallAlwaysOn(true);
        }
    }
    else
    {
        if (backend_->getPreferences()->firewallSettings().mode == FIREWALL_MODE_ALWAYS_ON)
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

void MainWindow::handleDisconnectWithError(const types::ConnectState &connectState)
{
    WS_ASSERT(connectState.disconnectReason == DISCONNECTED_WITH_ERROR);

    QString msg;
    if (connectState.connectError == NO_OPENVPN_SOCKET)
    {
        msg = tr("Can't connect to openvpn process.");
    }
    else if (connectState.connectError == CANT_RUN_OPENVPN)
    {
        msg = tr("Can't start openvpn process.");
    }
    else if (connectState.connectError == COULD_NOT_FETCH_CREDENTAILS)
    {
         msg = tr("Couldn't fetch server credentials. Please try again later.");
    }
    else if (connectState.connectError == LOCATION_NOT_EXIST || connectState.connectError == LOCATION_NO_ACTIVE_NODES)
    {
        qCDebug(LOG_BASIC) << "Location not exist or no active nodes, try connect to best location";
        LocationID bestLocation = backend_->locationsModelManager()->getBestLocationId();
        selectedLocation_->set(bestLocation);
        PersistentState::instance().setLastLocation(selectedLocation_->locationdId());
        if (selectedLocation_->isValid())
        {
            mainWindowController_->getConnectWindow()->updateLocationInfo(selectedLocation_->firstName(), selectedLocation_->secondName(),
                                                                          selectedLocation_->countryCode(), selectedLocation_->pingTime(),
                                                                          selectedLocation_->locationdId().isCustomConfigsLocation());
            onConnectWindowConnectClick();
        }
        else
        {
            qCDebug(LOG_BASIC) << "Best Location not exist or no active nodes, goto disconnected mode";
        }
        return;
    }
    else if (connectState.connectError == ALL_TAP_IN_USE)
    {
        msg = tr("All TAP-Windows adapters on this system are currently in use.");
    }
    else if (connectState.connectError == IKEV_FAILED_SET_ENTRY_WIN || connectState.connectError == IKEV_NOT_FOUND_WIN)
    {
        msg = tr("IKEv2 connection failed. Please send a debug log and open a support ticket. You can switch to UDP or TCP connection modes in the mean time.");
    }
    else if (connectState.connectError == IKEV_FAILED_MODIFY_HOSTS_WIN)
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
    else if (connectState.connectError == IKEV_NETWORK_EXTENSION_NOT_FOUND_MAC)
    {
        msg = tr("Failed to load the network extension framework.");
    }
    else if (connectState.connectError == IKEV_FAILED_SET_KEYCHAIN_MAC)
    {
        msg = tr("Failed set password to keychain.");
    }
    else if (connectState.connectError == IKEV_FAILED_START_MAC)
    {
        msg = tr("Failed to start IKEv2 connection.");
    }
    else if (connectState.connectError == IKEV_FAILED_LOAD_PREFERENCES_MAC)
    {
        msg = tr("Failed to load IKEv2 preferences.");
    }
    else if (connectState.connectError == IKEV_FAILED_SAVE_PREFERENCES_MAC)
    {
        msg = tr("Failed to create IKEv2 Profile. Please connect again and select \"Allow\".");
    }
    else if (connectState.connectError == WIREGUARD_CONNECTION_ERROR)
    {
        msg = tr("Failed to setup WireGuard connection.");
    }
#ifdef Q_OS_WIN
    else if (connectState.connectError == NO_INSTALLED_TUN_TAP)
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
    else if (connectState.connectError == CONNECTION_BLOCKED)
    {
        if (blockConnect_.isBlockedExceedTraffic()) {
            mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_UPGRADE);
            return;
        }
        msg = blockConnect_.message();
    }
    else if (connectState.connectError == CANNOT_OPEN_CUSTOM_CONFIG)
    {
        LocationID bestLocation{backend_->locationsModelManager()->getBestLocationId()};
        if (bestLocation.isValid())
        {
            selectedLocation_->set(bestLocation);
            PersistentState::instance().setLastLocation(selectedLocation_->locationdId());
            mainWindowController_->getConnectWindow()->updateLocationInfo(selectedLocation_->firstName(), selectedLocation_->secondName(),
                                                                          selectedLocation_->countryCode(), selectedLocation_->pingTime(),
                                                                          selectedLocation_->locationdId().isCustomConfigsLocation());
        }
        msg = tr("Failed to setup custom openvpn configuration.");
    }
    else if(connectState.connectError == WINTUN_DRIVER_REINSTALLATION_ERROR)
    {
        msg = tr("Wintun driver fatal error. Failed to reinstall it automatically. Please try to reinstall it manually.");
    }
    else if(connectState.connectError == TAP_DRIVER_REINSTALLATION_ERROR)
    {
        msg = tr("Tap driver Fatal error. Failed to reinstall it automatically. Please try to reinstall it manually.");
    }
    else if (connectState.connectError == EXE_VERIFY_WSTUNNEL_ERROR)
    {
        msg = tr("WSTunnel binary failed verification. Please re-install windscribe from trusted source.");
    }
    else if (connectState.connectError == EXE_VERIFY_STUNNEL_ERROR)
    {
        msg = tr("STunnel binary failed verification. Please re-install windscribe from trusted source.");
    }
    else if (connectState.connectError == EXE_VERIFY_WIREGUARD_ERROR)
    {
        msg = tr("Wireguard binary failed verification. Please re-install windscribe from trusted source.");
    }
    else if (connectState.connectError == EXE_VERIFY_OPENVPN_ERROR)
    {
        msg = tr("OpenVPN binary failed verification. Please re-install windscribe from trusted source.");
    }
    else if (connectState.connectError == WIREGUARD_ADAPTER_SETUP_FAILED)
    {
        msg = tr("WireGuard adapter setup failed. Please wait one minute and try the connection again. If adapter setup fails again,"
                 " please try restarting your computer.\n\nIf the problem persists after a restart, please send a debug log and open"
                 " a support ticket, then switch to a different connection mode.");
    }
    else
    {
         msg = tr("Error during connection (%1)").arg(QString::number(connectState.connectError));
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

void MainWindow::gotoLogoutWindow()
{
    if (mainWindowController_->currentWindow() == MainWindowController::WINDOW_ID_LOGOUT)
        return;
    isExitingFromPreferences_ = mainWindowController_->isPreferencesVisible();
    if (isExitingFromPreferences_)
        collapsePreferences();
    mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_LOGOUT);
}

void MainWindow::gotoExitWindow()
{
    if (mainWindowController_->currentWindow() == MainWindowController::WINDOW_ID_EXIT)
        return;
    isExitingFromPreferences_ = mainWindowController_->isPreferencesVisible();
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
    // We need to be able to recreate the icon overlay on Windows even if the icon type hasn't changed.
    #if !defined(Q_OS_WIN)
    if (currentAppIconType_ == type)
        return;
    #endif

    const QIcon *icon = nullptr;

    switch (type) {
    case AppIconType::DISCONNECTED:
        // No taskbar button overlay icon on Windows.
        #if !defined(Q_OS_WIN)
        icon = IconManager::instance().getDisconnectedIcon();
        #endif
        break;
    case AppIconType::DISCONNECTED_WITH_ERROR:
        #if defined(Q_OS_WIN)
        icon = IconManager::instance().getErrorOverlayIcon();
        #else
        icon = IconManager::instance().getDisconnectedIcon();
        #endif
        break;
    case AppIconType::CONNECTING:
        #if defined(Q_OS_WIN)
        icon = IconManager::instance().getConnectingOverlayIcon();
        #else
        icon = IconManager::instance().getConnectingIcon();
        #endif
        break;
    case AppIconType::CONNECTED:
        #if defined(Q_OS_WIN)
        icon = IconManager::instance().getConnectedOverlayIcon();
        #else
        icon = IconManager::instance().getConnectedIcon();
        #endif
        break;
    default:
        break;
    }

    #if defined(Q_OS_WIN)
    // Qt's setWindowIcon() API cannot change the taskbar icon on Windows.
    WidgetUtils_win::setTaskbarIconOverlay(*this, icon);
    #else
    if (icon) {
        qApp->setWindowIcon(*icon);
    }
    #endif

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

void MainWindow::onSelectedLocationChanged()
{
    WS_ASSERT(selectedLocation_->isValid());
    // If the best location has changed and we are not disconnected, then transform the current location into a normal one.
    if (selectedLocation_->locationdId().isBestLocation())
    {
        WS_ASSERT(selectedLocation_->prevLocationdId().isBestLocation());
        if (!backend_->isDisconnected())
        {
            selectedLocation_->set(selectedLocation_->prevLocationdId().bestLocationToApiLocation());
            PersistentState::instance().setLastLocation(selectedLocation_->locationdId());
            if (!selectedLocation_->isValid())
            {
                // Just don't update the connect window in this case
                return;
            }
        }
    }

    mainWindowController_->getConnectWindow()->updateLocationInfo(selectedLocation_->firstName(), selectedLocation_->secondName(),
                                                                  selectedLocation_->countryCode(), selectedLocation_->pingTime(),
                                                                  selectedLocation_->locationdId().isCustomConfigsLocation());

}

void MainWindow::onSelectedLocationRemoved()
{
    if (backend_->isDisconnected())
    {
        LocationID bestLocation = backend_->locationsModelManager()->getBestLocationId();
        WS_ASSERT(bestLocation.isValid());
        selectedLocation_->set(bestLocation);
        WS_ASSERT(selectedLocation_->isValid());
        PersistentState::instance().setLastLocation(selectedLocation_->locationdId());
        mainWindowController_->getConnectWindow()->updateLocationInfo(selectedLocation_->firstName(), selectedLocation_->secondName(),
                                                                      selectedLocation_->countryCode(), selectedLocation_->pingTime(),
                                                                      selectedLocation_->locationdId().isCustomConfigsLocation());
    }
}

void MainWindow::onHelperSplitTunnelingStartFailed()
{
    mainWindowController_->getConnectWindow()->setSplitTunnelingState(false);
    mainWindowController_->getPreferencesWindow()->setSplitTunnelingActive(false);

    QMessageBox::warning(g_mainWindow, tr("Windscribe"),
        tr("The split tunneling feature could not be started, and has been disabled in Preferences."));
}

void MainWindow::showTrayMessage(const QString &message)
{
    if (trayIcon_.isSystemTrayAvailable()) {
        trayIcon_.showMessage(tr("Windscribe"), message);
        return;
    }

#if defined(Q_OS_LINUX)
    QDBusInterface dbus("org.freedesktop.Notifications", "/org/freedesktop/Notifications",
                        "org.freedesktop.Notifications", QDBusConnection::sessionBus());

    if (!dbus.isValid()) {
        qCDebug(LOG_BASIC) << "MainWindow::showTrayMessage - could not connect to the notification manager using dbus."
                           << (dbus.lastError().isValid() ? dbus.lastError().message() : "");
        return;
    }

    // Leaving these here as documentation, and in case we need to use them in the future.
    uint replacesId = 0;
    QStringList actions;
    QMap<QString, QVariant> hints;

    // This will show the default 'information' icon.
    const char *appIcon = "";

    QDBusReply<uint> reply = dbus.call("Notify", "Windscribe VPN", replacesId, appIcon,
                                       "Windscribe", message, actions, hints, 10000);
    if (!reply.isValid()) {
        qCDebug(LOG_BASIC) << "MainWindow::showTrayMessage - could not display the message."
                           << (dbus.lastError().isValid() ? dbus.lastError().message() : "");
    }
#else
    qCDebug(LOG_BASIC) << "QSystemTrayIcon reports the system tray is not available";
#endif
}

types::Protocol MainWindow::getDefaultProtocolForNetwork(const QString &network)
{
    // if there is a preferred protocol for this network, it is the default
    if (backend_->getPreferences()->hasNetworkPreferredProtocol(network)) {
        return backend_->getPreferences()->networkPreferredProtocol(network).protocol();
    // next, if connection settings specifies a manual protocol, it is the default
    } else if (!backend_->getPreferences()->connectionSettings().isAutomatic()) {
        return backend_->getPreferences()->connectionSettings().protocol();
    } else {
        // otherwise, it's automatic.  if there is a last known good protocol, that is the default
        types::Protocol p = backend_->getPreferences()->networkLastKnownGoodProtocol(network);
        if (p.isValid()) {
            return p;
        }
        // if there's no valid last known good protocol, the default is wireguard
        return types::Protocol(types::Protocol::TYPE::WIREGUARD);
    }
}

void MainWindow::onMsgBoxClicked(QAbstractButton *button) {
    Q_UNUSED(button);
    SAFE_DELETE_LATER(tunnelTestMsgBox_);
}
