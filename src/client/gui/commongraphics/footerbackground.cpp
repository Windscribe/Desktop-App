#include "footerbackground.h"

#include <QPainterPath>
#include "dpiscalemanager.h"
#include "graphicresources/imageresourcessvg.h"

namespace CommonGraphics {

void drawFooter(QPainter *painter, const QRect &rect)
{
    painter->save();

    // We calculate these colors based on a midnight (0x09, 0x0e, 0x19) background, because the parent windows for the footer generally do not draw anything in this area.
    // background: 5% white on midnight
    QColor backgroundColor(21, 27, 37);
    double initOpacity = painter->opacity();

    painter->setClipRect(rect);
    painter->setPen(Qt::NoPen);
    painter->setBrush(backgroundColor);
    painter->drawRoundedRect(rect, 9*G_SCALE, 9*G_SCALE);
    // roundedRect leaves gap at corners -- cover it
    painter->fillRect(QRect(rect.x(), rect.y(), rect.width(), rect.height() / 2), QBrush(backgroundColor));

    painter->setOpacity(0.15*initOpacity);
    painter->setPen(QColor(255, 255, 255));
    painter->setBrush(Qt::NoBrush);
    painter->drawLine(rect.left() + 2*G_SCALE, rect.top(), rect.right(), rect.top());

    painter->setOpacity(initOpacity);
    QSharedPointer<IndependentPixmap> pixmapBorderBottom = ImageResourcesSvg::instance().getIndependentPixmap("background/MAIN_BORDER_BOTTOM_INNER");
    pixmapBorderBottom->draw(rect.x(), rect.bottom() - 31*G_SCALE, 350*G_SCALE, 32*G_SCALE, painter);

    painter->restore();
}

} // namespace CommonGraphics
