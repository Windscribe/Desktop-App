#include "trayicon.h"

#include <QDesktopServices>
#include <QElapsedTimer>
#include <QThread>
#include <QTimer>

#include "graphicresources/iconmanager.h"
#include "languagecontroller.h"
#include "locations/model/locationsmodel.h"
#include "mainwindow.h"
#include "themecontroller.h"
#include "utils/hardcodedsettings.h"
#include "widgetutils/widgetutils.h"

#ifdef Q_OS_WIN
#include "widgetutils/widgetutils_win.h"
#endif

#ifdef Q_OS_LINUX
#include <QDBusInterface>
#include <QDBusReply>
#endif

TrayIcon::TrayIcon(QObject *parent, Backend *backend, Preferences *preferences)
    : QObject(parent), backend_(backend), preferences_(preferences), trayIconColorWhite_(false), isLoggedIn_(false),
      lastScreenName_(""), trayIconPositionAvailable_(false), currentIconType_(AppIconType::DISCONNECTED)
{
    // Initialize "fallback" tray icon geometry.
    if (qApp->screens().size() == 0 || qApp->screens().at(0) == NULL) {
        qCCritical(LOG_BASIC) << "No screen for fallback tray icon init";
    } else {
        const QScreen *screen = qApp->screens().at(0);
        const QRect desktopAvailableRc = screen->availableGeometry();
        savedTrayIconRect_.setTopLeft(QPoint(desktopAvailableRc.right() - 800, 0));  // Rough estimate
        savedTrayIconRect_.setSize(QSize(22, 22));
    }

    updateTrayIconColor();

#ifdef Q_OS_MACOS
    if (!preferences_->isDockedToTray()) {
        trayIcon_.setContextMenu(&trayMenu_);
    }
#else
    trayIcon_.setContextMenu(&trayMenu_);
#endif
    connect(&trayMenu_, &QMenu::aboutToShow, this, &TrayIcon::onMenuAboutToShow);
    connect(&trayMenu_, &QMenu::aboutToHide, this, &TrayIcon::onMenuAboutToHide);
    connect(&trayIcon_, &QSystemTrayIcon::activated, this, &TrayIcon::onTrayActivated);

#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
    connect(preferences_, &Preferences::trayIconColorChanged, this, &TrayIcon::updateTrayIconColor);
#endif
    connect(&ThemeController::instance(), &ThemeController::osThemeChanged, this, &TrayIcon::updateTrayIconColor);
#ifdef Q_OS_MACOS
    connect(preferences_, &Preferences::isDockedToTrayChanged, this, &TrayIcon::onDockedModeChanged);
#endif

    trayIcon_.show();
    updateIconType(currentIconType_);

#ifdef Q_OS_MACOS
    if (preferences_->isDockedToTray()) {
        initialTrayIconRect_ = trayIcon_.geometry();
        qCInfo(LOG_BASIC) << "TrayIcon::TrayIcon - starting monitor timer";
        elapsedTimer_.start();
        monitorTimer_.setInterval(100);
        connect(&monitorTimer_, &QTimer::timeout, this, &TrayIcon::checkTrayIconPosition);
        monitorTimer_.start();
    } else {
        trayIconPositionAvailable_ = true;
    }
#endif
}

TrayIcon::~TrayIcon()
{
}

void TrayIcon::show()
{
    trayIcon_.show();
}

void TrayIcon::hide()
{
    trayIcon_.hide();
}

void TrayIcon::updateTooltip(const QString &tooltip)
{
#if defined(Q_OS_WIN)
    WidgetUtils_win::updateSystemTrayIcon(QPixmap(), tooltip);
#else
    trayIcon_.setToolTip(tooltip);
#endif
}

void TrayIcon::updateIconType(AppIconType type)
{
    currentIconType_ = type;
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

void TrayIcon::updateIconColor(bool isWhite)
{
    trayIconColorWhite_ = isWhite;
}

void TrayIcon::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    emit activated(reason);
}

