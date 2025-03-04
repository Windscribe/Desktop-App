#pragma once

#include <QSocketNotifier>
#include <QSystemTrayIcon>
#include <QWidget>
#include <QWidgetAction>
#include "generalmessagecontroller.h"
#include "mainwindowcontroller.h"

#include "locationswindow/locationswindow.h"
#include "backend/backend.h"
#include "localipcserver/localipcserver.h"
#include "backend/notificationscontroller.h"
#include "log/logviewerwindow.h"
#include "loginattemptscontroller.h"
#include "multipleaccountdetection/imultipleaccountdetection.h"
#include "blockconnect.h"
#include "freetrafficnotificationcontroller.h"
#include "graphicresources/iconmanager.h"
#include "systemtray/locationstraymenunative.h"
#include "systemtray/locationstraymenu.h"
#include "dialogs/advancedparametersdialog.h"
#include "locations/model/selectedlocation.h"
#include "api_responses/robertfilter.h"
#include "api_responses/checkupdate.h"
#include "protocolwindow/protocolwindowmode.h"

#if defined(Q_OS_MACOS)
#include "permissions/permissionmonitor_mac.h"
#endif

#if defined(Q_OS_MACOS)
#define USE_LOCATIONS_TRAY_MENU_NATIVE
#endif

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();

    virtual bool eventFilter(QObject *watched, QEvent *event);

    bool doClose(QCloseEvent *event = NULL, bool isFromSigTerm_mac = false);
    bool isActiveState() const { return activeState_; }
    QRect trayIconRect();
    void showAfterLaunch();

    bool handleKeyPressEvent(QKeyEvent *event);
#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
    void setSigTermHandler(int fd);
#endif

protected:
    bool event(QEvent *event);
    void closeEvent(QCloseEvent *event);

    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

    virtual void paintEvent(QPaintEvent *event);

signals:
    void wireGuardKeyLimitUserResponse(bool deleteOldestKey);

