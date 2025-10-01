#include "iconmanager.h"

#if defined(Q_OS_MACOS)
#include "dpiscalemanager.h"
#endif

#include "utils/log/categories.h"

#ifdef Q_OS_MACOS
#include "utils/macutils.h"
#endif

IconManager::IconManager()
{
#if defined(Q_OS_WIN)
    // Windscribe app reflects its state eigther in dock and system tray.
    // It uses mechanism of overlayed icons on Windows.
    // So it is necessary to add overlayed icons.
    initWinLinuxIcons(
        {
            {IconType::ICON_APP_MAIN,                ":/resources/icons/win/windscribe.ico"},
            {IconType::ICON_TRAY_DISCONNECTED_DARK,  ":/resources/icons/win/disconnected_dark.ico"},
            {IconType::ICON_TRAY_DISCONNECTED_LIGHT, ":/resources/icons/win/disconnected_light.ico"},
            {IconType::ICON_TRAY_CONNECTED_DARK,     ":/resources/icons/win/connected_dark.ico"},
            {IconType::ICON_TRAY_CONNECTED_LIGHT,    ":/resources/icons/win/connected_light.ico"},
            {IconType::ICON_TRAY_CONNECTING_DARK,    ":/resources/icons/win/connecting_dark.ico"},
            {IconType::ICON_TRAY_CONNECTING_LIGHT,   ":/resources/icons/win/connecting_light.ico"},
            {IconType::ICON_TRAY_ERROR_DARK,         ":/resources/icons/win/error_dark.ico"},
            {IconType::ICON_TRAY_ERROR_LIGHT,        ":/resources/icons/win/error_light.ico"},
            {IconType::ICON_OVERLAY_CONNECTING,      ":/resources/icons/win/overlay_connecting.ico"},
            {IconType::ICON_OVERLAY_CONNECTED,       ":/resources/icons/win/overlay_connected.ico"},
            {IconType::ICON_OVERLAY_ERROR,           ":/resources/icons/win/overlay_error.ico"},
        }
    );
#elif defined(Q_OS_MACOS)
    // Windscribe app reflects state either in dock and system tray.
    // Each state is represented by a separate icon.
    initMacIcons(
        {
            {IconType::ICON_APP_MAIN,                ":/resources/icons/mac/windscribe.icns"},
            {IconType::ICON_APP_CONNECTING,          ":/resources/icons/mac/connecting.icns"},
            {IconType::ICON_APP_CONNECTED,           ":/resources/icons/mac/connected.icns"},
            {IconType::ICON_APP_DISCONNECTED,        ":/resources/icons/mac/disconnected.icns"},
            {IconType::ICON_APP_ERROR,               ":/resources/icons/mac/error.icns"},
            {IconType::ICON_TRAY_DISCONNECTED_DARK,  ":/resources/icons/mac/disconnected_dark.icns"},
            {IconType::ICON_TRAY_DISCONNECTED_LIGHT, ":/resources/icons/mac/disconnected_light.icns"},
            {IconType::ICON_TRAY_CONNECTED_DARK,     ":/resources/icons/mac/connected_dark.icns"},
            {IconType::ICON_TRAY_CONNECTED_LIGHT,    ":/resources/icons/mac/connected_light.icns"},
            {IconType::ICON_TRAY_CONNECTING_DARK,    ":/resources/icons/mac/connecting_dark.icns"},
            {IconType::ICON_TRAY_CONNECTING_LIGHT,   ":/resources/icons/mac/connecting_light.icns"},
            {IconType::ICON_TRAY_ERROR_DARK,         ":/resources/icons/mac/error_dark.icns"},
            {IconType::ICON_TRAY_ERROR_LIGHT,        ":/resources/icons/mac/error_light.icns"},
        }
    );
#elif defined(Q_OS_LINUX)
    // Widscribe app doesn't reflect state in a dock.
    // Windscribe app reflects state in a tray.
    // System icons that are actually used on Linux are saved in src/installer/linux/common/usr/share/icons/hicolor
    //                                                       and src/installer/linux/png_icons
    //                                                       folders.
    // They are copied during installation.
    //
    initWinLinuxIcons(
        {
            {IconType::ICON_APP_MAIN,                ":/resources/icons/linux/windscribe.ico"},
            {IconType::ICON_TRAY_DISCONNECTED_DARK,  ":/resources/icons/linux/disconnected_dark.ico"},
            {IconType::ICON_TRAY_DISCONNECTED_LIGHT, ":/resources/icons/linux/disconnected_light.ico"},
            {IconType::ICON_TRAY_CONNECTED_DARK,     ":/resources/icons/linux/connected_dark.ico"},
            {IconType::ICON_TRAY_CONNECTED_LIGHT,    ":/resources/icons/linux/connected_light.ico"},
            {IconType::ICON_TRAY_CONNECTING_DARK,    ":/resources/icons/linux/connecting_dark.ico"},
            {IconType::ICON_TRAY_CONNECTING_LIGHT,   ":/resources/icons/linux/connecting_light.ico"},
            {IconType::ICON_TRAY_ERROR_DARK,         ":/resources/icons/linux/error_dark.ico"},
            {IconType::ICON_TRAY_ERROR_LIGHT,        ":/resources/icons/linux/error_light.ico"},
        }
    );
#endif
}

