#include "textpixmap.h"

namespace gui_locations {

TextPixmap::TextPixmap(const QString &text, const QFont &font, qreal devicePixelRatio) : text_(text)
{
    if (!text.isEmpty())
    {
        QFontMetrics fm(font);
        QRect rcText = fm.boundingRect(text);
        QPixmap pixmap(rcText.width()*devicePixelRatio, rcText.height()*devicePixelRatio);
        pixmap.setDevicePixelRatio(devicePixelRatio);
        pixmap.fill(Qt::transparent);
        {
            QPainter painter(&pixmap);
            painter.setPen(Qt::white);      // for current needs, we use only white color
            painter.setFont(font);
            painter.drawText(QRect(0, 0, rcText.width() + 1, rcText.height() + 1), text);
        }
        pixmap_ = IndependentPixmap(pixmap);
    }
}

void TextPixmaps::add(int id, const QString &text, const QFont &font, qreal devicePixelRatio)
{
    Q_ASSERT(!pixmaps_.contains(id));
    pixmaps_[id] = TextPixmap(text, font, devicePixelRatio);
}

void TextPixmaps::updateIfTextChanged(int id, const QString &text, const QFont &font, qreal devicePixelRatio)
{
    Q_ASSERT(pixmaps_.contains(id));
    auto it = pixmaps_.find(id);
    if (it != pixmaps_.end())
    {
        if (it.value().text() != text)
        {
            pixmaps_[id] = TextPixmap(text, font, devicePixelRatio);
        }
    }
}

IndependentPixmap TextPixmaps::pixmap(int id) const
{
    Q_ASSERT(pixmaps_.contains(id));
    return pixmaps_[id].pixmap();
}

} // namespace gui_locations