private slots:
    void setWindowToDpiScaleManager();

    void onMinimizeClick();
    void onCloseClick();
    void onEscapeClick();

    // init window signals
    void onAbortInitialization();

    // login window signals
    void onLoginClick(const QString &username, const QString &password, const QString &code2fa);
    void onLoginPreferencesClick();
    void onLoginHaveAccountYesClick();
    void onLoginBackToWelcomeClick();
    void onLoginEmergencyWindowClick();
    void onLoginExternalConfigWindowClick();
    void onLoginTwoFactorAuthWindowClick(const QString &username, const QString &password);
    void onLoginFirewallTurnOffClick();

    // connect window signals
    void onConnectWindowConnectClick();
    void onConnectWindowFirewallClick();
    void onConnectWindowNetworkButtonClick();
    void onConnectWindowLocationsClick();
    void onConnectWindowPreferencesClick();
    void onConnectWindowNotificationsClick();
    void onConnectWindowSplitTunnelingClick();
    void onConnectWindowProtocolsClick();

    // news feed window signals
    void onEscapeNotificationsClick();
    // protocol window signals
    void onEscapeProtocolsClick();
    void onProtocolWindowProtocolClick(const types::Protocol &protocol, uint port);
    void onProtocolWindowDisconnect();

    // preferences window signals
    void onPreferencesEscapeClick();
    void onPreferencesLogoutClick();
    void onPreferencesLoginClick();
    void onPreferencesViewLogClick();
    void onPreferencesExportSettingsClick();
    void onPreferencesImportSettingsClick();
    void onPreferencesSendConfirmEmailClick();
    void onSendDebugLogClick(); // also used by protocol window
    void onPreferencesManageAccountClick();
    void onPreferencesAddEmailButtonClick();
    void onPreferencesManageRobertRulesClick();
    void onPreferencesQuitAppClick();
    void onPreferencesAccountLoginClick();
    void onPreferencesCycleMacAddressClick();
    void onPreferencesWindowDetectPacketSizeClick();
    void onPreferencesAdvancedParametersClicked();
    void onPreferencesCustomConfigsPathChanged(QString path);
    void onPreferencesAdvancedParametersChanged(const QString &advParams);
    void onPreferencesLastKnownGoodProtocolChanged(const QString &network, const types::Protocol &protocol, uint port);
    void onPreferencesCustomConfigPathNeedsUpdate(const QString &path);
    void onPreferencesShowNotificationsChanged();

    // emergency window signals
    void onEmergencyConnectClick();
    void onEmergencyDisconnectClick();

    // external config window signals
    void onExternalConfigWindowNextClick();

    // 2FA window signals
    void onTwoFactorAuthWindowButtonAddClick(const QString &code2fa);

    // bottom window signals
    void onBottomWindowRenewClick();
    void onBottomWindowExternalConfigLoginClick();
    void onBottomWindowSharingFeaturesClick();

    // update app item signals
    void onUpdateAppItemClick();

    // update window signals
    void onUpdateWindowAccept();
    void onUpdateWindowCancel();
    void onUpdateWindowLater();

    // upgrade window signals
    void onUpgradeAccountAccept();
    void onUpgradeAccountCancel();

    // logout & exit window signals
    void onLogoutWindowAccept();
    void onExitWindowAccept();
    void onExitWindowReject();

    // locations window signals
    void onLocationSelected(const LocationID &lid);
    void onClickedOnPremiumStarCity();
    void onLocationsAddStaticIpClicked();
    void onLocationsClearCustomConfigClicked();
    void onLocationsAddCustomConfigClicked();

    void onLanguageChanged();

    // backend state signals
    void onBackendInitFinished(INIT_STATE initState);

    void onBackendLoginFinished(bool isLoginFromSavedSettings);
    void onBackendTryingBackupEndpoint(int num, int cnt);
    void onBackendLoginError(wsnet::LoginResult loginError, const QString &errorMessage);

    void onBackendSessionStatusChanged(const api_responses::SessionStatus &sessionStatus);
    void onBackendCheckUpdateChanged(const api_responses::CheckUpdate &checkUpdateInfo);
    void onBackendMyIpChanged(QString ip, bool isFromDisconnectedState);
    void onBackendConnectStateChanged(const types::ConnectState &connectState);
    void onBackendEmergencyConnectStateChanged(const types::ConnectState &connectState);
    void onBackendFirewallStateChanged(bool isEnabled);
    void onNetworkChanged(types::NetworkInterface network);
    void onSplitTunnelingStateChanged(bool isActive);
    void onBackendLogoutFinished();
    void onBackendCleanupFinished();
    void onBackendGotoCustomOvpnConfigModeFinished();
    void onBackendConfirmEmailResult(bool bSuccess);
    void onBackendDebugLogResult(bool bSuccess);
    void onBackendStatisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes);
    void onBackendProxySharingInfoChanged(const types::ProxySharingInfo &psi);
    void onBackendWifiSharingInfoChanged(const types::WifiSharingInfo &wsi);
    void onBackendWifiSharingFailed(WIFI_SHARING_ERROR error);
    void onBackendRequestCustomOvpnConfigCredentials(const QString &username);
    void onBackendRequestCustomOvpnConfigPrivKeyPassword();
    void onBackendSessionDeleted();
    void onBackendTestTunnelResult(bool success);
    void onBackendLostConnectionToHelper();
    void onBackendHighCpuUsage(const QStringList &processesList);
    void onBackendUserWarning(USER_WARNING_TYPE userWarningType);
    void onBackendInternetConnectivityChanged(bool connectivity);
    void onBackendProtocolPortChanged(const types::Protocol &protocol, const uint port);
    void onBackendPacketSizeDetectionStateChanged(bool on, bool isError);
    void onBackendUpdateVersionChanged(uint progressPercent, UPDATE_VERSION_STATE state, UPDATE_VERSION_ERROR error);
    void onBackendWebSessionTokenForManageAccount(const QString &tempSessionToken);
    void onBackendWebSessionTokenForAddEmail(const QString &tempSessionToken);
    void onBackendWebSessionTokenForManageRobertRules(const QString &tempSessionToken);
    void onBackendRobertFiltersChanged(bool success, const QVector<api_responses::RobertFilter> &filters);
    void onBackendSetRobertFilterResult(bool success);
    void onBackendSyncRobertResult(bool success);
    void onBackendProtocolStatusChanged(const QVector<types::ProtocolStatus> &status);
    void onHelperSplitTunnelingStartFailed();

    void onBackendEngineCrash();

    void onNotificationControllerNewPopupMessage(int messageId);

    // preferences changes signals
    void onPreferencesFirewallSettingsChanged(const types::FirewallSettings &fm);
    void onPreferencesShareProxyGatewayChanged(const types::ShareProxyGateway &sp);
    void onPreferencesShareSecureHotspotChanged(const types::ShareSecureHotspot &ss);
    void onPreferencesLocationOrderChanged(ORDER_LOCATION_TYPE o);
    void onPreferencesSplitTunnelingChanged(types::SplitTunneling st);
    void onPreferencesAllowLanTrafficChanged(bool bAllowLanTraffic);
    void onPreferencesLaunchOnStartupChanged(bool bEnabled);
    void onPreferencesConnectionSettingsChanged(types::ConnectionSettings connectionSettings);
    void onPreferencesNetworkPreferredProtocolsChanged(QMap<QString, types::ConnectionSettings> p);
    void onPreferencesIsDockedToTrayChanged(bool isDocked);
    void onPreferencesUpdateChannelChanged(UPDATE_CHANNEL updateChannel);
    void onPreferencesGetRobertFilters();
    void onPreferencesSetRobertFilter(const api_responses::RobertFilter &filter);
    void onPreferencesReportErrorToUser(const QString &title, const QString &desc);
