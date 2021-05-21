#include "badgepixmap.h"
#include "dpiscalemanager.h"


namespace ConnectWindow {

BadgePixmap::BadgePixmap(const QSize &size, int radius) : shadowColor_(0x02, 0x0D, 0x1C, 128), size_(size), radius_(radius), badgeBgColor_(255, 255, 255, 39)
{
    updatePixmap();
}

void BadgePixmap::draw(QPainter *painter, int x, int y)
{
    if (pixmap_.isNull())
    {
        updatePixmap();
    }
    painter->save();
    painter->setOpacity(badgeBgColor_.alphaF());
    painter->drawPixmap(x, y, pixmap_);
    painter->restore();
}

void BadgePixmap::setColor(const QColor &color)
{
    if (color != badgeBgColor_)
    {
        badgeBgColor_ = color;
        updatePixmap();
    }
}

void BadgePixmap::setSize(const QSize &size, int radius)
{
    if (size != size_ || radius != radius_)
    {
        size_ = size;
        radius_ = radius;
        updatePixmap();
    }
}

int BadgePixmap::width() const
{
    Q_ASSERT(!pixmap_.isNull());
    return pixmap_.width() / DpiScaleManager::instance().curDevicePixelRatio();
}

int BadgePixmap::height() const
{
    Q_ASSERT(!pixmap_.isNull());
    return pixmap_.height() / DpiScaleManager::instance().curDevicePixelRatio();
}

void BadgePixmap::updatePixmap()
{
    pixmap_ = QPixmap((size_.width() + SHADOW_OFFSET) * DpiScaleManager::instance().curDevicePixelRatio(), (size_.height() + SHADOW_OFFSET) * DpiScaleManager::instance().curDevicePixelRatio());
    pixmap_.setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());
    pixmap_.fill(Qt::transparent);
    {
        QPainter painter(&pixmap_);
        painter.setRenderHint(QPainter::Antialiasing);

        painter.setBrush(shadowColor_);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(QRect(SHADOW_OFFSET, SHADOW_OFFSET, size_.width(), size_.height()), radius_, radius_);

        QColor colorWithoutAlpha(badgeBgColor_.red(), badgeBgColor_.green(), badgeBgColor_.blue());
        painter.setBrush(colorWithoutAlpha);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(QRect(0, 0, size_.width(), size_.height()), radius_, radius_);
    }
}

} //namespace ConnectWindow
