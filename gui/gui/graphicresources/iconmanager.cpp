#include "iconmanager.h"
#include <QApplication>
#include <QDesktopWidget>
#include "dpiscalemanager.h"

#ifdef Q_OS_MAC
#include "utils/macutils.h"
#endif

IconManager::IconManager()
{
    const char *kIconPaths[NUM_ICON_TYPES] = {
        ":/Resources/icons/icon_disconnected.ico",  // ICON_APP_DISCONNECTED
        ":/Resources/icons/icon_connecting.ico",    // ICON_APP_CONNECTING
        ":/Resources/icons/icon_connected.ico",     // ICON_APP_CONNECTED
#if defined(Q_OS_MAC)
        // Provided we are using Qt 5.12.11 or greater there is no need to provide different icns for BigSur
        ":/Resources/icons/menubar/os10/off_dark.icns",    // ICON_MAC_OSX_DISCONNECTED_DARK
        ":/Resources/icons/menubar/os10/off_light.icns",   // ICON_MAC_OSX_DISCONNECTED_LIGHT
        ":/Resources/icons/menubar/os10/conn_dark.icns",   // ICON_MAC_OSX_CONNECTING_DARK
        ":/Resources/icons/menubar/os10/conn_light.icns",  // ICON_MAC_OSX_CONNECTING_LIGHT
        ":/Resources/icons/menubar/os10/on_dark.icns",     // ICON_MAC_OSX_CONNECTED_DARK
        ":/Resources/icons/menubar/os10/on_light.icns",    // ICON_MAC_OSX_CONNECTED_LIGHT
#endif  // Q_OS_MAC
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
#else
    for (int i = 0; i < NUM_ICON_TYPES; ++i) {
        icons_[i] = QIcon(kIconPaths[i]);
    }
#endif
}

#if defined(Q_OS_MAC)

const QIcon *IconManager::getDisconnectedTrayIconForMac(bool isDarkMode) const
{
    return &icons_[isDarkMode ? ICON_MAC_OSX_DISCONNECTED_DARK : ICON_MAC_OSX_DISCONNECTED_LIGHT];
}

const QIcon *IconManager::getConnectingTrayIconForMac(bool isDarkMode) const
{
    return &icons_[isDarkMode ? ICON_MAC_OSX_CONNECTING_DARK : ICON_MAC_OSX_CONNECTING_LIGHT];
}

const QIcon *IconManager::getConnectedTrayIconForMac(bool isDarkMode) const
{
    return &icons_[isDarkMode ? ICON_MAC_OSX_CONNECTED_DARK : ICON_MAC_OSX_CONNECTED_LIGHT];
}

#endif
