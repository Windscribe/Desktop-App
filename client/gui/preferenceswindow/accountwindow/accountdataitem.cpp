#include "accountdataitem.h"

#include <QPainter>
#include "dpiscalemanager.h"
#include "graphicresources/fontmanager.h"
#include "preferenceswindow/preferencesconst.h"

namespace PreferencesWindow {

AccountDataItem::AccountDataItem(ScalableGraphicsObject *parent, const QString &value1, const QString &value2)
  : BaseItem(parent, PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE), value1_(value1), value2_(value2)
{
}

void AccountDataItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setFont(*FontManager::instance().getFont(12, true));
    painter->setOpacity(OPACITY_FULL);
    painter->setPen(Qt::white);
    painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN*G_SCALE, PREFERENCES_MARGIN*G_SCALE, -PREFERENCES_MARGIN*G_SCALE, -PREFERENCES_MARGIN*G_SCALE), Qt::AlignLeft, value1_);

    painter->setFont(*FontManager::instance().getFont(12, false));
    painter->setOpacity(OPACITY_HALF);
    painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN*G_SCALE, PREFERENCES_MARGIN*G_SCALE, -PREFERENCES_MARGIN*G_SCALE, -PREFERENCES_MARGIN*G_SCALE), Qt::AlignRight, value2_);
}

void AccountDataItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE);
}

void AccountDataItem::setValue2(const QString &value)
{
    value2_ = value;
    update();
}

} // namespace PreferencesWindow
