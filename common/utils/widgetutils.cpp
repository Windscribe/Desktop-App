#include "widgetutils.h"

#include <QGuiApplication>
#include <QDebug>
#include "utils/logger.h"

#ifdef Q_OS_WIN
    #include "widgetutils_win.h"
#elif defined Q_OS_MAC
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
#elif defined Q_OS_MAC
    return WidgetUtils_mac::extractProgramIcon(filePath);
#elif defined Q_OS_LINUX
    //todo linux
    Q_ASSERT(false);
    Q_UNUSED(filePath);
    return QPixmap();
#endif 
}

QScreen *WidgetUtils::slightlySaferScreenAt(QPoint pt)
{
    QScreen *screen = QGuiApplication::screenAt(pt); // this can fail when screen coords are weird

    if (!screen)
    {
        // qDebug() << "Screen at point not found -- grabbing first screen available:";

        if (!QGuiApplication::screens().empty())
        {
            screen = QGuiApplication::screens().at(0);
            // qDebug() << "Backup screen: " << screen << " " << screen->geometry();
            return screen;
        }
        // qCDebug(LOG_BASIC) << "No screens available -- this should never happen";
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

