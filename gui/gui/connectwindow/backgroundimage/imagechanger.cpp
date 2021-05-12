#include "imagechanger.h"
#include "utils/utils.h"
#include "dpiscalemanager.h"

namespace ConnectWindow {

ImageChanger::ImageChanger(QObject *parent) : QObject(parent),
    pixmap_(nullptr), opacityCurImage_(1.0), opacityPrevImage_(0.0)
{
    opacityAnimation_.setStartValue(0.0);
    opacityAnimation_.setEndValue(1.0);
    opacityAnimation_.setDuration(500);
    connect(&opacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onOpacityChanged(QVariant)));
    connect(&opacityAnimation_, SIGNAL(finished()), SLOT(onOpacityFinished()));
}

ImageChanger::~ImageChanger()
{
    SAFE_DELETE(pixmap_);
}

QPixmap *ImageChanger::currentPixmap()
{
    return pixmap_;
}

void ImageChanger::setImage(QSharedPointer<IndependentPixmap> pixmap, bool bShowPrevChangeAnimation)
{
    if (!curImage_.isValid() || !bShowPrevChangeAnimation)
    {
        curImage_.isMovie = false;
        curImage_.pixmap = pixmap;
        opacityPrevImage_ = 0.0;
        opacityCurImage_ = 1.0;
        updatePixmap();
    }
    else
    {
        prevImage_ = curImage_;

        curImage_.clear();
        curImage_.pixmap = pixmap;

        opacityPrevImage_ = 1.0;
        opacityCurImage_ = 0.0;
        opacityAnimation_.start();
        updatePixmap();
    }
}

void ImageChanger::setMovie(QSharedPointer<QMovie> movie)
{

}

void ImageChanger::onOpacityChanged(const QVariant &value)
{
    opacityPrevImage_ = 1.0 - value.toDouble();
    opacityCurImage_ = value.toDouble();
    updatePixmap();
}

void ImageChanger::onOpacityFinished()
{
    opacityPrevImage_ = 0.0;
    opacityCurImage_ = 1.0;
    prevImage_.clear();
    updatePixmap();
}

void ImageChanger::updatePixmap()
{
    SAFE_DELETE(pixmap_);

    pixmap_ = new QPixmap(WIDTH * G_SCALE, 176 * G_SCALE);
    pixmap_->fill(Qt::transparent);
    {
        QPainter p(pixmap_);
        if (prevImage_.isValid())
        {
            p.setOpacity(opacityPrevImage_);
            prevImage_.pixmap->draw(0, 0, &p);
        }
        if (curImage_.isValid())
        {
            p.setOpacity(opacityCurImage_);
            curImage_.pixmap->draw(0, 0, &p);
        }
    }
    emit updated();
}



} //namespace ConnectWindow
