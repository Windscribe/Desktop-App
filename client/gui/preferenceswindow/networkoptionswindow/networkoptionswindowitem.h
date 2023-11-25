#pragma once

#include "commongraphics/basepage.h"
#include "backend/preferences/preferences.h"
#include "backend/preferences/preferenceshelper.h"
#include "preferenceswindow/toggleitem.h"
#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/linkitem.h"
#include "preferenceswindow/titleitem.h"
#include "networklistgroup.h"

namespace PreferencesWindow {

enum NETWORK_OPTIONS_SCREEN { NETWORK_OPTIONS_HOME, NETWORK_OPTIONS_DETAILS };

class NetworkOptionsWindowItem : public CommonGraphics::BasePage
{
    Q_OBJECT
public:
    explicit NetworkOptionsWindowItem(ScalableGraphicsObject *parent, Preferences *preferences);

    QString caption() const override;

    void setCurrentNetwork(types::NetworkInterface networkInterface);

    void clearNetworks();
    void addNetwork(types::NetworkInterface network, const NETWORK_TRUST_TYPE networkTrust);
    void addNetwork(types::NetworkInterface network);
    void addNetworks(QVector<types::NetworkInterface> list);

    void updateNetworks();

    NETWORK_OPTIONS_SCREEN getScreen();
    void setScreen(NETWORK_OPTIONS_SCREEN screen);

    void updateScaling() override;

    static QString trustTypeToString(NETWORK_TRUST_TYPE type);

signals:
    void currentNetworkUpdated(types::NetworkInterface);
    void networkClicked(types::NetworkInterface);

private slots:
    void onAutoSecureChanged(bool enabled);
    void onCurrentNetworkClicked();
    void onLanguageChanged();
    void onNetworkWhitelistChanged(QVector<types::NetworkInterface> l);
    void updateShownItems();

private:
    Preferences *preferences_;

    PreferenceGroup *desc_;
    PreferenceGroup *autosecureGroup_;
    ToggleItem *autosecureCheckbox_;
    TitleItem *currentNetworkTitle_;
    PreferenceGroup *currentNetworkGroup_;
    LinkItem *currentNetworkItem_;
    types::NetworkInterface currentNetwork_;
    LinkItem *placeholderItem_;
    TitleItem *otherNetworksTitle_;
    NetworkListGroup *otherNetworksGroup_;

    NETWORK_OPTIONS_SCREEN currentScreen_;

    const QString NO_NETWORKS_DETECTED = tr("No Networks Detected.\nConnect to a network first");
};

} // namespace PreferencesWindow
