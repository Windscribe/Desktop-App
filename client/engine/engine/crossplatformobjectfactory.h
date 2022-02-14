#ifndef CROSSPLATFORMOBJECTFACTORY_H
#define CROSSPLATFORMOBJECTFACTORY_H

#include "helper/ihelper.h"
#include "networkdetectionmanager/inetworkdetectionmanager.h"
#include "firewall/firewallcontroller.h"
#include "macaddresscontroller/imacaddresscontroller.h"

namespace CrossPlatformObjectFactory
{
    IHelper *createHelper(QObject *parent);
    INetworkDetectionManager *createNetworkDetectionManager(QObject *parent, IHelper *helper);
    FirewallController *createFirewallController(QObject *parent, IHelper *helper);
    IMacAddressController *createMacAddressController(QObject *parent, INetworkDetectionManager *ndManager, IHelper *helper);
}

#endif // CROSSPLATFORMOBJECTFACTORY_H
