#ifndef CROSSPLATFORMOBJECTFACTORY_H
#define CROSSPLATFORMOBJECTFACTORY_H

#include "Helper/ihelper.h"
#include "NetworkStateManager/inetworkstatemanager.h"
#include "NetworkDetectionManager/inetworkdetectionmanager.h"
#include "Firewall/firewallcontroller.h"
#include "MacAddressController/imacaddresscontroller.h"

namespace CrossPlatformObjectFactory
{
    IHelper *createHelper(QObject *parent);
    INetworkStateManager *createNetworkStateManager(QObject *parent,INetworkDetectionManager *networkDetectionManager);
    INetworkDetectionManager *createNetworkDetectionManager(QObject *parent, IHelper *helper);
    FirewallController *createFirewallController(QObject *parent, IHelper *helper);
    IMacAddressController *createMacAddressController(QObject *parent, INetworkDetectionManager *ndManager, IHelper *helper);
}

#endif // CROSSPLATFORMOBJECTFACTORY_H
