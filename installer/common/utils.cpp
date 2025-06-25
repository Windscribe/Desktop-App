#include "utils.h"

#include <QPainter>
#include <QPainterPath>

#include "themecontroller.h"

QPixmap getRoundedRectPixmap(const QString &imgPath, int width, int height, int radius)
{
    QPixmap pixmap(imgPath);

    QImage image = QImage(width, height, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    // Create a rounded rect that's 1px smaller than the background
    QPen pen = painter.pen();
    pen.setWidth(2);

    QPainterPathStroker stroker(pen);
    QPainterPath path;
    path.addRoundedRect(2, 2, width-4, height-4, radius, radius);
    painter.fillPath(path, ThemeController::instance().windowBackgroundColor());

    // Draw the background over the rounded rect
    if (!imgPath.isEmpty()) {
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.drawPixmap(1, 1, pixmap);
    }

    // Draw a 1px white border around the image
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.setPen(Qt::NoPen);
    painter.fillPath(stroker.createStroke(path), QColor(255, 255, 255, 38));
    painter.setPen(Qt::SolidLine);

    return QPixmap::fromImage(image);
}