#include "imageitem.h"

#include <QPainter>
#include "GraphicResources/imageresourcessvg.h"

ImageItem::ImageItem(const QString &imagePath, ScalableGraphicsObject *parent) : ScalableGraphicsObject(parent),
  imagePath_(imagePath)
{
    updateScaling();
}

QRectF ImageItem::boundingRect() const
{
    return QRectF(0, 0, width_, height_);
}

void ImageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    IndependentPixmap *p = ImageResourcesSvg::instance().getIndependentPixmap(imagePath_);
    p->draw(0, 0, painter);
}

void ImageItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    IndependentPixmap *p = ImageResourcesSvg::instance().getIndependentPixmap(imagePath_);
    width_ = p->width();
    height_ = p->height();
}

