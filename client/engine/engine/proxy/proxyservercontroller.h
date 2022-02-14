#ifndef PROXYSERVERCONTROLLER_H
#define PROXYSERVERCONTROLLER_H

#include "proxysettings.h"

// contains current proxy settings, autodetect proxy settings if need
class ProxyServerController
{
public:
    static ProxyServerController &instance()
    {
        static ProxyServerController s;
        return s;
    }

    bool updateProxySettings(const ProxySettings &proxySettings);
    const ProxySettings &getCurrentProxySettings();

private:
    ProxyServerController();

    ProxySettings proxySettings_;
    bool bInitialized_;
};

#endif // PROXYSERVERCONTROLLER_H
