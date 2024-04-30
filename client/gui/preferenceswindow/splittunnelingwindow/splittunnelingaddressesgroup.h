#pragma once

#include "backend/preferences/preferences.h"
#include "commongraphics/baseitem.h"
#include "preferenceswindow/preferencegroup.h"
#include "addressitem.h"
#include "newaddressitem.h"

namespace PreferencesWindow {

class SplitTunnelingAddressesGroup : public PreferenceGroup
{
    Q_OBJECT
public:
    static const int kMaxDomains = 50;

    explicit SplitTunnelingAddressesGroup(ScalableGraphicsObject *parent,
                                          const QString &desc = "",
                                          const QString &descUrl = "");

    void setAddresses(QList<types::SplitTunnelingNetworkRoute> addresses);
    void addAddress(types::SplitTunnelingNetworkRoute &address);
    void setFocusOnTextEntry();

    void setLoggedIn(bool loggedIn);

signals:
    void addressesUpdated(QList<types::SplitTunnelingNetworkRoute> addresses);
    void setError(QString msg);
    void clearError();
    void escape();

private slots:
    void onAddClicked(QString address);
    void onDeleteClicked();

private:
    enum ValidationCode {
        OK = 0,
        ERROR_EXISTS,
        ERROR_INVALID,
        ERROR_RESERVED,
    };

    void addAddress(QString &address);
    void addAddressInternal(types::SplitTunnelingNetworkRoute &address);
    AddressItem *itemByName(QString &address);
    ValidationCode validate(QString &address);

    NewAddressItem *newAddressItem_;
    QMap<AddressItem *, types::SplitTunnelingNetworkRoute> addresses_;
    int numDomains_;
};

} // namespace PreferencesWindow
