#include "twofactorauthokbutton.h"

#include <QPainter>
#include <QPainterPath>
#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"

namespace TwoFactorAuthWindow
{
TwoFactorAuthOkButton::TwoFactorAuthOkButton(ScalableGraphicsObject *parent, int width, int height)
        : ClickableGraphicsObject(parent), width_(width), height_(height), arcWidth_(16),
          arcHeight_(16), fontDescr_(14, false), text_()
{
    setButtonType(BUTTON_TYPE_ADD);
    setClickable(true);
    updateTargetFillColor();

    fillColorAnimation_.setTargetObject(this);
    fillColorAnimation_.setParent(this);
    connect(&fillColorAnimation_, SIGNAL(valueChanged(QVariant)), SLOT(onColorChanged(QVariant)));
}

QRectF TwoFactorAuthOkButton::boundingRect() const
{
    return QRectF(0,0, width_*G_SCALE, height_*G_SCALE);
}

void TwoFactorAuthOkButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    const qreal initialOpacity = painter->opacity();

    QRectF roundRect(0, 0, width_*G_SCALE, height_*G_SCALE);
    painter->setOpacity(initialOpacity);
    painter->setRenderHint(QPainter::Antialiasing, true);

    // fill
    painter->setOpacity(currentfillOpacity_ * initialOpacity);
    painter->setBrush(QBrush(currentFillColor_, Qt::SolidPattern)); // fill color
    painter->drawRoundedRect(roundRect, arcWidth_*G_SCALE, arcHeight_*G_SCALE);

    // text
    painter->setOpacity(initialOpacity);
    painter->setPen(QPen(FontManager::instance().getMidnightColor(), 1*G_SCALE));
    painter->setFont(*FontManager::instance().getFont(fontDescr_));
    QRectF textRect(0,0, width_*G_SCALE,height_*G_SCALE);
    painter->drawText(textRect, Qt::AlignCenter, text_);
}

void TwoFactorAuthOkButton::setClickable(bool clickable)
{
    ClickableGraphicsObject::setClickable(clickable);
    updateTargetFillColor();
}

void TwoFactorAuthOkButton::setClickableHoverable(bool clickable, bool hoverable)
{
    ClickableGraphicsObject::setClickableHoverable(clickable, hoverable);
    updateTargetFillColor();
}

void TwoFactorAuthOkButton::setButtonType(BUTTON_TYPE type)
{
    buttonType_ = type;
    switch (type) {
    case BUTTON_TYPE_ADD:
        text_ = tr("Add");
        break;
    case BUTTON_TYPE_LOGIN:
        text_ = tr("Login");
        break;
    }
}

void TwoFactorAuthOkButton::setFont(const FontDescr &fontDescr)
{
    fontDescr_ = fontDescr;
}

void TwoFactorAuthOkButton::onColorChanged(const QVariant &value)
{
    currentFillColor_ = value.value<QColor>();
    update();
}

void TwoFactorAuthOkButton::updateTargetFillColor()
{
    if (clickable_) {
        targetFillColor_ = FontManager::instance().getSeaGreenColor();
        currentfillOpacity_ = OPACITY_FULL;
    } else {
        targetFillColor_ = Qt::white;
        currentfillOpacity_ = 0.25;
    }
    startColorAnimation(fillColorAnimation_, currentFillColor_, targetFillColor_,
                        ANIMATION_SPEED_FAST);
    update();
}

void TwoFactorAuthOkButton::hoverEnterEvent(QGraphicsSceneHoverEvent *)
{
    if (clickable_) {
        startColorAnimation(fillColorAnimation_, currentFillColor_, QColor(255, 255, 255),
                            ANIMATION_SPEED_FAST);
    }
}

void TwoFactorAuthOkButton::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
    startColorAnimation(fillColorAnimation_, currentFillColor_, targetFillColor_,
                        ANIMATION_SPEED_FAST);
}
}  // namespace TwoFactorAuthWindow
