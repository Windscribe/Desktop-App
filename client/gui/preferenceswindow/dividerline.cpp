#include "dividerline.h"

#include <QPainter>
#include "dpiscalemanager.h"
#include "preferencesconst.h"

namespace PreferencesWindow {

DividerLine::DividerLine(ScalableGraphicsObject *parent)
  : CommonGraphics::BaseItem(parent, DIVIDER_HEIGHT*G_SCALE)
{
}

void DividerLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setOpacity(OPACITY_DIVIDER_LINE);
    painter->fillRect(boundingRect().adjusted(PREFERENCES_MARGIN*G_SCALE, 0, 0, 0).toRect(), QBrush(Qt::white));
}

} // namespace PreferencesWindow
