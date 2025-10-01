#include "simpleimagechanger.h"
#include <QPainter>
#include <QDebug>
#include "dpiscalemanager.h"
#include "utils/ws_assert.h"

namespace ConnectWindow {

SimpleImageChanger::SimpleImageChanger(QObject *parent, int animationDuration) : QObject(parent),
    animationDuration_(animationDuration), opacityCurImage_(1.0), opacityPrevImage_(0.0)
{
    connect(&opacityAnimation_, &QVariantAnimation::valueChanged, this, &SimpleImageChanger::onOpacityChanged);
}

QPixmap *SimpleImageChanger::currentPixmap()
{
    if (!pixmap_.isNull())
        return &pixmap_;
    else
        return nullptr;
}

void SimpleImageChanger::setImage(QSharedPointer<IndependentPixmap> image, bool bWithAnimation, double opacity)
{
    WS_ASSERT(!image.isNull());

    if (curImage_.isNull() || !bWithAnimation)
    {
        curImage_ = image;
        prevImage_.reset();
        opacityPrevImage_ = 0.0;
        opacityCurImage_ = opacity;
        updatePixmap();
    }
    else
    {
        if (curImage_.get() == image.get())
        {
            return;
        }

        prevImage_ = curImage_;
        curImage_ = image;

        opacityPrevImage_ = opacityCurImage_;
        opacityCurImage_ = opacity - opacityPrevImage_;
        fullOpacity_ = opacity;

        if (opacityAnimation_.state() == QVariantAnimation::Running)
        {
            opacityAnimation_.stop();
        }

        opacityAnimation_.setStartValue(opacityCurImage_);
        opacityAnimation_.setEndValue(fullOpacity_);
        opacityAnimation_.setDuration((fullOpacity_ - opacityCurImage_) * animationDuration_);
        opacityAnimation_.start();
        updatePixmap();
    }
}

void SimpleImageChanger::onOpacityChanged(const QVariant &value)
{
    opacityPrevImage_ = fullOpacity_ - value.toDouble();
    opacityCurImage_ = value.toDouble();
    updatePixmap();
}


void SimpleImageChanger::updatePixmap()
{
    if (!curImage_.isNull())
    {
        QSize sz(curImage_->originalPixmapSize());
        if (pixmap_.size() != sz || pixmap_.devicePixelRatio() != DpiScaleManager::instance().curDevicePixelRatio())
        {
            pixmap_ = QPixmap(sz);
            pixmap_.setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());
        }
        pixmap_.fill(Qt::transparent);

        {
            QPainter p(&pixmap_);

            if (!prevImage_.isNull())
            {
                p.setOpacity(opacityPrevImage_);
                prevImage_->draw(0, 0, &p);
            }

            p.setOpacity(opacityCurImage_);
            curImage_->draw(0, 0, &p);
        }
    }

    emit updated();
}



} //namespace ConnectWindow
