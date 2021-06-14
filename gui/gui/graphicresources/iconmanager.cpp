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
        ":/resources/icons/icon_disconnected.ico",  // ICON_APP_DISCONNECTED
        ":/resources/icons/icon_connecting.ico",    // ICON_APP_CONNECTING
        ":/resources/icons/icon_connected.ico",     // ICON_APP_CONNECTED
#if defined(Q_OS_MAC)
        ":/Resources/icons/menubar/os10/off_dark.icns",    // ICON_MAC_OSX_DISCONNECTED_DARK
        ":/Resources/icons/menubar/os10/off_light.icns",   // ICON_MAC_OSX_DISCONNECTED_LIGHT
        ":/Resources/icons/menubar/os10/conn_dark.icns",   // ICON_MAC_OSX_CONNECTING_DARK
        ":/Resources/icons/menubar/os10/conn_light.icns",  // ICON_MAC_OSX_CONNECTING_LIGHT
        ":/Resources/icons/menubar/os10/on_dark.icns",     // ICON_MAC_OSX_CONNECTED_DARK
        ":/Resources/icons/menubar/os10/on_light.icns",    // ICON_MAC_OSX_CONNECTED_LIGHT
        ":/Resources/icons/menubar/os11/off.icns",         // ICON_MAC_OS11_DISCONNECTED
        ":/Resources/icons/menubar/os11/conn.icns",        // ICON_MAC_OS11_CONNECTING
        ":/Resources/icons/menubar/os11/on.icns",          // ICON_MAC_OS11_CONNECTED
#endif  // Q_OS_MAC
    };

#if defined(Q_OS_MAC)
    const bool isDoublePixelRatio = DpiScaleManager::instance().curDevicePixelRatio() >= 2;
    for (int i = 0; i < NUM_ICON_TYPES; ++i) {
        QPixmap p(kIconPaths[i]);
        if (isDoublePixelRatio)
            p.setDevicePixelRatio(2);
        icons_[i] = QIcon(p);
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
    if (MacUtils::isOsVersionIsBigSur_or_greater())
        return &icons_[ICON_MAC_OS11_DISCONNECTED];
    return &icons_[isDarkMode ? ICON_MAC_OSX_DISCONNECTED_DARK : ICON_MAC_OSX_DISCONNECTED_LIGHT];
}

const QIcon *IconManager::getConnectingTrayIconForMac(bool isDarkMode) const
{
    if (MacUtils::isOsVersionIsBigSur_or_greater())
        return &icons_[ICON_MAC_OS11_CONNECTING];
    return &icons_[isDarkMode ? ICON_MAC_OSX_CONNECTING_DARK : ICON_MAC_OSX_CONNECTING_LIGHT];
}

const QIcon *IconManager::getConnectedTrayIconForMac(bool isDarkMode) const
{
    if (MacUtils::isOsVersionIsBigSur_or_greater())
        return &icons_[ICON_MAC_OS11_CONNECTED];
    return &icons_[isDarkMode ? ICON_MAC_OSX_CONNECTED_DARK : ICON_MAC_OSX_CONNECTED_LIGHT];
}

#endif
