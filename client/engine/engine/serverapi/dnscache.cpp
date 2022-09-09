#include "dnscache.h"
#include <QDateTime>
#include "utils/ws_assert.h"
#include "utils/ipvalidation.h"
#include "engine/dnsresolver/dnsrequest.h"
#include "engine/dnsresolver/dnsserversconfiguration.h"


DnsCache::DnsCache(QObject *parent) : QObject(parent)
{
}

DnsCache::~DnsCache()
{
}

void DnsCache::resolve(const QString &hostname, void *userData, qint64 requestStartTime)
{
    resolve(hostname, -1, userData, requestStartTime);
}

void DnsCache::resolve(const QString &hostname, int cacheTimeout, void *userData, qint64 requestStartTime)
{
    // if hostname this is IP then return immediatelly
    if (IpValidation::instance().isIp(hostname))
    {
        checkForNewIps(hostname);
        emit resolved(true, userData, requestStartTime, QStringList() << hostname);
        return;
    }

    if (cacheTimeout < 0)
        cacheTimeout = CACHE_TIMEOUT;

    if (cacheTimeout > 0) {
        auto it = resolvedHosts_.find(hostname);
        if (it != resolvedHosts_.end())
        {
            qint64 curTime = QDateTime::currentMSecsSinceEpoch();
            if ((curTime - it.value().time) <= cacheTimeout)
            {
                emit resolved(true, userData, requestStartTime, it.value().ips);
                return;
            }
        }
    }

    PendingResolvingHosts prh;
    prh.hostname = hostname;
    prh.userData = userData;
    prh.requestStartTime = requestStartTime;
    pendingHosts_ << prh;

    if (!resolvingHostsInProgress_.contains(hostname))
    {
        resolvingHostsInProgress_ << hostname;
        DnsRequest *dnsRequest = new DnsRequest(this, hostname, DnsServersConfiguration::instance().getCurrentDnsServers());
        connect(dnsRequest, SIGNAL(finished()), SLOT(onDnsRequestFinished()));
        dnsRequest->lookup();
    }
}

void DnsCache::onDnsRequestFinished()
{
    DnsRequest *dnsRequest = qobject_cast<DnsRequest *>(sender());
    WS_ASSERT(dnsRequest != nullptr);

    bool bSuccess = false;
    QStringList ips;
    if (!dnsRequest->isError())
    {
        checkForNewIps(dnsRequest->ips());

        ResolvedHostInfo rhi;
        rhi.time = QDateTime::currentMSecsSinceEpoch();
        for (const QString &ip : dnsRequest->ips())
        {
            ips << ip;
        }
        rhi.ips = ips;
        resolvedHosts_[dnsRequest->hostname()] = rhi;
        bSuccess = true;
    }

    auto it = pendingHosts_.begin();
    while (it != pendingHosts_.end())
    {
        if (it->hostname == dnsRequest->hostname())
        {
            emit resolved(bSuccess, it->userData, it->requestStartTime, ips);
            it = pendingHosts_.erase(it);
        }
        else
        {
            ++it;
        }
    }

    resolvingHostsInProgress_.remove(dnsRequest->hostname());

    dnsRequest->deleteLater();
}

void DnsCache::checkForNewIps(const QStringList &newIps)
{
    bool bNewIps = false;

    for (const QString &ip : newIps)
    {
        if (!resolvedIps_.contains(ip))
        {
            resolvedIps_ << ip;
            bNewIps = true;
        }
    }

    if (bNewIps)
    {
        QStringList ips;
        for (const QString &ip : qAsConst(resolvedIps_))
        {
            ips << ip;
        }
        emit ipsInCachChanged(ips);
    }
}

void DnsCache::checkForNewIps(const QString &ip)
{
    bool bNewIps = false;
    if (!resolvedIps_.contains(ip))
    {
        resolvedIps_ << ip;
        bNewIps = true;
    }

    if (bNewIps)
    {
        QStringList ips;
        for (const QString &resolved_ip : qAsConst(resolvedIps_))
        {
            ips << resolved_ip;
        }
        emit ipsInCachChanged(ips);
    }
}
