#include "imagechanger.h"
#include "utils/utils.h"
#include "dpiscalemanager.h"
#include "mainwindowstate.h"
#include "graphicresources/imageresourcessvg.h"

namespace ConnectWindow {

ImageChanger::ImageChanger(QObject *parent) : QObject(parent),
    pixmap_(nullptr), opacityCurImage_(1.0), opacityPrevImage_(0.0)
{
    connect(&opacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onOpacityChanged(QVariant)));
    connect(&opacityAnimation_, SIGNAL(finished()), SLOT(onOpacityFinished()));

    connect(&MainWindowState::instance(), SIGNAL(isActiveChanged(bool)), SLOT(onMainWindowIsActiveChanged(bool)));
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
        opacityPrevImage_ = 0.0;
        opacityCurImage_ = 1.0;
        updatePixmap();
    }
    else
    {
        prevImage_ = curImage_;

        curImage_.clear(nullptr);
        curImage_.pixmap = pixmap;

        opacityPrevImage_ = opacityCurImage_;
        opacityCurImage_ = 1.0 - opacityPrevImage_;

        if (opacityAnimation_.state() == QVariantAnimation::Running)
        {
            opacityAnimation_.stop();
        }

        opacityAnimation_.setStartValue(opacityCurImage_);
        opacityAnimation_.setEndValue(1.0);
        opacityAnimation_.setDuration((1.0 - opacityCurImage_) * ANIMATION_DURATION);

        opacityAnimation_.start();
        updatePixmap();
    }
    onMainWindowIsActiveChanged(MainWindowState::instance().isActive());
}

void ImageChanger::setMovie(QSharedPointer<QMovie> movie, bool bShowPrevChangeAnimation)
{
    generateCustomGradient(movie->scaledSize());

    if (!curImage_.isValid() || !bShowPrevChangeAnimation)
    {
        curImage_.clear(this);
        prevImage_.clear(this);
        curImage_.isMovie = true;
        curImage_.movie = movie;
        opacityPrevImage_ = 0.0;
        opacityCurImage_ = 1.0;
        connect(curImage_.movie.get(), SIGNAL(updated(QRect)), SLOT(updatePixmap()));
        curImage_.movie->start();
    }
    else
    {
        prevImage_ = curImage_;

        curImage_.clear(nullptr);
        curImage_.isMovie = true;
        curImage_.movie = movie;

        opacityPrevImage_ = opacityCurImage_;
        opacityCurImage_ = 1.0 - opacityPrevImage_;

        if (opacityAnimation_.state() == QVariantAnimation::Running)
        {
            opacityAnimation_.stop();
        }

        opacityAnimation_.setStartValue(opacityCurImage_);
        opacityAnimation_.setEndValue(1.0);
        opacityAnimation_.setDuration((1.0 - opacityCurImage_) * ANIMATION_DURATION);

        connect(curImage_.movie.get(), SIGNAL(updated(QRect)), SLOT(updatePixmap()));
        curImage_.movie->start();
        opacityAnimation_.start();
    }
    onMainWindowIsActiveChanged(MainWindowState::instance().isActive());
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
    pixmap_->fill(QColor(2, 13, 28));

    // prev and current gradient info
    enum GRADIENT { GRADIENT_NONE, GRADIENT_FLAG, GRADIENT_CUSTOM_BACKGROUND };
    GRADIENT prevGradient = GRADIENT_NONE;
    GRADIENT curGradient = GRADIENT_NONE;
    if (prevImage_.isValid())
    {
        if (!prevImage_.isMovie)
            prevGradient = GRADIENT_FLAG;
        else
            prevGradient = GRADIENT_CUSTOM_BACKGROUND;
    }
    if (curImage_.isValid())
    {
        if (!curImage_.isMovie)
            curGradient = GRADIENT_FLAG;
        else
            curGradient = GRADIENT_CUSTOM_BACKGROUND;
    }

    {
        QPainter p(pixmap_);

        if (prevImage_.isValid())
        {
            if (!prevImage_.isMovie)
            {
                p.setOpacity(opacityPrevImage_  * 0.4);
                prevImage_.pixmap->draw(0, 0, &p);

                if (curGradient != GRADIENT_FLAG)
                {
                    p.setOpacity(opacityPrevImage_);
                    QSharedPointer<IndependentPixmap> pixmap = ImageResourcesSvg::instance().getIndependentPixmap("background/FLAG_GRADIENT");
                    pixmap->draw(0, 0, &p);
                }
            }
            else
            {
                p.setOpacity(opacityPrevImage_);
                QPixmap framePixmap = prevImage_.movie->currentPixmap();
                framePixmap.setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());
                p.drawPixmap(0, ceil(7.0 * G_SCALE), framePixmap);

                if (curGradient != GRADIENT_CUSTOM_BACKGROUND)
                {
                    p.setOpacity(opacityPrevImage_);
                    p.drawPixmap(0, ceil(7.0 * G_SCALE), customGradient_);
                }
            }
        }
        if (curImage_.isValid())
        {
            if (!curImage_.isMovie)
            {
                p.setOpacity(opacityCurImage_  * 0.4);
                curImage_.pixmap->draw(0, 0, &p);

                // for non-movie (not custom background draw flag gradient)
                if (prevGradient == GRADIENT_CUSTOM_BACKGROUND)
                {
                    p.setOpacity(opacityCurImage_);
                }
                else
                {
                    p.setOpacity(1.0);
                }
                QSharedPointer<IndependentPixmap> pixmap = ImageResourcesSvg::instance().getIndependentPixmap("background/FLAG_GRADIENT");
                pixmap->draw(0, 0, &p);
            }
            else
            {
                p.setOpacity(opacityCurImage_);
                QPixmap framePixmap = curImage_.movie->currentPixmap();
                framePixmap.setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());
                p.drawPixmap(0, ceil(7.0 * G_SCALE), framePixmap);

                if (prevGradient == GRADIENT_FLAG)
                {
                    p.setOpacity(opacityCurImage_);
                }
                else
                {
                    p.setOpacity(1.0);
                }
                p.drawPixmap(0, ceil(7.0 * G_SCALE), customGradient_);
            }
        }
    }
    emit updated();
}

void ImageChanger::onMainWindowIsActiveChanged(bool isActive)
{
    if (curImage_.isValid() && curImage_.isMovie && curImage_.movie)
    {
        curImage_.movie->setPaused(!isActive);
    }
    if (prevImage_.isValid() && prevImage_.isMovie && prevImage_.movie)
    {
        prevImage_.movie->setPaused(!isActive);
    }
}

void ImageChanger::generateCustomGradient(const QSize &size)
{
    if (customGradient_.isNull() || customGradient_.size() != size)
    {
        customGradient_ = QPixmap(size);
        customGradient_.setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());
        customGradient_.fill(Qt::transparent);
        QPainter p(&customGradient_);

        QLinearGradient gradient(0, 0, 0, size.height() * 0.2);
        gradient.setColorAt(0, QColor(2, 13, 28, 255));
        gradient.setColorAt(1, QColor(2, 13, 28, 0));
        p.fillRect(QRect(0, 0, size.width(), size.height() * 0.2), gradient);
    }
}



} //namespace ConnectWindow
