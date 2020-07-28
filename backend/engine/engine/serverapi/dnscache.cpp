#include "dnscache.h"
#include <QDateTime>
#include "utils/ipvalidation.h"
#include "engine/dnsresolver/dnsresolver.h"

DnsCache::DnsCache(QObject *parent) : QObject(parent)
{
    connect(&DnsResolver::instance(), SIGNAL(resolved(QString,QHostInfo,void *, quint64)), SLOT(onDnsResolverFinished(QString, QHostInfo,void *, quint64)));
}

DnsCache::~DnsCache()
{
}

void DnsCache::resolve(const QString &hostname, bool bUseCustomDns, void *userData)
{
    resolve(hostname, -1, bUseCustomDns, userData);
}

void DnsCache::resolve(const QString &hostname, int cacheTimeout, bool bUseCustomDns, void *userData)
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
        DnsResolver::instance().lookup(hostname, bUseCustomDns, this, 0);
    }
}

void DnsCache::onDnsResolverFinished(const QString &hostname, const QHostInfo &hostInfo, void *userPointer, quint64 userId)
{
    Q_UNUSED(userId);
    if (userPointer == this)
    {
        //qDebug() << "DnsCache::onDnsResolverFinished, hostname=" << hostname << ", " << hostInfo.addresses();
        bool bSuccess = false;
        QStringList ips;
        if (hostInfo.error() == QHostInfo::NoError && hostInfo.addresses().count() > 0)
        {
            checkForNewIps(hostInfo);
            ///hookAddrInfo_->setCache(hostname, hostInfo.addresses());

            ResolvedHostInfo rhi;
            rhi.time = QDateTime::currentMSecsSinceEpoch();
            Q_FOREACH(const QHostAddress &ha, hostInfo.addresses())
            {
                ips << ha.toString();
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

void DnsCache::checkForNewIps(const QHostInfo &hostInfo)
{
    bool bNewIps = false;
    QList<QHostAddress> list = hostInfo.addresses();
    for (auto it = list.begin(); it != list.end(); ++it)
    {
        QString ip = it->toString();
        if (!resolvedIps_.contains(ip))
        {
            resolvedIps_ << ip;
            bNewIps = true;
        }
    }

    if (bNewIps)
    {
        QStringList ips;
        Q_FOREACH(const QString &ip, resolvedIps_)
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
        Q_FOREACH(const QString &ip, resolvedIps_)
        {
            ips << ip;
        }
        emit ipsInCachChanged(ips);
    }
}