void TrayIcon::onMenuAboutToShow()
{
    clearMenu();
    createMenuItems();
}

void TrayIcon::onMenuAboutToHide()
{
#ifdef Q_OS_WIN
    locationsMenu_.clear();
#endif
}

void TrayIcon::onLocationsTrayMenuLocationSelected(const LocationID &lid)
{
    emit locationSelected(lid);
}

void TrayIcon::createMenuItems()
{
    if (isLoggedIn_) {
        if (backend_->currentConnectState() == CONNECT_STATE_DISCONNECTED) {
            trayMenu_.addAction(tr("Connect"), this, &TrayIcon::connectClick);
        } else {
            trayMenu_.addAction(tr("Disconnect"), this, &TrayIcon::disconnectClick);
        }
        trayMenu_.addSeparator();

#ifndef Q_OS_LINUX

#ifdef USE_LOCATIONS_TRAY_MENU_NATIVE
        if (backend_->locationsModelManager()->sortedLocationsProxyModel()->rowCount() > 0) {
            QSharedPointer<LocationsTrayMenuNative> menu(new LocationsTrayMenuNative(nullptr, backend_->locationsModelManager()->sortedLocationsProxyModel()), &QObject::deleteLater);
            menu->setTitle(tr("Locations"));
            trayMenu_.addMenu(menu.get());
            connect(menu.get(), &LocationsTrayMenuNative::locationSelected, this, &TrayIcon::onLocationsTrayMenuLocationSelected);
            locationsMenu_.append(menu);
        }
        if (backend_->locationsModelManager()->favoriteCitiesProxyModel()->rowCount() > 0) {
            QSharedPointer<LocationsTrayMenuNative> menu(new LocationsTrayMenuNative(nullptr, backend_->locationsModelManager()->favoriteCitiesProxyModel()), &QObject::deleteLater);
            menu->setTitle(tr("Favourites"));
            trayMenu_.addMenu(menu.get());
            connect(menu.get(), &LocationsTrayMenuNative::locationSelected, this, &TrayIcon::onLocationsTrayMenuLocationSelected);
            locationsMenu_.append(menu);
        }
        if (backend_->locationsModelManager()->staticIpsProxyModel()->rowCount() > 0) {
            QSharedPointer<LocationsTrayMenuNative> menu(new LocationsTrayMenuNative(nullptr, backend_->locationsModelManager()->staticIpsProxyModel()), &QObject::deleteLater);
            menu->setTitle(tr("Static IPs"));
            trayMenu_.addMenu(menu.get());
            connect(menu.get(), &LocationsTrayMenuNative::locationSelected, this, &TrayIcon::onLocationsTrayMenuLocationSelected);
            locationsMenu_.append(menu);
        }
        if (backend_->locationsModelManager()->customConfigsProxyModel()->rowCount() > 0) {
            QSharedPointer<LocationsTrayMenuNative> menu(new LocationsTrayMenuNative(nullptr, backend_->locationsModelManager()->customConfigsProxyModel()), &QObject::deleteLater);
            menu->setTitle(tr("Custom configs"));
            trayMenu_.addMenu(menu.get());
            connect(menu.get(), &LocationsTrayMenuNative::locationSelected, this, &TrayIcon::onLocationsTrayMenuLocationSelected);
            locationsMenu_.append(menu);
        }
#else
        if (backend_->locationsModelManager()->sortedLocationsProxyModel()->rowCount() > 0) {
            QSharedPointer<LocationsTrayMenu> menu(new LocationsTrayMenu(backend_->locationsModelManager()->sortedLocationsProxyModel(), trayMenu_.font(), trayIcon_.geometry()), &QObject::deleteLater);
            menu->setTitle(tr("Locations"));
            trayMenu_.addMenu(menu.get());
            connect(menu.get(), &LocationsTrayMenu::locationSelected, this, &TrayIcon::onLocationsTrayMenuLocationSelected);
            locationsMenu_.append(menu);
        }
        if (backend_->locationsModelManager()->favoriteCitiesProxyModel()->rowCount() > 0) {
            QSharedPointer<LocationsTrayMenu> menu(new LocationsTrayMenu(backend_->locationsModelManager()->favoriteCitiesProxyModel(), trayMenu_.font(), trayIcon_.geometry()), &QObject::deleteLater);
            menu->setTitle(tr("Favourites"));
            trayMenu_.addMenu(menu.get());
            connect(menu.get(), &LocationsTrayMenu::locationSelected, this, &TrayIcon::onLocationsTrayMenuLocationSelected);
            locationsMenu_.append(menu);
        }
        if (backend_->locationsModelManager()->staticIpsProxyModel()->rowCount() > 0) {
            QSharedPointer<LocationsTrayMenu> menu(new LocationsTrayMenu(backend_->locationsModelManager()->staticIpsProxyModel(), trayMenu_.font(), trayIcon_.geometry()), &QObject::deleteLater);
            menu->setTitle(tr("Static IPs"));
            trayMenu_.addMenu(menu.get());
            connect(menu.get(), &LocationsTrayMenu::locationSelected, this, &TrayIcon::onLocationsTrayMenuLocationSelected);
            locationsMenu_.append(menu);
        }
        if (backend_->locationsModelManager()->customConfigsProxyModel()->rowCount() > 0) {
            QSharedPointer<LocationsTrayMenu> menu(new LocationsTrayMenu(backend_->locationsModelManager()->customConfigsProxyModel(), trayMenu_.font(), trayIcon_.geometry()), &QObject::deleteLater);
            menu->setTitle(tr("Custom configs"));
            trayMenu_.addMenu(menu.get());
            connect(menu.get(), &LocationsTrayMenu::locationSelected, this, &TrayIcon::onLocationsTrayMenuLocationSelected);
            locationsMenu_.append(menu);
        }
#endif
        trayMenu_.addSeparator();
#endif
    }

#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
    trayMenu_.addAction(tr("Show/Hide"), this, &TrayIcon::showHideClick);
#endif
    trayMenu_.addAction(tr("Preferences"), this, &TrayIcon::preferencesClick);
    trayMenu_.addSeparator();
    trayMenu_.addAction(tr("Help"), this, &TrayIcon::helpMeClick);
    trayMenu_.addAction(tr("Quit Windscribe"), this, &TrayIcon::quitClick);
}

