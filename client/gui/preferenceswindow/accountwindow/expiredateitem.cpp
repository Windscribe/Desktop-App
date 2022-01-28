#include "expiredateitem.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

ExpireDateItem::ExpireDateItem(ScalableGraphicsObject *parent) : BaseItem(parent, 50)
{
    dividerLine_ = new DividerLine(this, 276);
    dividerLine_->setPos(24, 50 - 3);
}

void ExpireDateItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    //painter->fillRect(boundingRect(), QBrush(QColor(0, 255, 0)));
    QFont *font = FontManager::instance().getFont(12, true);
    painter->setFont(*font);
    painter->setPen(QColor(255, 255, 255));
    painter->drawText(boundingRect().adjusted(16*G_SCALE, 1*G_SCALE, 0, -2*G_SCALE), Qt::AlignVCenter, tr("Expiry Date"));

    painter->drawText(boundingRect().adjusted(16*G_SCALE, 1*G_SCALE, -16*G_SCALE, -2*G_SCALE), Qt::AlignVCenter | Qt::AlignRight, dateStr_);
}

void ExpireDateItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(50*G_SCALE);
    dividerLine_->setPos(24*G_SCALE, (50 - 3)*G_SCALE);
}

void ExpireDateItem::setDate(const QString &date)
{
    dateStr_ = date;
    update();
}


} // namespace PreferencesWindow
