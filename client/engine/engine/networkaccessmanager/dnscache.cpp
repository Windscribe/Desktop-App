#include "dnscache.h"
#include <QDateTime>
#include <QTimer>
#include "utils/ws_assert.h"
#include "engine/dnsresolver/dnsrequest.h"

DnsCache::DnsCache(QObject *parent, int cacheTimeoutMs /*= 60000*/, int reviewCacheIntervalMs /*= 1000*/) : QObject(parent),
    cacheTimeoutMs_(cacheTimeoutMs)
{
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &DnsCache::onTimer);
    timer->start(reviewCacheIntervalMs);
}

DnsCache::~DnsCache()
{
}

void DnsCache::resolve(const QString &hostname, quint64 id, bool bypassCache /*= false*/, const QStringList &dnsServers /*= QStringList()*/, int timeoutMs /*= 5000*/)
{
    if (!bypassCache) {
        auto it = cache_.find(hostname);
        if (it != cache_.end()) {
            emit resolved(true, it.value().ips, id, true, 0);
            return;
        }
    }

    DnsRequest *dnsRequest = new DnsRequest(this, hostname, dnsServers, timeoutMs);
    dnsRequest->setProperty("requestId", id);
    connect(dnsRequest, &DnsRequest::finished, this, &DnsCache::onDnsRequestFinished);
    dnsRequest->lookup();
}

void DnsCache::onDnsRequestFinished()
{
    DnsRequest *dnsRequest = qobject_cast<DnsRequest *>(sender());
    WS_ASSERT(dnsRequest != nullptr);

    bool bOk;
    quint64 requestId = dnsRequest->property("requestId").toULongLong(&bOk);
    WS_ASSERT(bOk);

    bool bSuccess = false;
    if (!dnsRequest->isError()) {
        cache_[dnsRequest->hostname()].ips = dnsRequest->ips();
        cache_[dnsRequest->hostname()].time = QDateTime::currentMSecsSinceEpoch();
        bSuccess = true;
    }

    emit resolved(bSuccess, dnsRequest->ips(), requestId, false, dnsRequest->elapsedMs());
    dnsRequest->deleteLater();
}

void DnsCache::onTimer()
{
    // delete outdated IPs from cache
    auto it = cache_.begin();
    qint64 curTime = QDateTime::currentMSecsSinceEpoch();
    while (it != cache_.end())
        if ((curTime - it.value().time) > cacheTimeoutMs_)
            it = cache_.erase(it);
        else
            ++it;
}