void TrayIcon::clearMenu()
{
    trayMenu_.clear();
#ifndef Q_OS_LINUX
    locationsMenu_.clear();
#endif
}

void TrayIcon::updateTrayIconColor()
{
#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
    const auto trayIconColor = preferences_->trayIconColor();
    if (trayIconColor == TRAY_ICON_COLOR::TRAY_ICON_COLOR_WHITE) {
        trayIconColorWhite_ = true;
    } else if (trayIconColor == TRAY_ICON_COLOR::TRAY_ICON_COLOR_BLACK) {
        trayIconColorWhite_ = false;
    } else {
        // TRAY_ICON_COLOR_OS_THEME
        trayIconColorWhite_ = ThemeController::instance().isOsDarkTheme();
    }
#elif defined(Q_OS_MACOS)
    trayIconColorWhite_ = true;
#endif
    updateIconType(currentIconType_);
}

void TrayIcon::showMessage(const QString &title, const QString &message)
{
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        trayIcon_.showMessage(title, message);
        return;
    }

#if defined(Q_OS_LINUX)
    QDBusInterface dbus("org.freedesktop.Notifications", "/org/freedesktop/Notifications",
                        "org.freedesktop.Notifications", QDBusConnection::sessionBus());

    if (!dbus.isValid()) {
        qCWarning(LOG_BASIC) << "TrayIcon::showMessage - could not connect to the notification manager using dbus."
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
                                       title, message, actions, hints, 10000);
    if (!reply.isValid()) {
        qCWarning(LOG_BASIC) << "TrayIcon::showMessage - could not display the message."
                           << (dbus.lastError().isValid() ? dbus.lastError().message() : "");
    }
