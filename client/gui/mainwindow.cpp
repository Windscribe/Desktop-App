#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDesktopServices>
#include <QFileDialog>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QScreen>
#include <QStyleHints>
#include <QThread>
#include <QTimer>
#include <QWindow>
#include <QWidgetAction>

#if defined(Q_OS_MACOS)
#include <QPermission>
#elif defined(Q_OS_LINUX)
#include <QtDBus/QtDBus>
#endif

#include "application/windscribeapplication.h"
#include "backend/persistentstate.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/imageresourcesjpg.h"
#include "languagecontroller.h"
#include "launchonstartup/launchonstartup.h"
#include "locations/locationsmodel_roles.h"
#include "mainwindowstate.h"
#include "multipleaccountdetection/multipleaccountdetectionfactory.h"
#include "showingdialogstate.h"
#include "utils/authcheckerfactory.h"
#include "utils/extraconfig.h"
#include "utils/hardcodedsettings.h"
#include "utils/iauthchecker.h"
#include "utils/interfaceutils.h"
#include "utils/log/categories.h"
#include "utils/log/multiline_message_logger.h"
#include "utils/network_utils/network_utils.h"
#include "utils/utils.h"
#include "utils/writeaccessrightschecker.h"
#include "utils/ws_assert.h"

#if defined(Q_OS_WIN)
    #include "utils/network_utils/network_utils_win.h"
    #include "utils/winutils.h"
    #include "widgetutils/widgetutils_win.h"
    #include <windows.h>
#elif defined(Q_OS_LINUX)
    #include <unistd.h>
    #include "utils/authchecker_linux.h"
#else
    #include <unistd.h>
    #include "utils/authchecker_mac.h"
    #include "utils/macutils.h"
    #include "utils/network_utils/network_utils_mac.h"
    #include "widgetutils/widgetutils_mac.h"
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
    logoutReason_(LOGOUT_UNDEFINED),
    isInitializationAborted_(false),
    isLoginOkAndConnectWindowVisible_(false),
    revealingConnectWindow_(false),
    internetConnected_(false),
    backendAppActiveState_(true),
#ifdef Q_OS_MACOS
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
    sendDebugLogOnDisconnect_(false)
{
    g_mainWindow = this;

    // Initialize "fallback" tray icon geometry.
    const QScreen *screen = nullptr;
    QRect desktopAvailableRc;
    if (qApp->screens().size() == 0 || qApp->screens().at(0) == NULL) {
        qCCritical(LOG_BASIC) << "No screen for fallback tray icon init"; // This shouldn't ever happen
    } else {
        screen = qApp->screens().at(0);
        desktopAvailableRc = screen->availableGeometry();
        savedTrayIconRect_.setTopLeft(QPoint(desktopAvailableRc.right() - WINDOW_WIDTH * G_SCALE, 0));
        savedTrayIconRect_.setSize(QSize(22, 22));
    }

#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    // We should do this part for Linux in the other place of the constructor in order to avoid incorrect icon color setup.
    trayIconColorWhite_ = InterfaceUtils::isDarkMode();
    qCDebug(LOG_BASIC) << "OS in dark mode: " << trayIconColorWhite_;

    // Init and show tray icon.
    trayIcon_.setIcon(*IconManager::instance().getDisconnectedTrayIcon(trayIconColorWhite_));
#endif

    trayIcon_.show();

#ifdef Q_OS_MACOS
    if (screen) {
        const QRect desktopScreenRc = screen->geometry();
        if (desktopScreenRc.top() != desktopAvailableRc.top()) {
            while (trayIcon_.geometry().isEmpty())
                qApp->processEvents();
        }
    }
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

#if defined(Q_OS_MACOS)
    permissionMonitor_ = new PermissionMonitor_mac(this);
    connect(permissionMonitor_, &PermissionMonitor_mac::locationPermissionUpdated, this, &MainWindow::onLocationPermissionUpdated);
#endif

#ifdef Q_OS_MACOS
    WidgetUtils_mac::allowMinimizeForFramelessWindow(this);
#endif

    connect(backend_, &Backend::initFinished, this, &MainWindow::onBackendInitFinished);
    connect(backend_, &Backend::loginFinished, this, &MainWindow::onBackendLoginFinished);
    connect(backend_, &Backend::tryingBackupEndpoint, this, &MainWindow::onBackendTryingBackupEndpoint);
    connect(backend_, &Backend::loginError, this, &MainWindow::onBackendLoginError);
    connect(backend_, &Backend::logoutFinished, this, &MainWindow::onBackendLogoutFinished);
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
    connect(backend_, &Backend::requestCustomOvpnConfigPrivKeyPassword, this, &MainWindow::onBackendRequestCustomOvpnConfigPrivKeyPassword);
    connect(backend_, &Backend::proxySharingInfoChanged, this, &MainWindow::onBackendProxySharingInfoChanged);
    connect(backend_, &Backend::wifiSharingInfoChanged, this, &MainWindow::onBackendWifiSharingInfoChanged);
    connect(backend_, &Backend::wifiSharingFailed, this, &MainWindow::onBackendWifiSharingFailed);
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
    connect(backend_->getPreferences(), &Preferences::customConfigNeedsUpdate, this, &MainWindow::onPreferencesCustomConfigPathNeedsUpdate);
    connect(backend_->getPreferences(), &Preferences::isShowNotificationsChanged, this, &MainWindow::onPreferencesShowNotificationsChanged);

    localIpcServer_ = new LocalIPCServer(backend_, this);
    connect(localIpcServer_, &LocalIPCServer::showLocations, this, &MainWindow::onIpcOpenLocations);
    connect(localIpcServer_, &LocalIPCServer::connectToLocation, this, &MainWindow::onIpcConnect);
    connect(localIpcServer_, &LocalIPCServer::connectToStaticIpLocation, this, &MainWindow::onIpcConnectStaticIp);
    connect(localIpcServer_, &LocalIPCServer::attemptLogin, this, &MainWindow::onLoginClick);
    connect(localIpcServer_, &LocalIPCServer::update, this, &MainWindow::onIpcUpdate);

    mainWindowController_ = new MainWindowController(this, locationsWindow_, backend_->getPreferencesHelper(), backend_->getPreferences(), backend_->getAccountInfo());

    mainWindowController_->getConnectWindow()->updateMyIp(PersistentState::instance().lastExternalIp());
    mainWindowController_->getConnectWindow()->updateNotificationsState(notificationsController_.totalMessages(), notificationsController_.unreadMessages());
    mainWindowController_->getConnectWindow()->connect(
        &notificationsController_, &NotificationsController::stateChanged,
        mainWindowController_->getConnectWindow(),
        &ConnectWindow::ConnectWindowItem::updateNotificationsState);

    connect(&notificationsController_, &NotificationsController::newPopupMessage, this, &MainWindow::onNotificationControllerNewPopupMessage);
    connect(mainWindowController_->getNewsFeedWindow(), &NewsFeedWindow::NewsFeedWindowItem::messageRead, &notificationsController_, &NotificationsController::setNotificationRead);
    mainWindowController_->getNewsFeedWindow()->setMessages( notificationsController_.messages(), notificationsController_.shownIds());

    // init window signals
    connect(mainWindowController_->getInitWindow(), &LoginWindow::InitWindowItem::abortClicked, this, &MainWindow::onAbortInitialization);

    // login window signals
    connect(mainWindowController_->getLoginWindow(), &LoginWindow::LoginWindowItem::loginClick, this, &MainWindow::onLoginClick);
    connect(mainWindowController_->getLoginWindow(), &LoginWindow::LoginWindowItem::minimizeClick, this, &MainWindow::onMinimizeClick);
    connect(mainWindowController_->getLoginWindow(), &LoginWindow::LoginWindowItem::closeClick, this, &MainWindow::onCloseClick);
    connect(mainWindowController_->getLoginWindow(), &LoginWindow::LoginWindowItem::preferencesClick, this, &MainWindow::onLoginPreferencesClick);
    connect(mainWindowController_->getLoginWindow(), &LoginWindow::LoginWindowItem::haveAccountYesClick, this, &MainWindow::onLoginHaveAccountYesClick);
    connect(mainWindowController_->getLoginWindow(), &LoginWindow::LoginWindowItem::backToWelcomeClick, this, &MainWindow::onLoginBackToWelcomeClick);
    connect(mainWindowController_->getLoginWindow(), &LoginWindow::LoginWindowItem::emergencyConnectClick, this, &MainWindow::onLoginEmergencyWindowClick);
    connect(mainWindowController_->getLoginWindow(), &LoginWindow::LoginWindowItem::externalConfigModeClick, this, &MainWindow::onLoginExternalConfigWindowClick);
    connect(mainWindowController_->getLoginWindow(), &LoginWindow::LoginWindowItem::twoFactorAuthClick, this, &MainWindow::onLoginTwoFactorAuthWindowClick);
    connect(mainWindowController_->getLoginWindow(), &LoginWindow::LoginWindowItem::firewallTurnOffClick, this, &MainWindow::onLoginFirewallTurnOffClick);

    // connect window signals
    connect(mainWindowController_->getConnectWindow(), &ConnectWindow::ConnectWindowItem::minimizeClick, this, &MainWindow::onMinimizeClick);
    connect(mainWindowController_->getConnectWindow(), &ConnectWindow::ConnectWindowItem::closeClick, this, &MainWindow::onCloseClick);
    connect(mainWindowController_->getConnectWindow(), &ConnectWindow::ConnectWindowItem::connectClick, this, &MainWindow::onConnectWindowConnectClick);
    connect(mainWindowController_->getConnectWindow(), &ConnectWindow::ConnectWindowItem::firewallClick, this, &MainWindow::onConnectWindowFirewallClick);
    connect(mainWindowController_->getConnectWindow(), &ConnectWindow::ConnectWindowItem::networkButtonClick, this, &MainWindow::onConnectWindowNetworkButtonClick);
    connect(mainWindowController_->getConnectWindow(), &ConnectWindow::ConnectWindowItem::locationsClick, this, &MainWindow::onConnectWindowLocationsClick);
    connect(mainWindowController_->getConnectWindow(), &ConnectWindow::ConnectWindowItem::preferencesClick, this, &MainWindow::onConnectWindowPreferencesClick);
    connect(mainWindowController_->getConnectWindow(), &ConnectWindow::ConnectWindowItem::notificationsClick, this, &MainWindow::onConnectWindowNotificationsClick);
    connect(mainWindowController_->getConnectWindow(), &ConnectWindow::ConnectWindowItem::splitTunnelingButtonClick, this, &MainWindow::onConnectWindowSplitTunnelingClick);
    connect(mainWindowController_->getConnectWindow(), &ConnectWindow::ConnectWindowItem::protocolsClick, this, &MainWindow::onConnectWindowProtocolsClick);

    mainWindowController_->getConnectWindow()->connect(backend_, &Backend::firewallStateChanged, mainWindowController_->getConnectWindow(), &ConnectWindow::ConnectWindowItem::updateFirewallState);

    // news feed window signals
    connect(mainWindowController_->getNewsFeedWindow(), &NewsFeedWindow::NewsFeedWindowItem::escape, this, &MainWindow::onEscapeNotificationsClick);

    // protocols window signals
    connect(mainWindowController_->getProtocolWindow(), &ProtocolWindow::ProtocolWindowItem::escape, this, &MainWindow::onEscapeProtocolsClick);
    connect(mainWindowController_->getProtocolWindow(), &ProtocolWindow::ProtocolWindowItem::protocolClicked, this, &MainWindow::onProtocolWindowProtocolClick);
    connect(mainWindowController_->getProtocolWindow(), &ProtocolWindow::ProtocolWindowItem::stopConnection, this, &MainWindow::onProtocolWindowDisconnect);

    // preferences window signals
    connect(mainWindowController_->getPreferencesWindow(), &PreferencesWindow::PreferencesWindowItem::quitAppClick, this, &MainWindow::onPreferencesQuitAppClick);
    connect(mainWindowController_->getPreferencesWindow(), &PreferencesWindow::PreferencesWindowItem::escape, this, &MainWindow::onPreferencesEscapeClick);
    connect(mainWindowController_->getPreferencesWindow(), &PreferencesWindow::PreferencesWindowItem::logoutClick, this, &MainWindow::onPreferencesLogoutClick);
    connect(mainWindowController_->getPreferencesWindow(), &PreferencesWindow::PreferencesWindowItem::loginClick, this, &MainWindow::onPreferencesLoginClick);
    connect(mainWindowController_->getPreferencesWindow(), &PreferencesWindow::PreferencesWindowItem::viewLogClick, this, &MainWindow::onPreferencesViewLogClick);
    connect(mainWindowController_->getPreferencesWindow(), &PreferencesWindow::PreferencesWindowItem::exportSettingsClick, this, &MainWindow::onPreferencesExportSettingsClick);
    connect(mainWindowController_->getPreferencesWindow(), &PreferencesWindow::PreferencesWindowItem::importSettingsClick, this, &MainWindow::onPreferencesImportSettingsClick);
    connect(mainWindowController_->getPreferencesWindow(), &PreferencesWindow::PreferencesWindowItem::advancedParametersClicked, this, &MainWindow::onPreferencesAdvancedParametersClicked);
    connect(mainWindowController_->getPreferencesWindow(), &PreferencesWindow::PreferencesWindowItem::currentNetworkUpdated, this, &MainWindow::onCurrentNetworkUpdated);
    connect(mainWindowController_->getPreferencesWindow(), &PreferencesWindow::PreferencesWindowItem::sendConfirmEmailClick, this, &MainWindow::onPreferencesSendConfirmEmailClick);
    connect(mainWindowController_->getPreferencesWindow(), &PreferencesWindow::PreferencesWindowItem::sendDebugLogClick, this, &MainWindow::onSendDebugLogClick);
    connect(mainWindowController_->getPreferencesWindow(), &PreferencesWindow::PreferencesWindowItem::manageAccountClick, this, &MainWindow::onPreferencesManageAccountClick);
    connect(mainWindowController_->getPreferencesWindow(), &PreferencesWindow::PreferencesWindowItem::addEmailButtonClick, this, &MainWindow::onPreferencesAddEmailButtonClick);
    connect(mainWindowController_->getPreferencesWindow(), &PreferencesWindow::PreferencesWindowItem::manageRobertRulesClick, this, &MainWindow::onPreferencesManageRobertRulesClick);
    connect(mainWindowController_->getPreferencesWindow(), &PreferencesWindow::PreferencesWindowItem::accountLoginClick, this, &MainWindow::onPreferencesAccountLoginClick);
    connect(mainWindowController_->getPreferencesWindow(), &PreferencesWindow::PreferencesWindowItem::cycleMacAddressClick, this, &MainWindow::onPreferencesCycleMacAddressClick);
    connect(mainWindowController_->getPreferencesWindow(), &PreferencesWindow::PreferencesWindowItem::detectPacketSizeClick, this, &MainWindow::onPreferencesWindowDetectPacketSizeClick);
    connect(mainWindowController_->getPreferencesWindow(), &PreferencesWindow::PreferencesWindowItem::getRobertFilters, this, &MainWindow::onPreferencesGetRobertFilters);
    connect(mainWindowController_->getPreferencesWindow(), &PreferencesWindow::PreferencesWindowItem::setRobertFilter, this, &MainWindow::onPreferencesSetRobertFilter);
    connect(mainWindowController_->getPreferencesWindow(), &PreferencesWindow::PreferencesWindowItem::splitTunnelingAppsAddButtonClick, this, &MainWindow::onSplitTunnelingAppsAddButtonClick);

    // emergency window signals
    connect(mainWindowController_->getEmergencyConnectWindow(), &EmergencyConnectWindow::EmergencyConnectWindowItem::minimizeClick, this, &MainWindow::onMinimizeClick);
    connect(mainWindowController_->getEmergencyConnectWindow(), &EmergencyConnectWindow::EmergencyConnectWindowItem::closeClick, this, &MainWindow::onCloseClick);
    connect(mainWindowController_->getEmergencyConnectWindow(), &EmergencyConnectWindow::EmergencyConnectWindowItem::escapeClick, this, &MainWindow::onEscapeClick);
    connect(mainWindowController_->getEmergencyConnectWindow(), &EmergencyConnectWindow::EmergencyConnectWindowItem::connectClick, this, &MainWindow::onEmergencyConnectClick);
    connect(mainWindowController_->getEmergencyConnectWindow(), &EmergencyConnectWindow::EmergencyConnectWindowItem::disconnectClick, this, &MainWindow::onEmergencyDisconnectClick);

    // external config window signals
    connect(mainWindowController_->getExternalConfigWindow(), &ExternalConfigWindow::ExternalConfigWindowItem::buttonClick, this, &MainWindow::onExternalConfigWindowNextClick);
    connect(mainWindowController_->getExternalConfigWindow(), &ExternalConfigWindow::ExternalConfigWindowItem::escapeClick, this, &MainWindow::onEscapeClick);
    connect(mainWindowController_->getExternalConfigWindow(), &ExternalConfigWindow::ExternalConfigWindowItem::closeClick, this, &MainWindow::onCloseClick);
    connect(mainWindowController_->getExternalConfigWindow(), &ExternalConfigWindow::ExternalConfigWindowItem::minimizeClick, this, &MainWindow::onMinimizeClick);

    // 2FA window signals
    connect(mainWindowController_->getTwoFactorAuthWindow(), &TwoFactorAuthWindow::TwoFactorAuthWindowItem::addClick, this, &MainWindow::onTwoFactorAuthWindowButtonAddClick);
    connect(mainWindowController_->getTwoFactorAuthWindow(), &TwoFactorAuthWindow::TwoFactorAuthWindowItem::loginClick, this, &MainWindow::onLoginClick);
    connect(mainWindowController_->getTwoFactorAuthWindow(), &TwoFactorAuthWindow::TwoFactorAuthWindowItem::escapeClick, this, &MainWindow::onEscapeClick);
    connect(mainWindowController_->getTwoFactorAuthWindow(), &TwoFactorAuthWindow::TwoFactorAuthWindowItem::closeClick, this, &MainWindow::onCloseClick);
    connect(mainWindowController_->getTwoFactorAuthWindow(), &TwoFactorAuthWindow::TwoFactorAuthWindowItem::minimizeClick, this, &MainWindow::onMinimizeClick);

    // bottom window signals
    connect(mainWindowController_->getBottomInfoWindow(), &SharingFeatures::BottomInfoItem::upgradeClick, this, &MainWindow::onUpgradeAccountAccept);
    connect(mainWindowController_->getBottomInfoWindow(), &SharingFeatures::BottomInfoItem::renewClick, this, &MainWindow::onBottomWindowRenewClick);
    connect(mainWindowController_->getBottomInfoWindow(), &SharingFeatures::BottomInfoItem::loginClick, this, &MainWindow::onBottomWindowExternalConfigLoginClick);
    connect(mainWindowController_->getBottomInfoWindow(), &SharingFeatures::BottomInfoItem::proxyGatewayClick, this, &MainWindow::onBottomWindowSharingFeaturesClick);
    connect(mainWindowController_->getBottomInfoWindow(), &SharingFeatures::BottomInfoItem::secureHotspotClick, this, &MainWindow::onBottomWindowSharingFeaturesClick);

    // update app item signals
    connect(mainWindowController_->getUpdateAppItem(), &UpdateApp::UpdateAppItem::updateClick, this, &MainWindow::onUpdateAppItemClick);

    // update window signals
    connect(mainWindowController_->getUpdateWindow(), &UpdateWindowItem::acceptClick, this, &MainWindow::onUpdateWindowAccept);
    connect(mainWindowController_->getUpdateWindow(), &UpdateWindowItem::cancelClick, this, &MainWindow::onUpdateWindowCancel);
    connect(mainWindowController_->getUpdateWindow(), &UpdateWindowItem::laterClick, this, &MainWindow::onUpdateWindowLater);

    // upgrade window signals
    connect(mainWindowController_->getUpgradeWindow(), &UpgradeWindow::UpgradeWindowItem::acceptClick, this, &MainWindow::onUpgradeAccountAccept);
    connect(mainWindowController_->getUpgradeWindow(), &UpgradeWindow::UpgradeWindowItem::cancelClick, this, &MainWindow::onUpgradeAccountCancel);

    // logout & exit window signals
    connect(mainWindowController_->getLogoutWindow(), &GeneralMessageWindow::GeneralMessageWindowItem::acceptClick, this, &MainWindow::onLogoutWindowAccept);
    connect(mainWindowController_->getLogoutWindow(), &GeneralMessageWindow::GeneralMessageWindowItem::rejectClick, this, &MainWindow::onExitWindowReject);
    connect(mainWindowController_->getExitWindow(), &GeneralMessageWindow::GeneralMessageWindowItem::acceptClick, this, &MainWindow::onExitWindowAccept);
    connect(mainWindowController_->getExitWindow(), &GeneralMessageWindow::GeneralMessageWindowItem::rejectClick, this, &MainWindow::onExitWindowReject);

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
#if defined(Q_OS_MACOS)
    connect(backend_->getPreferences(), &Preferences::hideFromDockChanged, this, &MainWindow::onPreferencesHideFromDockChanged);
    connect(backend_->getPreferences(), &Preferences::multiDesktopBehaviorChanged, this, &MainWindow::onPreferencesMultiDesktopBehaviorChanged);
#elif defined(Q_OS_LINUX)
    connect(backend_->getPreferences(), &Preferences::trayIconColorChanged, this, &MainWindow::onPreferencesTrayIconColorChanged);
#endif
    connect(backend_->getPreferences(), &Preferences::networkLastKnownGoodProtocolPortChanged, this, &MainWindow::onPreferencesLastKnownGoodProtocolChanged);

    // WindscribeApplication signals
    WindscribeApplication * app = WindscribeApplication::instance();
    connect(app, &WindscribeApplication::clickOnDock, this, &MainWindow::onDockIconClicked);
    connect(app, &WindscribeApplication::activateFromAnotherInstance, this, &MainWindow::onAppActivateFromAnotherInstance);
    connect(app, &WindscribeApplication::shouldTerminate_mac, this, &MainWindow::onAppShouldTerminate_mac);
    connect(app, &WindscribeApplication::focusWindowChanged, this, &MainWindow::onFocusWindowChanged);
    connect(app, &WindscribeApplication::applicationCloseRequest, this, &MainWindow::onAppCloseRequest);
#if defined(Q_OS_WIN)
    connect(app, &WindscribeApplication::winIniChanged, this, &MainWindow::onAppWinIniChanged);
#endif

    mainWindowController_->getViewport()->installEventFilter(this);
    connect(mainWindowController_, &MainWindowController::shadowUpdated, this, qOverload<>(&MainWindow::update));
    connect(mainWindowController_, &MainWindowController::revealConnectWindowStateChanged, this, &MainWindow::onRevealConnectStateChanged);

    GeneralMessageController::instance().setMainWindowController(mainWindowController_);

# if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    setupTrayIcon();
#endif

    backend_->locationsModelManager()->setLocationOrder(backend_->getPreferences()->locationOrder());
    selectedLocation_.reset(new gui_locations::SelectedLocation(backend_->locationsModelManager()->locationsModel()));
    connect(selectedLocation_.get(), &gui_locations::SelectedLocation::changed, this, &MainWindow::onSelectedLocationChanged);
    connect(selectedLocation_.get(), &gui_locations::SelectedLocation::removed, this, &MainWindow::onSelectedLocationRemoved);

    connect(&DpiScaleManager::instance(), &DpiScaleManager::scaleChanged, this, &MainWindow::onScaleChanged);
    connect(&DpiScaleManager::instance(), &DpiScaleManager::newScreen, this, &MainWindow::onDpiScaleManagerNewScreen);

    backend_->init();

// Tray icon setup on Linux should be there.
#if defined(Q_OS_LINUX)
    trayIconColorWhite_ = (backend_->getPreferences()->trayIconColor() == TRAY_ICON_COLOR::TRAY_ICON_COLOR_WHITE);
    qCDebug(LOG_BASIC) << "Tray icon color is " << (trayIconColorWhite_ ? "white" : "black");
    trayIcon_.setIcon(*IconManager::instance().getDisconnectedTrayIcon(trayIconColorWhite_));
    setupTrayIcon();
#endif

    mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_INITIALIZATION);
    mainWindowController_->getInitWindow()->startWaitingAnimation();

    mainWindowController_->setIsDockedToTray(backend_->getPreferences()->isDockedToTray());


    if (!backend_->getPreferences()->isDockedToTray()) {
        mainWindowController_->setWindowPosFromPersistent();
    }

