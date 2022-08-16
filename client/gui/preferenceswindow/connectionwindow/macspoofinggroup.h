#ifndef MACSPOOFINGITEM_H
#define MACSPOOFINGITEM_H

#include "backend/preferences/preferences.h"
#include "preferenceswindow/checkboxitem.h"
#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/preferencegroup.h"
#include "macaddressitem.h"

namespace PreferencesWindow {

class MacSpoofingGroup : public PreferenceGroup
{
    Q_OBJECT
public:
    explicit MacSpoofingGroup(ScalableGraphicsObject *parent,
                              const QString &desc = "",
                              const QString &descUrl = "");

    void setMacSpoofingSettings(const types::MacAddrSpoofing &macSpoofing);
    void setCurrentNetwork(const types::NetworkInterface &networkInterface);

signals:
    void macAddrSpoofingChanged(const types::MacAddrSpoofing &macAddrSpoofing);
    void cycleMacAddressClick();

private slots:
    void onCheckBoxStateChanged(bool isChecked);
    void onAutoRotateMacStateChanged(bool isChecked);
    void onInterfaceItemChanged(const QVariant &value);

    void onCycleMacAddressClick();

private:
    void updateMode();

    CheckBoxItem *checkBoxEnable_;
    MacAddressItem *macAddressItem_;
    CheckBoxItem *autoRotateMacItem_;
    ComboBoxItem *comboBoxInterface_;

    QList<types::NetworkInterface> networks_;
    types::NetworkInterface curNetwork_;
    types::MacAddrSpoofing settings_;
};

} // namespace PreferencesWindow

#endif // MACSPOOFINGITEM_H
