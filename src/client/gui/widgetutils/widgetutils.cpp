#include "widgetutils.h"

#include <QGuiApplication>
#include <QDebug>
#include "utils/ws_assert.h"

#ifdef Q_OS_WIN
    #include "widgetutils_win.h"
#elif defined Q_OS_MACOS
    #include "widgetutils_mac.h"
#elif defined Q_OS_LINUX
#endif

#ifdef Q_OS_WIN
Q_GUI_EXPORT QPixmap qt_pixmapFromWinHICON(HICON icon);
#endif

QPixmap WidgetUtils::extractProgramIcon(QString filePath)
{
#ifdef Q_OS_WIN
    if (filePath.contains("WindowsApps"))
    {
        return WidgetUtils_win::extractWindowsAppProgramIcon(filePath);
    }
    else
    {
        return WidgetUtils_win::extractProgramIcon(filePath);
    }
#elif defined Q_OS_MACOS
    return WidgetUtils_mac::extractProgramIcon(filePath);
#elif defined Q_OS_LINUX
    //todo linux
    WS_ASSERT(false);
    Q_UNUSED(filePath);
    return QPixmap();
#endif
}

QScreen *WidgetUtils::slightlySaferScreenAt(QPoint pt)
{
    QScreen *screen = QGuiApplication::screenAt(pt); // this can fail when screen coords are weird

    if (!screen)
    {
        if (!QGuiApplication::screens().empty())
        {
            screen = QGuiApplication::screens().at(0);
            return screen;
        }
    }
    return screen;
}

QScreen *WidgetUtils::screenByName(const QString &name)
{
    const auto screens = QGuiApplication::screens();
    for (QScreen *screen : screens)
    {
        if (screen->name() == name)
        {
            return screen;
        }
    }
    return nullptr;
}

QScreen *WidgetUtils::screenContainingPt(const QPoint &pt)
{
    const auto screens = QGuiApplication::screens();
    for (QScreen *screen : screens)
    {
        // qDebug() << "Checking screen: " << screen->name() << " " <<  screen->geometry();
        if (screen->geometry().contains(pt))
        {
            // qDebug() << screen->name() << " contians the icon -- NOT STALE";
            return screen;
        }
    }
    return nullptr;
}

