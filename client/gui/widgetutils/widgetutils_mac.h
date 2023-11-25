#pragma once

#include <QPixmap>
#include <QWidget>

namespace WidgetUtils_mac {

QPixmap extractProgramIcon(const QString &filePath);
void allowMinimizeForFramelessWindow(QWidget *window);
void allowMoveBetweenSpacesForWindow(QWidget *window, bool allow, bool allowMoveBetweenVirtualDesktops = false);
void setNeedsDisplayForWindow(QWidget *window);

}
