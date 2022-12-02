#pragma once

#include "preferenceswindow/checkboxitem.h"
#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/preferencegroup.h"
#include "proxyipaddressitem.h"
#include "types/shareproxygateway.h"

namespace PreferencesWindow {

class ProxyGatewayGroup : public PreferenceGroup
{
    Q_OBJECT
public:
    explicit ProxyGatewayGroup(ScalableGraphicsObject *parent,
                               const QString &desc = "",
                               const QString &descUrl = "");

    void setProxyGatewaySettings(const types::ShareProxyGateway &sp);
    void setProxyGatewayAddress(const QString &address);

signals:
    void proxyGatewayPreferencesChanged(const types::ShareProxyGateway &sp);

private slots:
    void onCheckBoxStateChanged(bool isChecked);
    void onProxyTypeItemChanged(QVariant v);

    void onLanguageChanged();

protected:
    void hideOpenPopups() override;

private:    
    CheckBoxItem *checkBoxEnable_;
    ComboBoxItem *comboBoxProxyType_;
    ProxyIpAddressItem *proxyIpAddressItem_;

    types::ShareProxyGateway settings_;

    QString desc_;

    void updateMode();
};

} // namespace PreferencesWindow
