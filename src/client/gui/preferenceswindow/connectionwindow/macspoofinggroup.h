#pragma once

#include "macaddressitem.h"
#include "preferenceswindow/toggleitem.h"
#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/preferencegroup.h"
#include "types/macaddrspoofing.h"

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
    void setEnabled(bool enabled);

    void setDescription(const QString &desc, const QString &descUrl = "");

signals:
    void macAddrSpoofingChanged(const types::MacAddrSpoofing &macAddrSpoofing);
    void cycleMacAddressClick();

private slots:
    void onCheckBoxStateChanged(bool isChecked);
    void onAutoRotateMacStateChanged(bool isChecked);
    void onInterfaceItemChanged(const QVariant &value);
    void onCycleMacAddressClick();
    void onLanguageChanged();

private:
    void updateMode();

    ToggleItem *checkBoxEnable_;
    MacAddressItem *macAddressItem_;
    ToggleItem *autoRotateMacItem_;
    ComboBoxItem *comboBoxInterface_;

    QList<types::NetworkInterface> networks_;
    types::NetworkInterface curNetwork_;
    types::MacAddrSpoofing settings_;
};

} // namespace PreferencesWindow
