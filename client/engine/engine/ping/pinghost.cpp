#include "pinghost.h"
#include "utils/ws_assert.h"

const int typeIdPingType = qRegisterMetaType<PingHost::PING_TYPE>("PingHost::PING_TYPE");


PingHost::PingHost(QObject *parent, IConnectStateController *stateController) : QObject(parent),
    pingHostTcp_(this, stateController), pingHostIcmp_(this, stateController)
{
    connect(&pingHostTcp_, &PingHost_TCP::pingFinished, this, &PingHost::pingFinished);
#if defined(Q_OS_WIN)
    connect(&pingHostIcmp_, &PingHost_ICMP_win::pingFinished, this, &PingHost::pingFinished);
#else
    connect(&pingHostIcmp_, &PingHost_ICMP_mac::pingFinished, this, &PingHost::pingFinished);
#endif
}

void PingHost::init()
{
#ifdef Q_OS_WIN
    crashHandler_.reset(new Debug::CrashHandlerForThread());
#endif
}

void PingHost::finish()
{
    // The destructors for the tcp/icmp objects will run in a different thread.  Need to clear
    // these objects in the thread that created them, otherwise some objects (e.g. QTimer) will
    // complain about being killed by a thread that did not create them.
    clearPings();
#ifdef Q_OS_WIN
    crashHandler_.reset();
#endif
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
#ifdef Q_OS_WIN
    // The icmp ping object is not currently set up to work with a network proxy.
    if (pingType == PING_TCP && pingHostTcp_.isProxyEnabled()) {
        pingHostTcp_.addHostForPing(ip);
    } else {
        pingHostIcmp_.addHostForPing(ip);
    }
#else
    if (pingType == PING_TCP) {
        pingHostTcp_.addHostForPing(ip);
    }
    else if (pingType == PING_ICMP) {
        pingHostIcmp_.addHostForPing(ip);
    }
    else {
        WS_ASSERT(false);
    }
#endif
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
