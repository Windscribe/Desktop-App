#ifndef NETWORKSTATEMANAGER_LINUX_H
#define NETWORKSTATEMANAGER_LINUX_H

#include <QMutex>
#include "inetworkstatemanager.h"

// thread safe access
class NetworkStateManager_linux : public INetworkStateManager
{
    Q_OBJECT
public:
    explicit NetworkStateManager_linux(QObject *parent);
    ~NetworkStateManager_linux();

    bool isOnline() override;
};

#endif // MACNETWORKSTATEMANAGER_LINUX_H
