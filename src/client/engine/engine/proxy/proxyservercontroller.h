#pragma once

#include "types/proxysettings.h"

// contains current proxy settings, autodetect proxy settings if need
class ProxyServerController
{
public:
    static ProxyServerController &instance()
    {
        static ProxyServerController s;
        return s;
    }

    bool updateProxySettings(const types::ProxySettings &proxySettings);
    const types::ProxySettings &getCurrentProxySettings();

private:
    ProxyServerController();

    types::ProxySettings proxySettings_;
    bool bInitialized_;
};
