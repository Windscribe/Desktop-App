#include "proxyservercontroller.h"
#ifdef Q_OS_WIN
    #include "autodetectproxy_win.h"
#elif defined Q_OS_MACOS
    #include "autodetectproxy_mac.h"
#endif
#include "utils/ipvalidation.h"
#include "utils/ws_assert.h"

bool ProxyServerController::updateProxySettings(const types::ProxySettings &proxySettings)
{
    types::ProxySettings newProxySettings;

    if (proxySettings.option() == PROXY_OPTION_AUTODETECT) {
        bool bSuccess = false;
#ifdef Q_OS_WIN
        types::ProxySettings autoProxySettings = AutoDetectProxy_win::detect(bSuccess);
#elif defined Q_OS_MACOS
        types::ProxySettings autoProxySettings = AutoDetectProxy_mac::detect(bSuccess);
#elif defined Q_OS_LINUX
        //todo linux
        types::ProxySettings autoProxySettings;
#endif
        if (!bSuccess) {
            newProxySettings.setOption(PROXY_OPTION_NONE);
            newProxySettings.setAddress("");
            newProxySettings.setPassword("");
            newProxySettings.setPort(0);
            newProxySettings.setUsername("");
        } else {
            newProxySettings = autoProxySettings;
        }
    } else if (proxySettings.option() == PROXY_OPTION_HTTP || proxySettings.option() == PROXY_OPTION_SOCKS) {
        if (!IpValidation::isIpv4Address(proxySettings.address())) {
            newProxySettings.setOption(PROXY_OPTION_NONE);
            newProxySettings.setAddress("");
            newProxySettings.setPassword("");
            newProxySettings.setPort(0);
            newProxySettings.setUsername("");
        } else {
            newProxySettings = proxySettings;
        }
    } else {
        newProxySettings = proxySettings;
    }

    const bool isModified = !bInitialized_ || newProxySettings != proxySettings_;
    std::swap( proxySettings_, newProxySettings );
    bInitialized_ = true;
    return isModified;
}

const types::ProxySettings &ProxyServerController::getCurrentProxySettings()
{
    WS_ASSERT(bInitialized_);
    return proxySettings_;
}

ProxyServerController::ProxyServerController() : bInitialized_(false)
{

}
