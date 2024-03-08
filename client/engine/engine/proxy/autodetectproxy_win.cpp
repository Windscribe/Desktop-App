#include "autodetectproxy_win.h"
#include "Utils/logger.h"
#include <QNetworkProxy>
#include "utils/hardcodedsettings.h"

types::ProxySettings AutoDetectProxy_win::detect(bool &bSuccessfully)
{
    types::ProxySettings proxySettings;
    bSuccessfully = false;

    QNetworkProxyQuery npq(QUrl("https://"+ HardcodedSettings::instance().windscribeServerUrl()));
    QList<QNetworkProxy> listOfProxies = QNetworkProxyFactory::systemProxyForQuery(npq);
    if (listOfProxies.size())
    {
        bSuccessfully = true;
        QNetworkProxy proxy = listOfProxies[0];
        qCDebug(LOG_BASIC) << "Autodected proxy:" << proxy;

        if (proxy.type() == QNetworkProxy::NoProxy)
        {
            proxySettings.setOption(PROXY_OPTION_NONE);
            proxySettings.setAddress("");
            proxySettings.setPassword("");
            proxySettings.setPort(0);
            proxySettings.setUsername("");
            bSuccessfully = true;
        }
        else if (proxy.type() == QNetworkProxy::HttpProxy)
        {
            proxySettings.setOption(PROXY_OPTION_HTTP);
            proxySettings.setAddress(proxy.hostName());
            proxySettings.setPassword(proxy.password());
            proxySettings.setPort(proxy.port());
            proxySettings.setUsername(proxy.user());
            bSuccessfully = true;
        }
        else if (proxy.type() == QNetworkProxy::Socks5Proxy)
        {
            proxySettings.setOption(PROXY_OPTION_SOCKS);
            proxySettings.setAddress(proxy.hostName());
            proxySettings.setPassword(proxy.password());
            proxySettings.setPort(proxy.port());
            proxySettings.setUsername(proxy.user());
            bSuccessfully = true;
        }
    }
    return proxySettings;
}
