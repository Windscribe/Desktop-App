#include "connectionplatformpolicy.h"

#ifdef Q_OS_WIN
    #include "utils/winutils.h"
#elif defined Q_OS_MACOS
    #include "utils/macutils.h"
#endif

ConnectionPlatformPolicy::ConnectionPlatformPolicy(Helper *helper) : helper_(helper)
{
}

bool ConnectionPlatformPolicy::isLockdownMode() const
{
#ifdef Q_OS_MACOS
    // Toggling Lockdown Mode either direction requires a Mac restart, so the value is immutable for
    // the process lifetime; cache it — the OS read forks a blocking `defaults read`.
    if (!cachedLockdownMode_.has_value()) {
        cachedLockdownMode_ = MacUtils::isLockdownMode();
    }
    return *cachedLockdownMode_;
#else
    return false;
#endif
}

bool ConnectionPlatformPolicy::needsSleepEventAwareDisconnect() const
{
    // Ignoring special sleep event handling on non-Windows for now until we have evidence it is required.
    return platformNeedsSleepEventAwareDisconnect();
}

bool ConnectionPlatformPolicy::shouldReconnectOnOnlineStateChange() const
{
    return platformReconnectsOnOnlineStateChange();
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
