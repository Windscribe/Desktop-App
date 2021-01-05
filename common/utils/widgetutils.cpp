#include "widgetutils.h"

#include <QGuiApplication>
#include <QDebug>

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
        qDebug() << "Screen not found -- grabbing first screen available:";
        QList<QScreen*> screens = QGuiApplication::screens();
        for (int i = 0; i < screens.length(); i++)
        {
            qDebug() << "Screen: " << screens[i]->name() << " " << screens[i]->geometry();
        }

        if (!QGuiApplication::screens().empty())
        {
            screen = QGuiApplication::screens().at(0);
            qDebug() << "Backup screen: " << screen << " " << screen->geometry();
        }
    }
    return screen;
}
