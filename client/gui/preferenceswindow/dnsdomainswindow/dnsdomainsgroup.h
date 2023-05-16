#pragma once

#include "backend/preferences/preferences.h"
#include "commongraphics/baseitem.h"
#include "preferenceswindow/preferencegroup.h"
#include "dnsdomainaddressitem.h"
#include "../splittunnelingwindow/newaddressitem.h"

namespace PreferencesWindow {

class DnsDomainsGroup : public PreferenceGroup
{
    Q_OBJECT
public:
    explicit DnsDomainsGroup(ScalableGraphicsObject *parent,
                                          const QString &desc = "",
                                          const QString &descUrl = "");

    void setAddresses(const QStringList &addresses);
    void addAddress(const QString &address);
    void setFocusOnTextEntry();

    void setLoggedIn(bool loggedIn);

signals:
    void addressesUpdated(const QStringList &addresses);
    void setError(const QString &msg);
    void clearError();
    void escape();

private slots:
    void onAddClicked(const QString &address);
    void onDeleteClicked();

private:
    enum ValidationCode {
        OK = 0,
        ERROR_EXISTS,
        ERROR_INVALID
    };

    void addAddressInternal(const QString &address);
    DnsDomainAddressItem *itemByName(const QString &address);
    ValidationCode validate(const QString &address);

    NewAddressItem *newAddressItem_;
    QMap<DnsDomainAddressItem *, QString> addresses_;
};

} // namespace PreferencesWindow

