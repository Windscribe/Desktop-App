#pragma once

#include "backend/preferences/preferences.h"
#include "commongraphics/basepage.h"
#include "preferenceswindow/preferencegroup.h"
#include "dnsdomainsgroup.h"

namespace PreferencesWindow {

class DnsDomainsWindowItem : public CommonGraphics::BasePage
{
    Q_OBJECT
public:
    explicit DnsDomainsWindowItem(ScalableGraphicsObject *parent, Preferences *preferences);

    QString caption() const override;
    void setFocusOnTextEntry();

    void setLoggedIn(bool loggedIn);

signals:
    void escape();

private slots:
    void onAddressesUpdated(const QStringList &addresses);
    void onError(QString msg);
    void onClearError();
    void onConnectedDnsPreferencesChanged(const types::ConnectedDnsInfo &dns);
    void onLanguageChanged();


private:
    Preferences *preferences_;
    PreferenceGroup *desc_;
    DnsDomainsGroup *addressesGroup_;

    bool loggedIn_;
};

}

