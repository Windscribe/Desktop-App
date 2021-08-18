#ifndef WIDGETUTILS_H
#define WIDGETUTILS_H

#include <QPixmap>
#include <QScreen>

namespace WidgetUtils {

QPixmap *extractProgramIcon(QString filePath);

QScreen *slightlySaferScreenAt(QPoint pt);

QScreen *screenByName(const QString &name);
QScreen *screenContainingPt(const QPoint &pt);

}


#endif // WIDGETUTILS_H
