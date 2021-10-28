#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QSystemTrayIcon>
#include <QWidgetAction>
#include "mainwindowcontroller.h"

#include "locationswindow/locationswindow.h"
#include "../backend/backend.h"
#include "../backend/notificationscontroller.h"
#include "log/logviewerwindow.h"
#include "loginattemptscontroller.h"
#include "multipleaccountdetection/imultipleaccountdetection.h"
#include "blockconnect.h"
#include "freetrafficnotificationcontroller.h"
#include "graphicresources/iconmanager.h"
#include "guitest.h"
#include "systemtray/locationstraymenutypes.h"
#include "systemtray/locationstraymenunative.h"
#include "systemtray/locationstraymenuwidget.h"
#include "dialogs/advancedparametersdialog.h"

#if defined(Q_OS_MAC)
#define USE_LOCATIONS_TRAY_MENU_NATIVE
#endif

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();

    virtual bool eventFilter(QObject *watched, QEvent *event);

    void doClose(QCloseEvent *event = NULL, bool isFromSigTerm_mac = false);
    void updateConnectWindowStateProtocolPortDisplay(ProtoTypes::ConnectionSettings connectionSettings);
    bool isActiveState() const { return activeState_; }
    QRect trayIconRect();
    void showAfterLaunch();

protected:
    bool event(QEvent *event);
    void closeEvent(QCloseEvent *event);

    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);

    virtual void paintEvent(QPaintEvent *event);

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

    // news feed window signals
    void onEscapeNotificationsClick();

    // preferences window signals
    void onPreferencesEscapeClick();
    void onPreferencesSignOutClick();
    void onPreferencesLoginClick();
    void onPreferencesHelpClick();
    void onPreferencesViewLogClick();
    void onPreferencesSendConfirmEmailClick();
    void onPreferencesSendDebugLogClick();
    void onPreferencesQuitAppClick();
    void onPreferencesNoAccountLoginClick();
    void onPreferencesSetIpv6StateInOS(bool bEnabled, bool bRestartNow);
	void onPreferencesCycleMacAddressClick();
    void onPreferencesWindowDetectAppropriatePacketSizeButtonClicked();
    void onPreferencesAdvancedParametersClicked();
    void onPreferencesCustomConfigsPathChanged(QString path);

    // emergency window signals
    void onEmergencyConnectClick();
    void onEmergencyDisconnectClick();
    void onEmergencyWindscribeLinkClick();

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

    // general window signals
    void onGeneralMessageWindowAccept();

    // exit window signals
    void onExitWindowAccept();
    void onExitWindowReject();

    // locations window signals
    void onLocationSelected(LocationID id);
    void onClickedOnPremiumStarCity();
    void onLocationSwitchFavorite(LocationID id, bool isFavorite);
    void onLocationsAddStaticIpClicked();
    void onLocationsClearCustomConfigClicked();
    void onLocationsAddCustomConfigClicked();

    void onLanguageChanged();

    // backend state signals
    void onBackendInitFinished(ProtoTypes::InitState initState);
    void onBackendInitTooLong();

    //void onBackendLoginInfoChanged(const ProtoTypes::LoginInfo &loginInfo);
    void onBackendLoginFinished(bool isLoginFromSavedSettings);
    void onBackendLoginStepMessage(ProtoTypes::LoginMessage msg);
    void onBackendLoginError(ProtoTypes::LoginError loginError);


    void onBackendSessionStatusChanged(const ProtoTypes::SessionStatus &sessionStatus);
    void onBackendCheckUpdateChanged(const ProtoTypes::CheckUpdateInfo &checkUpdateInfo);
    void onBackendMyIpChanged(QString ip, bool isFromDisconnectedState);
    void onBackendConnectStateChanged(const ProtoTypes::ConnectState &connectState);
    void onBackendEmergencyConnectStateChanged(const ProtoTypes::ConnectState &connectState);
    void onBackendFirewallStateChanged(bool isEnabled);
    void onNetworkChanged(ProtoTypes::NetworkInterface network);
    void onSplitTunnelingStateChanged(bool isActive);
    void onBackendSignOutFinished();
    void onBackendCleanupFinished();
    void onBackendGotoCustomOvpnConfigModeFinished();
    void onBackendConfirmEmailResult(bool bSuccess);
    void onBackendDebugLogResult(bool bSuccess);
    void onBackendStatisticsUpdated(quint64 bytesIn, quint64 bytesOut, bool isTotalBytes);
    void onBackendProxySharingInfoChanged(const ProtoTypes::ProxySharingInfo &psi);
    void onBackendWifiSharingInfoChanged(const ProtoTypes::WifiSharingInfo &wsi);
    void onBackendRequestCustomOvpnConfigCredentials();
    void onBackendSessionDeleted();
    void onBackendTestTunnelResult(bool success);
    void onBackendLostConnectionToHelper();
    void onBackendHighCpuUsage(const QStringList &processesList);
    void onBackendUserWarning(ProtoTypes::UserWarningType userWarningType);
    void onBackendInternetConnectivityChanged(bool connectivity);
    void onBackendProtocolPortChanged(const ProtoTypes::Protocol &protocol, const uint port);
    void onBackendPacketSizeDetectionStateChanged(bool on, bool isError);
    void onBackendUpdateVersionChanged(uint progressPercent, ProtoTypes::UpdateVersionState state, ProtoTypes::UpdateVersionError error);
    void onBackendEngineCrash();
    void onBackendLocationsUpdated();

    void onNotificationControllerNewPopupMessage(int messageId);

    void onBestLocationChanged(const LocationID &bestLocation);

    // preferences changes signals
    void onPreferencesFirewallSettingsChanged(const ProtoTypes::FirewallSettings &fm);
    void onPreferencesShareProxyGatewayChanged(const ProtoTypes::ShareProxyGateway &sp);
    void onPreferencesShareSecureHotspotChanged(const ProtoTypes::ShareSecureHotspot &ss);
    void onPreferencesLocationOrderChanged(ProtoTypes::OrderLocationType o);
    void onPreferencesSplitTunnelingChanged(ProtoTypes::SplitTunneling st);
    void onPreferencesUpdateEngineSettings();
    void onPreferencesLaunchOnStartupChanged(bool bEnabled);
    void onPreferencesConnectionSettingsChanged(ProtoTypes::ConnectionSettings connectionSettings);
    void onPreferencesIsDockedToTrayChanged(bool isDocked);
    void onPreferencesUpdateChannelChanged(const ProtoTypes::UpdateChannel updateChannel);

    void onPreferencesReportErrorToUser(const QString &title, const QString &desc);

    void onPreferencesCollapsed();

