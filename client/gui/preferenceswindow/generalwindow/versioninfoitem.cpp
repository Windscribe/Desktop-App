#include "versioninfoitem.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

VersionInfoItem::VersionInfoItem(ScalableGraphicsObject *parent, const QString &caption, const QString &value) : BaseItem(parent, 50),
    strCaption_(caption), strValue_(value)
{

}

void VersionInfoItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    //painter->fillRect(boundingRect(), QBrush(QColor(25,255,100)));

    QFont *font = FontManager::instance().getFont(12, true);
    painter->setFont(*font);
    painter->setPen(QColor(255, 255, 255));
    painter->drawText(boundingRect().adjusted(16*G_SCALE, 0, 0, 0), Qt::AlignVCenter, strCaption_);

    QFont *font2 = FontManager::instance().getFont(12, false);
    painter->setFont(*font2);
    qreal initOpacity = painter->opacity();
    painter->setOpacity(OPACITY_UNHOVER_TEXT * initOpacity);
    painter->drawText(boundingRect().adjusted(0, 0, -16*G_SCALE, 0), Qt::AlignVCenter | Qt::AlignRight, strValue_);
}

void VersionInfoItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(50*G_SCALE);
}

} // namespace PreferencesWindow
