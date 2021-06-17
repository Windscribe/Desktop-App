#include "networkstatemanager_linux.h"


NetworkStateManager_linux::NetworkStateManager_linux(QObject *parent) : INetworkStateManager(parent)
{
}

NetworkStateManager_linux::~NetworkStateManager_linux()
{
}

bool NetworkStateManager_linux::isOnline()
{
    return true;
}

