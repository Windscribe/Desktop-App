#ifndef ICONMANAGER_H
#define ICONMANAGER_H

#include <QIcon>

class IconManager
{
    enum {
        ICON_APP_DISCONNECTED,
        ICON_APP_CONNECTING,
        ICON_APP_CONNECTED,
#if defined(Q_OS_MAC)
        ICON_MAC_OSX_DISCONNECTED_DARK,
        ICON_MAC_OSX_DISCONNECTED_LIGHT,
        ICON_MAC_OSX_CONNECTING_DARK,
        ICON_MAC_OSX_CONNECTING_LIGHT,
        ICON_MAC_OSX_CONNECTED_DARK,
        ICON_MAC_OSX_CONNECTED_LIGHT,
#endif  // Q_OS_MAC
        NUM_ICON_TYPES
    };

public:

    static IconManager &instance()
    {
        static IconManager im;
        return im;
    }

    const QIcon *getDisconnectedIcon() const { return &icons_[ICON_APP_DISCONNECTED]; }
    const QIcon *getConnectingIcon() const { return &icons_[ICON_APP_CONNECTING]; }
    const QIcon *getConnectedIcon() const { return &icons_[ICON_APP_CONNECTED]; }

#if defined(Q_OS_MAC)
    const QIcon *getDisconnectedTrayIconForMac(bool isDarkMode) const;
    const QIcon *getConnectingTrayIconForMac(bool isDarkMode) const;
    const QIcon *getConnectedTrayIconForMac(bool isDarkMode) const;
#endif  // Q_OS_MAC

private:
    explicit IconManager();

    QIcon icons_[NUM_ICON_TYPES];
};

#endif // ICONMANAGER_H
