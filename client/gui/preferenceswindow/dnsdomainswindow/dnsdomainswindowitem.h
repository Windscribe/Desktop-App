#pragma once

#include "backend/preferences/preferences.h"
#include "backend/preferences/preferenceshelper.h"
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
    void addressesUpdated(QList<types::SplitTunnelingNetworkRoute> addresses);
    void escape();

private slots:
    void onAddressesUpdated(const QStringList &addresses);
    void onError(QString title, QString msg);
    void onClearError();
    void onLanguageChanged();

private:
    Preferences *preferences_;
    PreferenceGroup *desc_;
    DnsDomainsGroup *addressesGroup_;

    bool loggedIn_;
};

}

