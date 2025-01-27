#include "crossplatformobjectfactory.h"

#ifdef Q_OS_WIN
    #include "helper/helper_win.h"
    #include "networkdetectionmanager/networkdetectionmanager_win.h"
    #include "firewall/firewallcontroller_win.h"
    #include "macaddresscontroller/macaddresscontroller_win.h"
    #include "connectionmanager/ctrldmanager/ctrldmanager_win.h"

#elif defined Q_OS_MACOS
    #include "helper/helper_mac.h"
    #include "networkdetectionmanager/networkdetectionmanager_mac.h"
    #include "firewall/firewallcontroller_mac.h"
    #include "macaddresscontroller/macaddresscontroller_mac.h"
    #include "connectionmanager/ctrldmanager/ctrldmanager_posix.h"
#elif defined Q_OS_LINUX
    #include "helper/helper_linux.h"
    #include "networkdetectionmanager/networkdetectionmanager_linux.h"
    #include "firewall/firewallcontroller_linux.h"
    #include "macaddresscontroller/macaddresscontroller_linux.h"
    #include "connectionmanager/ctrldmanager/ctrldmanager_posix.h"
#endif

IHelper *CrossPlatformObjectFactory::createHelper(QObject *parent)
{
#ifdef Q_OS_WIN
    return new Helper_win(parent);
#elif defined Q_OS_MACOS
    return new Helper_mac(parent);
#elif defined Q_OS_LINUX
    return new Helper_linux(parent);
#endif
}

FirewallController *CrossPlatformObjectFactory::createFirewallController(QObject *parent, IHelper *helper)
{
#ifdef Q_OS_WIN
    return new FirewallController_win(parent, helper);
#elif defined Q_OS_MACOS
    return new FirewallController_mac(parent, helper);
#elif defined Q_OS_LINUX
    return new FirewallController_linux(parent, helper);
#endif

}

INetworkDetectionManager *CrossPlatformObjectFactory::createNetworkDetectionManager(QObject *parent, IHelper *helper)
{
#ifdef Q_OS_WIN
    return new NetworkDetectionManager_win(parent, helper);
#elif defined Q_OS_MACOS
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
#elif defined Q_OS_MACOS
    return new MacAddressController_mac(parent, static_cast<NetworkDetectionManager_mac*>(ndManager), helper);
#elif defined Q_OS_LINUX
    return new MacAddressController_linux(parent, static_cast<NetworkDetectionManager_linux*>(ndManager), helper);
#endif
}

ICtrldManager *CrossPlatformObjectFactory::createCtrldManager(QObject *parent, IHelper *helper, bool isCreateLog)
{
#ifdef Q_OS_WIN
    Q_UNUSED(helper);
    return new CtrldManager_win(parent, isCreateLog);
#else
    return new CtrldManager_posix(parent, helper, isCreateLog);
#endif

}
