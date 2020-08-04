#ifndef NETWORKSTATEMANAGER_MAC_H
#define NETWORKSTATEMANAGER_MAC_H

#include <QMutex>
#include "inetworkstatemanager.h"

// thread safe access
class NetworkStateManager_mac : public INetworkStateManager
{
    Q_OBJECT
public:
    explicit NetworkStateManager_mac(QObject *parent);
    ~NetworkStateManager_mac();

    bool isOnline() override;

private slots:
    void onNetworkStateChanged();

private:

    bool checkOnline(QString &networkInterface);
    QString lastNetworkInterface_;
    bool bLastNetworkInterfaceInitialized_;
    QMutex mutex_;
};

#endif // MACNETWORKSTATEMANAGER_MAC_H
