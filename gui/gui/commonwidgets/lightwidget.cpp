#include "lightwidget.h"

#include "dpiscalemanager.h"
#include "commongraphics/commongraphics.h"
LightWidget::LightWidget(QObject *parent) : QObject(parent)
  , font_(QFont())
  , rect_(QRect())
  , text_("")
  , icon_("")
  , hovering_(false)
  , opacity_(OPACITY_HALF)
{

}

LightWidget::~LightWidget()
{

}

void LightWidget::setRect(const QRect &r)
{
    rect_ = r;
}

const QRect &LightWidget::rect()
{
    return rect_;
}

void LightWidget::setHovering(bool hovering)
{
    if (hovering_ != hovering)
    {
        hovering_ = hovering;
        emit hoveringChanged(hovering);
    }
}

void LightWidget::setText(const QString &text)
{
    text_ = text;
}

const QString &LightWidget::text()
{
    return text_;
}

void LightWidget::setFont(const QFont &font)
{
    font_ = font;
}

const QFont &LightWidget::font()
{
    return font_;
}

int LightWidget::truncatedTextWidth(int maxWidth)
{
    Q_ASSERT(font_ != QFont());
    return CommonGraphics::textWidth(CommonGraphics::maybeTruncatedText(text_, font_, maxWidth), font_);
}

int LightWidget::textWidth()
{
    Q_ASSERT(font_ != QFont());
    return CommonGraphics::textWidth(text_, font_);
}

int LightWidget::textHeight()
{
    Q_ASSERT(font_ != QFont());
    return CommonGraphics::textHeight(font_);
}

void LightWidget::setIcon(const QString &icon)
{
    icon_ = icon;
}

const QString &LightWidget::icon()
{
    return icon_;
}

void LightWidget::setOpacity(double opacity)
{
    opacity_ = opacity;
}

double LightWidget::opacity()
{
    return opacity_;
}
