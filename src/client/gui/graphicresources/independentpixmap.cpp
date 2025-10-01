#include "independentpixmap.h"
#include <QGraphicsPixmapItem>
#include <QIcon>
#include <QPixmap>

IndependentPixmap::IndependentPixmap(const QPixmap &pixmap): pixmap_(pixmap)
{
}

bool IndependentPixmap::isNull() const
{
    return pixmap_.isNull();
}

QSize IndependentPixmap::originalPixmapSize() const
{
    return pixmap_.size();
}

int IndependentPixmap::width() const
{
    return pixmap_.width() / pixmap_.devicePixelRatio();
}

int IndependentPixmap::height() const
{
    return pixmap_.height() / pixmap_.devicePixelRatio();
}

void IndependentPixmap::draw(int x, int y, QPainter *painter) const
{
    painter->drawPixmap(x, y, pixmap_);
}

void IndependentPixmap::draw(int x, int y, int w, int h, QPainter *painter)
{
    painter->drawPixmap(x, y, w, h, pixmap_);
}

void IndependentPixmap::draw(int x, int y, int w, int h, QPainter *painter, QColor color)
{
    QPainter p(&pixmap_);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(pixmap_.rect(), color);
    painter->drawPixmap(x, y, w, h, pixmap_);
}

void IndependentPixmap::draw(int x, int y, QPainter *painter, int x1, int y1, int w, int h)
{
    painter->drawPixmap(x, y, pixmap_, x1*pixmap_.devicePixelRatio(), y1*pixmap_.devicePixelRatio(), w*pixmap_.devicePixelRatio(), h*pixmap_.devicePixelRatio());
}

void IndependentPixmap::draw(int x, int y, int w, int h, QPainter *painter, int sx, int sy, int sw, int sh)
{
    painter->drawPixmap(x, y, w, h, pixmap_, sx, sy, sw, sh);
}

void IndependentPixmap::draw(const QRect &rect, QPainter *painter)
{
    painter->drawPixmap(rect, pixmap_);
}

QIcon IndependentPixmap::getIcon() const
{
    return QIcon(pixmap_);
}

QIcon IndependentPixmap::getScaledIcon() const
{
    QIcon icon(pixmap_.scaled(20,20,Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    return icon;
}

QPixmap IndependentPixmap::getScaledPixmap(int width, int height) const
{
    QPixmap pm(pixmap_.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    return pm;
}

QBitmap IndependentPixmap::mask() const
{
    return pixmap_.mask();
}
