#include "usernameitem.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

UsernameItem::UsernameItem(ScalableGraphicsObject *parent) : BaseItem(parent, 78)
{
    dividerLine_ = new DividerLine(this, 276);
    dividerLine_->setPos(24, 78 - 3);
}

void UsernameItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initialOpacity = painter->opacity();

    //painter->fillRect(boundingRect(), QBrush(QColor(0, 255, 255)));
    QFont *font = FontManager::instance().getFont(12, true);
    painter->setFont(*font);
    painter->setPen(QColor(255, 255, 255));
    painter->setOpacity(0.5 * initialOpacity);
    painter->drawText(boundingRect().adjusted(16*G_SCALE, 18*G_SCALE, 0, 0), Qt::AlignTop, tr("INFO"));

    painter->setOpacity(1.0 * initialOpacity);
    painter->drawText(boundingRect().adjusted(16*G_SCALE, 0, 0, -18*G_SCALE), Qt::AlignBottom, tr("Username"));

    painter->drawText(boundingRect().adjusted(16*G_SCALE, 0, -16*G_SCALE, -18*G_SCALE), Qt::AlignBottom | Qt::AlignRight, username_);
}

void UsernameItem::setUsername(const QString &username)
{
    username_ = username;
    update();
}

void UsernameItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(78*G_SCALE);
    dividerLine_->setPos(24*G_SCALE, (78 - 3)*G_SCALE);
}


} // namespace PreferencesWindow
