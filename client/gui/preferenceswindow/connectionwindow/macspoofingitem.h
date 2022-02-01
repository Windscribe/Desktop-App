#ifndef MACSPOOFINGITEM_H
#define MACSPOOFINGITEM_H

#include "../baseitem.h"
#include "../checkboxitem.h"
#include "../comboboxitem.h"
#include "backend/preferences/preferences.h"
#include "macaddressitem.h"
#include "autorotatemacitem.h"

namespace PreferencesWindow {

class MacSpoofingItem : public BaseItem
{
    Q_OBJECT
public:
    explicit MacSpoofingItem(ScalableGraphicsObject *parent);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setMacAddrSpoofing(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing);
    void setCurrentNetwork(const ProtoTypes::NetworkInterface &networkInterface);

    void updateScaling() override;
signals:
    void macAddrSpoofingChanged(const ProtoTypes::MacAddrSpoofing &macAddrSpoofing);
    void cycleMacAddressClick();

private slots:
    void onCheckBoxStateChanged(bool isChecked);
    void onAutoRotateMacStateChanged(bool isChecked);
    void onExpandAnimationValueChanged(const QVariant &value);
    void onInterfaceItemChanged(const QVariant &value);

    void onCycleMacAddressClick();

private:
    static constexpr int COLLAPSED_HEIGHT = 50;
    static constexpr int EXPANDED_HEIGHT = 50 + 43 + 43 + 43;

    CheckBoxItem *checkBoxEnable_;
    MacAddressItem *macAddressItem_;
    AutoRotateMacItem *autoRotateMacItem_;
    ComboBoxItem *comboBoxInterface_;

    ProtoTypes::NetworkInterface curNetworkInterface_;
    ProtoTypes::MacAddrSpoofing curMAS_;
    QList<ProtoTypes::NetworkInterface> interfaces_;

    QVariantAnimation expandEnimation_;

    void updateSpoofingSelection(ProtoTypes::MacAddrSpoofing &macAddrSpoofing);
    void updatePositions();
};

} // namespace PreferencesWindow

#endif // MACSPOOFINGITEM_H
