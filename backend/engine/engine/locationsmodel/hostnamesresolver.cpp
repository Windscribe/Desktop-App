#include "hostnamesresolver.h"
#include "engine/dnsresolver/dnsresolver.h"
#include "utils/logger.h"

namespace locationsmodel {

HostnamesResolver::HostnamesResolver(QObject *parent) : QObject(parent)
{
    connect(&DnsResolver::instance(), SIGNAL(resolved(QString,QHostInfo,void*)), SLOT(onResolved(QString,QHostInfo,void*)));
}

void HostnamesResolver::clear()
{
    hash_.clear();
}

void HostnamesResolver::setHostnames(const QStringList &hostnames)
{
    qCDebug(LOG_CUSTOM_OVPN) << "HostnamesResolver::setHostnames:" << hostnames;

    for (auto hostname : hostnames)
    {
        HostnameInfo hi;
        hash_[hostname] = hi;
        DnsResolver::instance().lookup(hostname, this);
    }
}

QStringList HostnamesResolver::allIps() const
{
    QStringList list;
    for (QHash<QString, HostnameInfo>::const_iterator it = hash_.begin(); it != hash_.end(); ++it)
    {
        Q_ASSERT(it.value().isResolved);
        list << it.value().ips;
    }
    return list;
}

QString HostnamesResolver::hostnameForIp(const QString &ip) const
{
    for (QHash<QString, HostnameInfo>::const_iterator it = hash_.begin(); it != hash_.end(); ++it)
    {
        Q_ASSERT(it.value().isResolved);

        for (const QString &ip_item : it.value().ips)
        {
            if (ip_item == ip)
            {
                return it.key();
            }
        }
    }
    Q_ASSERT(false);
    return "";
}

void HostnamesResolver::setLatencyForIp(const QString &ip, PingTime pingTime)
{
    hashLatencies_[ip] = pingTime.toInt();
}

void HostnamesResolver::clearLatencyInfo()
{
    hashLatencies_.clear();
}

PingTime HostnamesResolver::getLatencyForHostname(const QString &hostname) const
{
    bool bAllIpsHasLatency = true;

    auto it = hash_.find(hostname);
    for (auto ip : it.value().ips)
    {
        auto ipLatency = hashLatencies_.find(ip);
        if (ipLatency != hashLatencies_.end())
        {
            if (ipLatency.value() > 0)
            {
                return ipLatency.value();
            }
        }
        else
        {
            bAllIpsHasLatency = false;
        }
    }
    if (!bAllIpsHasLatency)
    {
        return PingTime::NO_PING_INFO;
    }
    else
    {
        return PingTime::PING_FAILED;
    }
}

void HostnamesResolver::onResolved(const QString &hostname, const QHostInfo &hostInfo, void *userPointer)
{
    if (userPointer == this)
    {
        auto it = hash_.find(hostname);
        if (it != hash_.end())
        {
            if (!hostInfo.addresses().isEmpty())
            {
                for (const QHostAddress ha : hostInfo.addresses())
                {
                    it.value().ips << ha.toString();
                }
            }

            it.value().isResolved = true;
        }

        bool isAllResolved = true;
        for (QHash<QString, HostnameInfo>::iterator it = hash_.begin(); it != hash_.end(); ++it)
        {
            if (!it.value().isResolved)
            {
                isAllResolved = false;
                break;
            }
        }
        if (isAllResolved)
        {
            emit allResolved();
        }
    }
}


} //namespace locationsmodel

