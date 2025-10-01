#include "dividerline.h"

#include <QPainter>
#include "dpiscalemanager.h"

namespace CommonGraphics {

DividerLine::DividerLine(ScalableGraphicsObject *parent, int width, int startX)
  : CommonGraphics::BaseItem(parent, DIVIDER_HEIGHT*G_SCALE, "", false, width), opacity_(OPACITY_FULL), startX_(startX)
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
    painter->fillRect(boundingRect().adjusted(startX_*G_SCALE, 0, 0, 0).toRect(), QColor(9, 15, 25));
}

void DividerLine::updateScaling()
{
    setHeight(DIVIDER_HEIGHT*G_SCALE);
}

} // namespace CommonGraphics
