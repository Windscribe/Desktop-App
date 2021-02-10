#ifndef SPLITTUNNELINGITEM_H
#define SPLITTUNNELINGITEM_H

#include "../baseitem.h"
#include "splittunnelingswitchitem.h"

namespace PreferencesWindow {

class SplitTunnelingItem : public BaseItem
{
    Q_OBJECT
public:
    explicit SplitTunnelingItem(ScalableGraphicsObject *parent);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setSettings(ProtoTypes::SplitTunnelingSettings settings);
    void setAppsCount(int count);
    void setIpsAndHostnamesCount(int count);

    void updateScaling() override;

signals:
    void settingsChanged(ProtoTypes::SplitTunnelingSettings settings);
    void appsPageClick();
    void ipsAndHostnamesPageClick();

private:
    SplitTunnelingSwitchItem *splitTunnelingSwitchItem_;
};

}
#endif // SPLITTUNNELINGITEM_H
