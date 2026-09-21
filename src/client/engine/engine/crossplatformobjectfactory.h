#pragma once

#include "connectivitydiagnostic/iconnectivitydiagnosticcollector.h"
#include "dns/ctrldmanager/ictrldmanager.h"
#include "firewall/firewallcontroller.h"
#include "helper/helper.h"
#include "macaddresscontroller/imacaddresscontroller.h"
#include "networkdetectionmanager/inetworkdetectionmanager.h"
#include "sleepevents/isleepevents.h"

namespace CrossPlatformObjectFactory
{
    Helper *createHelper(QObject *parent);
    INetworkDetectionManager *createNetworkDetectionManager(QObject *parent, Helper *helper, ISleepEvents *sleepEvents);
    FirewallController *createFirewallController(QObject *parent, Helper *helper);
    IMacAddressController *createMacAddressController(QObject *parent, INetworkDetectionManager *ndManager, Helper *helper);
    ICtrldManager *createCtrldManager(QObject *parent, Helper *helper, bool isCreateLog);
    ISleepEvents *createSleepEvents(QObject *parent);
    IConnectivityDiagnosticCollector *createConnectivityDiagnosticCollector(QObject *parent, FirewallController *firewallController);
}
