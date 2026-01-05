#include "versioninfoitem.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"
#include "preferenceswindow/preferencesconst.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

VersionInfoItem::VersionInfoItem(ScalableGraphicsObject *parent, const QString &caption, const QString &value)
  : BaseItem(parent, PREFERENCE_GROUP_ITEM_HEIGHT), strCaption_(caption), strValue_(value)
{
    setClickable(true);
}

void VersionInfoItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    //painter->fillRect(boundingRect(), QBrush(QColor(25,255,100)));

    QFont font = FontManager::instance().getFont(14, QFont::DemiBold);
    painter->setFont(font);
    painter->setPen(Qt::white);
    painter->drawText(boundingRect().adjusted(PREFERENCES_MARGIN_X*G_SCALE, 0, 0, 0), Qt::AlignVCenter, strCaption_);

    QFont font2 = FontManager::instance().getFont(14,  QFont::Normal);
    painter->setFont(font2);
    qreal initOpacity = painter->opacity();
    painter->setOpacity(OPACITY_SIXTY * initOpacity);
    painter->drawText(boundingRect().adjusted(0, 0, -PREFERENCES_MARGIN_X*G_SCALE, 0), Qt::AlignVCenter | Qt::AlignRight, strValue_);
}

void VersionInfoItem::setCaption(const QString &caption)
{
    strCaption_ = caption;
    update();
}

void VersionInfoItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(PREFERENCE_GROUP_ITEM_HEIGHT*G_SCALE);
}

} // namespace PreferencesWindow
