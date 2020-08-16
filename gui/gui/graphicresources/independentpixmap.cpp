#include "independentpixmap.h"
#include <QIcon>
#include <QPixmap>

IndependentPixmap::IndependentPixmap(QPixmap *pixmap): pixmap_(pixmap)
{
    Q_ASSERT(pixmap_ != NULL);
}

IndependentPixmap::~IndependentPixmap()
{
    if (pixmap_)
    {
        delete pixmap_;
    }
}

QSize IndependentPixmap::originalPixmapSize() const
{
    if (pixmap_)
    {
        return pixmap_->size();
    }
    else
    {
        return QSize(0,0);
    }
}

int IndependentPixmap::width() const
{
    if (pixmap_)
    {
        return pixmap_->width() / pixmap_->devicePixelRatio();
    }
    else
    {
        return 0;
    }
}

int IndependentPixmap::height() const
{
    if (pixmap_)
    {
        return pixmap_->height() / pixmap_->devicePixelRatio();
    }
    else
    {
        return 0;
    }
}

void IndependentPixmap::draw(int x, int y, QPainter *painter)
{
    if (pixmap_)
    {
        painter->drawPixmap(x, y, *pixmap_);
    }
}

void IndependentPixmap::draw(int x, int y, int w, int h, QPainter *painter)
{
    if (pixmap_)
    {
        painter->drawPixmap(x, y, w, h, *pixmap_);
    }
}

void IndependentPixmap::draw(int x, int y, QPainter *painter, int x1, int y1, int w, int h)
{
    if (pixmap_)
    {
        painter->drawPixmap(x, y, *pixmap_, x1*pixmap_->devicePixelRatio(), y1*pixmap_->devicePixelRatio(), w*pixmap_->devicePixelRatio(), h*pixmap_->devicePixelRatio());
    }
}

QIcon IndependentPixmap::getScaledIcon()
{
    QIcon icon(pixmap_->scaled(20,20,Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    return icon;
}

QPixmap IndependentPixmap::getScaledPixmap(int width, int height)
{
    QPixmap pm(pixmap_->scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    return pm;
}

QBitmap IndependentPixmap::mask() const
{
    if (pixmap_)
    {
        return pixmap_->mask();
    }
    else
    {
        return QBitmap();
    }
}
