#include "dividerline.h"

#include <QPainter>
#include "dpiscalemanager.h"

namespace PreferencesWindow {

DividerLine::DividerLine(ScalableGraphicsObject *parent, int width) : ScalableGraphicsObject(parent), width_(width)
{

}

QRectF DividerLine::boundingRect() const
{
    int height = 2*G_SCALE;
    return QRectF(0, 0, width_*G_SCALE, height);
}

void DividerLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal opacity = painter->opacity();

    int height = 2*G_SCALE;
    painter->setOpacity(0.15 * opacity);
    painter->fillRect(QRect(0, 0, width_*G_SCALE, height), QBrush(Qt::white));
}

} // namespace PreferencesWindow

