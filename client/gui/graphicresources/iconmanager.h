#pragma once

#include <QIcon>
#include <map>

class IconManager
{
    enum class IconType {
        ICON_APP_MAIN,
        // dock/task bar icons
        ICON_APP_DISCONNECTED,
        ICON_APP_CONNECTING,
        ICON_APP_CONNECTED,
        ICON_APP_ERROR,
        // needed for tray icons
        ICON_TRAY_DISCONNECTED_DARK,
        ICON_TRAY_DISCONNECTED_LIGHT,
        ICON_TRAY_CONNECTING_DARK,
        ICON_TRAY_CONNECTING_LIGHT,
        ICON_TRAY_CONNECTED_DARK,
        ICON_TRAY_CONNECTED_LIGHT,
        ICON_TRAY_ERROR_DARK,
        ICON_TRAY_ERROR_LIGHT,
#if defined(Q_OS_WIN)
        ICON_OVERLAY_CONNECTING,
        ICON_OVERLAY_CONNECTED,
        ICON_OVERLAY_ERROR,
#endif
        NUM_ICON_TYPES
    };

public:

    static IconManager &instance()
    {
        static IconManager im;
        return im;
    }

    // tray
    const QIcon *getDisconnectedTrayIcon(bool isDarkMode) const;
    const QIcon *getConnectingTrayIcon(bool isDarkMode) const;
    const QIcon *getConnectedTrayIcon(bool isDarkMode) const;
    const QIcon *getErrorTrayIcon(bool isDarkMode) const;

    // dock/taskbar
#if defined(Q_OS_WIN)
    const QIcon *getConnectingOverlayIcon() const;
    const QIcon *getConnectedOverlayIcon() const;
    const QIcon *getErrorOverlayIcon() const;
#else // Mac OS, Linux
    const QIcon *getDisconnectedIcon() const;
    const QIcon *getConnectingIcon() const;
    const QIcon *getConnectedIcon() const;
    const QIcon *getErrorIcon() const;
#endif

private:
    explicit IconManager();
    const QIcon* getIconByType(const IconType& iconType) const;
    std::map<IconType, QIcon> icons_;

#if defined(Q_OS_WIN) | defined(Q_OS_LINUX)
    void initWinLinuxIcons(const std::map<IconType, QString>& iconPaths);
#elif defined(Q_OS_MACOS)
    void initMacIcons(const std::map<IconType, QString>& iconPaths);
#endif
};
