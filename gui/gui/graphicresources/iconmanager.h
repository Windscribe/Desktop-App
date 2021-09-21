#ifndef ICONMANAGER_H
#define ICONMANAGER_H

#include <QIcon>

class IconManager
{
    enum {
        // dock/task bar icons
        ICON_APP_DISCONNECTED,
        ICON_APP_CONNECTING,
        ICON_APP_CONNECTED,
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
        // needed for tray icons
        ICON_TRAY_DISCONNECTED_DARK,
        ICON_TRAY_DISCONNECTED_LIGHT,
        ICON_TRAY_CONNECTING_DARK,
        ICON_TRAY_CONNECTING_LIGHT,
        ICON_TRAY_CONNECTED_DARK,
        ICON_TRAY_CONNECTED_LIGHT,
#endif
        NUM_ICON_TYPES
    };

public:

    static IconManager &instance()
    {
        static IconManager im;
        return im;
    }

    // dock/taskbar
    const QIcon *getDisconnectedIcon() const { return &icons_[ICON_APP_DISCONNECTED]; }
    const QIcon *getConnectingIcon() const { return &icons_[ICON_APP_CONNECTING]; }
    const QIcon *getConnectedIcon() const { return &icons_[ICON_APP_CONNECTED]; }

    // tray
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    const QIcon *getDisconnectedTrayIcon(bool isDarkMode) const;
    const QIcon *getConnectingTrayIcon(bool isDarkMode) const;
    const QIcon *getConnectedTrayIcon(bool isDarkMode) const;
#else
    const QIcon *getDisconnectedTrayIcon(bool isDarkMode) const {
        Q_UNUSED(isDarkMode)
        return getDisconnectedIcon();
    }
    const QIcon *getConnectingTrayIcon(bool isDarkMode) const {
        Q_UNUSED(isDarkMode)
        return getConnectingIcon();
    }
    const QIcon *getConnectedTrayIcon(bool isDarkMode) const {
        Q_UNUSED(isDarkMode)
        return getConnectedIcon();
    }
#endif

private:
    explicit IconManager();

    QIcon icons_[NUM_ICON_TYPES];
};

#endif // ICONMANAGER_H