#if defined(Q_OS_MACOS)
    hideShowDockIconTimer_.setSingleShot(true);
    connect(&hideShowDockIconTimer_, &QTimer::timeout, this, [this]() {
        hideShowDockIconImpl(true);
    });
#endif
    deactivationTimer_.setSingleShot(true);
    connect(&deactivationTimer_, &QTimer::timeout, this, &MainWindow::onWindowDeactivateAndHideImpl);

    QTimer::singleShot(0, this, &MainWindow::setWindowToDpiScaleManager);
}

MainWindow::~MainWindow()
{

}

void MainWindow::showAfterLaunch()
{
    if (!backend_) {
        qCWarning(LOG_BASIC) << "Backend is nullptr!";
    }

    // Report the tray geometry after we've given the app some startup time.
    qCDebug(LOG_BASIC) << "Tray Icon geometry:" << trayIcon_.geometry();

#ifdef Q_OS_MACOS
    // Do not showMinimized if hide from dock is enabled.  Otherwise, the app will fail to show
    // itself when the user selects 'Show' in the app's system tray menu.
    if (backend_) {
        if (backend_->getPreferences()->isHideFromDock()) {
            desiredDockIconVisibility_ = false;
            hideShowDockIconImpl(!backend_->getPreferences()->isStartMinimized());
            return;
        }

        onPreferencesMultiDesktopBehaviorChanged(backend_->getPreferences()->multiDesktopBehavior());
    }
#endif

    bool showAppMinimized = false;

    if (backend_ && backend_->getPreferences()->isStartMinimized()) {
        // Set active state to false; this affects the first click of the dock icon after launch if the app is docked.
        // This state is 'true' by default, and if left as is, the first click of the dock icon will not activate the app.
        activeState_ = false;
        showMinimized();
    }
    else {
        show();
    }

}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // qDebug() << "MainWindow eventFilter: " << event->type();

    if (watched == mainWindowController_->getViewport() && event->type() == QEvent::MouseButtonRelease) {
        mouseReleaseEvent((QMouseEvent *)event);
    }
    return QWidget::eventFilter(watched, event);
}

bool MainWindow::doClose(QCloseEvent *event, bool isFromSigTerm_mac)
{
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
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
            return false;
        }
    }
#endif  // Q_OS_WIN || Q_OS_MACOS

    setEnabled(false);

    // for startup fix (when app disabled in task manager)
    LaunchOnStartup::instance().setLaunchOnStartup(backend_->getPreferences()->isLaunchOnStartup());

