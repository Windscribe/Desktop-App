#include "crossplatformobjectfactory.h"

#ifdef Q_OS_WIN
    #include "helper/helper_win.h"
    #include "networkstatemanager/networkstatemanager_win.h"
    #include "networkdetectionmanager/networkdetectionmanager_win.h"
    #include "firewall/firewallcontroller_win.h"
    #include "macaddresscontroller/macaddresscontroller_win.h"

#elif defined Q_OS_MAC
    #include "helper/helper_mac.h"
    #include "networkstatemanager/networkstatemanager_mac.h"
    #include "networkdetectionmanager/networkdetectionmanager_mac.h"
    #include "firewall/firewallcontroller_mac.h"
    #include "macaddresscontroller/macaddresscontroller_mac.h"
#elif defined Q_OS_LINUX
    #include "helper/helper_linux.h"
    #include "networkstatemanager/networkstatemanager_linux.h"
    #include "networkdetectionmanager/networkdetectionmanager_linux.h"
    #include "firewall/firewallcontroller_linux.h"
    #include "macaddresscontroller/macaddresscontroller_linux.h"
#endif

IHelper *CrossPlatformObjectFactory::createHelper(QObject *parent)
{
#ifdef Q_OS_WIN
    return new Helper_win(parent);
#elif defined Q_OS_MAC
    return new Helper_mac(parent);
#elif defined Q_OS_LINUX
    return new Helper_linux(parent);
#endif
}

INetworkStateManager *CrossPlatformObjectFactory::createNetworkStateManager(QObject *parent, INetworkDetectionManager *networkDetectionManager)
{
#ifdef Q_OS_WIN
    return new NetworkStateManager_win(parent, networkDetectionManager);
#elif defined Q_OS_MAC
    Q_UNUSED(networkDetectionManager);
    return new NetworkStateManager_mac(parent);
#elif defined Q_OS_LINUX
    Q_UNUSED(networkDetectionManager);
    return new NetworkStateManager_linux(parent);
#endif
}

FirewallController *CrossPlatformObjectFactory::createFirewallController(QObject *parent, IHelper *helper)
{
#ifdef Q_OS_WIN
    return new FirewallController_win(parent, helper);
#elif defined Q_OS_MAC
    return new FirewallController_mac(parent, helper);
#elif defined Q_OS_LINUX
    return new FirewallController_linux(parent, helper);
#endif

}

INetworkDetectionManager *CrossPlatformObjectFactory::createNetworkDetectionManager(QObject *parent, IHelper *helper)
{
#ifdef Q_OS_WIN
    return new NetworkDetectionManager_win(parent, helper);
#elif defined Q_OS_MAC
    return new NetworkDetectionManager_mac(parent, helper);
#elif defined Q_OS_LINUX
    return new NetworkDetectionManager_linux(parent, helper);
#endif
}

IMacAddressController *CrossPlatformObjectFactory::createMacAddressController(QObject *parent, INetworkDetectionManager *ndManager, IHelper *helper)
{
#ifdef Q_OS_WIN
    Q_UNUSED(helper);
    return new MacAddressController_win(parent, static_cast<NetworkDetectionManager_win*>(ndManager));
#elif defined Q_OS_MAC
    return new MacAddressController_mac(parent, static_cast<NetworkDetectionManager_mac*>(ndManager), helper);
#elif defined Q_OS_LINUX
    return new MacAddressController_linux(parent, static_cast<NetworkDetectionManager_linux*>(ndManager), helper);
#endif
}
