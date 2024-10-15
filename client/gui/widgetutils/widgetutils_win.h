#pragma once

#include <QIcon>
#include <QPixmap>
#include <QRect>
#include <QScreen>
#include <QWidget>

namespace WidgetUtils_win {

QPixmap extractProgramIcon(QString filePath);
QPixmap extractWindowsAppProgramIcon(QString filePath);
void updateSystemTrayIcon(const QPixmap &pixmap, QString tooltip);
void fixSystemTrayIconDblClick();

void setTaskbarIconOverlay(const QWidget &appMainWindow, const QIcon *icon);

QRect availableGeometry(const QWidget &appMainWindow, const QScreen &screen);

}
