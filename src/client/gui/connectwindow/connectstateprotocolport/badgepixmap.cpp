#include "badgepixmap.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "utils/ws_assert.h"

namespace ConnectWindow {

BadgePixmap::BadgePixmap(const QSize &size, int radius) : size_(size), radius_(radius), badgeBgColor_(Qt::white)
{
    updatePixmap();
}

void BadgePixmap::draw(QPainter *painter, int x, int y)
{
    if (pixmap_.isNull())
    {
        updatePixmap();
    }
    painter->save();
    painter->setOpacity(OPACITY_FULL);
    painter->drawPixmap(x, y, pixmap_);
    painter->restore();
}

void BadgePixmap::setColor(const QColor &color)
{
    if (color != badgeBgColor_)
    {
        badgeBgColor_ = color;
        updatePixmap();
    }
}

void BadgePixmap::setSize(const QSize &size, int radius)
{
    if (size != size_ || radius != radius_)
    {
        size_ = size;
        radius_ = radius;
        updatePixmap();
    }
}

int BadgePixmap::width() const
{
    WS_ASSERT(!pixmap_.isNull());
    return pixmap_.width();
}

int BadgePixmap::height() const
{
    WS_ASSERT(!pixmap_.isNull());
    return pixmap_.height();
}

void BadgePixmap::updatePixmap()
{
    pixmap_ = QPixmap(size_.width(), size_.height());
    pixmap_.fill(Qt::transparent);
    {
        QPainter painter(&pixmap_);
        painter.setRenderHint(QPainter::Antialiasing);

        QColor brushColor = badgeBgColor_;
        brushColor.setAlpha(52);
        painter.setBrush(brushColor);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(QRect(0, 0, size_.width(), size_.height()), radius_, radius_);

        QColor penColor = badgeBgColor_;
        penColor.setAlpha(26);
        painter.setPen(penColor);
        painter.setBrush(Qt::transparent);
        painter.drawRoundedRect(QRect(1, 1, size_.width()-2, size_.height()-2), radius_, radius_);
    }
}

} //namespace ConnectWindow
