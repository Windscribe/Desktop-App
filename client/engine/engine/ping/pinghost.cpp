#include "pinghost.h"
#include "utils/ws_assert.h"

const int typeIdPingType = qRegisterMetaType<PingHost::PING_TYPE>("PingHost::PING_TYPE");


PingHost::PingHost(QObject *parent, IConnectStateController *stateController, NetworkAccessManager *networkAccessManager) : QObject(parent),
    pingHostCurl_(this, stateController, networkAccessManager), pingHostTcp_(this, stateController), pingHostIcmp_(this, stateController)
{
    connect(&pingHostCurl_, &PingHost_Curl::pingFinished, this, &PingHost::pingFinished);
    connect(&pingHostTcp_, &PingHost_TCP::pingFinished, this, &PingHost::pingFinished);
#if defined(Q_OS_WIN)
    connect(&pingHostIcmp_, &PingHost_ICMP_win::pingFinished, this, &PingHost::pingFinished);
#else
    connect(&pingHostIcmp_, &PingHost_ICMP_mac::pingFinished, this, &PingHost::pingFinished);
#endif
}

void PingHost::addHostForPing(const QString &id, const QString &ip, PING_TYPE pingType, const QString &hostname)
{
    QMetaObject::invokeMethod(this, "addHostForPingImpl", Q_ARG(QString, id), Q_ARG(QString, ip), Q_ARG(PingHost::PING_TYPE, pingType), Q_ARG(QString, hostname));
}

void PingHost::clearPings()
{
    QMetaObject::invokeMethod(this, "clearPingsImpl");
}

void PingHost::setProxySettings(const types::ProxySettings &proxySettings)
{
    QMetaObject::invokeMethod(this, "setProxySettingsImpl", Q_ARG(types::ProxySettings, proxySettings));
}

void PingHost::disableProxy()
{
    QMetaObject::invokeMethod(this, "disableProxyImpl");
}

void PingHost::enableProxy()
{
    QMetaObject::invokeMethod(this, "enableProxyImpl");
}

void PingHost::addHostForPingImpl(const QString &id, const QString &ip, PING_TYPE pingType, const QString &hostname)
{
    if (pingType == PING_CURL) {
        pingHostCurl_.addHostForPing(id, ip, hostname);
        return;
    }

#ifdef Q_OS_WIN
    // The icmp ping object is not currently set up to work with a network proxy.
    if (pingType == PING_TCP && pingHostTcp_.isProxyEnabled()) {
        pingHostTcp_.addHostForPing(id, ip);
    } else {
        pingHostIcmp_.addHostForPing(id, ip);
    }
#else
    if (pingType == PING_TCP) {
        pingHostTcp_.addHostForPing(id, ip);
    }
    else if (pingType == PING_ICMP) {
        pingHostIcmp_.addHostForPing(id, ip);
    }
    else {
        WS_ASSERT(false);
    }
#endif
}

void PingHost::clearPingsImpl()
{
    pingHostCurl_.clearPings();
    pingHostTcp_.clearPings();
    pingHostIcmp_.clearPings();
}

void PingHost::setProxySettingsImpl(const types::ProxySettings &proxySettings)
{
    pingHostCurl_.setProxySettings(proxySettings);
    pingHostTcp_.setProxySettings(proxySettings);
    pingHostIcmp_.setProxySettings(proxySettings);
}

void PingHost::disableProxyImpl()
{
    pingHostCurl_.disableProxy();
    pingHostTcp_.disableProxy();
    pingHostIcmp_.disableProxy();
}

void PingHost::enableProxyImpl()
{
    pingHostCurl_.enableProxy();
    pingHostTcp_.enableProxy();
    pingHostIcmp_.enableProxy();
}
