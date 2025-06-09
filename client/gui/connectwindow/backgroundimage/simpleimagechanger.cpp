#include "simpleimagechanger.h"
#include <QPainter>
#include <QDebug>
#include "dpiscalemanager.h"
#include "utils/ws_assert.h"

namespace ConnectWindow {

SimpleImageChanger::SimpleImageChanger(QObject *parent, int animationDuration) : QObject(parent),
    animationDuration_(animationDuration), opacityCurImage_(0.8), opacityPrevImage_(0.0)
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

void SimpleImageChanger::setImage(QSharedPointer<IndependentPixmap> image, bool bWithAnimation)
{
    WS_ASSERT(!image.isNull());

    if (curImage_.isNull() || !bWithAnimation)
    {
        curImage_ = image;
        prevImage_.reset();
        opacityPrevImage_ = 0.0;
        opacityCurImage_ = 0.8;
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
        opacityCurImage_ = 0.8 - opacityPrevImage_;

        if (opacityAnimation_.state() == QVariantAnimation::Running)
        {
            opacityAnimation_.stop();
        }

        opacityAnimation_.setStartValue(opacityCurImage_);
        opacityAnimation_.setEndValue(0.8);
        opacityAnimation_.setDuration((0.8 - opacityCurImage_) * animationDuration_);
        opacityAnimation_.start();
        updatePixmap();
    }
}

void SimpleImageChanger::onOpacityChanged(const QVariant &value)
{
    opacityPrevImage_ = 0.8 - value.toDouble();
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

            p.setOpacity(opacityCurImage_ * 0.8);
            curImage_->draw(0, 0, &p);
        }
    }

    emit updated();
}



} //namespace ConnectWindow
