#include "imagechanger.h"
#include "utils/utils.h"
#include "dpiscalemanager.h"
#include "mainwindowstate.h"
#include "graphicresources/imageresourcessvg.h"
#include <math.h>
#include <spdlog/spdlog.h>

namespace ConnectWindow {

ImageChanger::ImageChanger(QObject *parent, int animationDuration) : QObject(parent),
    pixmap_(nullptr), opacityCurImage_(1.0), opacityPrevImage_(0.0), animationDuration_(animationDuration)
{
    connect(&opacityAnimation_, &QVariantAnimation::valueChanged, this, &ImageChanger::onOpacityChanged);
    connect(&opacityAnimation_, &QVariantAnimation::finished, this, &ImageChanger::onOpacityFinished);

    connect(&MainWindowState::instance(), &MainWindowState::isActiveChanged, this, &ImageChanger::onMainWindowIsActiveChanged);
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
    if (!curImage_.isValid() || !bShowPrevChangeAnimation) {
        curImage_.clear(this);
        prevImage_.clear(this);
        curImage_.isMovie = false;
        curImage_.pixmap = pixmap;
        opacityPrevImage_ = 0.0;
        opacityCurImage_ = 1.0;
        updatePixmap();
    } else {
        prevImage_ = curImage_;

        curImage_.clear(nullptr);
        curImage_.pixmap = pixmap;

        opacityPrevImage_ = opacityCurImage_;
        opacityCurImage_ = 1.0 - opacityPrevImage_;

        if (opacityAnimation_.state() == QVariantAnimation::Running) {
            opacityAnimation_.stop();
        }

        opacityAnimation_.setStartValue(opacityCurImage_);
        opacityAnimation_.setEndValue(1.0);
        opacityAnimation_.setDuration((1.0 - opacityCurImage_) * animationDuration_);

        opacityAnimation_.start();
        updatePixmap();
    }
    onMainWindowIsActiveChanged(MainWindowState::instance().isActive());
}

void ImageChanger::setMovie(QSharedPointer<QMovie> movie, bool bShowPrevChangeAnimation)
{
    if (!curImage_.isValid() || !bShowPrevChangeAnimation) {
        curImage_.clear(this);
        prevImage_.clear(this);
        curImage_.isMovie = true;
        curImage_.movie = movie;
        opacityPrevImage_ = 0.0;
        opacityCurImage_ = 1.0;
        connect(curImage_.movie.get(), &QMovie::updated, this, &ImageChanger::updatePixmap);
        curImage_.movie->start();
    } else {
        prevImage_ = curImage_;

        curImage_.clear(nullptr);
        curImage_.isMovie = true;
        curImage_.movie = movie;

        opacityPrevImage_ = opacityCurImage_;
        opacityCurImage_ = 1.0 - opacityPrevImage_;

        if (opacityAnimation_.state() == QVariantAnimation::Running) {
            opacityAnimation_.stop();
        }

        opacityAnimation_.setStartValue(opacityCurImage_);
        opacityAnimation_.setEndValue(1.0);
        opacityAnimation_.setDuration((1.0 - opacityCurImage_) * animationDuration_);

        connect(curImage_.movie.get(), &QMovie::updated, this, &ImageChanger::updatePixmap);
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

    if (prevImage_.isValid()) {
        prevImage_.clear(this);
    }
    updatePixmap();
}

void ImageChanger::updatePixmap()
{
    SAFE_DELETE(pixmap_);

    pixmap_ = new QPixmap(WIDTH*G_SCALE*DpiScaleManager::instance().curDevicePixelRatio(), 238*G_SCALE*DpiScaleManager::instance().curDevicePixelRatio());
    pixmap_->setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());
    pixmap_->fill(QColor(9, 15, 25));

    // prev and current gradient info
    enum GRADIENT { GRADIENT_NONE, GRADIENT_FLAG, GRADIENT_CUSTOM_BACKGROUND };
    GRADIENT prevGradient = GRADIENT_NONE;
    GRADIENT curGradient = GRADIENT_NONE;
    if (prevImage_.isValid()) {
        if (!prevImage_.isMovie) {
            prevGradient = GRADIENT_FLAG;
        } else {
            prevGradient = GRADIENT_CUSTOM_BACKGROUND;
        }
    }

    if (curImage_.isValid()) {
        if (!curImage_.isMovie) {
            curGradient = GRADIENT_FLAG;
        } else {
            curGradient = GRADIENT_CUSTOM_BACKGROUND;
        }
    }

    QPainter p(pixmap_);

    if (prevImage_.isValid()) {
        if (!prevImage_.isMovie) {
            p.setOpacity(opacityPrevImage_  * 0.3);
            prevImage_.pixmap->draw(0, 0, &p);

            if (curGradient != GRADIENT_FLAG) {
                p.setOpacity(opacityPrevImage_);
                QSharedPointer<IndependentPixmap> pixmap = ImageResourcesSvg::instance().getIndependentPixmap("background/FLAG_GRADIENT");
                pixmap->draw(0, 0, &p);
            }
        } else {
            p.setOpacity(opacityPrevImage_);
            QPixmap framePixmap = prevImage_.movie->currentPixmap();
            framePixmap.setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());

            if (aspectRatioMode_ == ASPECT_RATIO_MODE_TILE) {
                // For tiling mode, we draw the image repeatedly to fill the entire area
                int frameWidth = framePixmap.width() / DpiScaleManager::instance().curDevicePixelRatio();
                int frameHeight = framePixmap.height() / DpiScaleManager::instance().curDevicePixelRatio();

                if (frameWidth > 0 && frameHeight > 0) {
                    for (int x = 0; x < WIDTH * G_SCALE; x += frameWidth) {
                        for (int y = 0; y < 197 * G_SCALE; y += frameHeight) {
                            p.drawPixmap(x, y, framePixmap, 0, 0, framePixmap.width(), (y + frameHeight > 197*G_SCALE) ? 197*G_SCALE - y : frameHeight);
                        }
                    }
                }
            } else {
                // Start at y = 0 if image height is 197 (16:9)
                int yOffset = (197*G_SCALE - framePixmap.height())/2;
                p.drawPixmap(0, yOffset, framePixmap, 0, 0, framePixmap.width(), 197*G_SCALE - yOffset);
            }

            if (curGradient != GRADIENT_CUSTOM_BACKGROUND) {
                p.setOpacity(opacityPrevImage_);
                QSharedPointer<IndependentPixmap> customGradient = ImageResourcesSvg::instance().getIndependentPixmap("background/CUSTOM_GRADIENT");
                customGradient->draw(0, 0, &p);
            }
        }
    }

