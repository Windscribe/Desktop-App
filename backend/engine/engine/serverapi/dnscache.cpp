#include "dnscache.h"
#include <QDateTime>
#include "utils/ipvalidation.h"
#include "engine/dnsresolver/dnsresolver.h"

DnsCache::DnsCache(QObject *parent) : QObject(parent)
{
    connect(&DnsResolver::instance(), SIGNAL(resolved(QString,QStringList,void *)), SLOT(onDnsResolverFinished(QString, QStringList,void *)));
}

DnsCache::~DnsCache()
{
}

void DnsCache::resolve(const QString &hostname, void *userData)
{
    resolve(hostname, -1, userData);
}

void DnsCache::resolve(const QString &hostname, int cacheTimeout, void *userData)
{
    // if hostname this is IP then return immediatelly
    if (IpValidation::instance().isIp(hostname))
    {
        checkForNewIps(hostname);
        emit resolved(true, userData, QStringList() << hostname);
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
                emit resolved(true, userData, it.value().ips);
                return;
            }
        }
    }

    PendingResolvingHosts prh;
    prh.hostname = hostname;
    prh.userData = userData;
    pendingHosts_ << prh;

    if (!resolvingHostsInProgress_.contains(hostname))
    {
        resolvingHostsInProgress_ << hostname;
        DnsResolver::instance().lookup(hostname, this);
    }
}

void DnsCache::onDnsResolverFinished(const QString &hostname, const QStringList &resolvedIps, void *userPointer)
{
    if (userPointer == this)
    {
        //qDebug() << "DnsCache::onDnsResolverFinished, hostname=" << hostname << ", " << hostInfo.addresses();
        bool bSuccess = false;
        QStringList ips;
        if (!resolvedIps.isEmpty())
        {
            checkForNewIps(resolvedIps);
            ///hookAddrInfo_->setCache(hostname, hostInfo.addresses());

            ResolvedHostInfo rhi;
            rhi.time = QDateTime::currentMSecsSinceEpoch();
            for (const QString &ip : resolvedIps)
            {
                ips << ip;
            }
            rhi.ips = ips;
            resolvedHosts_[hostname] = rhi;
            bSuccess = true;
        }

        auto it = pendingHosts_.begin();
        while (it != pendingHosts_.end())
        {
            if (it->hostname == hostname)
            {
                emit resolved(bSuccess, it->userData, ips);
                it = pendingHosts_.erase(it);
            }
            else
            {
                ++it;
            }
        }

        resolvingHostsInProgress_.remove(hostname);
    }
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
