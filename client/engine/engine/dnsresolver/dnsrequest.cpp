#include "dnsrequest.h"
#include "dnsresolver.h"
#include "utils/logger.h"

DnsRequest::DnsRequest(QObject *parent, const QString &hostname, const QStringList &dnsServers, int timeoutMs /*= 5000*/)
    : QObject(parent), hostname_(hostname), dnsServers_(dnsServers), timeoutMs_(timeoutMs), aresErrorCode_(ARES_SUCCESS)
{

}

DnsRequest::~DnsRequest()
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
    return aresErrorCode_ != ARES_SUCCESS || ips_.isEmpty();
}

QString DnsRequest::errorString()
{
    return QString::fromStdString(ares_strerror(aresErrorCode_));
}

void DnsRequest::lookup()
{
   QSharedPointer<DnsRequestPrivate> obj = QSharedPointer<DnsRequestPrivate>(new DnsRequestPrivate, &QObject::deleteLater);
   obj->moveToThread(this->thread());
   connect(obj.get(), SIGNAL(resolved(QStringList, int)), SLOT(onResolved(QStringList, int)));
   DnsResolver::instance().lookup(hostname_, obj.staticCast<QObject>(), dnsServers_, timeoutMs_);
}

void DnsRequest::lookupBlocked()
{
    ips_ = DnsResolver::instance().lookupBlocked(hostname_, dnsServers_, timeoutMs_, &aresErrorCode_);
}

void DnsRequest::onResolved(const QStringList &ips, int aresErrorCode)
{
    aresErrorCode_ = aresErrorCode;
    ips_ = ips;
    if (isError())
    {
        qCDebug(LOG_DNS_RESOLVER) << "Could not resolve" << hostname_ << "(servers:" << dnsServers_ << "):" << aresErrorCode;
    }
    emit finished();
}

void DnsRequestPrivate::onResolved(const QStringList &ips, int aresErrorCode)
{
    emit resolved(ips, aresErrorCode);
}
