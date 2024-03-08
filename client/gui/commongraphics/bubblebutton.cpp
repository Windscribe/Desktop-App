#include "bubblebutton.h"

#include <QPainter>
#include <QPainterPath>
#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"
#include "utils/logger.h"
#include "dpiscalemanager.h"

namespace CommonGraphics {

BubbleButton::BubbleButton(ScalableGraphicsObject *parent, Style style, int width, int height, int radius)
    : ClickableGraphicsObject(parent), style_(style), width_(width), height_(height), radius_(radius),
      fontDescr_(16, false), text_(""), curOutlineFillOpacity_(0), curTextOpacity_(OPACITY_FULL)
{
    setStyle(style);
    connect(&textOpacityAnimation_, &QVariantAnimation::valueChanged, this, &BubbleButton::onTextOpacityChanged);
    connect(&outlineOpacityAnimation_, &QVariantAnimation::valueChanged, this, &BubbleButton::onOutlineOpacityChanged);
    connect(&textColorAnimation_, &QVariantAnimation::valueChanged, this, &BubbleButton::onTextColorChanged);
    connect(&fillColorAnimation_, &QVariantAnimation::valueChanged, this, &BubbleButton::onFillColorChanged);
    setClickable(true);
}

QRectF BubbleButton::boundingRect() const
{
    return QRectF(0, 0, width_*G_SCALE, height_*G_SCALE);
}

void BubbleButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    if (style_ == kOutline) {
        // outline
        QColor outlineColor = QColor(255, 255, 255);
        outlineColor.setAlpha(curTextOpacity_ * curOutlineFillOpacity_ * 255);
        QPen pen(outlineColor, 2*G_SCALE);
        painter->setPen(pen); // thick line
    } else {
        painter->setPen(Qt::NoPen);
    }

    // fill
    painter->setOpacity(curTextOpacity_);
    QRectF roundRect(0, 0, width_*G_SCALE, height_*G_SCALE);
    painter->setBrush(QBrush(curFillColor_, Qt::SolidPattern)); // fill color
    painter->drawRoundedRect(roundRect, radius_*G_SCALE, radius_*G_SCALE);

    // text
    painter->setOpacity(curTextOpacity_);
    painter->setPen(QPen(curTextColor_, 1*G_SCALE));
    QFont *font = FontManager::instance().getFont(fontDescr_);
    painter->setFont(*font);
    QRectF textRect(0, 0, width_*G_SCALE, height_*G_SCALE);
    painter->drawText(textRect, Qt::AlignCenter, text_);
}

void BubbleButton::setStyle(Style style)
{
    style_ = style;

    if (style == kBright) {
        curFillColor_ = FontManager::instance().getSeaGreenColor();
        curTextColor_ = FontManager::instance().getMidnightColor();
    } else if (style == kDark) {
        curFillColor_ = QColor(255, 255, 255, 35);
        curTextColor_ = QColor(255, 255, 255);
    } else if (style == kOutline) {
        curFillColor_ = FontManager::instance().getMidnightColor();
        curTextColor_ = QColor(255, 255, 255);
        curOutlineFillOpacity_ = OPACITY_UNHOVER_ICON_STANDALONE;
    }

    fillColor_ = curFillColor_;
    textColor_ = curTextColor_;

    update();
}

void BubbleButton::animateHide(int animationSpeed)
{
    setClickable(false);
    startAnAnimation(outlineOpacityAnimation_, curOutlineFillOpacity_, OPACITY_HIDDEN, animationSpeed);
    startAnAnimation(textOpacityAnimation_, curTextOpacity_, OPACITY_HIDDEN, animationSpeed);
}

void BubbleButton::animateShow(int animationSpeed)
{
    setClickable(true);
    startAnAnimation(outlineOpacityAnimation_, curOutlineFillOpacity_, OPACITY_UNHOVER_ICON_STANDALONE, animationSpeed);
    startAnAnimation(textOpacityAnimation_, curTextOpacity_, OPACITY_FULL, animationSpeed);
    startAnAnimation(textColorAnimation_, curTextColor_, textColor_, animationSpeed);
    startAnAnimation(fillColorAnimation_, curFillColor_, fillColor_, animationSpeed);
}

void BubbleButton::quickHide()
{
    setClickable(false);

    outlineOpacityAnimation_.stop();
    textColorAnimation_.stop();
    textColorAnimation_.stop();
    fillColorAnimation_.stop();

    curOutlineFillOpacity_ = OPACITY_HIDDEN;
    curTextOpacity_ = OPACITY_HIDDEN;

    // unhover colors
    curTextColor_ = textColor_;
    curFillColor_ = fillColor_;
    update();
}

void BubbleButton::quickShow()
{
    setClickable(true);

    outlineOpacityAnimation_.stop();
    textColorAnimation_.stop();

    curOutlineFillOpacity_ = OPACITY_UNHOVER_ICON_STANDALONE;
    curTextOpacity_ = OPACITY_FULL;
}

void BubbleButton::setText(QString text)
{
    text_ = text;
    update();
}

void BubbleButton::setFillColor(QColor newFillColor)
{
    fillColor_ = newFillColor;
    curFillColor_ = newFillColor;
    update();
}

void BubbleButton::setTextColor(QColor newTextColor)
{
    curTextColor_ = newTextColor;
    update();
}

void BubbleButton::setFont(const FontDescr &fontDescr)
{
    fontDescr_ = fontDescr;
    update();
}

void BubbleButton::setWidth(int width)
{
    width_ = width;
    update();
}

void BubbleButton::onTextOpacityChanged(const QVariant &value)
{
    curTextOpacity_ = value.toDouble();
    update();
}

void BubbleButton::onOutlineOpacityChanged(const QVariant &value)
{
    curOutlineFillOpacity_ = value.toDouble();
    update();
}

void BubbleButton::onTextColorChanged(const QVariant &value)
{
    curTextColor_ = value.value<QColor>();

    update();
}

void BubbleButton::onFillColorChanged(const QVariant &value)
{
    curFillColor_ = value.value<QColor>();
    update();
}

void BubbleButton::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    hover();
}

void BubbleButton::hover()
{
    if (hoverable_) {
        if (clickable_) {
            startAnAnimation(textColorAnimation_, curTextColor_, FontManager::instance().getMidnightColor(), ANIMATION_SPEED_FAST);
            startAnAnimation(fillColorAnimation_, curFillColor_, QColor(255, 255, 255), ANIMATION_SPEED_FAST);
            startAnAnimation(outlineOpacityAnimation_, curOutlineFillOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
        }

        emit hoverEnter();
    }
}

void BubbleButton::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    unhover();
}

void BubbleButton::unhover()
{
    startAnAnimation(textColorAnimation_, curTextColor_, textColor_, ANIMATION_SPEED_FAST);
    startAnAnimation(fillColorAnimation_, curFillColor_, fillColor_, ANIMATION_SPEED_FAST);
    startAnAnimation(outlineOpacityAnimation_, curOutlineFillOpacity_, OPACITY_UNHOVER_ICON_STANDALONE, ANIMATION_SPEED_FAST);

    emit hoverLeave();
}

}