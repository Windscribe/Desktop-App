#include "pinghost.h"

PingHost::PingHost(QObject *parent, IConnectStateController *stateController) : QObject(parent),
    pingHostTcp_(this, stateController), pingHostIcmp_(this, stateController)
{
    connect(&pingHostTcp_, SIGNAL(pingFinished(bool,int,QString,bool)), SIGNAL(pingFinished(bool,int,QString,bool)));
    connect(&pingHostIcmp_, SIGNAL(pingFinished(bool,int,QString,bool)), SIGNAL(pingFinished(bool,int,QString,bool)));
}

void PingHost::addHostForPing(const QString &ip, PingHost::PING_TYPE pingType)
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

void PingHost::clearPings()
{
    pingHostTcp_.clearPings();
    pingHostIcmp_.clearPings();
}

void PingHost::setProxySettings(const ProxySettings &proxySettings)
{
    pingHostTcp_.setProxySettings(proxySettings);
    pingHostIcmp_.setProxySettings(proxySettings);
}

void PingHost::disableProxy()
{
    pingHostTcp_.disableProxy();
    pingHostIcmp_.disableProxy();
}

void PingHost::enableProxy()
{
    pingHostTcp_.enableProxy();
    pingHostIcmp_.enableProxy();
}
