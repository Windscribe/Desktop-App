#include "pingmultiplehosts.h"

#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "utils/ws_assert.h"
#include "utils/extraconfig.h"

#include "pinghost_curl.h"
#include "pinghost_tcp.h"

#ifdef Q_OS_WIN
    #include "pinghost_icmp_win.h"
#else
    #include "pinghost_icmp_posix.h"
#endif

PingMultipleHosts::PingMultipleHosts(QObject *parent, IConnectStateController *connectStateController, NetworkAccessManager *networkAccessManager) :
    QObject(parent), connectStateController_(connectStateController), networkAccessManager_(networkAccessManager)
{
}

void PingMultipleHosts::addHostForPing(const QString &ip, const QString &hostname, PingType pingType)
{
    // is host already pinging?
    if (pingingHosts_.contains(ip))
        return;

    pingingHosts_.insert(ip);

    // If custom options ws-use-icmp-pings is enabled, then override method of ping
    IPingHost *pingHost = createPingHost(ip, hostname, ExtraConfig::instance().getUseICMPPings() ? PingType::kIcmp : pingType);
    connect(pingHost, &IPingHost::pingFinished, this, &PingMultipleHosts::onPingFinished);

    // We do curl requests right away
    if (pingType == PingType::kCurl) {
        pingHost->setProperty("fromDisconnectedState", connectStateController_ ? connectStateController_->currentState() == CONNECT_STATE_DISCONNECTED : false);
        pingHost->setProperty("isParrallelPing", false);
        pingHost->ping();
    // while tcp and icmp are in parallel
    } else {
        pingHost->setProperty("isParrallelPing", true);
        waitingPingsQueue_.enqueue(pingHost);
        processNextPingsInQueue();
    }
}

void PingMultipleHosts::setProxySettings(const types::ProxySettings &proxySettings)
{
    proxySettings_ = proxySettings;
}

void PingMultipleHosts::disableProxy()
{
    isProxyEnabled_ = false;
}

void PingMultipleHosts::enableProxy()
{
    isProxyEnabled_ = true;
}

void PingMultipleHosts::onPingFinished(bool bSuccess, int timems, const QString &ip)
{
    QObject *obj = sender();
    bool bFromDisconnectedState = obj->property("fromDisconnectedState").toBool();
    obj->deleteLater();
    pingingHosts_.remove(ip);

    if (obj->property("isParrallelPing").toBool()) {
        curParallelPings_--;
        WS_ASSERT(curParallelPings_ >= 0);
    }

    emit pingFinished(bSuccess, timems, ip, bFromDisconnectedState);
    processNextPingsInQueue();
}

void PingMultipleHosts::processNextPingsInQueue()
{
    if (curParallelPings_ < MAX_PARALLEL_PINGS && !waitingPingsQueue_.isEmpty()) {
        IPingHost *pingHost = waitingPingsQueue_.dequeue();
        pingHost->setProperty("fromDisconnectedState", connectStateController_ ? connectStateController_->currentState() == CONNECT_STATE_DISCONNECTED : false);
        curParallelPings_++;
        pingHost->ping();
    }
}

IPingHost *PingMultipleHosts::createPingHost(const QString &ip, const QString &hostname, PingType pingType)
{
    if (pingType == PingType::kCurl) {
        return new PingHost_Curl(this, networkAccessManager_, ip, hostname);
    } else if (pingType == PingType::kIcmp) {
#if defined(Q_OS_WIN)
        return new PingHost_ICMP_win(this, ip);
#else
        return new PingHost_ICMP_posix(this, ip);
#endif
    } else if (pingType == PingType::kTcp) {
        return new PingHost_TCP(this, ip, proxySettings_, isProxyEnabled_);
    }

    WS_ASSERT(false);
    return nullptr;
}
