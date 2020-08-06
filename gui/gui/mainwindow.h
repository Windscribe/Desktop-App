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
#include "localhttpserver/localhttpserver.h"
#include "guitest.h"
#include "systemtray/locationstraymenuwidget.h"


class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    virtual bool eventFilter(QObject *watched, QEvent *event);

    void doClose(QCloseEvent *event = NULL, bool isFromSigTerm_mac = false);
    void updateScaling();
    void updateConnectWindowStateProtocolPortDisplay(ProtoTypes::ConnectionSettings connectionSettings);
    QRect trayIconRect();

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
    void onMinimizeClick();
    void onCloseClick();

    // login window signals
    void onLoginClick(const QString &username, const QString &password);
    void onLoginPreferencesClick();
    void onLoginHaveAccountYesClick();
    void onLoginEmergencyWindowClick();
    void onLoginExternalConfigWindowClick();
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
    void onPreferencesClearServerRatingsClick();
    void onPreferencesSetIpv6StateInOS(bool bEnabled, bool bRestartNow);
	void onPreferencesCycleMacAddressClick();
    void onPreferencesWindowDetectPacketMssButtonClicked();

    // emergency window signals
    void onEmergencyEscapeClick();
    void onEmergencyConnectClick();
    void onEmergencyDisconnectClick();
    void onEmergencyWindscribeLinkClick();

    // external config window signals
    void onExternalConfigWindowNextClick();
    void onExternalConfigWindowEscapeClick();

    // bottom window signals
    void onBottomWindowUpgradeAccountClick();
    void onBottomWindowRenewClick();
    void onBottomWindowExternalConfigLoginClick();
    void onBottomWindowSharingFeaturesClick();

    // update app item signals
    void onUpdateAppItemClick();

    // update window signals
    void onUpdateWindowAccept();
    void onUpdateWindowCancel();

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
    void onLocationSwitchFavorite(LocationID id, bool isFavorite);
    void onLocationsAddStaticIpClicked();
    void onLocationsAddCustomConfigClicked();

    void onLanguageChanged();

    // backend state signals
    void onBackendInitFinished(ProtoTypes::InitState initState);

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
    void onBackendPacketSizeDetectionStateChanged(bool on);
    void onBackendEngineCrash();

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
#ifdef Q_OS_MAC
    void onPreferencesHideFromDockChanged(bool hideFromDock);
#endif
    // WindscribeApplications signals
    void toggleVisibilityIfDocked();
    void onAppActivateFromAnotherInstance();
    void onAppShouldTerminate_mac();
    void onReceivedOpenLocationsMessage();

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
    void onLocationsTrayMenuLocationSelected(int locationId);

    void onFreeTrafficNotification(const QString &message);
    void onNativeInfoErrorMessage(QString title, QString desc);
    void onSplitTunnelingAppsAddButtonClick();
    void onRevealConnectStateChanged(bool revealingConnect);

    void onMainWindowControllerSendServerRatingUp();
    void onMainWindowControllerSendServerRatingDown();

    void onScaleChanged();

private:
    void gotoLoginWindow();
    void collapsePreferences();

    Backend *backend_;
    NotificationsController notificationsController_;

    MainWindowController *mainWindowController_;

    LocationsWindow *locationsWindow_;
    LogViewer::LogViewerWindow *logViewerWindow_;

    QWidgetAction *listWidgetAction_;
    QMenu trayMenu_;
    QMenu locationsMenu_;
    QSystemTrayIcon trayIcon_;
    IconManager iconManager_;
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

    bool isLoginOkAndConnectWindowVisible_;
    bool isCustomConfigMode_;
    const int TIME_BEFORE_SHOW_SHUTDOWN_WINDOW = 1500;   // ms

    void hideSupplementaryWidgets();

    void backToLoginWithErrorMessage(ILoginWindow::ERROR_MESSAGE_TYPE errorMessage);
    void setupTrayIcon();
    QString getConnectionTime();
    QString getConnectionTransferred();
    void setInitialFirewallState();
    void handleDisconnectWithError(const ProtoTypes::ConnectState &connectState);
    void setVariablesToInitState();

    void openStaticIpExternalWindow();
    void addCustomConfigFolder();

    LocalHttpServer *localHttpServer_;
    bool onLocalHttpServerCommand(QString authHash, bool isConnectCmd, bool isDisconnectCmd, QString location);

    bool revealingConnectWindow_;
    bool internetConnected_;

    bool currentlyShowingUserWarningMessage_;

    LocationsTrayMenuWidget *locationsTrayMenuWidget_;
    void activateAndShow();
    void deactivateAndHide();
    void loadTrayMenuItems();

    bool backendAppActiveState_;
    void setBackendAppActiveState(bool state);

    bool activeState_;
    qint64 lastWindowStateChange_;
#ifdef Q_OS_MAC
    void hideShowDockIcon(bool hideFromDock);
#endif

    void minimizeToTray();
};

#endif // MAINWINDOW_H
