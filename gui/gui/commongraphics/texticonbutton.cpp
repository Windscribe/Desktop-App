#include "texticonbutton.h"

#include <QPainter>
#include <QCursor>
#include "graphicresources/fontmanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "commongraphics.h"
#include "dpiscalemanager.h"


namespace CommonGraphics {

TextIconButton::TextIconButton(int spacerWidth, const QString text, const QString &imagePath, ScalableGraphicsObject *parent, bool bSetClickable) : ClickableGraphicsObject(parent),
    width_(0), height_(0), spacerWidth_(spacerWidth), iconPath_(imagePath), text_(text),
    curTextOpacity_(OPACITY_UNHOVER_TEXT), curIconOpacity_(OPACITY_UNHOVER_ICON_TEXT_DARK),
    fontDescr_(16, true)
{
    recalcHeight();
    recalcWidth();

    connect(&textOpacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onTextOpacityChanged(QVariant)));
    connect(&iconOpacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onIconOpacityChanged(QVariant)));

    setClickable(bSetClickable);
}

QRectF TextIconButton::boundingRect() const
{
    return QRectF(0, 0, width_, height_);
}

void TextIconButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initOpacity = painter->opacity();

    // display text
    QFont *font = FontManager::instance().getFont(fontDescr_);
    painter->setOpacity(curTextOpacity_ * initOpacity);
    painter->setFont(*font);
    painter->setPen(Qt::white);
    painter->drawText(0, height_/2 + CommonGraphics::textHeight(*font)/8 + 2*G_SCALE, text_);

    // Right Arrow
    painter->setOpacity(curIconOpacity_ * initOpacity);
    IndependentPixmap *iconPixmap = ImageResourcesSvg::instance().getIndependentPixmap(iconPath_);
    iconPixmap->draw(CommonGraphics::textWidth(text_, *font) + spacerWidth_, 0, painter);

}

void TextIconButton::setFont(const FontDescr &fontDescr)
{
   fontDescr_ = fontDescr;
   recalcWidth();
   recalcHeight();
   update();
}

void TextIconButton::animateHide()
{
    startAnAnimation<double>(textOpacityAnimation_, curTextOpacity_, OPACITY_HIDDEN, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(iconOpacityAnimation_, curIconOpacity_, OPACITY_HIDDEN, ANIMATION_SPEED_FAST);
}

void TextIconButton::animateShow()
{
    startAnAnimation<double>(textOpacityAnimation_, curTextOpacity_, OPACITY_UNHOVER_TEXT, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(iconOpacityAnimation_, curIconOpacity_, OPACITY_UNHOVER_ICON_TEXT_DARK, ANIMATION_SPEED_FAST);
}

const QString TextIconButton::text()
{
    return text_;
}

void TextIconButton::setText(QString text)
{
    text_ = text;
    recalcWidth();
    update();
}

int TextIconButton::getWidth()
{
    return width_;
}

int TextIconButton::getHeight()
{
    return height_;
}

void TextIconButton::updateScaling()
{
    ClickableGraphicsObject::updateScaling();
    recalcWidth();
    recalcHeight();
}

void TextIconButton::setOpacityByFactor(double opacityFactor)
{
    curTextOpacity_ = opacityFactor * OPACITY_UNHOVER_TEXT;
    curIconOpacity_ = opacityFactor * OPACITY_UNHOVER_ICON_TEXT_DARK;
    update();
}

void TextIconButton::hoverEnterEvent(QGraphicsSceneHoverEvent * /*event*/)
{
    if (clickable_)
    {
        startAnAnimation<double>(textOpacityAnimation_, curTextOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
        startAnAnimation<double>(iconOpacityAnimation_, curIconOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
    }
    emit hoverEnter();
}

void TextIconButton::hoverLeaveEvent(QGraphicsSceneHoverEvent * /*event*/)
{
    startAnAnimation<double>(textOpacityAnimation_, curTextOpacity_, OPACITY_UNHOVER_TEXT, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(iconOpacityAnimation_, curIconOpacity_, OPACITY_UNHOVER_ICON_TEXT_DARK, ANIMATION_SPEED_FAST);

    emit hoverLeave();
}

void TextIconButton::onTextOpacityChanged(const QVariant &value)
{
    curTextOpacity_ = value.toDouble();
    update();
}

void TextIconButton::onIconOpacityChanged(const QVariant &value)
{
    curIconOpacity_ = value.toDouble();
    update();
}

void TextIconButton::recalcWidth()
{
    IndependentPixmap *iconPixmap = ImageResourcesSvg::instance().getIndependentPixmap(iconPath_);

    QFont *font = FontManager::instance().getFont(fontDescr_);
    int newWidth = iconPixmap->width();
    newWidth += spacerWidth_;
    newWidth += CommonGraphics::textWidth(text_, *font);

    if (newWidth != width_)
    {
        prepareGeometryChange();
        width_ = newWidth;
        emit widthChanged(newWidth);
    }
}

void TextIconButton::recalcHeight()
{
    QFont *font = FontManager::instance().getFont(fontDescr_);

    const int newHeight = CommonGraphics::textHeight(*font);

    if (newHeight != height_)
    {
        prepareGeometryChange();
        height_ = newHeight;
        emit heightChanged(newHeight);
    }
}

QString TextIconButton::spacer()
{
    QString spacer = "";
    for (int i=0; i < spacerWidth_; i++)
    {
        spacer += " ";
    }
    return spacer;
}

}