#else
    qCWarning(LOG_BASIC) << "QSystemTrayIcon reports the system tray is not available";
#endif
}

QRect TrayIcon::trayIconRect()
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

#ifdef Q_OS_MACOS
const QRect TrayIcon::bestGuessForTrayIconRectFromLastScreen(const QPoint &pt)
{
    QRect lastScreenTrayRect = trayIconRectForLastScreen();
    if (lastScreenTrayRect.isValid()) {
        return lastScreenTrayRect;
    }
    return trayIconRectForScreenContainingPt(pt);
}

const QRect TrayIcon::trayIconRectForLastScreen()
{
    if (lastScreenName_ != "") {
        QRect rect = generateTrayIconRectFromHistory(lastScreenName_);
        if (rect.isValid()) {
            return rect;
        }
    }
    return QRect(0,0,0,0); // invalid
}

const QRect TrayIcon::trayIconRectForScreenContainingPt(const QPoint &pt)
{
    QScreen *screen = WidgetUtils::slightlySaferScreenAt(pt);
    if (!screen) {
        return QRect(0,0,0,0);
    }
    return guessTrayIconLocationOnScreen(screen);
}

const QRect TrayIcon::generateTrayIconRectFromHistory(const QString &screenName)
{
    if (systemTrayIconRelativeGeoScreenHistory_.contains(screenName)) {
        // ensure is in current list
        QScreen *screen = WidgetUtils::screenByName(screenName);

        if (screen) {
            const QRect screenGeo = screen->geometry();

            TrayIconRelativeGeometry &rect = systemTrayIconRelativeGeoScreenHistory_[lastScreenName_];
            QRect newIconRect = QRect(screenGeo.x() + rect.x(),
                                      screenGeo.y() + rect.y(),
                                      rect.width(), rect.height());
            return newIconRect;
        }
        return QRect(0,0,0,0);
    }
    return QRect(0,0,0,0);
}

QRect TrayIcon::guessTrayIconLocationOnScreen(QScreen *screen)
{
    const QRect screenGeo = screen->geometry();
    // Use a default width estimate since we don't have WINDOW_WIDTH here
    const int defaultIconWidth = savedTrayIconRect_.isValid() ? savedTrayIconRect_.width() : 22;
    const int defaultIconHeight = savedTrayIconRect_.isValid() ? savedTrayIconRect_.height() : 22;

    QRect newIconRect = QRect(screenGeo.right() - 200,  // Rough estimate
                              screenGeo.top(),
                              defaultIconWidth,
                              defaultIconHeight);
    return newIconRect;
}
#endif

void TrayIcon::setLoggedIn(bool loggedIn)
{
    if (isLoggedIn_ != loggedIn) {
        isLoggedIn_ = loggedIn;
    }
}

bool TrayIcon::isTrayIconPositionAvailable() const
{
    return trayIconPositionAvailable_;
}

void TrayIcon::checkTrayIconPosition()
{
    QRect currentRect = trayIcon_.geometry();

    if (!currentRect.isEmpty() && currentRect != initialTrayIconRect_) {
        trayIconPositionAvailable_ = true;
        monitorTimer_.stop();
        qCInfo(LOG_BASIC) << "TrayIcon::checkTrayIconPosition - tray icon position available after" << elapsedTimer_.elapsed() << "ms";
        emit trayIconPositionAvailable();
        return;
    }

    if (elapsedTimer_.elapsed() >= 2000) {
        trayIconPositionAvailable_ = true;
        monitorTimer_.stop();
        qCInfo(LOG_BASIC) << "TrayIcon::checkTrayIconPosition - using fallback after timeout";
        emit trayIconPositionAvailable();
    }
}

void TrayIcon::onDockedModeChanged(bool isDocked)
{
#ifdef Q_OS_MACOS
    if (isDocked) {
        trayIcon_.setContextMenu(nullptr);
    } else {
        trayIcon_.setContextMenu(&trayMenu_);
    }
#else
    Q_UNUSED(isDocked);
#endif
}
