#ifndef SPLITTUNNELINGIPHOSTNAMEWINDOWITEM_H
#define SPLITTUNNELINGIPHOSTNAMEWINDOWITEM_H

#include "preferenceswindow/basepage.h"
#include "backend/preferences/preferences.h"
#include "backend/preferences/preferenceshelper.h"
#include "splittunnelingipsandhostnamesitem.h"

namespace PreferencesWindow {

class SplitTunnelingIpsAndHostnamesWindowItem : public BasePage
{
    Q_OBJECT
public:
    explicit SplitTunnelingIpsAndHostnamesWindowItem(ScalableGraphicsObject *parent, Preferences *preferences);

    QString caption();
    void setLoggedIn(bool loggedIn);
    void setFocusOnTextEntry();

signals:
    void networkRoutesUpdated(QList<types::SplitTunnelingNetworkRoute> routes);
    void nativeInfoErrorMessage(QString title, QString desc);

private slots:
    void onNetworkRoutesUpdated(QList<types::SplitTunnelingNetworkRoute> names);

private:
    Preferences *preferences_;
    SplitTunnelingIpsAndHostnamesItem *ipsAndHostnamesItem_;

};

}
#endif // SPLITTUNNELINGIPHOSTNAMEWINDOWITEM_H
