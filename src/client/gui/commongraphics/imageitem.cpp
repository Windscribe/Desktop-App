#include "imageitem.h"

#include <QPainter>
#include "commongraphics/commongraphics.h"
#include "graphicresources/imageresourcespng.h"
#include "graphicresources/imageresourcessvg.h"

ImageItem::ImageItem(ScalableGraphicsObject *parent, const QString &imagePath, const QString &shadowImagePath) : ScalableGraphicsObject(parent),
  imagePath_(imagePath), shadowImagePath_(shadowImagePath)
{
    if (!shadowImagePath_.isEmpty()) {
        imageWithShadow_.reset(new ImageWithShadow(imagePath_, shadowImagePath_));
    }
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

    painter->setOpacity(opacity());

    if (imageWithShadow_) {
        imageWithShadow_->draw(painter, 0, 0);
    } else {
        QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap(imagePath_);
        if (p.isNull()) {
            // If SVG not found, try PNG
            p = ImageResourcesPng::instance().getIndependentPixmap(imagePath_);
        }
        if (p.isNull()) {
            // Still not found, do nothing
            return;
        }
        p->draw(0, 0, painter);
    }
}

void ImageItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    if (imageWithShadow_) {
        imageWithShadow_->updatePixmap();
        width_ = imageWithShadow_->width();
        height_ = imageWithShadow_->height();
    } else {
        QSharedPointer<IndependentPixmap> p = ImageResourcesSvg::instance().getIndependentPixmap(imagePath_);
        if (p.isNull()) {
            // If SVG not found, try PNG
            p = ImageResourcesPng::instance().getIndependentPixmap(imagePath_);
        }
        if (p.isNull()) {
            width_ = 0;
            height_ = 0;
            return;
        }
        width_ = p->width();
        height_ = p->height();
    }
}

