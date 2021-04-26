#include "dnsrequest.h"
#include "dnsresolver.h"
#include "dnsserversconfiguration.h"
#include <QDebug>

DnsRequest::DnsRequest(QObject *parent, const QString &hostname) : QObject(parent), hostname_(hostname)
{
}

QStringList DnsRequest::ips() const
{
    return ips_;
}

QString DnsRequest::hostname() const
{
    return hostname_;
}

bool DnsRequest::isError() const
{
    return ips_.isEmpty();
}

void DnsRequest::lookup()
{
   QSharedPointer<DnsRequestPrivate> obj = QSharedPointer<DnsRequestPrivate>(new DnsRequestPrivate, &QObject::deleteLater);
   obj->moveToThread(this->thread());
   connect(obj.get(), SIGNAL(resolved(QStringList)), SLOT(onResolved(QStringList)));
   DnsResolver::instance().lookup(hostname_, obj.staticCast<QObject>(), DnsServersConfiguration::instance().getCurrentDnsServers());
}

void DnsRequest::lookupBlocked()
{
    ips_ = DnsResolver::instance().lookupBlocked(hostname_, DnsServersConfiguration::instance().getCurrentDnsServers());
}

void DnsRequest::onResolved(const QStringList &ips)
{
    ips_ = ips;
    emit finished();
}

void DnsRequestPrivate::onResolved(const QStringList &ips)
{
    emit resolved(ips);
}
