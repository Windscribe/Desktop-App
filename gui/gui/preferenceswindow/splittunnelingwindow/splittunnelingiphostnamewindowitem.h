#ifndef SPLITTUNNELINGIPHOSTNAMEWINDOWITEM_H
#define SPLITTUNNELINGIPHOSTNAMEWINDOWITEM_H

#include "PreferencesWindow/basepage.h"
#include "../Backend/Preferences/preferences.h"
#include "../Backend/Preferences/preferenceshelper.h"
#include "splittunnelingipsandhostnamesitem.h"

namespace PreferencesWindow {

class SplitTunnelingIpsAndHostnamesWindowItem : public BasePage
{
    Q_OBJECT
public:
    explicit SplitTunnelingIpsAndHostnamesWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper);

    QString caption();
    void setLoggedIn(bool loggedIn);
    void setFocusOnTextEntry();

signals:
    void networkRoutesUpdated(QList<ProtoTypes::SplitTunnelingNetworkRoute> routes);
    void nativeInfoErrorMessage(QString title, QString desc);

private slots:
    void onNetworkRoutesUpdated(QList<ProtoTypes::SplitTunnelingNetworkRoute> names);

private:
    Preferences *preferences_;
    SplitTunnelingIpsAndHostnamesItem *ipsAndHostnamesItem_;

};

}
#endif // SPLITTUNNELINGIPHOSTNAMEWINDOWITEM_H
