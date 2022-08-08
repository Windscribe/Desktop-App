#include "pinghost.h"

const int typeIdPingType = qRegisterMetaType<PingHost::PING_TYPE>("PingHost::PING_TYPE");


PingHost::PingHost(QObject *parent, IConnectStateController *stateController) : QObject(parent),
    pingHostTcp_(this, stateController), pingHostIcmp_(this, stateController)
{
    connect(&pingHostTcp_, SIGNAL(pingFinished(bool,int,QString,bool)), SIGNAL(pingFinished(bool,int,QString,bool)));
    connect(&pingHostIcmp_, SIGNAL(pingFinished(bool,int,QString,bool)), SIGNAL(pingFinished(bool,int,QString,bool)));
}

void PingHost::addHostForPing(const QString &ip, PingHost::PING_TYPE pingType)
{
    QMetaObject::invokeMethod(this, "addHostForPingImpl", Q_ARG(QString, ip), Q_ARG(PingHost::PING_TYPE, pingType));
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

void PingHost::addHostForPingImpl(const QString &ip, PingHost::PING_TYPE pingType)
{
    if (pingType == PING_TCP)
    {
        pingHostTcp_.addHostForPing(ip);
    }
    else if (pingType == PING_ICMP)
    {
        pingHostIcmp_.addHostForPing(ip);
    }
    else
    {
        Q_ASSERT(false);
    }
}

void PingHost::clearPingsImpl()
{
    pingHostTcp_.clearPings();
    pingHostIcmp_.clearPings();
}

void PingHost::setProxySettingsImpl(const types::ProxySettings &proxySettings)
{
    pingHostTcp_.setProxySettings(proxySettings);
    pingHostIcmp_.setProxySettings(proxySettings);
}

void PingHost::disableProxyImpl()
{
    pingHostTcp_.disableProxy();
    pingHostIcmp_.disableProxy();
}

void PingHost::enableProxyImpl()
{
    pingHostTcp_.enableProxy();
    pingHostIcmp_.enableProxy();
}
