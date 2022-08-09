#ifndef NETWORKWHITELISTWINDOWITEM_H
#define NETWORKWHITELISTWINDOWITEM_H

#include "../basepage.h"
#include "backend/preferences/preferences.h"
#include "backend/preferences/preferenceshelper.h"
#include "../checkboxitem.h"
#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/textitem.h"
#include "networklistitem.h"
#include "currentnetworkitem.h"

namespace PreferencesWindow {

class NetworkWhiteListWindowItem : public BasePage
{
    Q_OBJECT
public:
    explicit NetworkWhiteListWindowItem(ScalableGraphicsObject *parent, Preferences *preferences);

    QString caption();

    void setCurrentNetwork(types::NetworkInterface networkInterface);

    void clearNetworks();
    void initNetworks();
    void addNetwork(types::NetworkInterface network, const NETWORK_TRUST_TYPE networkTrust);
    void addNetwork(types::NetworkInterface networkEntry);
    void addNetworks(QVector<types::NetworkInterface> list);

    void updateDescription();
    void addNewlyFoundNetworks();

    void updateScaling() override;

signals:
    void currentNetworkUpdated(types::NetworkInterface);

private slots:
    void onNetworkWhiteListPreferencesChanged(QVector<types::NetworkInterface> l);
    void onLanguageChanged();
    void onNetworkListChanged(types::NetworkInterface network);
    void onCurrentNetworkTrustChanged(types::NetworkInterface network);

private:
    Preferences *preferences_;

    TextItem *textItem_;
    CurrentNetworkItem *currentNetworkItem_;
    TextItem *comboListLabelItem_;
    NetworkListItem *networkListItem_;

    NETWORK_TRUST_TYPE trustTypeFromPreferences(types::NetworkInterface networkName);

    const QString NO_NETWORKS_DETECTED = QT_TR_NOOP("No Networks Detected.\nConnect to a network first");
};

} // namespace PreferencesWindow

#endif // NETWORKWHITELISTWINDOWITEM_H