#ifdef Q_OS_MAC
    void onPreferencesHideFromDockChanged(bool hideFromDock);
    void hideShowDockIconImpl();
#endif
    // WindscribeApplications signals
    void toggleVisibilityIfDocked();
    void onAppActivateFromAnotherInstance();
    void onAppShouldTerminate_mac();
    void onReceivedOpenLocationsMessage();
    void onAppCloseRequest();
#if defined(Q_OS_WIN)
    void onAppWinIniChanged();
#endif

    void showShutdownWindow();

    void onCurrentNetworkUpdated(ProtoTypes::NetworkInterface networkInterface);

    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onTrayMenuConnect();
    void onTrayMenuDisconnect();
    void onTrayMenuPreferences();
    void onTrayMenuHide();
    void onTrayMenuShow();
    void onTrayMenuHelpMe();
    void onTrayMenuQuit();
    void onTrayMenuAboutToShow();
    void onLocationsTrayMenuLocationSelected(int type, QString locationTitle, int cityIndex);

    void onFreeTrafficNotification(const QString &message);
    void onNativeInfoErrorMessage(QString title, QString desc);
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

private:
    void gotoLoginWindow();
    void gotoExitWindow();
    void collapsePreferences();

    Backend *backend_;
    NotificationsController notificationsController_;

    MainWindowController *mainWindowController_;

    LocationsWindow *locationsWindow_;
    LogViewer::LogViewerWindow *logViewerWindow_;
    AdvancedParametersDialog *advParametersWindow_;

    QMenu trayMenu_;

