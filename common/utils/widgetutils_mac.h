#ifndef WIDGETUTILS_MAC_H
#define WIDGETUTILS_MAC_H

#include <QPixmap>
#include <QWidget>

namespace WidgetUtils_mac {

QPixmap *extractProgramIcon(const QString &filePath);
void allowMinimizeForFramelessWindow(QWidget *window);
void setNeedsDisplayForWindow(QWidget *window);

}


#endif // WIDGETUTILS_MAC_H