#if defined(Q_OS_WIN) | defined(Q_OS_LINUX)
void IconManager::initWinLinuxIcons(const std::map<IconType, QString>& iconPaths)
{
    for (const auto& iconPath : iconPaths) {
        QIcon ico(iconPath.second);
        if (ico.isNull()) {
            qCWarning(LOG_BASIC) << "IconManager failed to load: " << iconPath.second;
        }
        icons_[iconPath.first] = ico;
    }
}
#elif defined(Q_OS_MACOS)
void IconManager::initMacIcons(const std::map<IconType, QString>& iconPaths) {
    const bool isDoublePixelRatio = DpiScaleManager::instance().curDevicePixelRatio() >= 2;
    for (const auto& iconPath : iconPaths) {
        QPixmap p;
        if (!p.load(iconPath.second)) {
            qCWarning(LOG_BASIC) << "IconManager failed to load: " << iconPath.second;
        }
        if (isDoublePixelRatio)
            p.setDevicePixelRatio(2);
        QIcon icon = QIcon(p);
        icon.setIsMask(true); //  same as NSImage::setTemplate(true) - this provides light, dark and tinting switching
        icons_[iconPath.first] = icon;
    }
}
#endif

#if defined(Q_OS_LINUX) | defined(Q_OS_MACOS)
const QIcon *IconManager::getDisconnectedIcon() const
{
    return getIconByType(IconType::ICON_APP_DISCONNECTED);
}

const QIcon *IconManager::getConnectingIcon() const
{
    return getIconByType(IconType::ICON_APP_CONNECTING);
}

const QIcon *IconManager::getConnectedIcon() const
{
    return getIconByType(IconType::ICON_APP_CONNECTED);
}

const QIcon *IconManager::getErrorIcon() const
{
    return getIconByType(IconType::ICON_APP_ERROR);
}
#endif

const QIcon *IconManager::getIconByType(const IconType& iconType) const
{
    if (icons_.find(iconType) != icons_.cend())
        return &icons_.at(iconType);
    else
        return &icons_.at(IconType::ICON_APP_MAIN);
}

const QIcon *IconManager::getDisconnectedTrayIcon(bool isDarkMode) const
{
    return isDarkMode
        ? getIconByType(IconType::ICON_TRAY_DISCONNECTED_DARK)
        : getIconByType(IconType::ICON_TRAY_DISCONNECTED_LIGHT);
}

const QIcon *IconManager::getConnectingTrayIcon(bool isDarkMode) const
{
    return isDarkMode
        ? getIconByType(IconType::ICON_TRAY_CONNECTING_DARK)
        : getIconByType(IconType::ICON_TRAY_CONNECTING_LIGHT);
}

const QIcon *IconManager::getConnectedTrayIcon(bool isDarkMode) const
{
    return isDarkMode
        ? getIconByType(IconType::ICON_TRAY_CONNECTED_DARK)
        : getIconByType(IconType::ICON_TRAY_CONNECTED_LIGHT);
}

const QIcon *IconManager::getErrorTrayIcon(bool isDarkMode) const
{
    return isDarkMode
        ? getIconByType(IconType::ICON_TRAY_ERROR_DARK)
        : getIconByType(IconType::ICON_TRAY_ERROR_LIGHT);
}

#if defined(Q_OS_WIN)
const QIcon *IconManager::getConnectingOverlayIcon() const
{
    return getIconByType(IconType::ICON_OVERLAY_CONNECTING);
}

const QIcon *IconManager::getConnectedOverlayIcon() const
{
    return getIconByType(IconType::ICON_OVERLAY_CONNECTED);
}

const QIcon *IconManager::getErrorOverlayIcon() const
{
    return getIconByType(IconType::ICON_OVERLAY_ERROR);
}
#endif
