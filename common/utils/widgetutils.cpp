#include "widgetutils.h"

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
