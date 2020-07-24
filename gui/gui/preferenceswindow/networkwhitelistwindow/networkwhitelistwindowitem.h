#ifndef NETWORKWHITELISTWINDOWITEM_H
#define NETWORKWHITELISTWINDOWITEM_H

#include "../basepage.h"
#include "../backend/preferences/preferences.h"
#include "../backend/preferences/preferenceshelper.h"
#include "../checkboxitem.h"
#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/textitem.h"
#include "ipc/generated_proto/types.pb.h"
#include "networklistitem.h"
#include "currentnetworkitem.h"

namespace PreferencesWindow {

class NetworkWhiteListWindowItem : public BasePage
{
    Q_OBJECT
public:
    explicit NetworkWhiteListWindowItem(ScalableGraphicsObject *parent, Preferences *preferences, PreferencesHelper *preferencesHelper);

    QString caption();

    void setCurrentNetwork(ProtoTypes::NetworkInterface networkInterface);

    void clearNetworks();
    void initNetworks();
    void addNetwork(ProtoTypes::NetworkInterface network, const ProtoTypes::NetworkTrustType networkTrust);
    void addNetwork(ProtoTypes::NetworkInterface networkEntry);
    void addNetworks(ProtoTypes::NetworkWhiteList list);

    void updateDescription();
    void addNewlyFoundNetworks();

    virtual void updateScaling();

signals:
    void currentNetworkUpdated(ProtoTypes::NetworkInterface);

private slots:
    void onNetworkWhiteListPreferencesChanged(ProtoTypes::NetworkWhiteList l);
    void onLanguageChanged();
    void onNetworkListChanged(ProtoTypes::NetworkInterface network);
    void onCurrentNetworkTrustChanged(ProtoTypes::NetworkInterface network);

private:
    Preferences *preferences_;

    TextItem *textItem_;
    CurrentNetworkItem *currentNetworkItem_;
    TextItem *comboListLabelItem_;
    NetworkListItem *networkListItem_;

    ProtoTypes::NetworkTrustType trustTypeFromPreferences(ProtoTypes::NetworkInterface networkName);

    const QString NO_NETWORKS_DETECTED = QT_TR_NOOP("No Networks Detected.\nConnect to a network first");
};

} // namespace PreferencesWindow

#endif // NETWORKWHITELISTWINDOWITEM_H
