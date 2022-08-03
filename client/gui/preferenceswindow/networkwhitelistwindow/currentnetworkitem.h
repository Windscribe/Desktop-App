#ifndef CURRENTNETWORKITEM_H
#define CURRENTNETWORKITEM_H

#include <QGraphicsObject>
#include "../baseitem.h"
#include "../comboboxitem.h"
#include "types/networkinterface.h"
#include "utils/protobuf_includes.h"

namespace PreferencesWindow {

class CurrentNetworkItem : public BaseItem
{
    Q_OBJECT
public:
    explicit CurrentNetworkItem(ScalableGraphicsObject *parent);

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;

    void setComboVisible(bool visible);
    void setNetworkInterface(types::NetworkInterface network);
    void setNetworkInterface(types::NetworkInterface network, NETWORK_TRUST_TYPE newTrustType);

    types::NetworkInterface currentNetworkInterface();
    void hideOpenPopups() override;
    void updateReadableText();

    void updateScaling() override;

signals:
    void currentNetworkTrustChanged(types::NetworkInterface network);

private slots:
    void onTrustTypeChanged(const QVariant &value);

private:
    types::NetworkInterface networkInterface_;

    ComboBoxItem * currentNetworkCombo_;

    void updatePositions();
};

}

#endif // CURRENTNETWORKITEM_H
