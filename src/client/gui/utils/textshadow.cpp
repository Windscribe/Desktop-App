#include "textshadow.h"
#include "utils/ws_assert.h"
#include "dpiscalemanager.h"
#include <math.h>

TextShadow::TextShadow() : shadowColor_(0x09, 0x0E, 0x19, 128), lastFlags_(0)
{
}

void TextShadow::drawText(QPainter *painter, const QRect &rect, int flags, const QString &text, QFont &font, const QColor &color)
{
    if (pixmap_.isNull() || rect != lastRect_ || flags != lastFlags_ || text != lastText_ || font != lastFont_ || color != lastColor_)
    {
        lastRect_ = rect;
        lastFlags_ = flags;
        lastText_ = text;
        lastFont_ = font;
        lastColor_ = color;
        updatePixmap();
    }
    painter->drawPixmap(lastBoundingRect_.topLeft(), pixmap_);
}

int TextShadow::width() const
{
    WS_ASSERT(!pixmap_.isNull());
    return pixmap_.width() / pixmap_.devicePixelRatio();
}

int TextShadow::height() const
{
    WS_ASSERT(!pixmap_.isNull());
    return pixmap_.height() / pixmap_.devicePixelRatio();
}

void TextShadow::updatePixmap()
{
    const int SHADOW_OFFSET = ceil(G_SCALE);
    QFontMetrics fm(lastFont_);
    lastBoundingRect_ = fm.boundingRect(lastRect_, lastFlags_, lastText_);

    pixmap_ = QPixmap(QSize(lastBoundingRect_.width() + SHADOW_OFFSET, lastBoundingRect_.height() + SHADOW_OFFSET) * DpiScaleManager::instance().curDevicePixelRatio());
    pixmap_.setDevicePixelRatio(DpiScaleManager::instance().curDevicePixelRatio());
    pixmap_.fill(Qt::transparent);
    {
        QPainter painter(&pixmap_);
        painter.setFont(lastFont_);

        painter.setPen(shadowColor_);
        painter.drawText(QRect(SHADOW_OFFSET, SHADOW_OFFSET, lastBoundingRect_.width(), lastBoundingRect_.height()), lastText_);

        painter.setPen(lastColor_);
        painter.drawText(QRect(0, 0, lastBoundingRect_.width(), lastBoundingRect_.height()), lastText_);
    }
}
