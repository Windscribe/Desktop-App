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
        curImage_.clear(this);
        prevImage_.clear(this);
        curImage_.isMovie = false;
        curImage_.pixmap = pixmap;
        curImage_.movie.reset();
        opacityPrevImage_ = 0.0;
        opacityCurImage_ = 1.0;
        updatePixmap();
    }
    else
    {
        prevImage_ = curImage_;

        curImage_.clear(this);
        curImage_.pixmap = pixmap;

        opacityPrevImage_ = 1.0;
        opacityCurImage_ = 0.0;
        opacityAnimation_.start();
        updatePixmap();
    }
}

void ImageChanger::setMovie(QSharedPointer<QMovie> movie, bool bShowPrevChangeAnimation)
{
    if (!curImage_.isValid() || !bShowPrevChangeAnimation)
    {
        curImage_.clear(this);
        prevImage_.clear(this);
        curImage_.isMovie = true;
        curImage_.pixmap.reset();
        curImage_.movie = movie;
        opacityPrevImage_ = 0.0;
        opacityCurImage_ = 1.0;
        connect(curImage_.movie.get(), SIGNAL(updated(QRect)), SLOT(updatePixmap()));
        curImage_.movie->start();
    }
    else
    {
        prevImage_ = curImage_;

        curImage_.clear(this);
        curImage_.isMovie = true;
        curImage_.movie = movie;

        connect(curImage_.movie.get(), SIGNAL(updated(QRect)), SLOT(updatePixmap()));
        curImage_.movie->start();

        opacityPrevImage_ = 1.0;
        opacityCurImage_ = 0.0;
        opacityAnimation_.start();
    }
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

    if (prevImage_.isValid())
    {
        prevImage_.clear(this);
    }
    updatePixmap();
}

void ImageChanger::updatePixmap()
{
    SAFE_DELETE(pixmap_);

    pixmap_ = new QPixmap(WIDTH * G_SCALE * DpiScaleManager::instance().curDevicePixelRatio(), 176 * G_SCALE * DpiScaleManager::instance().curDevicePixelRatio());
    pixmap_->setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());
    pixmap_->fill(Qt::transparent);
    {
        QPainter p(pixmap_);
        if (prevImage_.isValid())
        {
            p.setOpacity(opacityPrevImage_);

            if (!prevImage_.isMovie)
            {
                prevImage_.pixmap->draw(0, 0, &p);
            }
            else
            {
                QPixmap framePixmap = prevImage_.movie->currentPixmap();
                framePixmap.setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());
                p.drawPixmap(0, 0, framePixmap);
            }
        }
        if (curImage_.isValid())
        {
            p.setOpacity(opacityCurImage_);

            if (!curImage_.isMovie)
            {
                curImage_.pixmap->draw(0, 0, &p);
            }
            else
            {
                QPixmap framePixmap = curImage_.movie->currentPixmap();
                framePixmap.setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());
                p.drawPixmap(0, 0, framePixmap);
            }
        }
    }
    emit updated();
}



} //namespace ConnectWindow
