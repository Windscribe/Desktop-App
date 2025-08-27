#include "utils.h"

#include <QPainter>
#include <QPainterPath>

#include "themecontroller.h"

QPixmap getBorderedPixmap(const QString &imgPath, int width, int height, int radius, bool roundTop, bool roundBottom)
{
    QPixmap pixmap(imgPath);

    QImage image = QImage(width, height, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    QPen pen = painter.pen();
    pen.setWidth(2);

    QPainterPathStroker stroker(pen);
    QPainterPath path;
    
    if (roundTop && roundBottom) {
        path.addRoundedRect(2, 2, width-4, height-4, radius, radius);
    } else if (roundTop && !roundBottom) {
        path.addRoundedRect(2, 2, width-4, height+radius, radius, radius);
    } else if (!roundTop && roundBottom) {
        path.addRoundedRect(2, -radius-2, width-4, height+radius, radius, radius);
    } else {
        path.addRect(2, -radius-2, width-4, height+2*radius+4);
    }
    
    painter.fillPath(path, ThemeController::instance().windowBackgroundColor());

    if (!imgPath.isEmpty()) {
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.drawPixmap(1, 1, pixmap);
    }

    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.setPen(Qt::NoPen);
    painter.fillPath(stroker.createStroke(path), QColor(71, 71, 71));
    painter.setPen(Qt::SolidLine);

    return QPixmap::fromImage(image);
}