#if defined(Q_OS_WIN)
    // In Windows, If user is logging off, but not restarting, turn off the firewall so it does not affect other users on the same system
    // Note that this must occur before the backend cleanup call.
    if (PersistentState::instance().isFirewallOn() &&
        backend_->getPreferences()->firewallSettings().mode == FIREWALL_MODE_AUTOMATIC &&
        // This is how to detect a user is logging off (See https://devblogs.microsoft.com/oldnewthing/20180705-00/?p=99175)
        GetSystemMetrics(SM_SHUTTINGDOWN) &&
        !WindscribeApplication::instance()->isExitWithRestart())
    {
        qCInfo(LOG_BASIC) << "Turning off firewall for non-restart logout";
        backend_->firewallOff();
    }
#endif

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
                qCInfo(LOG_BASIC) << "Setting firewall persistence to false for restart-triggered shutdown";
                PersistentState::instance().setFirewallState(false);
            }
        }
        else { // non-restart close
            if (!backend_->getPreferences()->isAutoConnect()) {
                qCInfo(LOG_BASIC) << "Setting firewall persistence to false for non-restart auto-mode";
                PersistentState::instance().setFirewallState(false);
            }
        }
    } else if (backend_->getPreferences()->firewallSettings().mode == FIREWALL_MODE_ALWAYS_ON) {
        PersistentState::instance().setFirewallState(true);
    }

    qCInfo(LOG_BASIC) << "Firewall on next startup: " << PersistentState::instance().isFirewallOn();

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

    ImageResourcesSvg::instance().finishGracefully();

    if (WindscribeApplication::instance()->isExitWithRestart() || isFromSigTerm_mac || isSpontaneousCloseEvent_) {
        // Since we may process events below, disable UI updates and prevent the slot for this signal
        // from attempting to close this widget. We have encountered instances of that occurring during
        // the app update process on macOS.
        setUpdatesEnabled(false);
        disconnect(WindscribeApplication::instance(), &WindscribeApplication::shouldTerminate_mac, this, nullptr);

        if (isFromSigTerm_mac) {
            qCInfo(LOG_BASIC) << "close main window with SIGTERM";
        } else if (isSpontaneousCloseEvent_) {
            qCInfo(LOG_BASIC) << "close main window with close event";
        } else {
            qCInfo(LOG_BASIC) << "close main window with restart OS";
        }
        isSpontaneousCloseEvent_ = false;

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
        isSpontaneousCloseEvent_ = false;
        qCInfo(LOG_BASIC) << "close main window";
        if (event) {
            event->ignore();
            QTimer::singleShot(TIME_BEFORE_SHOW_SHUTDOWN_WINDOW, this, &MainWindow::showShutdownWindow);
        }
    }
    return true;
}

void MainWindow::minimizeToTray()
{
    trayIcon_.show();
    QTimer::singleShot(0, this, &MainWindow::hide);
    MainWindowState::instance().setActive(false);
#if defined(Q_OS_MACOS)
    MacUtils::hideDockIcon();
#endif
}

bool MainWindow::event(QEvent *event)
{
    // qDebug() << "Event Type: " << event->type();

    if (event->type() == QEvent::WindowStateChange) {
        if (this->windowState() == Qt::WindowMinimized) {
            MainWindowState::instance().setActive(false);
        }

        deactivationTimer_.stop();
        if (backend_ && backend_->getPreferences()->isMinimizeAndCloseToTray()) {
            QWindowStateChangeEvent *e = static_cast<QWindowStateChangeEvent *>(event);
            // make sure we only do this for minimize events
            if ((e->oldState() != Qt::WindowMinimized) && isMinimized()) {
                minimizeToTray();
                event->ignore();
            }
        }
    }

#if defined(Q_OS_MACOS)
    if (event->type() == QEvent::PaletteChange)
    {
        trayIconColorWhite_ = InterfaceUtils::isDarkMode();
        qCDebug(LOG_BASIC) << "PaletteChanged, dark mode: " << trayIconColorWhite_;
    }
#endif

    if (event->type() == QEvent::WindowActivate) {
        MainWindowState::instance().setActive(true);
        // qDebug() << "WindowActivate";
        if (backend_->getPreferences()->isDockedToTray()) {
            activateAndShow();
        }
        setBackendAppActiveState(true);
        activeState_ = true;
    } else if (event->type() == QEvent::WindowDeactivate) {
        // qDebug() << "WindowDeactivate";
        setBackendAppActiveState(false);
        activeState_ = false;
    }

    return QWidget::event(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // on MacOS we get the closeEvent twice when pressing Cmd-Q.  Ignore next event if we detect this.
#if defined (Q_OS_MACOS)
    static bool ignoreEvent = false;
    if (ignoreEvent) {
        event->ignore();
        ignoreEvent = false;
        return;
    }
#endif

    if (backend_->isAppCanClose()) {
        QWidget::closeEvent(event);
        QApplication::closeAllWindows();
        QApplication::quit();
    } else {
        if (!doClose(event)) {
#if defined (Q_OS_MACOS)
            ignoreEvent = true;
#endif
        }
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (!backend_->getPreferences()->isDockedToTray()) {
        if (event->button() == Qt::LeftButton) {
            // Ignore drag on shadow
            QPoint dragPosition = event->globalPosition().toPoint() - this->frameGeometry().topLeft();
            if (dragPosition.x() < mainWindowController_->getShadowMargin() ||
                dragPosition.x() > this->width() - mainWindowController_->getShadowMargin() ||
                dragPosition.y() < mainWindowController_->getShadowMargin() ||
                dragPosition.y() > this->height() - mainWindowController_->getShadowMargin())
            {
                return;
            }

            this->window()->windowHandle()->startSystemMove();
            bMovingWindow_ = true;
        }
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (bMovingWindow_) {
        bMovingWindow_ = false;
        mainWindowController_->updateMaximumHeight();
    }
}

bool MainWindow::handleKeyPressEvent(QKeyEvent *event)
{
    if (ExtraConfig::instance().getUsingScreenTransitionHotkeys()) {
        if (event->modifiers() & Qt::ControlModifier) {
            if (event->key() == Qt::Key_E) {
                mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_EMERGENCY);
            } else if (event->key() == Qt::Key_A) {
                mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_UPGRADE);
            } else if (event->key() == Qt::Key_L) {
                if (event->modifiers() & Qt::ShiftModifier) {
                    mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_LOGGING_IN);
                } else {
                    mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_LOGIN);
                }
            } else if (event->key() == Qt::Key_C) {
                mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_CONNECT);
            } else if (event->key() == Qt::Key_X) {
                mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_EXTERNAL_CONFIG);
            } else if (event->key() == Qt::Key_G) {
                mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_UPGRADE);
            } else if (event->key() == Qt::Key_D) {
                mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_UPDATE);
            } else if (event->key() == Qt::Key_I) {
                if (event->modifiers() & Qt::ShiftModifier) {
                    mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_LOGOUT);
                } else {
                    mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_EXIT);
                }
            } else if (event->key() == Qt::Key_P) {
                if (event->modifiers() & Qt::ShiftModifier) {
                    collapsePreferences();
                } else {
                    if (mainWindowController_->isPreferencesVisible()) {
                        PREFERENCES_TAB_TYPE tab = (PREFERENCES_TAB_TYPE)((mainWindowController_->getPreferencesWindow()->currentTab() + 1) % TAB_UNDEFINED);
                        mainWindowController_->getPreferencesWindow()->setCurrentTab(tab);
                    }
                    mainWindowController_->expandPreferences();
                }
            } else if (event->key() == Qt::Key_O) {
                if (event->modifiers() & Qt::ShiftModifier) {
                    mainWindowController_->collapseLocations();
                } else {
                    mainWindowController_->expandLocations();
                }
            } else if (event->key() == Qt::Key_N) {
                if (event->modifiers() & Qt::ShiftModifier) {
                    mainWindowController_->collapseNewsFeed();
                } else {
                    mainWindowController_->getNewsFeedWindow()->setMessages(
                        notificationsController_.messages(), notificationsController_.shownIds());
                    mainWindowController_->expandNewsFeed();
                }
            } else if (event->key() == Qt::Key_R) {
                if (event->modifiers() & Qt::ShiftModifier) {
                    mainWindowController_->collapseProtocols();
                } else {
                    mainWindowController_->expandProtocols(ProtocolWindowMode::kChangeProtocol);
                }
            } else if (event->key() == Qt::Key_U) {
                if (event->modifiers() & Qt::ShiftModifier) {
                    mainWindowController_->hideUpdateWidget();
                } else {
                    mainWindowController_->showUpdateWidget();
                }
            } else if (event->key() == Qt::Key_M) {
                if (event->modifiers() & Qt::ShiftModifier) {
                    GeneralMessageController::instance().showMessage(
                        "WARNING_WHITE",
                        "Test Message",
                        "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.",
                        GeneralMessageController::tr(GeneralMessageController::kOk));
                } else {
                    GeneralMessageController::instance().showMessage(
                        "WARNING_YELLOW",
                        "Test Message",
                        "Lorem ipsum dolor sit amet",
                        GeneralMessageController::tr(GeneralMessageController::kOk));
                }
            }
        }
    }

    if (mainWindowController_->isLocationsExpanded())
    {
        if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_Space)
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

#ifdef Q_OS_MACOS
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
#ifdef Q_OS_MACOS
    if (backend_->getPreferences()->isMinimizeAndCloseToTray()) {
        minimizeToTray();
        return;
    }
#endif
    showMinimized();
}

void MainWindow::onCloseClick()
{
    if (backend_->getPreferences()->isMinimizeAndCloseToTray()) {
        minimizeToTray();
    } else {
        mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_EXIT);
    }
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
        mainWindowController_->getLoginWindow()->setErrorMessage(LoginWindow::ERR_MSG_USERNAME_IS_EMAIL, QString());
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
        TwoFactorAuthWindow::TwoFactorAuthWindowItem::ERR_MSG_EMPTY);
    mainWindowController_->getTwoFactorAuthWindow()->setLoginMode(false);
    mainWindowController_->getTwoFactorAuthWindow()->setCurrentCredentials(username, password);
    mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_TWO_FACTOR_AUTH);
}

