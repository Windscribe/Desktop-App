#include "connectionplatformpolicy.h"

#ifdef Q_OS_WIN
    #include "utils/winutils.h"
#elif defined Q_OS_MACOS
    #include "utils/macutils.h"
#endif

ConnectionPlatformPolicy::ConnectionPlatformPolicy(Helper *helper) : helper_(helper)
{
}

bool ConnectionPlatformPolicy::isLockdownBlockingIkev2() const
{
#ifdef Q_OS_MACOS
    return MacUtils::isLockdownMode();
#else
    return false;
#endif
}

void ConnectionPlatformPolicy::disableDohIfNeeded()
{
#ifdef Q_OS_WIN
    if (WinUtils::isDohSupported()) {
        helper_->disableDohSettings();
    }
#endif
}

void ConnectionPlatformPolicy::setGaiIpv4PriorityEnabled(bool isEnabled)
{
#ifdef Q_OS_LINUX
    helper_->setGaiIpv4PriorityEnabled(isEnabled);
#else
    Q_UNUSED(isEnabled);
#endif
}

AdapterGatewayInfo ConnectionPlatformPolicy::detectDefaultAdapter()
{
    return AdapterGatewayInfo::detectAndCreateDefaultAdapterInfo();
}