#if defined(Q_OS_MACOS)
    void onPreferencesMultiDesktopBehaviorChanged(MULTI_DESKTOP_BEHAVIOR m);
#endif

    void onPreferencesCollapsed();

#if defined(Q_OS_MACOS)
    void onPreferencesHideFromDockChanged(bool hideFromDock);
    void hideShowDockIconImpl(bool bAllowActivateAndShow);
#elif defined(Q_OS_LINUX)
    void onPreferencesTrayIconColorChanged(TRAY_ICON_COLOR c);
#endif
    // WindscribeApplications signals
    void onDockIconClicked();
    void onAppActivateFromAnotherInstance();
    void onAppShouldTerminate();
    void onAppCloseRequest();
#if defined(Q_OS_WIN)
    void onAppWinIniChanged();
#endif

    // LocalIPCServer signals
    void onIpcOpenLocations(IPC::CliCommands::LocationType type);
    void onIpcConnect(const LocationID &id, const types::Protocol &protocol);
    void onIpcConnectStaticIp(const QString &location, const types::Protocol &protocol);
    void onIpcUpdate();

    void showShutdownWindow();

    void onCurrentNetworkUpdated(types::NetworkInterface networkInterface);
    void onAutoConnectUpdated(bool on);

    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onTrayMenuConnect();
    void onTrayMenuDisconnect();
    void onTrayMenuPreferences();
    void onTrayMenuShowHide();
    void onTrayMenuHelpMe();
    void onTrayMenuQuit();
    void onTrayMenuAboutToShow();
    void onTrayMenuAboutToHide();

    void onLocationsTrayMenuLocationSelected(const LocationID &lid);

    void onFreeTrafficNotification(const QString &message);
    void onSplitTunnelingAppsAddButtonClick();
    void onRevealConnectStateChanged(bool revealingConnect);

    void onMainWindowControllerSendServerRatingUp();
    void onMainWindowControllerSendServerRatingDown();

    void onScaleChanged();
    void onDpiScaleManagerNewScreen(QScreen *screen);
    void onFocusWindowChanged(QWindow *focusWindow);
    void onWindowDeactivateAndHideImpl();

    void onAdvancedParametersOkClick();
    void onAdvancedParametersCancelClick();

    void onWireGuardAtKeyLimit();

    // Selected location signals
    void onSelectedLocationChanged();
    void onSelectedLocationRemoved();

    void onLocationPermissionUpdated();

#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
    void onSigTerm();
#endif

private:
    void gotoLoginWindow();
    void gotoLogoutWindow();
    void gotoExitWindow();
    void collapsePreferences();
    void selectLocation(const LocationID &lid, const types::Protocol &protocol);

    Backend *backend_;
    LocalIPCServer *localIpcServer_;
    NotificationsController notificationsController_;

    MainWindowController *mainWindowController_;

    LocationsWindow *locationsWindow_;
    LogViewer::LogViewerWindow *logViewerWindow_;
    AdvancedParametersDialog *advParametersWindow_;

    QMenu trayMenu_;

#ifndef Q_OS_LINUX

#if defined(USE_LOCATIONS_TRAY_MENU_NATIVE)
    QVector<QSharedPointer<LocationsTrayMenuNative> > locationsMenu_;
#else
    QVector<QSharedPointer<LocationsTrayMenu> > locationsMenu_;
#endif

