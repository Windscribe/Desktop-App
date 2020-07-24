#include "bubblebuttonbright.h"

#include <QPainter>
#include <QPainterPath>
#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"

namespace CommonGraphics {

BubbleButtonBright::BubbleButtonBright(ScalableGraphicsObject *parent, int width, int height, int arcWidth, int arcHeight) : ClickableGraphicsObject(parent),
     width_(width), height_(height), arcWidth_(arcWidth), arcHeight_(arcHeight), fontDescr_(16, false)
{
    curOutlineFillOpacity_ = OPACITY_FULL;
    curTextOpacity_ = OPACITY_FULL;

    curFillColor_ = FontManager::instance().getSeaGreenColor();
    curTextColor_ = FontManager::instance().getMidnightColor();

    text_ = QT_TR_NOOP("Disconnect");

    connect(&outlineOpacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onOutlineOpacityChanged(QVariant)));
    connect(&textOpacityAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onTextOpacityChanged(QVariant)));

    outlineHoverAnimation_.setTargetObject(this);
    outlineHoverAnimation_.setParent(this);
    connect(&outlineHoverAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onButtonColorChanged(QVariant)));

    setClickable(true);
}

QRectF BubbleButtonBright::boundingRect() const
{
    return QRectF(0,0, width_*G_SCALE, height_*G_SCALE);
}

void BubbleButtonBright::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initialOpacity = painter->opacity();

    QRectF roundRect(0, 0, width_*G_SCALE, height_*G_SCALE);
    painter->setOpacity(curOutlineFillOpacity_ * initialOpacity);

    // outline
    QColor buttonOutlineColor = curFillColor_;
    buttonOutlineColor.setAlpha(0);
    QPen pen(buttonOutlineColor, 1*G_SCALE);
    painter->setPen(pen);

    // fill
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

void BubbleButtonBright::animateOpacityChange(double newOutlineOpacity, double newTextOpacity, int animationSpeed)
{
    startAnAnimation(outlineOpacityAnimation_, curOutlineFillOpacity_, newOutlineOpacity, animationSpeed);
    startAnAnimation(textOpacityAnimation_, curTextOpacity_, newTextOpacity, animationSpeed);
}

void BubbleButtonBright::animateHide(int animationSpeed)
{
    setClickable(false);

    animateOpacityChange(OPACITY_HIDDEN, OPACITY_HIDDEN, animationSpeed);
    startColorAnimation(outlineHoverAnimation_, curFillColor_, FontManager::instance().getSeaGreenColor(), ANIMATION_SPEED_FAST);
}

void BubbleButtonBright::animateShow(int animationSpeed)
{
    setClickable(true);

    animateOpacityChange(OPACITY_FULL, OPACITY_FULL, animationSpeed);
}

void BubbleButtonBright::quickHide()
{
    setClickable(false);

    outlineOpacityAnimation_.stop();
    textOpacityAnimation_.stop();

    curOutlineFillOpacity_ = OPACITY_HIDDEN;
    curTextOpacity_ = OPACITY_HIDDEN;
}

void BubbleButtonBright::quickShow()
{
    setClickable(true);

    outlineOpacityAnimation_.stop();
    textOpacityAnimation_.stop();

    curOutlineFillOpacity_ = OPACITY_FULL;
    curTextOpacity_ = OPACITY_FULL;
}

void BubbleButtonBright::setText(QString text)
{
    text_ = text;
    update();
}

void BubbleButtonBright::setFont(const FontDescr &fontDescr)
{
    fontDescr_ = fontDescr;
}

void BubbleButtonBright::setTextColor(QColor color)
{
    curTextColor_ = color;
    update();
}

double BubbleButtonBright::curFillOpacity()
{
    return curOutlineFillOpacity_;
}

void BubbleButtonBright::setFillOpacity(double opacity)
{
    curOutlineFillOpacity_ = opacity;
    update();
}

void BubbleButtonBright::onOutlineOpacityChanged(const QVariant &value)
{
    curOutlineFillOpacity_ = value.toDouble();
    update();
}

void BubbleButtonBright::onTextOpacityChanged(const QVariant &value)
{
    curTextOpacity_ = value.toDouble();
    update();
}

void BubbleButtonBright::onButtonColorChanged(const QVariant &value)
{
    curFillColor_ = value.value<QColor>();
    update();
}

void BubbleButtonBright::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);

    if (clickable_)
    {
        startColorAnimation(outlineHoverAnimation_, curFillColor_, QColor(255,255,255), ANIMATION_SPEED_FAST);
    }
}

void BubbleButtonBright::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);

    startColorAnimation(outlineHoverAnimation_, curFillColor_, FontManager::instance().getSeaGreenColor(), ANIMATION_SPEED_FAST);
}

}
