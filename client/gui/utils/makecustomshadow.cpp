#include "makecustomshadow.h"

#include <QPainter>

QT_BEGIN_NAMESPACE
  extern void qt_blurImage(QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0 );
QT_END_NAMESPACE

// only correct for 100% scaling -- user code should scale the pixmap
QPixmap MakeCustomShadow::makeShadowForPixmap(QPixmap &sourcePixmap, qreal distance, qreal blurRadius, const QColor& color)
{
    QSize szi(sourcePixmap.width() + 2 * distance , sourcePixmap.height() + 2* distance );

    QImage tmp(szi, QImage::Format_ARGB32_Premultiplied);
    tmp.fill(0);

    QPainter tmpPainter(&tmp);
    tmpPainter.drawPixmap(QPointF(distance, distance), sourcePixmap);
    tmpPainter.end();

    // blur the alpha channel
    QImage blurred(szi, QImage::Format_ARGB32_Premultiplied);
    blurred.fill(0);
    QPainter blurPainter(&blurred);
    qt_blurImage(&blurPainter, tmp, blurRadius, false, false);
    blurPainter.end();

    tmp = blurred;

    // blacken the image
    tmpPainter.begin(&tmp);
    tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    //tmpPainter.setOpacity(0.8);
    tmpPainter.fillRect(tmp.rect(), color);
    tmpPainter.end();

    return QPixmap::fromImage(tmp);
}
