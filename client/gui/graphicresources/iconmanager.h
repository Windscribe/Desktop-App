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
        // needed for tray icons
        ICON_TRAY_DISCONNECTED_DARK,
        ICON_TRAY_DISCONNECTED_LIGHT,
        ICON_TRAY_CONNECTING_DARK,
        ICON_TRAY_CONNECTING_LIGHT,
        ICON_TRAY_CONNECTED_DARK,
        ICON_TRAY_CONNECTED_LIGHT,
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
    const QIcon *getDisconnectedTrayIcon(bool isDarkMode) const;
    const QIcon *getConnectingTrayIcon(bool isDarkMode) const;
    const QIcon *getConnectedTrayIcon(bool isDarkMode) const;

private:
    explicit IconManager();

    QIcon icons_[NUM_ICON_TYPES];
};

#endif // ICONMANAGER_H
