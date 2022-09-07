#include "dividerline.h"

#include <QPainter>
#include "dpiscalemanager.h"

namespace CommonGraphics {

DividerLine::DividerLine(ScalableGraphicsObject *parent, int width)
  : CommonGraphics::BaseItem(parent, DIVIDER_HEIGHT*G_SCALE, "", false, width), opacity_(OPACITY_DIVIDER_LINE)
{
}

void DividerLine::setOpacity(double opacity)
{
    opacity_ = opacity;
}

void DividerLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setOpacity(opacity_);
    painter->fillRect(boundingRect().adjusted(16*G_SCALE, 0, 0, 0).toRect(), QBrush(Qt::white));
}

} // namespace CommonGraphics
