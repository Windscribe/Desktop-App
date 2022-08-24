#ifndef PROXYGATEWAYITEM_H
#define PROXYGATEWAYITEM_H

#include "backend/preferences/preferences.h"
#include "backend/preferences/preferenceshelper.h"
#include "commongraphics/baseitem.h"
#include "commongraphics/checkboxbutton.h"
#include "preferenceswindow/checkboxitem.h"
#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/preferencegroup.h"
#include "proxyipaddressitem.h"

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
    void updateMode();

    CheckBoxItem *checkBoxEnable_;
    ComboBoxItem *comboBoxProxyType_;
    ProxyIpAddressItem *proxyIpAddressItem_;

    types::ShareProxyGateway settings_;

    QString desc_;
};

} // namespace PreferencesWindow

#endif // PROXYGATEWAYITEM_H
