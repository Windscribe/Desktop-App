#pragma once

#include <QPixmap>
#include <QWidget>

#include "types/enums.h"

namespace WidgetUtils_mac {

QPixmap extractProgramIcon(const QString &filePath);
void allowMinimizeForFramelessWindow(QWidget *window);
void allowMoveBetweenSpacesForWindow(QWidget *window, bool docked, bool moveWindow);
void setNeedsDisplayForWindow(QWidget *window);

}
