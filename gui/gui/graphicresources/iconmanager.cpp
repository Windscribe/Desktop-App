#include "iconmanager.h"
#include <QApplication>
#include <QDesktopWidget>
#include "dpiscalemanager.h"

IconManager::IconManager()
{
#ifdef Q_OS_WIN
    disconnectedIcon_ = QIcon(":/Resources/icons/icon_disconnected.ico");
    connectingIcon_ = QIcon(":/Resources/icons/icon_connecting.ico");
    connectedIcon_ = QIcon(":/Resources/icons/icon_connected.ico");
#elif defined Q_OS_MAC
    int pixelRatio = DpiScaleManager::instance().curDevicePixelRatio();
    if (pixelRatio >= 2)
    {
        QPixmap p(":/Resources/icons/icon_disconnected.ico");
        p.setDevicePixelRatio(2);
        disconnectedIcon_ = QIcon(p);
        //disconnectedIcon_.setIsMask(true);

        QPixmap p2(":/Resources/icons/icon_connecting.ico");
        p2.setDevicePixelRatio(2);
        connectingIcon_ = QIcon(p2);
        //connectingIcon_.setIsMask(true);

        QPixmap p3(":/Resources/icons/icon_connected.ico");
        p3.setDevicePixelRatio(2);
        connectedIcon_ = QIcon(p3);
        //connectedIcon_.setIsMask(true);
    }
    else
    {
        QPixmap p(":/Resources/icons/icon_disconnected.ico");
        disconnectedIcon_ = QIcon(p);
        //disconnectedIcon_.setIsMask(true);

        QPixmap p2(":/Resources/icons/icon_connecting.ico");
        connectingIcon_ = QIcon(p2);
        //connectingIcon_.setIsMask(true);

        QPixmap p3(":/Resources/icons/icon_connected.ico");
        connectedIcon_ = QIcon(p3);
        //connectedIcon_.setIsMask(true);
    }
#endif
}