    if (curImage_.isValid()) {
        int yOffset = 0;
        if (!curImage_.isMovie) {
            p.setOpacity(opacityCurImage_  * 0.3);
            yOffset = (238*G_SCALE - curImage_.pixmap->height())/2;
            curImage_.pixmap->draw(0, yOffset, &p);

            // for non-movie (not custom background draw flag gradient)
            if (prevGradient == GRADIENT_CUSTOM_BACKGROUND) {
                p.setOpacity(opacityCurImage_);
            } else {
                p.setOpacity(1.0);
            }
            QSharedPointer<IndependentPixmap> pixmap = ImageResourcesSvg::instance().getIndependentPixmap("background/FLAG_GRADIENT");
            pixmap->draw(0, yOffset, curImage_.pixmap->width(), curImage_.pixmap->height(), &p);
        } else {
            p.setOpacity(opacityCurImage_);
            QPixmap framePixmap = curImage_.movie->currentPixmap();
            framePixmap.setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());
            int frameWidth = framePixmap.width() / DpiScaleManager::instance().curDevicePixelRatio();
            int frameHeight = framePixmap.height() / DpiScaleManager::instance().curDevicePixelRatio();

            if (aspectRatioMode_ == ASPECT_RATIO_MODE_TILE) {
                // For tiling mode, we draw the image repeatedly to fill the entire area
                if (frameWidth > 0 && frameHeight > 0) {
                    for (int x = 0; x < WIDTH * G_SCALE; x += frameWidth) {
                        for (int y = 0; y < 197 * G_SCALE; y += frameHeight) {
                            p.drawPixmap(x, y, framePixmap, 0, 0, framePixmap.width(), (y + frameHeight > 197*G_SCALE) ? (197*G_SCALE - y) * DpiScaleManager::instance().curDevicePixelRatio() : frameHeight * DpiScaleManager::instance().curDevicePixelRatio());
                        }
                    }
                }
            } else {
                // Start at y = 0 if image height is 197 (16:9)
                yOffset = (197*G_SCALE - frameHeight)/2;
                p.drawPixmap(0, yOffset, framePixmap, 0, 0, framePixmap.width(), (197*G_SCALE - yOffset) * DpiScaleManager::instance().curDevicePixelRatio());
            }

            if (prevGradient == GRADIENT_FLAG) {
                p.setOpacity(opacityCurImage_);
            } else {
                p.setOpacity(1.0);
            }
            QSharedPointer<IndependentPixmap> customGradient = ImageResourcesSvg::instance().getIndependentPixmap("background/CUSTOM_GRADIENT");
            customGradient->draw(0, 0, &p);
        }
    }
    emit updated();
}

void ImageChanger::onMainWindowIsActiveChanged(bool isActive)
{
    if (curImage_.isValid() && curImage_.isMovie && curImage_.movie) {
        curImage_.movie->setPaused(!isActive);
    }
    if (prevImage_.isValid() && prevImage_.isMovie && prevImage_.movie) {
        prevImage_.movie->setPaused(!isActive);
    }
}

} //namespace ConnectWindow
