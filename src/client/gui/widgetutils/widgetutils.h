#pragma once

#include <QPixmap>
#include <QScreen>

namespace WidgetUtils {

QPixmap extractProgramIcon(QString filePath);

QScreen *slightlySaferScreenAt(QPoint pt);

QScreen *screenByName(const QString &name);
QScreen *screenContainingPt(const QPoint &pt);

}