#endif

    enum class AppIconType { DISCONNECTED, DISCONNECTED_WITH_ERROR, CONNECTING, CONNECTED };
    void updateAppIconType(AppIconType type);
    void updateTrayIconType(AppIconType type);
    void updateTrayTooltip(QString tooltip);
    AppIconType currentAppIconType_;
    QSystemTrayIcon trayIcon_;
    QRect savedTrayIconRect_;
    bool bNotificationConnectedShowed_;
    QElapsedTimer connectionElapsedTimer_;
    quint64 bytesTransferred_;
    bool bMovingWindow_;

    enum LOGOUT_REASON { LOGOUT_UNDEFINED, LOGOUT_FROM_MENU, LOGOUT_SESSION_EXPIRED, LOGOUT_WITH_MESSAGE, LOGOUT_GO_TO_LOGIN };
    LOGOUT_REASON logoutReason_;
    LoginWindow::ERROR_MESSAGE_TYPE logoutMessageType_;
    QString logoutErrorMessage_;

    LoginAttemptsController loginAttemptsController_;
    QSharedPointer<IMultipleAccountDetection> multipleAccountDetection_;
    BlockConnect blockConnect_;
    FreeTrafficNotificationController *freeTrafficNotificationController_;

    bool isInitializationAborted_;
    bool isLoginOkAndConnectWindowVisible_;
    static constexpr int TIME_BEFORE_SHOW_SHUTDOWN_WINDOW = 1500;   // ms

    QScopedPointer<gui_locations::SelectedLocation> selectedLocation_;

    void hideSupplementaryWidgets();

    void backToLoginWithErrorMessage(LoginWindow::ERROR_MESSAGE_TYPE errorMessageType, const QString &errorMessage);
    void setupTrayIcon();
    QString getConnectionTime();
    QString getConnectionTransferred();
    void setInitialFirewallState();
    void handleDisconnectWithError(const types::ConnectState &connectState);
    void setVariablesToInitState();

    void openStaticIpExternalWindow();
    void openUpgradeExternalWindow();

    bool onLocalHttpServerCommand(QString authHash, bool isConnectCmd, bool isDisconnectCmd, QString location);

    bool revealingConnectWindow_;
    bool internetConnected_;

    QSet<USER_WARNING_TYPE> alreadyShownWarnings_;

    types::NetworkInterface curNetwork_;

    void activateAndShow();
    void deactivateAndHide();
    void createTrayMenuItems();

    bool backendAppActiveState_;
    void setBackendAppActiveState(bool state);

    bool trayIconColorWhite_;

#if defined(Q_OS_MACOS)
    void hideShowDockIcon(bool hideFromDock);
    QTimer hideShowDockIconTimer_;
    bool currentDockIconVisibility_;
    bool desiredDockIconVisibility_;

    typedef QRect TrayIconRelativeGeometry ;
    QMap<QString, TrayIconRelativeGeometry> systemTrayIconRelativeGeoScreenHistory_;
    QString lastScreenName_;

    const QRect bestGuessForTrayIconRectFromLastScreen(const QPoint &pt);
    const QRect trayIconRectForLastScreen();
    const QRect trayIconRectForScreenContainingPt(const QPoint &pt);
    const QRect generateTrayIconRectFromHistory(const QString &screenName);
#endif
    QTimer deactivationTimer_;

    bool activeState_;
    qint64 lastWindowStateChange_;

    bool isExitingFromPreferences_;
    bool isSpontaneousCloseEvent_;
    bool isExitingAfterUpdate_;

    void minimizeToTray();

    bool downloadRunning_;
    bool ignoreUpdateUntilNextRun_;
    void cleanupAdvParametersWindow();
    void cleanupLogViewerWindow();

    QRect guessTrayIconLocationOnScreen(QScreen *screen);
    void showUserWarning(USER_WARNING_TYPE userWarningType);
    void openBrowserToMyAccountWithToken(const QString &tempSessionToken);
    void updateConnectWindowStateProtocolPortDisplay();
    void showTrayMessage(const QString &message);

    types::Protocol getDefaultProtocolForNetwork(const QString &network);
    bool userProtocolOverride_;

    bool sendDebugLogOnDisconnect_;

    QSocketNotifier *socketNotifier_;
    int fd_;

    void normalizeConnectionSettings(types::ConnectionSettings &cs);

#ifdef Q_OS_MACOS
    PermissionMonitor_mac *permissionMonitor_;
#endif
    void checkLocationPermission();
    void checkNotificationEnabled();

    void checkCustomConfigPath(const QString &path);
};
