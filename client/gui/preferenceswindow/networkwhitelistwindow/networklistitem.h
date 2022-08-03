#ifndef COMBOBOXLISTITEM_H
#define COMBOBOXLISTITEM_H

#include "../baseitem.h"
#include "../comboboxitem.h"
#include "types/networkinterface.h"
#include "utils/protobuf_includes.h"

namespace PreferencesWindow {

class NetworkListItem : public BaseItem
{
    Q_OBJECT
public:
    explicit NetworkListItem(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;
    void hideOpenPopups() override;

    void clearNetworks();
    void addNetwork(types::NetworkInterface network, NETWORK_TRUST_TYPE trustType);
    void setCurrentNetwork(types::NetworkInterface network);
    QVector<types::NetworkInterface> networkWhiteList();

    void updateNetworkCombos();
    void updateScaling() override;

signals:
    void networkItemsChanged(types::NetworkInterface network);

private slots:
    void onNetworkItemChanged(QVariant data);

private:
    QList<ComboBoxItem *> networks_;
    types::NetworkInterface currentNetwork_;

    void recalcHeight();
    void removeNetworkCombo(ComboBoxItem *combo);

};

}
#endif // COMBOBOXLISTITEM_H
