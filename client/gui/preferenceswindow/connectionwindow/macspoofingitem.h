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

    void setMacAddrSpoofing(const types::MacAddrSpoofing &macAddrSpoofing);
    void setCurrentNetwork(const types::NetworkInterface &networkInterface);

    void updateScaling() override;
signals:
    void macAddrSpoofingChanged(const types::MacAddrSpoofing &macAddrSpoofing);
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

    types::NetworkInterface curNetworkInterface_;
    types::MacAddrSpoofing curMAS_;
    QList<types::NetworkInterface> interfaces_;

    QVariantAnimation expandEnimation_;

    void updateSpoofingSelection(types::MacAddrSpoofing &macAddrSpoofing);
    void updatePositions();
};

} // namespace PreferencesWindow

#endif // MACSPOOFINGITEM_H
