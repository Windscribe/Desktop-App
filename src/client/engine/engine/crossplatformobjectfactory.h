#pragma once

#include "helper/helper.h"
#include "networkdetectionmanager/inetworkdetectionmanager.h"
#include "firewall/firewallcontroller.h"
#include "macaddresscontroller/imacaddresscontroller.h"
#include "connectionmanager/ctrldmanager/ictrldmanager.h"
#include "connectivitydiagnostic/iconnectivitydiagnosticcollector.h"

namespace CrossPlatformObjectFactory
{
    Helper *createHelper(QObject *parent);
    INetworkDetectionManager *createNetworkDetectionManager(QObject *parent, Helper *helper);
    FirewallController *createFirewallController(QObject *parent, Helper *helper);
    IMacAddressController *createMacAddressController(QObject *parent, INetworkDetectionManager *ndManager, Helper *helper);
    ICtrldManager *createCtrldManager(QObject *parent, Helper *helper, bool isCreateLog);
    IConnectivityDiagnosticCollector *createConnectivityDiagnosticCollector(QObject *parent, FirewallController *firewallController);
}
