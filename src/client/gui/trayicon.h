#pragma once

#include <QGuiApplication>
#include <QMap>
#include <QMenu>
#include <QScreen>
#include <QSystemTrayIcon>
#include <QTimer>
#include "backend/backend.h"
#include "backend/preferences/preferences.h"
#include "types/connectstate.h"
#include "types/enums.h"

#if defined(Q_OS_MACOS)
#define USE_LOCATIONS_TRAY_MENU_NATIVE
#endif

#ifdef USE_LOCATIONS_TRAY_MENU_NATIVE
#include "systemtray/locationstraymenunative.h"
#else
#include "systemtray/locationstraymenu.h"
#endif

enum class AppIconType; // forward declaration of type in MainWindow

class TrayIcon : public QObject
{
    Q_OBJECT

public:
    explicit TrayIcon(QObject *parent, Backend *backend, Preferences *preferences);
    ~TrayIcon();

    void show();
    void hide();
    void updateTooltip(const QString &tooltip);
    void updateIconType(AppIconType type);
    void updateIconColor(bool isWhite);
    void showMessage(const QString &title, const QString &message);
    QRect trayIconRect();
    void setLoggedIn(bool loggedIn);
    bool isTrayIconPositionAvailable() const;

signals:
    void activated(QSystemTrayIcon::ActivationReason reason);
    void connectClick();
    void disconnectClick();
    void preferencesClick();
    void showHideClick();
    void helpMeClick();
    void quitClick();
    void locationSelected(const LocationID &lid);
    void trayIconPositionAvailable();

private slots:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onMenuAboutToShow();
    void onMenuAboutToHide();
    void onLocationsTrayMenuLocationSelected(const LocationID &lid);
    void checkTrayIconPosition();
    void onDockedModeChanged(bool isDocked);

private:
    void createMenuItems();
    void clearMenu();
    void updateTrayIconColor();

#ifdef Q_OS_MACOS
    const QRect bestGuessForTrayIconRectFromLastScreen(const QPoint &pt);
    const QRect trayIconRectForLastScreen();
    const QRect trayIconRectForScreenContainingPt(const QPoint &pt);
    const QRect generateTrayIconRectFromHistory(const QString &screenName);
    QRect guessTrayIconLocationOnScreen(QScreen *screen);
#endif

    Backend *backend_;
    Preferences *preferences_;
    QSystemTrayIcon trayIcon_;
    QMenu trayMenu_;
    bool trayIconColorWhite_;
    bool isLoggedIn_;
    AppIconType currentIconType_;

#ifdef USE_LOCATIONS_TRAY_MENU_NATIVE
    QVector<QSharedPointer<LocationsTrayMenuNative>> locationsMenu_;
#else
    QVector<QSharedPointer<LocationsTrayMenu>> locationsMenu_;
#endif

    QRect savedTrayIconRect_;
#ifdef Q_OS_MACOS
    typedef QRect TrayIconRelativeGeometry;
    QMap<QString, TrayIconRelativeGeometry> systemTrayIconRelativeGeoScreenHistory_;
#endif
    QString lastScreenName_;
    QTimer monitorTimer_;
    bool trayIconPositionAvailable_;
    QRect initialTrayIconRect_;
    QElapsedTimer elapsedTimer_;
};
