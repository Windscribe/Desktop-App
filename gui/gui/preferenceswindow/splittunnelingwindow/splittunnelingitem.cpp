#include "splittunnelingitem.h"

#include <QPainter>
#include "graphicresources/fontmanager.h"
#include "dpiscalemanager.h"

namespace PreferencesWindow {

SplitTunnelingItem::SplitTunnelingItem(ScalableGraphicsObject *parent) : BaseItem (parent, 340*G_SCALE)
{
    setFlags(flags() | QGraphicsItem::ItemClipsChildrenToShape);

    splitTunnelingSwitchItem_ = new SplitTunnelingSwitchItem(this);
    connect(splitTunnelingSwitchItem_, SIGNAL(settingsChanged(ProtoTypes::SplitTunnelingSettings)), SIGNAL(settingsChanged(ProtoTypes::SplitTunnelingSettings)));
    connect(splitTunnelingSwitchItem_, SIGNAL(appsPageClick()), SIGNAL(appsPageClick()));
    connect(splitTunnelingSwitchItem_, SIGNAL(ipsAndHostnamesPageClick()), SIGNAL(ipsAndHostnamesPageClick()));
    connect(splitTunnelingSwitchItem_, SIGNAL(showTooltip(TooltipInfo)), SIGNAL(showTooltip(TooltipInfo)));
    connect(splitTunnelingSwitchItem_, SIGNAL(hideTooltip(TooltipId)), SIGNAL(hideTooltip(TooltipId)));

    updateScaling();
}

void SplitTunnelingItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal initOpacity = painter->opacity();


    painter->setOpacity(0.5 * initOpacity);
    painter->setPen(Qt::white);
    painter->setFont(*FontManager::instance().getFont(12, false));
    QString desc = tr("Choose which apps, IPs or hostnames to exclude or include from Windscribe when connected");
    painter->drawText(boundingRect().adjusted(16*G_SCALE, 16*G_SCALE, -32*G_SCALE, 0), desc);

#ifdef Q_OS_MAC
    // additional description for Mac
    painter->setFont(*FontManager::instance().getFont(12, true));
    QString firewallDesc = tr("Firewall will not function in this mode");
    painter->drawText(boundingRect().adjusted(16*G_SCALE, 72*G_SCALE, -32*G_SCALE, 0), firewallDesc);
#endif
}

void SplitTunnelingItem::setSettings(ProtoTypes::SplitTunnelingSettings settings)
{
    splitTunnelingSwitchItem_->setSettings(settings);
}

void SplitTunnelingItem::updateScaling()
{
    BaseItem::updateScaling();
    setHeight(340*G_SCALE);
#ifdef Q_OS_MAC
    // additional description for Mac
    splitTunnelingSwitchItem_->setPos(0, 86*G_SCALE);
#else
    splitTunnelingSwitchItem_->setPos(0, 65*G_SCALE);
#endif
}

void SplitTunnelingItem::setAppsCount(int count)
{
    splitTunnelingSwitchItem_->setAppsCount(count);
}

void SplitTunnelingItem::setIpsAndHostnamesCount(int count)
{
    splitTunnelingSwitchItem_->setIpsAndHostnamesCount(count);
}


}
