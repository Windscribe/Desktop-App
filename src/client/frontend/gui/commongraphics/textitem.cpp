#include "textitem.h"

#include <QPainter>
#include <QFontMetrics>
#include "graphicresources/fontmanager.h"
#include "commongraphics.h"
#include "dpiscalemanager.h"

namespace CommonGraphics {

TextItem::TextItem(ScalableGraphicsObject *parent, const QString &text, const FontDescr &fd, QColor color, int maxWidth) : ScalableGraphicsObject(parent),
    text_(text), fontDescr_(fd), color_(color),  maxWidth_(maxWidth)
{
    recalcBoundingRect();
}

QRectF TextItem::boundingRect() const
{
    return QRectF(0, 0, width_, height_);
}

void TextItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initialOpacity = painter->opacity();
    painter->setOpacity(initialOpacity);

    QFont font = getFont();
    QFontMetrics fm(font);

    painter->setFont(font);
    painter->setPen(color_);

    // Draw text aligned to the left, with word wrapping if necessary
    QRect textRect(0, 0, width_, height_);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, text_);
}

void TextItem::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    recalcBoundingRect();
    update();
}

void TextItem::setText(const QString &text)
{
    text_ = text;
    recalcBoundingRect();
    update();
}

void TextItem::setMaxWidth(int maxWidth)
{
    maxWidth_ = maxWidth;
    recalcBoundingRect();
    update();
}

int TextItem::getHeight() const
{
    return height_;
}

QFont TextItem::getFont() const
{
    return FontManager::instance().getFont(fontDescr_);
}

void TextItem::recalcBoundingRect()
{
    QFont font = getFont();
    QFontMetrics fm(font);    prepareGeometryChange();

    // Width is constrained by maxWidth_
    width_ = maxWidth_;

    // Calculate height based on text wrapping within maxWidth_
    QRect boundingRect = fm.boundingRect(0, 0, maxWidth_, 0, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, text_);
    height_ = boundingRect.height();
}

}
