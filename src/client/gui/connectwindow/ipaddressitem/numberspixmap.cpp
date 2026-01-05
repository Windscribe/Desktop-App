#include "numberspixmap.h"
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "utils/utils.h"
#include "dpiscalemanager.h"
#include <QPainter>

NumbersPixmap::NumbersPixmap(): pixmap_(NULL), dotPixmap_(NULL), naPixmap_(NULL)
{
    rescale();
}

NumbersPixmap::~NumbersPixmap()
{
    SAFE_DELETE(pixmap_);
    SAFE_DELETE(dotPixmap_);
    SAFE_DELETE(naPixmap_);
}

int NumbersPixmap::width() const
{
    return itemWidth_;
}

int NumbersPixmap::height() const
{
    return itemHeight_;
}

QFont NumbersPixmap::getFont()
{
    return font_;
}

IndependentPixmap *NumbersPixmap::getPixmap()
{
    return pixmap_;
}

IndependentPixmap *NumbersPixmap::getDotPixmap()
{
    return dotPixmap_;
}

IndependentPixmap *NumbersPixmap::getNAPixmap()
{
    return naPixmap_;
}

int NumbersPixmap::dotWidth() const
{
    return dotWidth_;
}

void NumbersPixmap::rescale()
{
    SAFE_DELETE(pixmap_);
    SAFE_DELETE(dotPixmap_);
    SAFE_DELETE(naPixmap_);

    font_ = FontManager::instance().getFont(15,  QFont::Medium);

    QFontMetrics fm(font_);
    itemHeight_ = fm.height();

    itemWidth_ = 0;
    for (int i = 0; i <= 9; ++i)
    {
        int w = fm.horizontalAdvance(QString::number(i));
        if (w > itemWidth_)
        {
            itemWidth_ = w;
        }
    }

    dotWidth_ = fm.horizontalAdvance(".");

    QPixmap pixmap(QSize(itemWidth_, itemHeight_ * 11) * DpiScaleManager::instance().curDevicePixelRatio());
    pixmap.setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());
    pixmap.fill(QColor(Qt::transparent));

    QPainter painter(&pixmap);
    painter.setFont(font_);
    painter.setPen(Qt::white);
    painter.setOpacity(OPACITY_SEVENTY);

    for (int i = 0; i <= 10; ++i)
    {
        if (i == 10)
        {
            painter.drawText(QRect(0, i*itemHeight_, itemWidth_, itemHeight_), "0");
        }
        else
        {
            painter.drawText(QRect(0, i*itemHeight_, itemWidth_, itemHeight_), QString::number(i));
        }
    }

    pixmap_ = new IndependentPixmap(pixmap);

    QPixmap dotPixmap(QSize(dotWidth_, itemHeight_) * DpiScaleManager::instance().curDevicePixelRatio());
    dotPixmap.setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());
    dotPixmap.fill(QColor(Qt::transparent));
    QPainter painter2(&dotPixmap);
    painter2.setFont(font_);
    painter2.setPen(Qt::white);
    painter2.setOpacity(OPACITY_SEVENTY);
    painter2.drawText(QRect(0, 0, itemWidth_, itemHeight_), ".");
    dotPixmap_ = new IndependentPixmap(dotPixmap);

    int naWidth = fm.horizontalAdvance("---.---.---.---");
    QPixmap naPixmap(QSize(naWidth, itemHeight_) * DpiScaleManager::instance().curDevicePixelRatio());
    naPixmap.setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());
    naPixmap.fill(QColor(Qt::transparent));
    QPainter painter3(&naPixmap);
    painter3.setFont(font_);
    painter3.setPen(Qt::white);
    painter3.setOpacity(OPACITY_SEVENTY);
    painter3.drawText(QRect(0, 0, naWidth, itemHeight_), "---.---.---.---");
    naPixmap_ = new IndependentPixmap(naPixmap);
}
