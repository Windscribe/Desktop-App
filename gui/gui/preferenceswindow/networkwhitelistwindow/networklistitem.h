#ifndef COMBOBOXLISTITEM_H
#define COMBOBOXLISTITEM_H

#include "../baseitem.h"
#include "../comboboxitem.h"
#include "IPC/generated_proto/types.pb.h"

namespace PreferencesWindow {

class NetworkListItem : public BaseItem
{
    Q_OBJECT
public:
    explicit NetworkListItem(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;
    void hideOpenPopups() override;

    void clearNetworks();
    void addNetwork(ProtoTypes::NetworkInterface network, ProtoTypes::NetworkTrustType trustType);
    void setCurrentNetwork(ProtoTypes::NetworkInterface network);
    ProtoTypes::NetworkWhiteList networkWhiteList();

    void updateNetworkCombos();
    void updateScaling() override;

signals:
    void networkItemsChanged(ProtoTypes::NetworkInterface network);

private slots:
    void onNetworkItemChanged(QVariant data);

private:
    QList<ComboBoxItem *> networks_;
    ProtoTypes::NetworkInterface currentNetwork_;

    void recalcHeight();
    void removeNetworkCombo(ComboBoxItem *combo);

};

}
#endif // COMBOBOXLISTITEM_H
