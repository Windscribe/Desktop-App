#pragma once

#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/editboxitem.h"
#include "preferenceswindow/preferencegroup.h"
#include "preferenceswindow/toggleitem.h"
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

    void setDescription(const QString &desc, const QString &descUrl = "");

signals:
    void proxyGatewayPreferencesChanged(const types::ShareProxyGateway &sp);

private slots:
    void onCheckBoxStateChanged(bool isChecked);
    void onProxyTypeItemChanged(QVariant v);
    void onPortChanged(QVariant v);
    void onPortEditClicked();
    void onPortCancelClicked();
    void onWhileConnectedStateChanged(bool isChecked);

    void onLanguageChanged();

protected:
    void hideOpenPopups() override;

private:
    ToggleItem *checkBoxEnable_;
    EditBoxItem *editBoxPort_;
    ComboBoxItem *comboBoxProxyType_;
    ProxyIpAddressItem *proxyIpAddressItem_;
    ToggleItem *checkBoxWhileConnected_;

    types::ShareProxyGateway settings_;

    QString desc_;

    void updateMode();
};

} // namespace PreferencesWindow
