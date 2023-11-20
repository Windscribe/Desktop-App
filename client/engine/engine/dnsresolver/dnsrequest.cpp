#include "dnsrequest.h"
#include "idnsresolver.h"
#include "utils/logger.h"
#include "dnsresolver_cares.h"

DnsRequest::DnsRequest(QObject *parent, const QString &hostname, const QStringList &dnsServers, int timeoutMs /*= 5000*/)
    : QObject(parent),
    dnsResolver_(DnsResolver_cares::instance()),
    hostname_(hostname),
    dnsServers_(dnsServers),
    timeoutMs_(timeoutMs)
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
    return !error_.isEmpty() || ips_.isEmpty();
}

QString DnsRequest::errorString() const
{
    return error_;
}

qint64 DnsRequest::elapsedMs() const
{
    return elapsedMs_;
}

void DnsRequest::lookup()
{
   privateDnsRequestObject_ = QSharedPointer<DnsRequestPrivate>(new DnsRequestPrivate, &QObject::deleteLater);
   privateDnsRequestObject_->moveToThread(this->thread());
   connect(privateDnsRequestObject_.staticCast<DnsRequestPrivate>().get(), &DnsRequestPrivate::resolved, this, &DnsRequest::onResolved);
   dnsResolver_.lookup(hostname_, privateDnsRequestObject_.staticCast<QObject>(), dnsServers_, timeoutMs_);
}

void DnsRequest::lookupBlocked()
{
    ips_ = dnsResolver_.lookupBlocked(hostname_, dnsServers_, timeoutMs_, &error_);
}

void DnsRequest::onResolved(const QStringList &ips, const QString &error, qint64 elapsedMs)
{
    elapsedMs_ = elapsedMs;
    error_ = error;
    ips_ = ips;
    if (isError()) {
        qCDebug(LOG_NETWORK) << "Could not resolve" << hostname_ << "(servers:" << dnsServers_ << "):" << error_;
    }
    emit finished();
}

void DnsRequestPrivate::onResolved(const QStringList &ips, const QString &error, qint64 elapsedMs)
{
    emit resolved(ips, error, elapsedMs);
}
