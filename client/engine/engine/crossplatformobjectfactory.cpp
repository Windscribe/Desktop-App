#include "crossplatformobjectfactory.h"
#include "utils/log/logger.h"

#ifdef Q_OS_WIN
    #include "helper/helperbackend_win.h"
    #include "networkdetectionmanager/networkdetectionmanager_win.h"
    #include "firewall/firewallcontroller_win.h"
    #include "macaddresscontroller/macaddresscontroller_win.h"
    #include "connectionmanager/ctrldmanager/ctrldmanager_win.h"

#elif defined Q_OS_MACOS
    #include "helper/helperbackend_mac.h"
    #include "networkdetectionmanager/networkdetectionmanager_mac.h"
    #include "firewall/firewallcontroller_mac.h"
    #include "macaddresscontroller/macaddresscontroller_mac.h"
    #include "connectionmanager/ctrldmanager/ctrldmanager_posix.h"
#elif defined Q_OS_LINUX
    #include "helper/helperbackend_linux.h"
    #include "networkdetectionmanager/networkdetectionmanager_linux.h"
    #include "firewall/firewallcontroller_linux.h"
    #include "macaddresscontroller/macaddresscontroller_linux.h"
    #include "connectionmanager/ctrldmanager/ctrldmanager_posix.h"
#endif

Helper *CrossPlatformObjectFactory::createHelper(QObject *parent)
{
    IHelperBackend *backend;
#ifdef Q_OS_WIN
    backend = new HelperBackend_win(parent);
#elif defined Q_OS_MACOS
    backend = new HelperBackend_mac(parent, log_utils::Logger::instance().getSpdLogger("basic"));
#elif defined Q_OS_LINUX
    backend = new HelperBackend_linux(parent);
#endif
    return new Helper(std::unique_ptr<IHelperBackend>(backend));
}

FirewallController *CrossPlatformObjectFactory::createFirewallController(QObject *parent, Helper *helper)
{
#ifdef Q_OS_WIN
    return new FirewallController_win(parent, helper);
#elif defined Q_OS_MACOS
    return new FirewallController_mac(parent, helper);
#elif defined Q_OS_LINUX
    return new FirewallController_linux(parent, helper);
#endif

}

INetworkDetectionManager *CrossPlatformObjectFactory::createNetworkDetectionManager(QObject *parent, Helper *helper)
{
#ifdef Q_OS_WIN
    return new NetworkDetectionManager_win(parent, helper);
#elif defined Q_OS_MACOS
    return new NetworkDetectionManager_mac(parent, helper);
#elif defined Q_OS_LINUX
    return new NetworkDetectionManager_linux(parent, helper);
#endif
}

IMacAddressController *CrossPlatformObjectFactory::createMacAddressController(QObject *parent, INetworkDetectionManager *ndManager, Helper *helper)
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

ICtrldManager *CrossPlatformObjectFactory::createCtrldManager(QObject *parent, Helper *helper, bool isCreateLog)
{
#ifdef Q_OS_WIN
    Q_UNUSED(helper);
    return new CtrldManager_win(parent, isCreateLog);
#else
    return new CtrldManager_posix(parent, helper, isCreateLog);
#endif

}