void MainWindow::onConnectWindowConnectClick()
{
    if (backend_->isDisconnected()) {
        mainWindowController_->collapseLocations();
        if (!selectedLocation_->isValid()) {
            LocationID bestLocation = backend_->locationsModelManager()->getBestLocationId();
            // If we are in external config mode, there may be no best or selected location; do not attempt connect
            if(!bestLocation.isValid()) {
                return;
            }
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

void MainWindow::onProtocolWindowDisconnect()
{
    backend_->sendDisconnect();
}

void MainWindow::onPreferencesEscapeClick()
{
    collapsePreferences();
}

void MainWindow::onPreferencesLogoutClick()
{
    gotoLogoutWindow();
}

void MainWindow::onPreferencesLoginClick()
{
    if (backend_->getPreferencesHelper()->isExternalConfigMode()) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        logoutReason_ = LOGOUT_GO_TO_LOGIN;
        backend_->logout(false);
    } else {
        collapsePreferences();
        mainWindowController_->getLoginWindow()->transitionToUsernameScreen();
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

void MainWindow::onPreferencesExportSettingsClick()
{
    QString settingsFilename;

    ShowingDialogState::instance().setCurrentlyShowingExternalDialog(true);
#if !defined(Q_OS_LINUX)
    settingsFilename = QFileDialog::getSaveFileName(this, tr("Export Preferences To"), "", tr("JSON Files (*.json)"));
#else
    QFileDialog dialog(this, tr("Export Preferences To"), "", tr("JSON Files (*.json)"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDefaultSuffix("json");
    dialog.setFileMode(QFileDialog::AnyFile);
    if (!dialog.exec()) {
        ShowingDialogState::instance().setCurrentlyShowingExternalDialog(false);
        return;
    }
    settingsFilename = dialog.selectedFiles().first();
#endif
    ShowingDialogState::instance().setCurrentlyShowingExternalDialog(false);

    if (settingsFilename.isEmpty()) {
        return;
    }

    QFile file(settingsFilename);
    if (!file.open(QIODevice::WriteOnly)) {
        onPreferencesReportErrorToUser(tr("Unable to export preferences"), tr("Could not open file for writing.  Check your permissions and try again."));
        return;
    }

    QJsonDocument doc(backend_->getPreferences()->toJson());
    file.write(doc.toJson());
    qCDebug(LOG_BASIC) << "Exported preferences to the file.";
}

void MainWindow::onPreferencesImportSettingsClick()
{
    if (backend_->currentConnectState() != CONNECT_STATE::CONNECT_STATE_DISCONNECTED) {
        onPreferencesReportErrorToUser(tr("Unable to import preferences"), tr("Preferences can only be imported when the app is disconnected. Please disconnect and try again."));
        return;
    }

    ShowingDialogState::instance().setCurrentlyShowingExternalDialog(true);
    const QString settingsFilename = QFileDialog::getOpenFileName(this, tr("Import Preferences From"), "", tr("JSON Files (*.json)"));
    ShowingDialogState::instance().setCurrentlyShowingExternalDialog(false);
    if (settingsFilename.isEmpty()) {
        return;
    }

    QFile file(settingsFilename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        onPreferencesReportErrorToUser(tr("Unable to import preferences"), tr("Could not open file."));
        return;
    }

    QByteArray jsonData = file.readAll();
    file.close();
    QJsonParseError parseError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        onPreferencesReportErrorToUser(tr("Unable to import preferences"), tr("The selected file's format is incorrect."));
        qDebug() << "Error parsing JSON while importing preferences from :" << parseError.errorString();
        return;
    }

    if (!jsonDoc.isObject()) {
        onPreferencesReportErrorToUser(tr("Unable to import preferences"), tr("The selected file's format is incorrect."));
        qDebug() << "Expected JSON object not found when importing preferences";
        return;
    }

    backend_->getPreferences()->updateFromJson(jsonDoc.object());

    // Preferences itself does not have any information on the current portmap.  Check that connection setting ports are valid, otherwise use default port
    types::ConnectionSettings cs = backend_->getPreferences()->connectionSettings();
    normalizeConnectionSettings(cs);
    backend_->getPreferences()->setConnectionSettings(cs);

    // Same for per-network settings
    const QMap<QString, types::ConnectionSettings> map = backend_->getPreferences()->networkPreferredProtocols();
    QMap<QString, types::ConnectionSettings> newMap;
    for (auto key : map.keys()) {
        types::ConnectionSettings cs = map[key];
        normalizeConnectionSettings(cs);
        newMap[key] = cs;
    }
    backend_->getPreferences()->setNetworkPreferredProtocols(newMap);

    qCDebug(LOG_BASIC) << "Imported preferences from the file.";
    mainWindowController_->getPreferencesWindow()->setPreferencesImportCompleted();
}

void MainWindow::normalizeConnectionSettings(types::ConnectionSettings &cs)
{
    // Check that the port in cs is valid per the portmap
    QVector<uint> ports = backend_->getPreferencesHelper()->getAvailablePortsForProtocol(cs.protocol());
    if (!ports.contains(cs.port())) {
        qCInfo(LOG_BASIC) << "Port" << cs.port() << "does not exist in port map, setting to default port.";
        cs.setPort(types::Protocol::defaultPortForProtocol(cs.protocol()));
    }
}

void MainWindow::onPreferencesSendConfirmEmailClick()
{
    backend_->sendConfirmEmail();
}

void MainWindow::onSendDebugLogClick()
{
    // capture the routing table in the debug log since it is useful to catch configuration issues
    qCDebugMultiline(LOG_NETWORK) << NetworkUtils::getRoutingTable();
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
    mainWindowController_->getLoginWindow()->transitionToUsernameScreen();
}

void MainWindow::onPreferencesCycleMacAddressClick()
{
    if (internetConnected_) {
        QString title = tr("Rotating MAC Address");
        QString desc = tr("Rotating your MAC address will result in a disconnect event from the current network. Are you sure?");
        GeneralMessageController::instance().showMessage(
            "WARNING_WHITE",
            title,
            desc,
            GeneralMessageController::tr(GeneralMessageController::kYes),
            GeneralMessageController::tr(GeneralMessageController::kNo),
            "",
            [this](bool b) { backend_->cycleMacAddress(); },
            std::function<void(bool)>(nullptr),
            std::function<void(bool)>(nullptr),
            GeneralMessage::kFromPreferences);
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
    GeneralMessageController::instance().showMessage("WARNING_YELLOW", title, desc, GeneralMessageController::tr(GeneralMessageController::kOk));
}

void MainWindow::onPreferencesCollapsed()
{
    backend_->getPreferences()->validateAndUpdateIfNeeded();
}

#if defined(Q_OS_LINUX)
void MainWindow::onPreferencesTrayIconColorChanged(TRAY_ICON_COLOR c)
{
    const bool trayIconColorWhite = (c == TRAY_ICON_COLOR::TRAY_ICON_COLOR_WHITE);
    if (trayIconColorWhite != trayIconColorWhite_) {
        trayIconColorWhite_ = trayIconColorWhite;
        updateTrayIconType(currentAppIconType_);
    }
}
#endif

void MainWindow::onEmergencyConnectClick()
{
    backend_->emergencyConnectClick();
}

void MainWindow::onEmergencyDisconnectClick()
{
    backend_->emergencyDisconnectClick();
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
    mainWindowController_->getUpdateAppItem()->setMode(UpdateApp::UpdateAppItem::UPDATE_APP_ITEM_MODE_PROGRESS);
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

void MainWindow::onIpcUpdate()
{
    onUpdateAppItemClick();
    onUpdateWindowAccept();
}

void MainWindow::onUpdateWindowCancel()
{
    backend_->cancelUpdateVersion();
    mainWindowController_->getUpdateWindow()->changeToPromptScreen();
    downloadRunning_ = false;
}

void MainWindow::onUpdateWindowLater()
{
    mainWindowController_->getUpdateAppItem()->setMode(UpdateApp::UpdateAppItem::UPDATE_APP_ITEM_MODE_PROMPT);
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

void MainWindow::onLogoutWindowAccept()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    setEnabled(false);
    logoutReason_ = LOGOUT_FROM_MENU;
    isExitingFromPreferences_ = false;
    selectedLocation_->clear();
    backend_->logout(false);
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
    selectLocation(lid, types::Protocol());
}

void MainWindow::selectLocation(const LocationID &lid, const types::Protocol &protocol)
{
    qCDebug(LOG_USER) << "Location selected:" << lid.getHashString();

    selectedLocation_->set(lid);
    PersistentState::instance().setLastLocation(selectedLocation_->locationdId());
    if (selectedLocation_->isValid()) {
        mainWindowController_->getConnectWindow()->updateLocationInfo(selectedLocation_->firstName(), selectedLocation_->secondName(),
                                                                      selectedLocation_->countryCode(), selectedLocation_->pingTime(),
                                                                      selectedLocation_->locationdId().isCustomConfigsLocation());
        mainWindowController_->collapseLocations();

        uint port = 0;
        if (protocol.isValid()) {
            // determine default port for protocol
            QVector<uint> ports = backend_->getPreferencesHelper()->getAvailablePortsForProtocol(protocol);
            if (ports.size() == 0) {
                qCWarning(LOG_BASIC) << "Could not determine port for protocol" << protocol.toLongString();
            } else {
                port = ports[0];
            }
        }

        if (port != 0) {
            // If chosen protocol is different from preferred, turn off the badge
            types::ProtocolStatus ps = mainWindowController_->getConnectWindow()->getProtocolStatus();
            if (protocol != ps.protocol || port != ps.port) {
                mainWindowController_->getConnectWindow()->setIsPreferredProtocol(false);
            }

            mainWindowController_->getConnectWindow()->setProtocolPort(protocol, port);
            backend_->sendConnect(lid, types::ConnectionSettings(protocol, port, false));
            userProtocolOverride_ = true;
        } else {
            backend_->sendConnect(lid);
        }
    } else {
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
    if (selectedLocation_->locationdId().isCustomConfigsLocation() && backend_->currentConnectState() != CONNECT_STATE_DISCONNECTED) {
        backend_->sendDisconnect();
    }
    backend_->getPreferences()->setCustomOvpnConfigsPath(QString());
}

void MainWindow::onLocationsAddCustomConfigClicked()
{
    ShowingDialogState::instance().setCurrentlyShowingExternalDialog(true);
    QString path = QFileDialog::getExistingDirectory(
        this, tr("Select Custom Config Folder"), "", QFileDialog::ShowDirsOnly);
    ShowingDialogState::instance().setCurrentlyShowingExternalDialog(false);

    checkCustomConfigPath(path);
}

void MainWindow::onPreferencesCustomConfigPathNeedsUpdate(const QString &path)
{
#ifdef Q_OS_WIN
    // On Windows, the user gets an UAC prompt (possibly requesting admin credentials)
    // The prompt contains the context of why we need admin credentials, so we don't need to show an additional message.
    checkCustomConfigPath(path);
#else
    // For Mac & Linux, show an alert that describes why we're about to prompt for admin password,
    // because the prompt does not show any useful context.
    GeneralMessageController::instance().showMessage(
        "WARNING_WHITE",
        tr("Custom Config Directory Import"),
        tr("A custom config directory is being imported.  Windscribe will prompt for your admin password to check for correct permissions."),
        GeneralMessageController::tr(GeneralMessageController::kOk),
        "",
        "",
        [this, path](bool b) { checkCustomConfigPath(path); });
#endif
}

void MainWindow::checkCustomConfigPath(const QString &path)
{
    if (path.isEmpty()) {
        return;
    }

    WriteAccessRightsChecker checker(path);
    if (checker.isWriteable()) {
        if (!checker.isElevated()) {
            std::unique_ptr<IAuthChecker> authChecker = AuthCheckerFactory::createAuthChecker();

            AuthCheckerError err = authChecker->authenticate();
            if (err == AuthCheckerError::AUTH_AUTHENTICATION_ERROR) {
                qCWarning(LOG_BASIC) << "Cannot change path to non-system directory when windscribe is not elevated.";
                const QString desc = tr(
                    "Cannot select this directory because it is writeable for non-privileged users. "
                    "Custom configs in this directory may pose a potential security risk. "
                    "Please authenticate with an admin user to select this directory.");
                GeneralMessageController::instance().showMessage("WARNING_YELLOW", tr("Can't select directory"), desc, GeneralMessageController::tr(GeneralMessageController::kOk));
                return;
            } else if (err == AuthCheckerError::AUTH_HELPER_ERROR) {
                qCWarning(LOG_AUTH_HELPER) << "Failed to verify AuthHelper, binary may be corrupted.";
                const QString desc = tr("The application is corrupted.  Please reinstall Windscribe.");
                GeneralMessageController::instance().showMessage("WARNING_YELLOW", tr("Validation Error"), desc, GeneralMessageController::tr(GeneralMessageController::kOk));
                return;
            }
        }

        // warn, but still allow path setting
        const QString desc = tr(
            "The selected directory is writeable for non-privileged users. "
            "Custom configs in this directory may pose a potential security risk.");
        GeneralMessageController::instance().showMessage("WARNING_YELLOW", tr("Security Risk"), desc, GeneralMessageController::tr(GeneralMessageController::kOk));
    }

    // set the path
    backend_->getPreferences()->setCustomOvpnConfigsPath(path);
}

void MainWindow::onBackendInitFinished(INIT_STATE initState)
{
    setVariablesToInitState();

    if (initState == INIT_STATE_SUCCESS) {
        setInitialFirewallState();

        Preferences *p = backend_->getPreferences();
        p->validateAndUpdateIfNeeded();

        backend_->sendSplitTunneling(p->splitTunneling());

#ifdef Q_OS_MACOS
        // on MacOS 14.4 and later, MAC spoofing no longer works.  If it was enabled, disable the feature.
        if (MacUtils::isOsVersionAtLeast(14, 4) && p->macAddrSpoofing().isEnabled) {
            types::MacAddrSpoofing mas = p->macAddrSpoofing();
            mas.isEnabled = false;
            p->setMacAddrSpoofing(mas);
        }
#endif

        // enable wifi/proxy sharing, if checked
        if (p->shareSecureHotspot().isEnabled) {
            onPreferencesShareSecureHotspotChanged(p->shareSecureHotspot());
        }
        if (p->shareProxyGateway().isEnabled) {
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
        } else if (backend_->isCanLoginWithAuthHash()) {
            if (!backend_->isSavedApiSettingsExists()) {
                mainWindowController_->getLoggingInWindow()->setMessage(tr("Logging you in..."));
                mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_LOGGING_IN);
            }
            backend_->loginWithAuthHash();
        } else {
            mainWindowController_->getInitWindow()->startSlideAnimation();
            gotoLoginWindow();
        }

        // Reset last known good protocol/port
        backend_->getPreferences()->clearLastKnownGoodProtocols();

        updateConnectWindowStateProtocolPortDisplay();

        // Start the IPC server last to give the above commands time to finish before we
        // start accepting commands from the CLI.
        localIpcServer_->start();
    } else if (initState == INIT_STATE_BFE_SERVICE_NOT_STARTED) {
        GeneralMessageController::instance().showMessage("WARNING_YELLOW",
                                               tr("Enable Service?"),
                                               tr("Enable \"Base Filtering Engine\" service? This is required for Windscribe to function."),
                                               GeneralMessageController::tr(GeneralMessageController::kYes),
                                               GeneralMessageController::tr(GeneralMessageController::kNo),
                                               "",
                                               [this](bool b) { backend_->enableBFE_win(); },
                                               [this](bool b) { QTimer::singleShot(0, this, &MainWindow::close); });
    } else if (initState == INIT_STATE_BFE_SERVICE_FAILED_TO_START) {
        GeneralMessageController::instance().showMessage("ERROR_ICON",
                                               tr("Failed to Enable Service"),
                                               tr("Could not start 'Base Filtering Engine' service.  Please enable this service manually in Windows Services."),
                                               GeneralMessageController::tr(GeneralMessageController::kOk),
                                               "",
                                               "",
                                               [this](bool b) { QTimer::singleShot(0, this, &MainWindow::close); });
    } else if (initState == INIT_STATE_HELPER_FAILED) {
        GeneralMessageController::instance().showMessage("ERROR_ICON",
                                               tr("Failed to Start"),
                                               tr("Windscribe is malfunctioning.  Please restart the application."),
                                               GeneralMessageController::tr(GeneralMessageController::kOk),
                                               "",
                                               "",
                                               [this](bool b) { QTimer::singleShot(0, this, &MainWindow::close); });
    } else if (initState == INIT_STATE_HELPER_USER_CANCELED) {
        // close without message box
        QTimer::singleShot(0, this, &MainWindow::close);
    } else {
        if (!isInitializationAborted_) {
            qCCritical(LOG_BASIC) << "Engine failed to start.";
            WS_ASSERT(false);
        }
        QTimer::singleShot(0, this, &MainWindow::close);
    }
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
                qCCritical(LOG_BASIC) << "Fatal error: MainWindow::onBackendLoginFinished, WS_ASSERT(bestLocation.isValid());";
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
        QDesktopServices::openUrl(QUrl( QString("https://%1/installed/desktop?%2").arg(HardcodedSettings::instance().windscribeServerUrl()).arg(curUserId)));
    }
    PersistentState::instance().setFirstLogin(false);

    checkLocationPermission();
    checkNotificationEnabled();
}

void MainWindow::onBackendTryingBackupEndpoint(int num, int cnt)
{
    QString additionalMessage = tr("Trying Backup Endpoints %1/%2").arg(num).arg(cnt);
    mainWindowController_->getLoggingInWindow()->setAdditionalMessage(additionalMessage);
}

void MainWindow::onBackendLoginError(wsnet::LoginResult loginError, const QString &errorMessage)
{
    // This error is special in that we can show the prompt any time
    if (loginError == wsnet::LoginResult::kSslError) {
        GeneralMessageController::instance().showMessage(
            "WARNING_WHITE",
            tr("SSL Error"),
            tr("We detected that SSL requests may be intercepted on your network. This could be due to a firewall configured on your computer, or Windscribe being blocked by your network administrator. Ignore SSL errors?"),
            GeneralMessageController::tr(GeneralMessageController::kYes),
            GeneralMessageController::tr(GeneralMessageController::kNo),
            "",
            [this](bool b) {
                backend_->getPreferences()->setIgnoreSslErrors(true);
                if (!isLoginOkAndConnectWindowVisible_) {
                    mainWindowController_->getLoggingInWindow()->setMessage(tr("Logging you in..."));
                    mainWindowController_->getLoggingInWindow()->setAdditionalMessage("");
                    mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_LOGGING_IN);
                    backend_->loginWithLastLoginSettings();
                }
            },
            [this](bool b) {
                if (!isLoginOkAndConnectWindowVisible_) {
                    mainWindowController_->getLoginWindow()->setErrorMessage(LoginWindow::ERR_MSG_NO_API_CONNECTIVITY);
                    gotoLoginWindow();
                }
            });
        return;
    }

    if (isLoginOkAndConnectWindowVisible_) {
        // If we have already activated at some point, we never log out regardless of API errors,
        // except for messages indicating the session is no longer valid
        if (loginError == wsnet::LoginResult::kSessionInvalid) {
            onBackendSessionDeleted();
        } else {
            qCWarning(LOG_BASIC) << "Session error while logged in: " << (int)loginError;
        }
        return;
    }

    if (loginError == wsnet::LoginResult::kBadUsername) {
        if (backend_->isLastLoginWithAuthHash()) {
            qCCritical(LOG_BASIC) << "Got 'bad username' with auth hash login";
            WS_ASSERT(false);
        } else {
            loginAttemptsController_.pushIncorrectLogin();
            mainWindowController_->getLoginWindow()->setErrorMessage(loginAttemptsController_.currentMessage(), QString());
            // It's possible we were passed invalid credentials by the CLI or installer auto-login.  Ensure we transition
            // the user to the username/password entry screen so they can see the error message and attempt a manual login.
            mainWindowController_->getLoginWindow()->transitionToUsernameScreen();
            gotoLoginWindow();
        }
    } else if (loginError == wsnet::LoginResult::kBadCode2fa || loginError == wsnet::LoginResult::kMissingCode2fa) {
        const bool is_missing_code2fa = (loginError == wsnet::LoginResult::kMissingCode2fa);
        mainWindowController_->getTwoFactorAuthWindow()->setErrorMessage(
            is_missing_code2fa ? TwoFactorAuthWindow::TwoFactorAuthWindowItem::ERR_MSG_NO_CODE
                               : TwoFactorAuthWindow::TwoFactorAuthWindowItem::ERR_MSG_INVALID_CODE);
        mainWindowController_->getTwoFactorAuthWindow()->setLoginMode(true);
        mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_TWO_FACTOR_AUTH);
        return;
    } else if (loginError == wsnet::LoginResult::kNoConnectivity) {
        mainWindowController_->getLoginWindow()->setErrorMessage(LoginWindow::ERR_MSG_NO_INTERNET_CONNECTIVITY);
    } else if (loginError == wsnet::LoginResult::kNoApiConnectivity) {
        mainWindowController_->getLoginWindow()->setErrorMessage(LoginWindow::ERR_MSG_NO_API_CONNECTIVITY);
    } else if (loginError == wsnet::LoginResult::kIncorrectJson) {
        mainWindowController_->getLoginWindow()->setErrorMessage(LoginWindow::ERR_MSG_INVALID_API_RESPONSE);
    } else if (loginError == wsnet::LoginResult::kAccountDisabled) {
        mainWindowController_->getLoginWindow()->setErrorMessage(LoginWindow::ERR_MSG_ACCOUNT_DISABLED, errorMessage);
    } else if (loginError == wsnet::LoginResult::kSessionInvalid) {
        mainWindowController_->getLoginWindow()->setErrorMessage(LoginWindow::ERR_MSG_SESSION_EXPIRED);
    } else if (loginError == wsnet::LoginResult::kRateLimited) {
        mainWindowController_->getLoginWindow()->setErrorMessage(LoginWindow::ERR_MSG_RATE_LIMITED);
    }

    mainWindowController_->getLoginWindow()->setEmergencyConnectState(false);
    gotoLoginWindow();
}

void MainWindow::onBackendSessionStatusChanged(const api_responses::SessionStatus &sessionStatus)
{
    blockConnect_.setNotBlocking();
    qint32 status = sessionStatus.getStatus();
    // multiple account abuse detection
    QString entryUsername;
    bool bEntryIsPresent = multipleAccountDetection_->entryIsPresent(entryUsername);
    if (bEntryIsPresent && (!sessionStatus.isPremium()) && sessionStatus.getAlc().size() == 0 && sessionStatus.getStatus() == 1 && entryUsername != sessionStatus.getUsername())
    {
        status = 2;
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
                backend_->sendDisconnect(DISCONNECTED_BY_DATA_LIMIT);
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

void MainWindow::onBackendCheckUpdateChanged(const api_responses::CheckUpdate &checkUpdateInfo)
{
    if (checkUpdateInfo.isAvailable())
    {
        qCInfo(LOG_BASIC) << "Update available";
        if (!checkUpdateInfo.isSupported())
        {
            blockConnect_.setNeedUpgrade();
        }

        QString betaStr;
        betaStr = "-" + QString::number(checkUpdateInfo.latestBuild());
        if (checkUpdateInfo.updateChannel() == UPDATE_CHANNEL_BETA)
        {
            betaStr += "b";
        }
        else if (checkUpdateInfo.updateChannel() == UPDATE_CHANNEL_GUINEA_PIG)
        {
            betaStr += "g";
        }

        //updateWidget_->setText(tr("Update available - v") + version + betaStr);

        mainWindowController_->getUpdateAppItem()->setVersionAvailable(checkUpdateInfo.version(), checkUpdateInfo.latestBuild());
        mainWindowController_->getUpdateWindow()->setVersion(checkUpdateInfo.version(), checkUpdateInfo.latestBuild());

        if (!ignoreUpdateUntilNextRun_)
        {
            mainWindowController_->showUpdateWidget();
        }
    }
    else
    {
        qCInfo(LOG_BASIC) << "No available update";
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

    if (connectState.location.isValid()) {
        // if connecting/connected location not equal current selected location, then change current selected location and update in GUI
        if (selectedLocation_->locationdId() != connectState.location) {
            selectedLocation_->set(connectState.location);
            PersistentState::instance().setLastLocation(selectedLocation_->locationdId());
            if (selectedLocation_->isValid()) {
                mainWindowController_->getConnectWindow()->updateLocationInfo(selectedLocation_->firstName(), selectedLocation_->secondName(),
                                                                              selectedLocation_->countryCode(), selectedLocation_->pingTime(),
                                                                              selectedLocation_->locationdId().isCustomConfigsLocation());
            }
            else {
                qCCritical(LOG_BASIC) << "Fatal error: MainWindow::onBackendConnectStateChanged, WS_ASSERT(selectedLocation_.isValid());";
                WS_ASSERT(false);
            }
        }
    } else {
        // If we no longer have a valid location, select the best location if possible
        if (!selectedLocation_->isValid()) {
            LocationID bestLocation = backend_->locationsModelManager()->getBestLocationId();
            if(bestLocation.isValid()) {
                selectedLocation_->set(bestLocation);
            } else {
                // In external config mode, we don't have a best location, so reset back to blank location.
                selectedLocation_->clear();
            }
            mainWindowController_->getConnectWindow()->updateLocationInfo(selectedLocation_->firstName(), selectedLocation_->secondName(),
                                                                          selectedLocation_->countryCode(), selectedLocation_->pingTime(),
                                                                          !selectedLocation_->isValid() || selectedLocation_->locationdId().isCustomConfigsLocation());
            PersistentState::instance().setLastLocation(selectedLocation_->locationdId());
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

    } else if (connectState.connectState == CONNECT_STATE_DISCONNECTED) {
        updateConnectWindowStateProtocolPortDisplay();

        if (connectState.disconnectReason == DISCONNECTED_WITH_ERROR) {
            updateAppIconType(AppIconType::DISCONNECTED_WITH_ERROR);
        } else {
            updateAppIconType(AppIconType::DISCONNECTED);
        }

        // Ensure the icon has been updated, as QSystemTrayIcon::showMessage displays this icon
        // in the notification window on Windows.
        if (connectState.disconnectReason == DISCONNECTED_WITH_ERROR) {
            updateTrayIconType(AppIconType::DISCONNECTED_WITH_ERROR);
        } else {
            updateTrayIconType(AppIconType::DISCONNECTED);
        }


        if (bNotificationConnectedShowed_) {
            if (backend_->getPreferences()->isShowNotifications()) {
                showTrayMessage(tr("Connection to Windscribe has been terminated.\n%1 transferred in %2").arg(getConnectionTransferred(), getConnectionTime()));
            }
            bNotificationConnectedShowed_ = false;
        }

        if (connectState.disconnectReason == DISCONNECTED_WITH_ERROR) {
            handleDisconnectWithError(connectState);
        }

        mainWindowController_->getProtocolWindow()->resetProtocolStatus();
        userProtocolOverride_ = false;

        if (sendDebugLogOnDisconnect_) {
            sendDebugLogOnDisconnect_ = false;
            backend_->sendDebugLog();
        }
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
    qCInfo(LOG_BASIC) << "Network Changed: "
                       << "Index: " << network.interfaceIndex
                       << ", Network/SSID: " << network.networkOrSsid
                       << ", MAC: " << network.physicalAddress
                       << " friendly: " << network.friendlyName;

    mainWindowController_->getConnectWindow()->updateNetworkState(network);
    mainWindowController_->getPreferencesWindow()->updateNetworkState(network);
    curNetwork_ = network;
    if (backend_->isDisconnected()) {
        updateConnectWindowStateProtocolPortDisplay();
    } else {
        const types::ConnectionSettings cs = backend_->getPreferences()->networkPreferredProtocol(network.networkOrSsid);
        types::ProtocolStatus ps = mainWindowController_->getConnectWindow()->getProtocolStatus();
        if (!cs.isAutomatic() && cs.protocol() == ps.protocol && cs.port() == ps.port) {
            mainWindowController_->getConnectWindow()->setIsPreferredProtocol(true);
        } else {
            mainWindowController_->getConnectWindow()->setIsPreferredProtocol(false);
        }
    }

    if (isLoginOkAndConnectWindowVisible_) {
        checkLocationPermission();
    }
}

void MainWindow::onSplitTunnelingStateChanged(bool isActive)
{
    mainWindowController_->getConnectWindow()->setSplitTunnelingState(isActive);
}

void MainWindow::onBackendLogoutFinished()
{
    selectedLocation_->clear();
    mainWindowController_->getConnectWindow()->updateLocationInfo(selectedLocation_->firstName(), selectedLocation_->secondName(),
                                                                  selectedLocation_->countryCode(), selectedLocation_->pingTime(),
                                                                  true);

    loginAttemptsController_.reset();
    mainWindowController_->getPreferencesWindow()->setLoggedIn(false);
    isLoginOkAndConnectWindowVisible_ = false;
    backend_->getPreferencesHelper()->setIsExternalConfigMode(false);
    mainWindowController_->getBottomInfoWindow()->setDataRemaining(-1, -1);

    if (logoutReason_ == LOGOUT_FROM_MENU) {
        mainWindowController_->getLoginWindow()->resetState();
        mainWindowController_->getLoginWindow()->setErrorMessage(LoginWindow::ERR_MSG_EMPTY, QString());
    } else if (logoutReason_ == LOGOUT_SESSION_EXPIRED) {
        mainWindowController_->getLoginWindow()->transitionToUsernameScreen();
        mainWindowController_->getLoginWindow()->setErrorMessage(LoginWindow::ERR_MSG_SESSION_EXPIRED, QString());
    } else if (logoutReason_ == LOGOUT_WITH_MESSAGE) {
        mainWindowController_->getLoginWindow()->transitionToUsernameScreen();
        mainWindowController_->getLoginWindow()->setErrorMessage(logoutMessageType_, logoutErrorMessage_);
    } else if (logoutReason_ == LOGOUT_GO_TO_LOGIN) {
        mainWindowController_->getLoginWindow()->transitionToUsernameScreen();
        mainWindowController_->getLoginWindow()->setErrorMessage(LoginWindow::ERR_MSG_EMPTY, QString());
    } else {
        WS_ASSERT(false);
        mainWindowController_->getLoginWindow()->resetState();
        mainWindowController_->getLoginWindow()->setErrorMessage(LoginWindow::ERR_MSG_EMPTY, QString());
    }

    mainWindowController_->getLoginWindow()->setEmergencyConnectState(false);
    gotoLoginWindow();

    mainWindowController_->hideUpdateWidget();
    setEnabled(true);
    QApplication::restoreOverrideCursor();
}

void MainWindow::onBackendCleanupFinished()
{
    qCInfo(LOG_BASIC) << "Backend Cleanup Finished";
    close();
}

void MainWindow::onBackendGotoCustomOvpnConfigModeFinished()
{
    if (backend_->getPreferences()->firewallSettings().mode == FIREWALL_MODE_ALWAYS_ON) {
        backend_->firewallOn();
        mainWindowController_->getConnectWindow()->setFirewallAlwaysOn(true);
    }

    if (!isLoginOkAndConnectWindowVisible_) {
        // Choose latest location if it's a custom config location; first valid custom config
        // location otherwise.
        selectedLocation_->set(PersistentState::instance().lastLocation());
        if (selectedLocation_->isValid() && selectedLocation_->locationdId().isCustomConfigsLocation()) {
            mainWindowController_->getConnectWindow()->updateLocationInfo(selectedLocation_->firstName(), selectedLocation_->secondName(),
                                                                          selectedLocation_->countryCode(), selectedLocation_->pingTime(),
                                                                          selectedLocation_->locationdId().isCustomConfigsLocation());
        } else {
            LocationID firstValidCustomLocation = backend_->locationsModelManager()->getFirstValidCustomConfigLocationId();
            if (firstValidCustomLocation.isValid()) {
                selectedLocation_->set(firstValidCustomLocation);
                PersistentState::instance().setLastLocation(selectedLocation_->locationdId());
            }
            // |selectedLocation_| can be empty (nopt valid) here, so this will reset current location.
            mainWindowController_->getConnectWindow()->updateLocationInfo(selectedLocation_->firstName(), selectedLocation_->secondName(),
                                                                          selectedLocation_->countryCode(), selectedLocation_->pingTime(),
                                                                          !selectedLocation_->isValid() || selectedLocation_->locationdId().isCustomConfigsLocation());
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

void MainWindow::onBackendWifiSharingFailed(WIFI_SHARING_ERROR error)
{
    mainWindowController_->getBottomInfoWindow()->setSecureHotspotFeatures(false, "");
    mainWindowController_->getBottomInfoWindow()->setSecureHotspotUsersCount(0);

    if (error == WIFI_SHARING_ERROR_RADIO_OFF) {
        GeneralMessageController::instance().showMessage(
            "WARNING_WHITE",
            tr("Wi-Fi is off"),
            tr("Windscribe has detected that Wi-Fi is currently turned off. To use Secure Hotspot, Wi-Fi must be turned on."),
            GeneralMessageController::tr(GeneralMessageController::kOk),
            "",
            "",
            [this](bool b) {
                // Turn off the toggle
                types::ShareSecureHotspot sh = backend_->getPreferences()->shareSecureHotspot();
                sh.isEnabled = false;
                backend_->getPreferences()->setShareSecureHotspot(sh);
            });
    } else {
        backend_->getPreferencesHelper()->setWifiSharingSupported(false);
        GeneralMessageController::instance().showMessage(
            "WARNING_YELLOW",
            tr("Could not start Secure Hotspot"),
            tr("Your network adapter may not support this feature. It has been disabled in preferences."),
            GeneralMessageController::tr(GeneralMessageController::kOk));
    }
}

void MainWindow::onBackendRequestCustomOvpnConfigCredentials(const QString &username)
{
    GeneralMessageController::instance().showCredentialPrompt(
        "WARNING_WHITE",
        tr("Enter Connection Credentials"),
        username.isEmpty() ? "" :  tr("...hmm are you sure this is correct?"), // description
        username,
        GeneralMessageController::tr(GeneralMessageController::kOk),
        GeneralMessageController::tr(GeneralMessageController::kCancel),
        "", // tertiary text
        [this](const QString &username, const QString &password, bool b) {
            backend_->continueWithCredentialsForOvpnConfig(username, password, b);
        },
        [this](bool b) {
            backend_->continueWithCredentialsForOvpnConfig("", "", false);
        });
}

void MainWindow::onBackendRequestCustomOvpnConfigPrivKeyPassword()
{
    GeneralMessageController::instance().showCredentialPrompt(
        "WARNING_WHITE",
        tr("Enter Private Key Password"),
        "", // description
        "", // username
        GeneralMessageController::tr(GeneralMessageController::kOk),
        GeneralMessageController::tr(GeneralMessageController::kCancel),
        "", // tertiary text
        [this](const QString &username, const QString &password, bool b) {
            backend_->continueWithPrivKeyPasswordForOvpnConfig(password, b);
        },
        [this](bool b) {
            backend_->continueWithPrivKeyPasswordForOvpnConfig("", false);
        },
        std::function<void(bool)>(nullptr),
        GeneralMessage::kNoUsername);
}

void MainWindow::onBackendSessionDeleted()
{
    qCInfo(LOG_BASIC) << "Handle deleted session";

    QApplication::setOverrideCursor(Qt::WaitCursor);
    setEnabled(false);
    logoutReason_ = LOGOUT_SESSION_EXPIRED;
    selectedLocation_->clear();
    backend_->logout(true);
}

void MainWindow::onBackendTestTunnelResult(bool success)
{
    if (!ExtraConfig::instance().getIsTunnelTestNoError() && !success) {
        types::ConnectedDnsInfo cdi = backend_->getPreferences()->connectedDnsInfo();

        if (cdi.type == CONNECTED_DNS_TYPE_CUSTOM && backend_->osDnsServersListContains(cdi.upStream1.toStdWString())) {
            GeneralMessageController::instance().showMessage(
                "WARNING_YELLOW",
                tr("Invalid DNS Settings"),
                tr("Your \"Connected DNS\" server is set to an OS default DNS server, which would result in a DNS leak.  It has been changed to Auto."),
                GeneralMessageController::tr(GeneralMessageController::kOk),
                "",
                "",
                [this](bool b) {
                    types::ConnectedDnsInfo cdi = backend_->getPreferences()->connectedDnsInfo();
                    cdi.type = CONNECTED_DNS_TYPE_AUTO;
                    backend_->getPreferences()->setConnectedDnsInfo(cdi);
                    backend_->reconnect();
                });
        } else {
            GeneralMessageController::instance().showMessage(
                "WARNING_YELLOW",
                tr("Network Settings Interference"),
                tr("We've detected that your network settings may interfere with Windscribe.  Please send us a debug log to troubleshoot."),
                tr("Send Debug Log"),
                GeneralMessageController::tr(GeneralMessageController::kCancel),
                "",
                [this](bool b) { sendDebugLogOnDisconnect_ = true; });
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
                    QString title = QString(tr("Set “%1” as preferred protocol?")).arg(ps.protocol.toLongString());
                    GeneralMessageController::instance().showMessage("WARNING_WHITE",
                                                                     title,
                                                                     tr("Windscribe will always use this protocol to connect on this network in the future to avoid any interruptions."),
                                                                     tr("Set as Preferred"),
                                                                     GeneralMessageController::tr(GeneralMessageController::kCancel),
                                                                     "",
                                                                     [this, ps](bool b) {
                                                                        if (curNetwork_.isValid()) {
                                                                            backend_->getPreferences()->setNetworkPreferredProtocol(curNetwork_.networkOrSsid, types::ConnectionSettings(ps.protocol, ps.port, false));
                                                                        }
                                                                     });
                }
                userProtocolOverride_ = false;
            }
        }
    }

    mainWindowController_->getConnectWindow()->setTestTunnelResult(success);
}

void MainWindow::onBackendLostConnectionToHelper()
{
    qCWarning(LOG_BASIC) << "Helper connection was lost";
    GeneralMessageController::instance().showMessage("ERROR_ICON",
                                           tr("Service Error"),
                                           tr("Windscribe is malfunctioning.  Please restart the application."),
                                           GeneralMessageController::tr(GeneralMessageController::kOk));
}

void MainWindow::onBackendHighCpuUsage(const QStringList &processesList)
{
    if (!PersistentState::instance().isIgnoreCpuUsageWarnings())
    {
        QString processesListString;
        for (const QString &processName : processesList) {
            if (!processesListString.isEmpty()) {
                processesListString += ", ";
            }
            processesListString += processName;
        }

        qCInfo(LOG_BASIC) << "Detected high CPU usage in processes:" << processesListString;

        QString msg = QString(tr("Windscribe has detected that %1 is using a high amount of CPU due to a potential conflict with the VPN connection. Do you want to disable the Windscribe TCP socket termination feature that may be causing this issue?").arg(processesListString));

        GeneralMessageController::instance().showMessage(
            "WARNING_YELLOW",
            tr("High CPU Usage"),
            msg,
            GeneralMessageController::tr(GeneralMessageController::kYes),
            GeneralMessageController::tr(GeneralMessageController::kNo),
            "",
#if defined(Q_OS_WIN)
            [this](bool b) { backend_->getPreferences()->setTerminateSockets(false); },
#else
            std::function<void(bool)>(nullptr),
#endif
            [this](bool b) { if (b) PersistentState::instance().setIgnoreCpuUsageWarnings(true); },
            std::function<void(bool)>(nullptr),
            GeneralMessage::kShowBottomPanel,
            QString("https://%1/support/article/20/tcp-socket-termination").arg(HardcodedSettings::instance().windscribeServerUrl()));
    }
}

void MainWindow::showUserWarning(USER_WARNING_TYPE userWarningType)
{
    QString titleText;
    QString descText;
    if (userWarningType == USER_WARNING_MAC_SPOOFING_FAILURE_HARD) {
        titleText = tr("MAC Spoofing Failed");
        descText = tr("Your network adapter does not support MAC spoofing. Try a different adapter.");
    } else if (userWarningType == USER_WARNING_MAC_SPOOFING_FAILURE_SOFT) {
        titleText = tr("MAC Spoofing Failed");
        descText = tr("Could not spoof MAC address.  Please try a different network interface or contact support.");
    }

    if (!titleText.isEmpty() && !descText.isEmpty()) {
        GeneralMessageController::instance().showMessage("WARNING_YELLOW", titleText, descText, GeneralMessageController::tr(GeneralMessageController::kOk));
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
    const types::ConnectionSettings cs = backend_->getPreferences()->networkPreferredProtocol(curNetwork_.networkOrSsid);

    if (!cs.isAutomatic() && cs.protocol() == protocol && cs.port() == port) {
        mainWindowController_->getConnectWindow()->setIsPreferredProtocol(true);
    } else {
        mainWindowController_->getConnectWindow()->setIsPreferredProtocol(false);
    }

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
    qCInfo(LOG_USER) << "Requesting ROBERT filters from server";
    backend_->getRobertFilters();
}

void MainWindow::onBackendRobertFiltersChanged(bool success, const QVector<api_responses::RobertFilter> &filters)
{
    qCInfo(LOG_USER) << "Get ROBERT filters response: " << success;
    if (success)
    {
        mainWindowController_->getPreferencesWindow()->setRobertFilters(filters);
    }
    else
    {
        mainWindowController_->getPreferencesWindow()->setRobertFiltersError();
    }
}

void MainWindow::onPreferencesSetRobertFilter(const api_responses::RobertFilter &filter)
{
    qCInfo(LOG_USER) << "Set ROBERT filter: " << filter.id << ", " << filter.status;
    backend_->setRobertFilter(filter);
}

void MainWindow::onBackendSetRobertFilterResult(bool success)
{
    qCInfo(LOG_BASIC) << "Set ROBERT filter response:" << success;
    if (success)
    {
        backend_->syncRobert();
    }
}

void MainWindow::onBackendSyncRobertResult(bool success)
{
    qCInfo(LOG_BASIC) << "Sync ROBERT response:" << success;
}

void MainWindow::onBackendProtocolStatusChanged(const QVector<types::ProtocolStatus> &status)
{
    mainWindowController_->getProtocolWindow()->setProtocolStatus(status);
    if (mainWindowController_->getProtocolWindow()->hasMoreAttempts()) {
        mainWindowController_->expandProtocols();
    } else {
        GeneralMessageController::instance().showMessage(
            "WARNING_WHITE",
            tr("This network hates us"),
            tr("We couldn’t connect you on this network. Send us your debug log so we can figure out what happened."),
            tr("Send Debug Log"),
            GeneralMessageController::tr(GeneralMessageController::kCancel),
            "",
            [this](bool b) {
                backend_->sendDebugLog();
                GeneralMessageController::instance().showMessage(
                    "CHECKMARK_IN_CIRCLE",
                    tr("Debug Sent!"),
                    tr("Your debug log has been received. Please contact support if you want assistance with this issue."),
                    tr("Contact Support"),
                    GeneralMessageController::tr(GeneralMessageController::kCancel),
                    "",
                    [](bool b) { QDesktopServices::openUrl(QUrl(QString("https://%1/support/ticket").arg(HardcodedSettings::instance().windscribeServerUrl()))); });
            });
    }
}

void MainWindow::onBackendUpdateVersionChanged(uint progressPercent, UPDATE_VERSION_STATE state, UPDATE_VERSION_ERROR error)
{
    // qDebug() << "Mainwindow::onBackendUpdateVersionChanged: " << progressPercent << ", " << state;

    if (state == UPDATE_VERSION_STATE_DONE) {
        if (downloadRunning_) { // not cancelled by user
            if (error == UPDATE_VERSION_ERROR_NO_ERROR) {
                isExitingAfterUpdate_ = true; // the flag for prevent firewall off for some states
                // nothing todo, because installer will close app here
            } else {
                QString titleText = tr("Auto-Update Failed");
                QString descText;
                if (error == UPDATE_VERSION_ERROR_DL_FAIL) {
                    descText = tr("Could not download update.  Please try again or use a different network.");
                    if (backend_->isDisconnected()) {
                        descText += tr("  If you are on a restrictive network, please connect the VPN before trying the download again.");
                    }
                } else {
                    descText = tr("Could not run updater (Error %1).  Please contact support").arg(error);
                }

                // Hide the update available header while the error message window is visible, otherwise we'll see
                // a progress bar above the message window.
                mainWindowController_->getUpdateAppItem()->setMode(UpdateApp::UpdateAppItem::UPDATE_APP_ITEM_MODE_PROMPT);
                mainWindowController_->getUpdateAppItem()->setProgress(0);
                mainWindowController_->getUpdateAppItem()->setVisible(false);

                // Reset the update window to its default state.
                mainWindowController_->getUpdateWindow()->stopAnimation();
                mainWindowController_->getUpdateWindow()->changeToPromptScreen();

                GeneralMessageController::instance().showMessage("ERROR_ICON", titleText, descText, GeneralMessageController::tr(GeneralMessageController::kOk), "", "",
                    [this](bool b) {
                        mainWindowController_->getUpdateAppItem()->setVisible(true);
                        // Need to hide the update window here before transitioning back to the connect screen.
                        mainWindowController_->getUpdateWindow()->hide();
                        mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_CONNECT);
                    });
            }
        } else {
            mainWindowController_->getUpdateAppItem()->setProgress(0);
            mainWindowController_->getUpdateWindow()->stopAnimation();
            mainWindowController_->getUpdateWindow()->changeToPromptScreen();
        }
        downloadRunning_ = false;
    }
    else if (state == UPDATE_VERSION_STATE_DOWNLOADING) {
        // qDebug() << "Running -- updating progress";
        mainWindowController_->getUpdateAppItem()->setProgress(progressPercent);
        mainWindowController_->getUpdateWindow()->setProgress(progressPercent);
    }
    else if (state == UPDATE_VERSION_STATE_RUNNING) {
        // Send main window center coordinates from the GUI, to position the installer properly.
        const bool is_visible = isVisible() && !isMinimized();
        qint32 center_x = INT_MAX;
        qint32 center_y = INT_MAX;

        if (is_visible) {
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
            center_x = geometry().x() + geometry().width() / 2;
            center_y = geometry().y() + geometry().height() / 2;
#elif defined Q_OS_MACOS
            MacUtils::getNSWindowCenter((void *)this->winId(), center_x, center_y);
#endif
        }
        backend_->sendUpdateWindowInfo(center_x, center_y);
    }
}

void MainWindow::openBrowserToMyAccountWithToken(const QString &tempSessionToken)
{
    QString getUrl = QString("https://%1/myaccount?temp_session=%2")
                        .arg(HardcodedSettings::instance().windscribeServerUrl())
                        .arg(tempSessionToken);
    QDesktopServices::openUrl(QUrl(getUrl));
}

void MainWindow::onBackendWebSessionTokenForManageAccount(const QString &tempSessionToken)
{
    mainWindowController_->getPreferencesWindow()->setWebSessionCompleted();
    if (!tempSessionToken.isEmpty()) {
        openBrowserToMyAccountWithToken(tempSessionToken);
    }
}

void MainWindow::onBackendWebSessionTokenForAddEmail(const QString &tempSessionToken)
{
    mainWindowController_->getPreferencesWindow()->setWebSessionCompleted();
    if (!tempSessionToken.isEmpty()) {
        openBrowserToMyAccountWithToken(tempSessionToken);
    }
}

void MainWindow::onBackendWebSessionTokenForManageRobertRules(const QString &tempSessionToken)
{
    mainWindowController_->getPreferencesWindow()->setWebSessionCompleted();
    QString getUrl = QString("https://%1/myaccount?temp_session=%2#robertrules")
                        .arg(HardcodedSettings::instance().windscribeServerUrl())
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
    if (sp.isEnabled) {
        backend_->startProxySharing((PROXY_SHARING_TYPE)sp.proxySharingMode, sp.port, sp.whileConnected);
    } else {
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

#ifdef Q_OS_MACOS
void MainWindow::hideShowDockIcon(bool hideFromDock)
{
    desiredDockIconVisibility_ = !hideFromDock;
    hideShowDockIconTimer_.start(300);
}

const QRect MainWindow::bestGuessForTrayIconRectFromLastScreen(const QPoint &pt)
{
    QRect lastScreenTrayRect = trayIconRectForLastScreen();
    if (lastScreenTrayRect.isValid()) {
        return lastScreenTrayRect;
    }
    // qDebug() << "No valid history of last screen";
    return trayIconRectForScreenContainingPt(pt);
}

const QRect MainWindow::trayIconRectForLastScreen()
{
    if (lastScreenName_ != "") {
        QRect rect = generateTrayIconRectFromHistory(lastScreenName_);
        if (rect.isValid()) {
            return rect;
        }
    }
    // qDebug() << "No valid last screen";
    return QRect(0,0,0,0); // invalid
}

const QRect MainWindow::trayIconRectForScreenContainingPt(const QPoint &pt)
{
    QScreen *screen = WidgetUtils::slightlySaferScreenAt(pt);
    if (!screen) {
        return QRect(0,0,0,0);
    }
    return guessTrayIconLocationOnScreen(screen);
}

const QRect MainWindow::generateTrayIconRectFromHistory(const QString &screenName)
{
    if (systemTrayIconRelativeGeoScreenHistory_.contains(screenName)) {
        // ensure is in current list
        QScreen *screen = WidgetUtils::screenByName(screenName);

        if (screen) {
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
#ifdef Q_OS_MACOS
    if (!backend_->getPreferences()->isHideFromDock()) {
        MacUtils::showDockIcon();
    }

#endif
    mainWindowController_->updateMainAndViewGeometry(true);

    if (!isVisible()) {
        showNormal();
#ifdef Q_OS_WIN

        // Windows will have removed our icon overlay if the app was hidden.
        // We should postpone creating overlay icon since Qt 6.5.1 version was introduced.
        // It has internal implementation that updates icon only after window was shown.
        QTimer::singleShot(200, this, std::bind(&MainWindow::updateAppIconType, this, currentAppIconType_));
#endif
    }
    else if (isMinimized())
        showNormal();

    if (!isActiveWindow())
        activateWindow();
#ifdef Q_OS_MACOS
    MacUtils::activateApp();
#endif

    lastWindowStateChange_ = QDateTime::currentMSecsSinceEpoch();
}

void MainWindow::deactivateAndHide()
{
    MainWindowState::instance().setActive(false);
#if defined(Q_OS_MACOS)
    if (backend_ && backend_->getPreferences()->isMinimizeAndCloseToTray()) {
        MacUtils::hideDockIcon();
    }
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

void MainWindow::onDockIconClicked()
{
    // Toggle visibility if docked
    if (backend_->getPreferences()->isDockedToTray()) {
        if (isVisible() && activeState_) {
            deactivateAndHide();
            setBackendAppActiveState(false);
        } else {
            activateAndShow();
            setBackendAppActiveState(true);
        }
    } else {
        activateAndShow();
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

void MainWindow::onIpcOpenLocations(IPC::CliCommands::LocationType type)
{
    activateAndShow();

#ifdef Q_OS_MACOS
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
    QTimer::singleShot(500, [this, type](){
        mainWindowController_->expandLocations();
        if (type == IPC::CliCommands::LocationType::kStaticIp) {
            mainWindowController_->getLocationsWindow()->setTab(LOCATION_TAB_STATIC_IPS_LOCATIONS);
        } else {
            mainWindowController_->getLocationsWindow()->setTab(LOCATION_TAB_ALL_LOCATIONS);
        }
    });
}

void MainWindow::onIpcConnect(const LocationID &id, const types::Protocol &protocol)
{
    selectLocation(id, protocol);
}

void MainWindow::onIpcConnectStaticIp(const QString &location, const types::Protocol &protocol)
{
    QList<LocationID> list;

    for (int i = 0; i < backend_->locationsModelManager()->staticIpsProxyModel()->rowCount(); i++) {
        QModelIndex miStatic = backend_->locationsModelManager()->staticIpsProxyModel()->index(i, 0);

        // If location doesn't match either the location nor IP, skip this entry
        if (location != miStatic.data(gui_locations::kName).toString() && location != miStatic.data(gui_locations::kNick).toString()) {
            continue;
        }

        list << qvariant_cast<LocationID>(miStatic.data(gui_locations::kLocationId));
    }

    if (list.size() > 0) {
        // Randomly pick one of the candidates
        onIpcConnect(list[QRandomGenerator::global()->bounded(list.size())], protocol);
    }
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
    if (!isVisible()) {
        activateAndShow();
    }
}

#if defined(Q_OS_WIN)
void MainWindow::onAppWinIniChanged()
{
    bool newDarkMode = InterfaceUtils::isDarkMode();
    if (newDarkMode != trayIconColorWhite_)
    {
        trayIconColorWhite_ = newDarkMode;
        qCDebug(LOG_BASIC) << "updating dark mode: " << trayIconColorWhite_;
        updateTrayIconType(currentAppIconType_);
    }
}
#endif

void MainWindow::showShutdownWindow()
{
    setEnabled(true);
    mainWindowController_->hideUpdateWidget();
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
#if defined(Q_OS_MACOS)
    if (trayIcon_.isVisible()) {
        const QRect rc = trayIcon_.geometry();

        // check for valid tray icon
        if (!rc.isValid()) {
            QRect lastGuess = bestGuessForTrayIconRectFromLastScreen(rc.topLeft());
            if (lastGuess.isValid()) return lastGuess;
            return savedTrayIconRect_;
        }

        // check for valid screen
        QScreen *screen = QGuiApplication::screenAt(rc.center());
        if (!screen) {
            QRect bestGuess = trayIconRectForScreenContainingPt(rc.topLeft());
            if (bestGuess.isValid()) {
                return bestGuess;
            }
            return savedTrayIconRect_;
        }

        QRect screenGeo = screen->geometry();

        // valid screen and tray icon -- update the cache
        systemTrayIconRelativeGeoScreenHistory_[screen->name()] = QRect(abs(rc.x() - screenGeo.x()), abs(rc.y() - screenGeo.y()), rc.width(), rc.height());
        lastScreenName_ = screen->name();
        savedTrayIconRect_ = rc;
        return savedTrayIconRect_;
    }

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

    switch (reason) {
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
#else
            if (backend_->getPreferences()->isDockedToTray()) {
                onDockIconClicked();
            } else if (!isVisible()) { // closed to tray
                activateAndShow();
                setBackendAppActiveState(true);
            }
#endif
            break;
        }
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
    if (isMinimized() || !isVisible()) {
        activateAndShow();
        setBackendAppActiveState(true);
    } else {
        deactivateAndHide();
        setBackendAppActiveState(false);
    }
}

void MainWindow::onTrayMenuHelpMe()
{
    QDesktopServices::openUrl(QUrl( QString("https://%1/help").arg(HardcodedSettings::instance().windscribeServerUrl())));
}

void MainWindow::onTrayMenuQuit()
{
    doClose();
}

void MainWindow::onFreeTrafficNotification(const QString &message)
{
    showTrayMessage(message);
}

void MainWindow::onSplitTunnelingAppsAddButtonClick()
{
    ShowingDialogState::instance().setCurrentlyShowingExternalDialog(true);
#ifdef Q_OS_WIN
    QString filename = QFileDialog::getOpenFileName(this, tr("Select an application"), "C:\\");
#else
    QString filename = QFileDialog::getOpenFileName(this, tr("Select an application"), "/");
#endif
    ShowingDialogState::instance().setCurrentlyShowingExternalDialog(false);

    if (!filename.isEmpty()) {
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
            trayMenu_.addAction(tr("Connect"), this, &MainWindow::onTrayMenuConnect);
        }
        else
        {
            trayMenu_.addAction(tr("Disconnect"), this, &MainWindow::onTrayMenuDisconnect);
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

#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
    trayMenu_.addAction(tr("Show/Hide"), this, &MainWindow::onTrayMenuShowHide);
#endif

    if (!mainWindowController_->isPreferencesVisible())
    {
        trayMenu_.addAction(tr("Preferences"), this, &MainWindow::onTrayMenuPreferences);
    }

    trayMenu_.addAction(tr("Help"), this, &MainWindow::onTrayMenuHelpMe);
    trayMenu_.addAction(tr("Exit"), this, &MainWindow::onTrayMenuQuit);
}

void MainWindow::onTrayMenuAboutToShow()
{
    trayMenu_.clear();
#ifndef Q_OS_LINUX
    locationsMenu_.clear();
#endif
#ifdef Q_OS_MACOS
    if (!backend_->getPreferences()->isDockedToTray()) {
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
    mainWindowController_->updateScaling();
    updateTrayIconType(currentAppIconType_);
}

void MainWindow::onDpiScaleManagerNewScreen(QScreen *screen)
{
    Q_UNUSED(screen)
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    // There is a bug that causes the app to be drawn in strange locations under the following scenario:
    // On Windows/Mac: when laptop lid is closed/opened and app is docked
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

void MainWindow::backToLoginWithErrorMessage(LoginWindow::ERROR_MESSAGE_TYPE errorMessageType, const QString &errorMessage)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    setEnabled(false);
    logoutMessageType_ = errorMessageType;
    logoutReason_ = LOGOUT_WITH_MESSAGE;
    logoutErrorMessage_ = errorMessage;
    selectedLocation_->clear();
    backend_->logout(false);
}

void MainWindow::setupTrayIcon()
{
    updateTrayTooltip(tr("Disconnected") + "\n" + PersistentState::instance().lastExternalIp());

    // Create tray menu items here, because it seems like Qt on Linux does not even trigger aboutToShow()
    // if the menu is empty, and since aboutToShow() is never called, we never populate the menu, ad nauseum.
    // Calling createTrayMenuItems() once here makes everything work.
    createTrayMenuItems();

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
    qCInfo(LOG_BASIC) << "Firewall state from last app start:" << bFirewallStateOn;

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
    if (connectState.connectError == LOCATION_NOT_EXIST || connectState.connectError == LOCATION_NO_ACTIVE_NODES) {
        qCWarning(LOG_BASIC) << "Location not exist or no active nodes, try connect to best location";
        LocationID bestLocation = backend_->locationsModelManager()->getBestLocationId();
        selectedLocation_->set(bestLocation);
        PersistentState::instance().setLastLocation(selectedLocation_->locationdId());
        if (selectedLocation_->isValid()) {
            mainWindowController_->getConnectWindow()->updateLocationInfo(selectedLocation_->firstName(), selectedLocation_->secondName(),
                                                                          selectedLocation_->countryCode(), selectedLocation_->pingTime(),
                                                                          selectedLocation_->locationdId().isCustomConfigsLocation());
            onConnectWindowConnectClick();
        } else {
            qCWarning(LOG_BASIC) << "Best Location not exist or no active nodes, goto disconnected mode";
        }
        return;
    } else if (connectState.connectError == IKEV_FAILED_MODIFY_HOSTS_WIN) {
        GeneralMessageController::instance().showMessage("WARNING_WHITE",
                                               tr("Read-only file"),
                                               tr("Your hosts file is read-only. IKEv2 connectivity requires for it to be writable. Fix the issue automatically?"),
                                               GeneralMessageController::tr(GeneralMessageController::kYes),
                                               GeneralMessageController::tr(GeneralMessageController::kNo),
                                               "",
                                               [this](bool b) { if (backend_) backend_->sendMakeHostsFilesWritableWin(); });
        return;
#ifdef Q_OS_WIN
    } else if (connectState.connectError == NO_INSTALLED_TUN_TAP) {
        return;
#elif defined(Q_OS_MACOS)
    } else if (connectState.connectError == LOCKDOWN_MODE_IKEV2) {
        msg = tr("IKEv2 connectivity is not available in MacOS Lockdown Mode. Please disable Lockdown Mode in System Settings or change your connection settings.");
#endif
    } else if (connectState.connectError == CONNECTION_BLOCKED) {
        if (blockConnect_.isBlockedExceedTraffic()) {
            mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_UPGRADE);
            return;
        }
        msg = blockConnect_.message();
    } else if (connectState.connectError == CANNOT_OPEN_CUSTOM_CONFIG) {
        LocationID bestLocation{backend_->locationsModelManager()->getBestLocationId()};
        if (bestLocation.isValid()) {
            selectedLocation_->set(bestLocation);
            PersistentState::instance().setLastLocation(selectedLocation_->locationdId());
            mainWindowController_->getConnectWindow()->updateLocationInfo(selectedLocation_->firstName(), selectedLocation_->secondName(),
                                                                          selectedLocation_->countryCode(), selectedLocation_->pingTime(),
                                                                          selectedLocation_->locationdId().isCustomConfigsLocation());
        }
        msg = tr("The custom configuration could not be loaded.  Please check that it’s correct or contact support.");
    } else if (connectState.connectError == CTRLD_START_FAILED) {
        msg = tr("Unable to start custom DNS service.  Please ensure you don't have any other local DNS services running, or contact support.");
    } else if (connectState.connectError == WIREGUARD_ADAPTER_SETUP_FAILED) {
        msg = tr("WireGuard adapter setup failed. Please wait one minute and try the connection again. If adapter setup fails again,"
                 " please try restarting your computer.\n\nIf the problem persists after a restart, please send a debug log and open"
                 " a support ticket, then switch to a different connection mode.");
    } else if (connectState.connectError == WIREGUARD_COULD_NOT_RETRIEVE_CONFIG) {
        msg = tr("Windscribe could not retrieve server configuration. Please try another protocol.");
    } else {
        msg = tr("An unexpected error occurred establishing the VPN connection (Error %1).  If this error persists, try using a different protocol or contact support.").arg(connectState.connectError);
    }

    GeneralMessageController::instance().showMessage("ERROR_ICON",
                                                     tr("Connection Error"),
                                                     msg,
                                                     GeneralMessageController::tr(GeneralMessageController::kOk),
                                                     "",
                                                     "",
                                                     [&](bool) {
                                                         this->updateAppIconType(AppIconType::DISCONNECTED);
                                                         this->updateTrayIconType(AppIconType::DISCONNECTED);
                                                     },
                                                     [&](bool) {
                                                         this->updateAppIconType(AppIconType::DISCONNECTED);
                                                         this->updateTrayIconType(AppIconType::DISCONNECTED);
                                                     }
    );
}

void MainWindow::setVariablesToInitState()
{
    logoutReason_ = LOGOUT_UNDEFINED;
    isLoginOkAndConnectWindowVisible_ = false;
    bNotificationConnectedShowed_ = false;
    bytesTransferred_ = 0;
    backend_->getPreferencesHelper()->setIsExternalConfigMode(false);
}

void MainWindow::openStaticIpExternalWindow()
{
    QDesktopServices::openUrl(QUrl( QString("https://%1/staticips?cpid=app_windows").arg(HardcodedSettings::instance().windscribeServerUrl())));
}

void MainWindow::openUpgradeExternalWindow()
{
    QDesktopServices::openUrl(QUrl( QString("https://%1/upgrade?pcpid=desktop_upgrade").arg(HardcodedSettings::instance().windscribeServerUrl())));
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
        icon = IconManager::instance().getErrorIcon();
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
        icon = IconManager::instance().getDisconnectedTrayIcon(trayIconColorWhite_);
        break;
    case AppIconType::CONNECTING:
        icon = IconManager::instance().getConnectingTrayIcon(trayIconColorWhite_);
        break;
    case AppIconType::CONNECTED:
        icon = IconManager::instance().getConnectedTrayIcon(trayIconColorWhite_);
        break;
    case AppIconType::DISCONNECTED_WITH_ERROR:
        icon = IconManager::instance().getErrorTrayIcon(trayIconColorWhite_);
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
    GeneralMessageController::instance().showMessage(
        "WARNING_WHITE",
        tr("Reached Key Limit"),
        tr("You have reached your limit of WireGuard public keys. Do you want to delete your oldest key?"),
        GeneralMessageController::tr(GeneralMessageController::kYes),
        GeneralMessageController::tr(GeneralMessageController::kNo),
        "",
        [this](bool b) { emit wireGuardKeyLimitUserResponse(true); },
        [this](bool b) { emit wireGuardKeyLimitUserResponse(false); });
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

    PersistentState::instance().setLastLocation(selectedLocation_->locationdId());
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

    GeneralMessageController::instance().showMessage("WARNING_YELLOW",
                                           tr("Error Starting Service"),
                                           tr("The split tunneling feature could not be started, and has been disabled in Preferences."),
                                           GeneralMessageController::tr(GeneralMessageController::kOk));
}

void MainWindow::showTrayMessage(const QString &message)
{
    if (trayIcon_.isSystemTrayAvailable()) {
        trayIcon_.showMessage("Windscribe", message);
        return;
    }

#if defined(Q_OS_LINUX)
    QDBusInterface dbus("org.freedesktop.Notifications", "/org/freedesktop/Notifications",
                        "org.freedesktop.Notifications", QDBusConnection::sessionBus());

    if (!dbus.isValid()) {
        qCWarning(LOG_BASIC) << "MainWindow::showTrayMessage - could not connect to the notification manager using dbus."
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
        qCWarning(LOG_BASIC) << "MainWindow::showTrayMessage - could not display the message."
                           << (dbus.lastError().isValid() ? dbus.lastError().message() : "");
    }
#else
    qCWarning(LOG_BASIC) << "QSystemTrayIcon reports the system tray is not available";
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

#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
void MainWindow::setSigTermHandler(int fd)
{
    fd_ = fd;

    socketNotifier_ = new QSocketNotifier(fd_, QSocketNotifier::Read, this);
    connect(socketNotifier_, &QSocketNotifier::activated, this, &MainWindow::onSigTerm);
}

void MainWindow::onSigTerm()
{
    socketNotifier_->setEnabled(false);
    char tmp;
    if (::read(fd_, &tmp, sizeof(tmp)) < 0) {
        qCWarning(LOG_BASIC) << "Could not read from signal socket";
        return;
    }

    doClose(nullptr, true);
    qApp->quit();
}
#endif

void MainWindow::checkLocationPermission()
{
    if (curNetwork_.interfaceType != NETWORK_INTERFACE_WIFI) {
        return;
    }

#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    if (PersistentState::instance().isIgnoreLocationServicesDisabled()) {
        qCInfo(LOG_BASIC) << "Location permission prompt not required";
        return;
    }

#ifdef Q_OS_MACOS
    if (MacUtils::isOsVersionAtLeast(15, 0) && !NetworkUtils_mac::isLocationServicesOn()) {
#else
    if (!NetworkUtils_win::isSsidAccessAvailable()) {
#endif
        GeneralMessageController::instance().showMessage(
            "WARNING_YELLOW",
            tr("Location Services is disabled"),
#ifdef Q_OS_MACOS
            tr("Windscribe requires Location Services to determine your Wi-Fi SSID. If it is not enabled, per-network settings will apply to all Wi-Fi networks. Please enable Location Services and grant the permission to Windscribe in your System Settings."),
#else
            tr("Windscribe requires Location Services to determine your Wi-Fi SSID. If it is not enabled, per-network settings will apply to all Wi-Fi networks. Please enable Location Services in your System Settings."),
#endif
            GeneralMessageController::tr(GeneralMessageController::kOk),
            "",
            "",
            [this](bool b) {
                PersistentState::instance().setIgnoreLocationServicesDisabled(b);
                if (isLoginOkAndConnectWindowVisible_) {
                    mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_CONNECT);
                } else {
                    mainWindowController_->changeWindow(MainWindowController::WINDOW_ID_LOGIN);
                }
            },
            std::function<void(bool)>(nullptr),
            std::function<void(bool)>(nullptr),
            GeneralMessage::kShowBottomPanel);
#ifdef Q_OS_MACOS
    } else {
        if (qApp->checkPermission(QLocationPermission{}) == Qt::PermissionStatus::Undetermined) {
            qApp->requestPermission(QLocationPermission{}, this, &MainWindow::onLocationPermissionUpdated);
        }
#endif
    }
#endif
}

void MainWindow::onLocationPermissionUpdated()
{
#if defined(Q_OS_MACOS)
    if (qApp->checkPermission(QLocationPermission{}) == Qt::PermissionStatus::Granted) {
        qCInfo(LOG_BASIC) << "Location permission granted";
    } else if (qApp->checkPermission(QLocationPermission{}) == Qt::PermissionStatus::Denied) {
        qCInfo(LOG_BASIC) << "Location permission denied";
    }
    backend_->updateCurrentNetworkInterface();
#endif
}

void MainWindow::checkNotificationEnabled()
{
#ifdef Q_OS_WIN
    if (!backend_->getPreferences()->isShowNotifications()) {
        // Not showing notifications, ignore
        return;
    }

    if (WinUtils::isNotificationEnabled()) {
        // System notifications are enabled, ignore
        return;
    }

    if (PersistentState::instance().isIgnoreNotificationDisabled()) {
        // User previously selected 'Ignore warnings'
        qCInfo(LOG_BASIC) << "Notification prompt not required";
        return;
    }

    GeneralMessageController::instance().showMessage(
        "WARNING_YELLOW",
        tr("System notifications are disabled"),
        tr("You have chosen to show notifications, but system notifications are disabled. Please enable system notifications in your System Settings."),
        GeneralMessageController::tr(GeneralMessageController::kOk),
        GeneralMessageController::tr(GeneralMessageController::kCancel),
        "",
        [this](bool b) {
            // User clicked "Ok", bring up the notification settings
            WinUtils::openNotificationSettings();
            // If user selected 'Ignore warnings', don't show the prompt again
            PersistentState::instance().setIgnoreNotificationDisabled(b);
        },
        [this](bool b) {
            // User clicked "Cancel"
            // If user selected 'Ignore warnings', don't show the prompt again
            PersistentState::instance().setIgnoreNotificationDisabled(b);
        },
        std::function<void(bool)>(nullptr),
        GeneralMessage::kShowBottomPanel);
#endif
}

void MainWindow::onPreferencesShowNotificationsChanged()
{
    checkNotificationEnabled();
}

#if defined(Q_OS_MACOS)
void MainWindow::onPreferencesMultiDesktopBehaviorChanged(MULTI_DESKTOP_BEHAVIOR m)
{
    MULTI_DESKTOP_BEHAVIOR mdb = backend_->getPreferences()->multiDesktopBehavior();
    if (mdb == MULTI_DESKTOP_AUTO) {
        // When set to auto, duplicate window if docked, otherwise move window
        WidgetUtils_mac::allowMoveBetweenSpacesForWindow(this, backend_->getPreferences()->isDockedToTray(), true);
    } else {
        WidgetUtils_mac::allowMoveBetweenSpacesForWindow(this, mdb == MULTI_DESKTOP_DUPLICATE, mdb == MULTI_DESKTOP_MOVE_WINDOW);
    }
}
#endif
