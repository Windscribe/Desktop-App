#include "imagewithshadow.h"
#include "dpiscalemanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "utils/ws_assert.h"
#include <math.h>

ImageWithShadow::ImageWithShadow(const QString &originalName, const QString &shadowName): originalName_(originalName), shadowName_(shadowName)
{
    updatePixmap();
}

void ImageWithShadow::draw(QPainter *painter, int x, int y)
{
    WS_ASSERT(!pixmap_.isNull());
    painter->drawPixmap(x, y, pixmap_);
}

int ImageWithShadow::width() const
{
    WS_ASSERT(!pixmap_.isNull());
    return pixmap_.width() / DpiScaleManager::instance().curDevicePixelRatio();
}

int ImageWithShadow::height() const
{
    WS_ASSERT(!pixmap_.isNull());
    return pixmap_.height() / DpiScaleManager::instance().curDevicePixelRatio();
}

void ImageWithShadow::updatePixmap()
{
    const int SHADOW_OFFSET = ceil(G_SCALE);
    QSharedPointer<IndependentPixmap> originalImg = ImageResourcesSvg::instance().getIndependentPixmap(originalName_);
    QSharedPointer<IndependentPixmap> shadowImg = ImageResourcesSvg::instance().getIndependentPixmap(shadowName_);
    WS_ASSERT(originalImg->width() == shadowImg->width());
    WS_ASSERT(originalImg->height() == shadowImg->height());


    QSize sz = originalImg->originalPixmapSize();
    pixmap_ = QPixmap(QSize(sz.width() + SHADOW_OFFSET * DpiScaleManager::instance().curDevicePixelRatio(), sz.height() + SHADOW_OFFSET *DpiScaleManager::instance().curDevicePixelRatio()));
    pixmap_.setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());
    pixmap_.fill(Qt::transparent);
    {
        QPainter painter(&pixmap_);

        painter.setOpacity(0.5);
        shadowImg->draw(SHADOW_OFFSET, SHADOW_OFFSET, &painter);
        painter.setOpacity(1.0);
        originalImg->draw(0, 0, &painter);
    }
}
