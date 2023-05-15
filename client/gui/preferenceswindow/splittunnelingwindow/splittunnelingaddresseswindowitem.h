#ifndef SPLITTUNNELINGADDRESSESWINDOWITEM_H
#define SPLITTUNNELINGADDRESSESWINDOWITEM_H

#include "backend/preferences/preferences.h"
#include "backend/preferences/preferenceshelper.h"
#include "commongraphics/basepage.h"
#include "preferenceswindow/preferencegroup.h"
#include "splittunnelingaddressesgroup.h"

namespace PreferencesWindow {

class SplitTunnelingAddressesWindowItem : public CommonGraphics::BasePage
{
    Q_OBJECT
public:
    explicit SplitTunnelingAddressesWindowItem(ScalableGraphicsObject *parent, Preferences *preferences);

    QString caption() const override;
    void setFocusOnTextEntry();

    void setLoggedIn(bool loggedIn);

signals:
    void addressesUpdated(QList<types::SplitTunnelingNetworkRoute> addresses);
    void escape();

private slots:
    void onAddressesUpdated(QList<types::SplitTunnelingNetworkRoute> addresses);
    void onError(QString title, QString msg);
    void onClearError();
    void onLanguageChanged();

private:
    Preferences *preferences_;
    PreferenceGroup *desc_;
    SplitTunnelingAddressesGroup *addressesGroup_;

    bool loggedIn_;
};

}

#endif // SPLITTUNNELINGADDRESSESWINDOWITEM_H
