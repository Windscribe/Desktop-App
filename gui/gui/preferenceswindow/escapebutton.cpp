#include "escapebutton.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QCursor>
#include "graphicresources/imageresourcessvg.h"
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

EscapeButton::EscapeButton(ScalableGraphicsObject *parent) : ScalableGraphicsObject(parent)
{
    iconButton_ = new IconButton(24, 24, "preferences/ESC_BUTTON", this);
    connect(iconButton_, SIGNAL(clicked()), SIGNAL(clicked()));
}

QRectF EscapeButton::boundingRect() const
{
    return QRectF(0, 0, 24 * G_SCALE, (24 + 24) * G_SCALE);
}

void EscapeButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QFont *font = FontManager::instance().getFont(10, true);
    painter->setFont(*font);
    painter->setPen(QColor(103, 106, 118));
    painter->drawText(QRect(0, 24 * G_SCALE, 24 * G_SCALE, 24 * G_SCALE), Qt::AlignVCenter | Qt::AlignHCenter, tr("ESC"));
}

void EscapeButton::setClickable(bool isClickable)
{
    iconButton_->setClickable(isClickable);
}

} // namespace PreferencesWindow
