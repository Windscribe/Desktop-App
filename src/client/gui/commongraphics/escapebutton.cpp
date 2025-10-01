#include "escapebutton.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QCursor>
#include "commongraphics/commongraphics.h"
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/fontmanager.h"
#include "languagecontroller.h"
#include "dpiscalemanager.h"

namespace CommonGraphics {

EscapeButton::EscapeButton(ScalableGraphicsObject *parent) : ScalableGraphicsObject(parent), textPosition_(TEXT_POSITION_BOTTOM)
{
    setAcceptHoverEvents(true);

    iconButton_ = new IconButton(BUTTON_SIZE, BUTTON_SIZE, "ESC_BUTTON", "", this);
    connect(iconButton_, &IconButton::clicked, this, &EscapeButton::clicked);

    isClickable_ = true;
    currentOpacity_ = OPACITY_HALF;
    connect(&textOpacityAnim_, &QVariantAnimation::valueChanged, this, &EscapeButton::onTextOpacityChanged);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &EscapeButton::onLanguageChanged);
    onLanguageChanged();
}

QRectF EscapeButton::boundingRect() const
{
    if (textPosition_ == TEXT_POSITION_BOTTOM) {
        return QRectF(0, 0, BUTTON_SIZE * G_SCALE, (BUTTON_SIZE + 15)*G_SCALE);
    } else {
        QFont font = FontManager::instance().getFont(9, QFont::Medium);
        QFontMetrics fm(font);
        return QRectF(0, 0, (16 + BUTTON_SIZE)*G_SCALE + fm.horizontalAdvance(tr("ESC")), BUTTON_SIZE*G_SCALE);
    }
}

void EscapeButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QFont font = FontManager::instance().getFont(9, QFont::Medium);
    painter->setFont(font);

    painter->setPen(Qt::white);
    painter->setOpacity(currentOpacity_);
    if (textPosition_ == TEXT_POSITION_BOTTOM) {
        painter->drawText(boundingRect().adjusted(0, BUTTON_SIZE*G_SCALE, 0, 0),
                          Qt::AlignVCenter | Qt::AlignHCenter, tr("ESC"));
    } else if (textPosition_ == TEXT_POSITION_LEFT) {
        painter->drawText(boundingRect().adjusted(0, 0, -BUTTON_SIZE*G_SCALE, 0),
                          Qt::AlignVCenter | Qt::AlignHCenter, tr("ESC"));
    }
}

void EscapeButton::setClickable(bool isClickable)
{
    isClickable_ = isClickable;
    iconButton_->setClickable(isClickable);
}

void EscapeButton::setTextPosition(TEXT_POSITION loc)
{
    textPosition_ = loc;
    if (textPosition_ == TEXT_POSITION_LEFT) {
        iconButton_->setPos(boundingRect().width() - BUTTON_SIZE*G_SCALE, 0);
    } else if (textPosition_ == TEXT_POSITION_BOTTOM) {
        iconButton_->setPos(0, 0);
    }
    update();
}

void EscapeButton::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
    setTextPosition(textPosition_);
}

void EscapeButton::onLanguageChanged() {
    setTextPosition(textPosition_);
    update();
}

void EscapeButton::onTextOpacityChanged(const QVariant &value)
{
    currentOpacity_ = value.toDouble();
    update();
}

void EscapeButton::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    hover();
}

void EscapeButton::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    hover();
}

void EscapeButton::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    if (isClickable_) {
        setCursor(QCursor(Qt::ArrowCursor));
        startAnAnimation<double>(textOpacityAnim_, currentOpacity_, OPACITY_HALF, ANIMATION_SPEED_FAST);
        iconButton_->unhover();
    }
}

void EscapeButton::hover()
{
    if (isClickable_) {
        setCursor(QCursor(Qt::PointingHandCursor));
        startAnAnimation<double>(textOpacityAnim_, currentOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
        iconButton_->hover();
    }
}

void EscapeButton::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (isClickable_) {
        emit clicked();
    }
}

} // namespace CommonGraphics
