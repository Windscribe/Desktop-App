#include "proxyservercontroller.h"
#ifdef Q_OS_WIN
    #include "autodetectproxy_win.h"
#elif defined Q_OS_MAC
    #include "autodetectproxy_mac.h"
#endif
#include <QHostInfo>

bool ProxyServerController::updateProxySettings(const ProxySettings &proxySettings)
{
    ProxySettings newProxySettings;

    if (proxySettings.option() == PROXY_OPTION_AUTODETECT)
    {
        bool bSuccess = false;
#ifdef Q_OS_WIN
        ProxySettings autoProxySettings = AutoDetectProxy_win::detect(bSuccess);
#elif defined Q_OS_MAC
        ProxySettings autoProxySettings = AutoDetectProxy_mac::detect(bSuccess);
#endif
        // cppcheck-suppress knownConditionTrueFalse
        if (!bSuccess)
        {
            newProxySettings.setOption(PROXY_OPTION_NONE);
            newProxySettings.setAddress("");
            newProxySettings.setPassword("");
            newProxySettings.setPort(0);
            newProxySettings.setUsername("");
        }
        else
        {
            newProxySettings = autoProxySettings;
        }
    }
    else if (proxySettings.option() == PROXY_OPTION_HTTP || proxySettings.option() == PROXY_OPTION_SOCKS)
    {
        QHostInfo hi = QHostInfo::fromName(proxySettings.address());
        if (hi.error() == QHostInfo::NoError && hi.addresses().count() > 0)
        {
            newProxySettings = proxySettings;
            newProxySettings.setAddress(hi.addresses()[0].toString());
        }
        else
        {
            newProxySettings.setOption(PROXY_OPTION_NONE);
            newProxySettings.setAddress("");
            newProxySettings.setPassword("");
            newProxySettings.setPort(0);
            newProxySettings.setUsername("");
        }
    }
    else
    {
        newProxySettings = proxySettings;
    }

    const bool isModified = !bInitialized_ || newProxySettings != proxySettings_;
    std::swap( proxySettings_, newProxySettings );
    bInitialized_ = true;
    return isModified;
}

const ProxySettings &ProxyServerController::getCurrentProxySettings()
{
    Q_ASSERT(bInitialized_);
    return proxySettings_;
}

ProxyServerController::ProxyServerController() : bInitialized_(false)
{

}
