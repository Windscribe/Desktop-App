#include "bubblebuttondark.h"

#include <QPainter>
#include <QPainterPath>
#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"

namespace CommonGraphics {

BubbleButtonDark::BubbleButtonDark(ScalableGraphicsObject *parent, int width, int height, int arcWidth, int arcHeight) : ClickableGraphicsObject(parent),
    width_(width), height_(height), arcWidth_(arcWidth), arcHeight_(arcHeight), fontDescr_(16, false),
    text_(QT_TR_NOOP("Connect")), curOutlineFillOpacity_(OPACITY_UNHOVER_TEXT),
    curTextOpacity_(OPACITY_FULL), curOutlineColor_(255, 255, 255),
    curFillColor_(FontManager::instance().getMidnightColor()), curTextColor_(255, 255, 255)
{
    connect(&outlineOpacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onOutlineOpacityChanged(QVariant)));
    connect(&textOpacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onTextOpacityChanged(QVariant)));

    textColorAnimation_.setTargetObject(this);
    textColorAnimation_.setParent(this);
    textColorAnimation_.setPropertyName("textColor");

    fillColorAnimation_.setTargetObject(this);
    fillColorAnimation_.setParent(this);
    fillColorAnimation_.setPropertyName("fillColor");

    connect(&textColorAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onTextColorChanged(QVariant)));
    connect(&fillColorAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onFillColorChanged(QVariant)));

    setClickable(true);
}

QRectF BubbleButtonDark::boundingRect() const
{
    return QRectF(0,0, width_*G_SCALE, height_*G_SCALE);
}

void BubbleButtonDark::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initialOpacity = painter->opacity();

    // outline
    painter->setOpacity(curOutlineFillOpacity_ * initialOpacity);
    QColor outlineColor = curOutlineColor_;
    outlineColor.setAlpha(curTextOpacity_  * initialOpacity * 255);
    QPen pen(outlineColor, 2*G_SCALE);
    painter->setPen(pen); // thick line

    // fill
    QRectF roundRect(0, 0, width_*G_SCALE, height_*G_SCALE);
    painter->setBrush(QBrush(curFillColor_, Qt::SolidPattern)); // fill color
    painter->drawRoundedRect(roundRect, arcWidth_*G_SCALE, arcHeight_*G_SCALE);

    // text
    painter->setOpacity(curTextOpacity_ * initialOpacity);
    painter->setPen(QPen(curTextColor_, 1*G_SCALE));
    QFont *font = FontManager::instance().getFont(fontDescr_);
    painter->setFont(*font);
    QRectF textRect(0,0, width_*G_SCALE,height_*G_SCALE);
    painter->drawText(textRect, Qt::AlignCenter, tr(text_.toStdString().c_str()));
}

void BubbleButtonDark::animateColorChange(QColor textColor, QColor outlineColor, int animationSpeed)
{
    startColorAnimation(textColorAnimation_, curTextColor_, textColor, animationSpeed);
    startColorAnimation(fillColorAnimation_, curFillColor_, outlineColor, animationSpeed);
}

void BubbleButtonDark::animateOpacityChange(double newOutlineOpacity, double newTextOpacity, int animationSpeed)
{
    startAnAnimation(outlineOpacityAnimation_, curOutlineFillOpacity_, newOutlineOpacity, animationSpeed);
    startAnAnimation(textOpacityAnimation_, curTextOpacity_, newTextOpacity, animationSpeed);
}

void BubbleButtonDark::animateHide(int animationSpeed)
{
    setClickable(false);
    animateOpacityChange(OPACITY_HIDDEN, OPACITY_HIDDEN, animationSpeed);
}

void BubbleButtonDark::animateShow(int animationSpeed)
{
    setClickable(true);

    animateOpacityChange(OPACITY_UNHOVER_ICON_STANDALONE, OPACITY_FULL, animationSpeed);

    startColorAnimation(textColorAnimation_, curTextColor_, QColor(255,255,255), animationSpeed);
    startColorAnimation(fillColorAnimation_, curFillColor_, FontManager::instance().getMidnightColor(), animationSpeed);
}

void BubbleButtonDark::quickHide()
{
    setClickable(false);

    outlineOpacityAnimation_.stop();
    textOpacityAnimation_.stop();
    textColorAnimation_.stop();
    fillColorAnimation_.stop();

    curOutlineFillOpacity_ = OPACITY_HIDDEN;
    curTextOpacity_ = OPACITY_HIDDEN;

    // unhover colors
    curTextColor_ = QColor(255,255,255);
    curFillColor_ = FontManager::instance().getMidnightColor();
}

void BubbleButtonDark::quickShow()
{
    setClickable(true);

    outlineOpacityAnimation_.stop();
    textOpacityAnimation_.stop();

    curOutlineFillOpacity_ = OPACITY_UNHOVER_ICON_STANDALONE;
    curTextOpacity_ = OPACITY_FULL;
}

void BubbleButtonDark::setText(QString text)
{
    text_ = text;
}

void BubbleButtonDark::setFillColor(QColor newFillColor)
{
    curFillColor_ = newFillColor;
}

void BubbleButtonDark::setTextColor(QColor newTextColor)
{
    curTextColor_ = newTextColor;
}

void BubbleButtonDark::setFont(const FontDescr &fontDescr)
{
    fontDescr_ = fontDescr;
}

void BubbleButtonDark::onOutlineOpacityChanged(const QVariant &value)
{
    curOutlineFillOpacity_ = value.toDouble();

    update();
}

void BubbleButtonDark::onTextOpacityChanged(const QVariant &value)
{
    curTextOpacity_ = value.toDouble();

    update();
}

void BubbleButtonDark::onTextColorChanged(const QVariant &value)
{
    curTextColor_ = value.value<QColor>();

    update();
}

void BubbleButtonDark::onFillColorChanged(const QVariant &value)
{
    curFillColor_ = value.value<QColor>();

    update();
}

void BubbleButtonDark::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);

    if (hoverable_)
    {
        if (clickable_)
        {
            startColorAnimation(textColorAnimation_, curTextColor_, FontManager::instance().getMidnightColor(), ANIMATION_SPEED_FAST);
            startColorAnimation(fillColorAnimation_, curFillColor_, QColor(255,255,255), ANIMATION_SPEED_FAST);
            startAnAnimation(outlineOpacityAnimation_, curOutlineFillOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
        }

        emit hoverEnter();
    }
}

void BubbleButtonDark::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);

    startColorAnimation(textColorAnimation_, curTextColor_, QColor(255,255,255), ANIMATION_SPEED_FAST);
    startColorAnimation(fillColorAnimation_, curFillColor_, FontManager::instance().getMidnightColor(), ANIMATION_SPEED_FAST);
    startAnAnimation(outlineOpacityAnimation_, curOutlineFillOpacity_, OPACITY_UNHOVER_TEXT, ANIMATION_SPEED_FAST);

    emit hoverLeave();
}

}
