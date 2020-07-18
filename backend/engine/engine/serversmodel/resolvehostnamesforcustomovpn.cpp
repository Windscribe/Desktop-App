#include "resolvehostnamesforcustomovpn.h"
#include "utils/ipvalidation.h"
#include "utils/logger.h"

ResolveHostnamesForCustomOvpn::ResolveHostnamesForCustomOvpn(QObject *parent) : QObject(parent),
    nextUniqueId_(0)
{
    connect(&DnsResolver::instance(), SIGNAL(resolved(QString,QHostInfo,void *, quint64)), SLOT(onDnsResolvedFinished(QString, QHostInfo,void *, quint64)));
}

bool ResolveHostnamesForCustomOvpn::resolve(QSharedPointer<ServerLocation> location)
{
    location_ = *location;

    dnsResolversInProgress_.clear();
    QVector<ServerNode> &nodes = location_.getNodes();
    for (int i = 0; i < nodes.count(); ++i)
    {
        if (!IpValidation::instance().isIp(nodes[i].getIpForPing()))
        {
            dnsResolversInProgress_[nodes[i].getIpForPing()] = nextUniqueId_++;
        }
    }

    if (dnsResolversInProgress_.count() > 0)
    {
        for (auto it = dnsResolversInProgress_.begin(); it != dnsResolversInProgress_.end(); ++it)
        {
            DnsResolver::instance().lookup(it.key(), true, this, it.value());
        }
        return true;
    }
    else
    {
        return false;
    }
}

void ResolveHostnamesForCustomOvpn::onDnsResolvedFinished(const QString &hostname, const QHostInfo &hostInfo, void *userPointer, quint64 userId)
{
    if (userPointer == this)
    {
        bool bFound = false;
        for (auto it = dnsResolversInProgress_.begin(); it != dnsResolversInProgress_.end(); ++it)
        {
            if (it.value() == userId)
            {
                bFound = true;
                break;
            }
        }
        if (bFound)
        {
            QVector<ServerNode> &nodes = location_.getNodes();
            for (int i = 0; i < nodes.count(); ++i)
            {
                if (nodes[i].getIpForPing() == hostname)
                {
                    if (hostInfo.error() == QHostInfo::NoError && hostInfo.addresses().count() > 0)
                    {
                        nodes[i].setIpForCustomOvpnConfig(hostInfo.addresses()[0].toString());
                    }
                    else
                    {
                        qCDebug(LOG_CUSTOM_OVPN) << "Failed to resolve" << hostname << "for custom config";
                    }
                }
            }

            dnsResolversInProgress_.remove(hostname);

            if (dnsResolversInProgress_.isEmpty())
            {
                QSharedPointer<ServerLocation> newLocation(new ServerLocation);
                *newLocation = location_;
                emit resolved(newLocation);
            }
        }
    }
}
