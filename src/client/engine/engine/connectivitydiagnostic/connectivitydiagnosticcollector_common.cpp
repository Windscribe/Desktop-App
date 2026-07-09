#include "connectivitydiagnosticcollector_common.h"

#include <QList>
#include <QNetworkProxy>
#include <QUrl>

#include "engine/proxy/proxyservercontroller.h"
#include "types/proxysettings.h"
#include "utils/hardcodedsettings.h"

namespace connectivity_diagnostic_collector {

QString interfaceTypeToken(NETWORK_INTERFACE_TYPE type)
{
    switch (type) {
    case NETWORK_INTERFACE_ETH:              return QStringLiteral("eth");
    case NETWORK_INTERFACE_WIFI:             return QStringLiteral("wifi");
    case NETWORK_INTERFACE_PPP:              return QStringLiteral("ppp");
    case NETWORK_INTERFACE_MOBILE_BROADBAND: return QStringLiteral("mobile");
    case NETWORK_INTERFACE_NONE:             return QStringLiteral("none");
    }
    return QStringLiteral("none");
}

ProxyMode determineProxyMode()
{
    const types::ProxySettings &appProxy = ProxyServerController::instance().getCurrentProxySettings();
    if (appProxy.isProxyEnabled()) {
        return ProxyMode::kApp;
    }

    QNetworkProxyQuery npq(QUrl(QStringLiteral("https://") + HardcodedSettings::instance().windscribeServerUrl()));
    const QList<QNetworkProxy> proxies = QNetworkProxyFactory::systemProxyForQuery(npq);
    for (const QNetworkProxy &proxy : proxies) {
        if (proxy.type() != QNetworkProxy::NoProxy && proxy.type() != QNetworkProxy::DefaultProxy) {
            return ProxyMode::kSystem;
        }
    }

    return ProxyMode::kNone;
}

} // namespace connectivity_diagnostic_collector
