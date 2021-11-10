#ifndef WIDGETUTILS_WIN_H
#define WIDGETUTILS_WIN_H

#include <QPixmap>

namespace WidgetUtils_win {

QPixmap extractProgramIcon(QString filePath);
QPixmap extractWindowsAppProgramIcon(QString filePath);
void updateSystemTrayIcon(const QPixmap &pixmap, QString tooltip);
void fixSystemTrayIconDblClick();

}


#endif // WIDGETUTILS_WIN_H
