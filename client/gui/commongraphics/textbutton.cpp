#include "textbutton.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QCursor>
#include "graphicresources/fontmanager.h"
#include "commongraphics.h"
#include "dpiscalemanager.h"

namespace CommonGraphics {

TextButton::TextButton(QString text, const FontDescr &fd, QColor color, bool bSetClickable, ScalableGraphicsObject *parent, int addWidth, bool bDrawWithShadow) : ClickableGraphicsObject(parent),
    text_(text), color_(color), fontDescr_(fd), width_(0), height_(0), addWidth_(addWidth), margin_(MARGIN_HEIGHT),
    curTextOpacity_(OPACITY_UNHOVER_TEXT), unhoverOpacity_(OPACITY_UNHOVER_TEXT), isHovered_(false),
    textAlignment_(Qt::AlignLeft | Qt::AlignVCenter), textUnderline_(false), unhoverOnClick_(true)
{
    if (bDrawWithShadow) {
        textShadow_.reset(new TextShadow());
    }

    connect(&textOpacityAnimation_, &QVariantAnimation::valueChanged, this, &TextButton::onTextHoverOpacityChanged);

    // Direct constructor call to ClickableGraphicsObject::setCursor() crashes due to "pure virtual function call" for some reason only in this class...
    setClickable(bSetClickable);

    recalcBoundingRect();
}

QRectF TextButton::boundingRect() const
{
    return QRectF(0, 0, width_, height_ + margin_*G_SCALE );
}

void TextButton::updateScaling()
{
    ClickableGraphicsObject::updateScaling();
    recalcBoundingRect();
    update();
}

void TextButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initialOpacity = painter->opacity();

    painter->setOpacity(curTextOpacity_ * initialOpacity);
    QFont font = getFont();

    // Only elide the text if we have been constrained to a maximum width.
    QFontMetrics fm(font);
    QString elidedText = text_;
    if (maxWidth_ > 0 && width_ < fm.horizontalAdvance(text_)) {
        elidedText = fm.elidedText(text_, Qt::ElideRight, width_, 0);
    }

    if (textShadow_) {
        textShadow_->drawText(painter, boundingRect().toRect(), textAlignment_, elidedText, &font, color_);
    }
    else {
        painter->setFont(font);
        painter->setPen(color_);
        painter->drawText(boundingRect(), textAlignment_ , elidedText);
    }
}

double TextButton::getOpacity() const
{
    return curTextOpacity_;
}

void TextButton::setColor(QColor color)
{
    color_ = color;
}

void TextButton::setUnderline(bool on)
{
    textUnderline_ = on;
}

void TextButton::setMarginHeight(int height)
{
    margin_ = height;
    update();
}

void TextButton::quickSetOpacity(double opacity)
{
    curTextOpacity_ = opacity;
    update();
}

void TextButton::quickHide()
{
    setClickable(false);
    curTextOpacity_ = OPACITY_HIDDEN;
}

void TextButton::animateShow(int animationSpeed)
{
    setClickable(true);
    startAnAnimation<double>(textOpacityAnimation_, curTextOpacity_, unhoverOpacity_, animationSpeed);
}

void TextButton::animateHide(int animationSpeed)
{
    setClickable(false);

    startAnAnimation<double>(textOpacityAnimation_, curTextOpacity_, OPACITY_HIDDEN, animationSpeed);
}

QFont TextButton::getFont() const
{
    QFont font(*FontManager::instance().getFont(fontDescr_));
    font.setUnderline(textUnderline_);
    return font;
}

int TextButton::getWidth() const
{
    return textWidth(text_, getFont());
}

void TextButton::setText(QString text)
{
    text_ = text;
    recalcBoundingRect();
    update();
}

QString TextButton::text() const
{
    return text_;
}

void TextButton::setCurrentOpacity(double opacity)
{
    curTextOpacity_ = opacity;
    update();
}

void TextButton::setUnhoverOpacity(double unhoverOpacity)
{
    unhoverOpacity_ = unhoverOpacity;
}

void TextButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (clickable_)
    {
        if (event->button() == Qt::LeftButton)
        {
            if (pressed_)
            {
                pressed_ = false;

                if (contains(event->pos()))
                {
                    setCursor(Qt::ArrowCursor);
                    if (unhoverOnClick_) {
                        unhover();
                    }
                    emit clicked();
                }
            }
        }
    }
    else
    {
        event->ignore();
        return;
    }
}

void TextButton::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    hover();
}

void TextButton::hover()
{
    if (hoverable_) {
        startAnAnimation<double>(textOpacityAnimation_, curTextOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
        if (clickable_) {
            setCursor(Qt::PointingHandCursor);
        }
        isHovered_ = true;
        emit hoverEnter();
    }
}

void TextButton::unhover()
{
    if (!isHovered_) {
        return;
    }

    startAnAnimation<double>(textOpacityAnimation_, curTextOpacity_, unhoverOpacity_, ANIMATION_SPEED_FAST);
    setCursor(Qt::ArrowCursor);

    isHovered_ = false;
    emit hoverLeave();
}

void TextButton::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    unhover();
}

void TextButton::onTextHoverOpacityChanged(const QVariant &value)
{
    curTextOpacity_ = value.toDouble();
    update();
}

void TextButton::recalcBoundingRect()
{
    QFontMetrics fm(getFont());
    prepareGeometryChange();
    if (maxWidth_ > 0) {
        width_ = maxWidth_;
    }
    else {
        width_ = fm.boundingRect(text_).width() + addWidth_*G_SCALE;
    }
    height_ = fm.height();
}

void TextButton::setTextAlignment(int alignment)
{
    textAlignment_ = alignment;
    update();
}

void TextButton::setUnhoverOnClick(bool on)
{
    unhoverOnClick_ = on;
}

void TextButton::setMaxWidth(int width)
{
    maxWidth_ = width;
    update();
}

}
