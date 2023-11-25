#include "escapebutton.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QCursor>
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/fontmanager.h"
#include "languagecontroller.h"
#include "dpiscalemanager.h"

namespace CommonGraphics {

EscapeButton::EscapeButton(ScalableGraphicsObject *parent) : ScalableGraphicsObject(parent), textPosition_(TEXT_POSITION_BOTTOM)
{
    iconButton_ = new IconButton(BUTTON_SIZE, BUTTON_SIZE, "ESC_BUTTON", "", this);
    connect(iconButton_, &IconButton::clicked, this, &EscapeButton::clicked);

    connect(&LanguageController::instance(), &LanguageController::languageChanged, this, &EscapeButton::onLanguageChanged);
    onLanguageChanged();
}

QRectF EscapeButton::boundingRect() const
{
    if (textPosition_ == TEXT_POSITION_BOTTOM)
    {
        return QRectF(0, 0, BUTTON_SIZE * G_SCALE, BUTTON_SIZE * 2 * G_SCALE);
    }
    else
    {
        QFont *font = FontManager::instance().getFont(10, true);
        QFontMetrics fm(*font);
        return QRectF(0, 0, (16 + BUTTON_SIZE)*G_SCALE + fm.horizontalAdvance(tr("ESC")), BUTTON_SIZE*G_SCALE);
    }
}

void EscapeButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QFont *font = FontManager::instance().getFont(10, true);
    painter->setFont(*font);
    painter->setPen(QColor(103, 106, 118));

    if (textPosition_ == TEXT_POSITION_BOTTOM)
    {
        painter->drawText(boundingRect().adjusted(0, BUTTON_SIZE*G_SCALE, 0, 0),
                          Qt::AlignVCenter | Qt::AlignHCenter, tr("ESC"));
    }
    else if (textPosition_ == TEXT_POSITION_LEFT)
    {
        painter->drawText(boundingRect().adjusted(0, 0, -BUTTON_SIZE*G_SCALE, 0),
                          Qt::AlignVCenter | Qt::AlignHCenter, tr("ESC"));
    }
}

void EscapeButton::setClickable(bool isClickable)
{
    iconButton_->setClickable(isClickable);
}

void EscapeButton::setTextPosition(TEXT_POSITION loc)
{
    textPosition_ = loc;
    if (textPosition_ == TEXT_POSITION_LEFT)
    {
        iconButton_->setPos(boundingRect().width() - BUTTON_SIZE*G_SCALE, 0);
    }
    else if (textPosition_ == TEXT_POSITION_BOTTOM)
    {
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

} // namespace CommonGraphics
