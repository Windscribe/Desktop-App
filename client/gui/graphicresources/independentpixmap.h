#ifndef INDEPENDENTPIXMAP_H
#define INDEPENDENTPIXMAP_H

#include <QObject>
#include <QBitmap>
#include <QPainter>
#include <QRect>

// everywhere we use this class instead of QPixmap
// for Mac retina, use pixelRatio = 2, for the rest pixelRatio = 1;
// since the size of the QPixmap depends on the pixelRatio, this class is made for convenience
class IndependentPixmap
{
public:
    explicit IndependentPixmap(const QPixmap &pixmap);
    virtual ~IndependentPixmap();

    QSize originalPixmapSize() const;

    int width() const;
    int height() const;

    void draw(int x, int y, QPainter *painter);
    void draw(int x, int y, int w, int h, QPainter *painter);
    void draw(int x, int y, QPainter *painter, int x1, int y1, int w, int h);
    void draw(const QRect &rect, QPainter *painter);

    QIcon getIcon() const;
    QIcon getScaledIcon() const;
    QPixmap getScaledPixmap(int width, int height) const;

    QBitmap mask() const;

private:
    QPixmap pixmap_;
};

#endif // INDEPENDENTPIXMAP_H
