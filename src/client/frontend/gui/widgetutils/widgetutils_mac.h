#pragma once

#include <QPixmap>
#include <QWidget>

#include "types/enums.h"

namespace WidgetUtils_mac {

void allowMinimizeForFramelessWindow(QWidget *window);
void allowMoveBetweenSpacesForWindow(QWidget *window, bool docked, bool moveWindow);
void setNeedsDisplayForWindow(QWidget *window);

}
