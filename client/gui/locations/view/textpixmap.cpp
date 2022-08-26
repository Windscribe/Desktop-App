#include "textpixmap.h"

namespace gui_locations {

TextPixmap::TextPixmap(const QString &text, const QFont &font, qreal devicePixelRatio)
{
    QFontMetrics fm(font);
    QRect rcText = fm.boundingRect(text);
    QPixmap pixmap(rcText.width()*devicePixelRatio, rcText.height()*devicePixelRatio);
    pixmap.setDevicePixelRatio(devicePixelRatio);
    pixmap.fill(Qt::transparent);
    {
        QPainter painter(&pixmap);
        painter.setPen(Qt::white);
        painter.setFont(font);
        painter.drawText(QRect(0, 0, rcText.width() + 1, rcText.height() + 1), text);
    }
    pixmap_.reset(new IndependentPixmap(pixmap));
}

} // namespace gui_locations
