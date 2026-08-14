#pragma once

#include <QPixmap>
#include <QScreen>

namespace WidgetUtils {

QScreen *slightlySaferScreenAt(QPoint pt);

QScreen *screenByName(const QString &name);
QScreen *screenContainingPt(const QPoint &pt);

}
