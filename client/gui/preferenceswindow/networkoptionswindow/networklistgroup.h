#pragma once

#include "commongraphics/baseitem.h"
#include "preferenceswindow/comboboxitem.h"
#include "preferenceswindow/linkitem.h"
#include "preferenceswindow/preferencegroup.h"
#include "types/networkinterface.h"

namespace PreferencesWindow {

class NetworkListGroup: public PreferenceGroup
{
    Q_OBJECT
public:
    explicit NetworkListGroup(ScalableGraphicsObject *parent,
                              const QString &desc = "",
                              const QString &descUrl = "");

    void addNetwork(types::NetworkInterface network, NETWORK_TRUST_TYPE type);
    void removeNetwork(types::NetworkInterface network);
    void setCurrentNetwork(types::NetworkInterface network);
    void setCurrentNetwork(types::NetworkInterface network, NETWORK_TRUST_TYPE type);
    void setTrustType(types::NetworkInterface network, NETWORK_TRUST_TYPE type);
    void updateNetworks(QVector<types::NetworkInterface> list);
    types::NetworkInterface currentNetwork();
    void clear();
    int isEmpty();

    void updateNetworkCombos();
    void updateScaling() override;

signals:
    void networkClicked(types::NetworkInterface network);
    void isEmptyChanged();

private slots:
    void onNetworkClicked();
    void updateDisplay();

private:
    QString trustTypeToString(NETWORK_TRUST_TYPE type);
    NETWORK_TRUST_TYPE stringToTrustType(QString str);
    types::NetworkInterface networkInterfaceByName(QString networkOrSsid);

    QString currentNetwork_;
    QMap<QString, types::NetworkInterface> networks_;

    int shownItems_;
};

}