#ifndef Q_OS_LINUX

#if defined(USE_LOCATIONS_TRAY_MENU_NATIVE)
    LocationsTrayMenuNative locationsMenu_[LOCATIONS_TRAY_MENU_NUM_TYPES];
#else
    QMenu locationsMenu_[LOCATIONS_TRAY_MENU_NUM_TYPES];
    QWidgetAction *listWidgetAction_[LOCATIONS_TRAY_MENU_NUM_TYPES];
    LocationsTrayMenuWidget *locationsTrayMenuWidget_[LOCATIONS_TRAY_MENU_NUM_TYPES];
#endif

#endif

    enum class AppIconType { DISCONNECTED, CONNECTING, CONNECTED };
    void updateAppIconType(AppIconType type);
    void updateTrayIconType(AppIconType type);
    void updateTrayTooltip(QString tooltip);
    AppIconType currentAppIconType_;
    QSystemTrayIcon trayIcon_;
    QRect savedTrayIconRect_;
    bool bNotificationConnectedShowed_;
    QElapsedTimer connectionElapsedTimer_;
    quint64 bytesTransferred_;

    QPoint dragPosition_;
    QPoint dragPositionForTooltip_;
    bool bMousePressed_;
    bool bMoveEnabled_;

    enum SIGN_OUT_REASON { SIGN_OUT_UNDEFINED, SIGN_OUT_FROM_MENU, SIGN_OUT_SESSION_EXPIRED, SIGN_OUT_WITH_MESSAGE };
    SIGN_OUT_REASON signOutReason_;
    ILoginWindow::ERROR_MESSAGE_TYPE signOutMessageType_;

    LoginAttemptsController loginAttemptsController_;
    GuiTest *guiTest_;
    QSharedPointer<IMultipleAccountDetection> multipleAccountDetection_;
    BlockConnect blockConnect_;
    FreeTrafficNotificationController *freeTrafficNotificationController_;

    int prevSessionStatus_;
    bool isPrevSessionStatusInitialized_;

    bool bDisconnectFromTrafficExceed_;

    bool isInitializationAborted_;
    bool isLoginOkAndConnectWindowVisible_;
    static constexpr int TIME_BEFORE_SHOW_SHUTDOWN_WINDOW = 1500;   // ms

    void hideSupplementaryWidgets();

    void backToLoginWithErrorMessage(ILoginWindow::ERROR_MESSAGE_TYPE errorMessage);
    void setupTrayIcon();
    QString getConnectionTime();
    QString getConnectionTransferred();
    void setInitialFirewallState();
    void handleDisconnectWithError(const ProtoTypes::ConnectState &connectState);
    void setVariablesToInitState();

    void openStaticIpExternalWindow();
    void openUpgradeExternalWindow();

    bool onLocalHttpServerCommand(QString authHash, bool isConnectCmd, bool isDisconnectCmd, QString location);

    bool revealingConnectWindow_;
    bool internetConnected_;

    bool currentlyShowingUserWarningMessage_;

    bool bGotoUpdateWindowAfterGeneralMessage_;

    void activateAndShow();
    void deactivateAndHide();
    void loadTrayMenuItems();

    bool backendAppActiveState_;
    void setBackendAppActiveState(bool state);

    bool isRunningInDarkMode_;

#if defined(Q_OS_MAC)
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

    void minimizeToTray();

    bool downloadRunning_;
    bool ignoreUpdateUntilNextRun_;
    void cleanupAdvParametersWindow();
    void cleanupLogViewerWindow();

    QRect guessTrayIconLocationOnScreen(QScreen *screen);
};

#endif // MAINWINDOW_H
