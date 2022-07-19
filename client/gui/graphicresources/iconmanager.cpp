#include "iconmanager.h"
#include <QApplication>
#include "dpiscalemanager.h"

#ifdef Q_OS_MAC
#include "utils/macutils.h"
#endif

IconManager::IconManager()
{
    const char *kIconPaths[NUM_ICON_TYPES] = {
        ":/resources/icons/icon_disconnected.ico",  // ICON_APP_DISCONNECTED
        ":/resources/icons/icon_connecting.ico",    // ICON_APP_CONNECTING
        ":/resources/icons/icon_connected.ico",     // ICON_APP_CONNECTED
#if defined(Q_OS_MAC)
        // Provided we are using Qt 5.12.11 or greater there is no need to provide different icns for BigSur
        ":/Resources/icons/menubar/os10/off_dark.icns",    // ICON_TRAY_DISCONNECTED_DARK
        ":/Resources/icons/menubar/os10/off_light.icns",   // ICON_TRAY_DISCONNECTED_LIGHT
        ":/Resources/icons/menubar/os10/conn_dark.icns",   // ICON_TRAY_CONNECTING_DARK
        ":/Resources/icons/menubar/os10/conn_light.icns",  // ICON_TRAY_CONNECTING_LIGHT
        ":/Resources/icons/menubar/os10/on_dark.icns",     // ICON_TRAY_CONNECTED_DARK
        ":/Resources/icons/menubar/os10/on_light.icns",    // ICON_TRAY_CONNECTED_LIGHT
#else // windows and linux
        ":/resources/icons/win/OFF_WHITE.ico",        // ICON_TRAY_DISCONNECTED_DARK
        ":/resources/icons/win/OFF_BLACK.ico",        // ICON_TRAY_DISCONNECTED_LIGHT
        ":/resources/icons/win/CONNECTING_WHITE.ico", // ICON_TRAY_CONNECTING_DARK
        ":/resources/icons/win/CONNECTING_BLACK.ico", // ICON_TRAY_CONNECTING_LIGHT
        ":/resources/icons/win/ON_WHITE.ico",         // ICON_TRAY_CONNECTED_DARK
        ":/resources/icons/win/ON_BLACK.ico",         // ICON_TRAY_CONNECTED_LIGHT
#endif
    };

#if defined(Q_OS_MAC)
    const bool isDoublePixelRatio = DpiScaleManager::instance().curDevicePixelRatio() >= 2;
    for (int i = 0; i < NUM_ICON_TYPES; ++i) {
        QPixmap p(kIconPaths[i]);
        if (isDoublePixelRatio)
            p.setDevicePixelRatio(2);
        QIcon icon = QIcon(p);
        icon.setIsMask(true); //  same as NSImage::setTemplate(true) - this provides light, dark and tinting switching
        icons_[i] = icon;
    }
#else // windows and linux
    for (int i = 0; i < NUM_ICON_TYPES; ++i) {
        icons_[i] = QIcon(kIconPaths[i]);
    }
#endif
}

const QIcon *IconManager::getDisconnectedTrayIcon(bool isDarkMode) const
{
    return &icons_[isDarkMode ? ICON_TRAY_DISCONNECTED_DARK : ICON_TRAY_DISCONNECTED_LIGHT];
}

const QIcon *IconManager::getConnectingTrayIcon(bool isDarkMode) const
{
    return &icons_[isDarkMode ? ICON_TRAY_CONNECTING_DARK : ICON_TRAY_CONNECTING_LIGHT];
}

const QIcon *IconManager::getConnectedTrayIcon(bool isDarkMode) const
{
    return &icons_[isDarkMode ? ICON_TRAY_CONNECTED_DARK : ICON_TRAY_CONNECTED_LIGHT];
}
