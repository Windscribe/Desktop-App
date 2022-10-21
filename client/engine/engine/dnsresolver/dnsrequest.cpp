#include "dnsrequest.h"
#include "dnsresolver.h"
#include "utils/logger.h"
#include "ares.h"

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

QString DnsRequest::errorString() const
{
    return QString::fromStdString(ares_strerror(aresErrorCode_));
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
   DnsResolver::instance().lookup(hostname_, privateDnsRequestObject_.staticCast<QObject>(), dnsServers_, timeoutMs_);
}

void DnsRequest::lookupBlocked()
{
    ips_ = DnsResolver::instance().lookupBlocked(hostname_, dnsServers_, timeoutMs_, &aresErrorCode_);
}

void DnsRequest::onResolved(const QStringList &ips, int aresErrorCode, qint64 elapsedMs)
{
    elapsedMs_ = elapsedMs;
    aresErrorCode_ = aresErrorCode;
    ips_ = ips;
    if (isError()) {
        qCDebug(LOG_NETWORK) << "Could not resolve" << hostname_ << "(servers:" << dnsServers_ << "):" << aresErrorCode;
    }
    emit finished();
}

void DnsRequestPrivate::onResolved(const QStringList &ips, int aresErrorCode, qint64 elapsedMs)
{
    emit resolved(ips, aresErrorCode, elapsedMs);
}
