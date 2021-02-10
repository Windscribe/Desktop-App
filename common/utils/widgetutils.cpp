#include "widgetutils.h"

#include <QGuiApplication>
#include <QDebug>
#include "utils/logger.h"
#include "dpiscalemanager.h"

#ifdef Q_OS_WIN
    #include "widgetutils_win.h"
#else
    #include "widgetutils_mac.h"
#endif

#ifdef Q_OS_WIN
Q_GUI_EXPORT QPixmap qt_pixmapFromWinHICON(HICON icon);
#endif

QPixmap *WidgetUtils::extractProgramIcon(QString filePath)
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
#else 
    return WidgetUtils_mac::extractProgramIcon(filePath);
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
            qDebug() << "Backup screen: " << screen << " " << screen->geometry();
            return screen;
        }
        qCDebug(LOG_BASIC) << "No screens available -- this should never happen";
    }
    return screen;
}

QScreen *WidgetUtils::screenByName(const QString &name)
{
    Q_FOREACH (QScreen *screen, QGuiApplication::screens())
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
    Q_FOREACH (QScreen *screen, QGuiApplication::screens())
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

