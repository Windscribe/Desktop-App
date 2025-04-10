#include "dnschecker.h"

#include <QDnsLookup>
#include <QTimer>
#include <QHostAddress>
#include <QEventLoop>

namespace NetworkUtils {

DnsChecker::DnsChecker(QObject *parent) : QObject(parent), dnsLookup_(new QDnsLookup(this))
{
    connect(dnsLookup_, &QDnsLookup::finished, this, &DnsChecker::handleLookupFinished);
    connect(&timer_, &QTimer::timeout, this, &DnsChecker::handleTimeout);

    timer_.setSingleShot(true);
}

DnsChecker::~DnsChecker()
{
}

void DnsChecker::checkAvailability(const QString &dnsAddress, uint port, int timeoutMs)
{
    dnsLookup_->setNameserver(QHostAddress(dnsAddress), port);
    dnsLookup_->setType(QDnsLookup::A);
    dnsLookup_->setName("windscribe.com");

    timer_.start(timeoutMs);
    dnsLookup_->lookup();
}

void DnsChecker::handleLookupFinished()
{
    if (timer_.isActive()) {
        timer_.stop();
        if (dnsLookup_->error() == QDnsLookup::ResolverError ||
            dnsLookup_->error() == QDnsLookup::TimeoutError)
        {
            emit dnsCheckCompleted(false);
        } else {
            emit dnsCheckCompleted(true);
        }
    }
}

void DnsChecker::handleTimeout()
{
    if (dnsLookup_->isFinished() == false) {
        dnsLookup_->abort();
    }

    emit dnsCheckCompleted(false);
}

bool DnsChecker::checkAvailabilityBlocking(const QString &dnsAddress, uint port, int timeoutMs)
{
    QEventLoop eventLoop;
    DnsChecker dnsChecker;
    bool result = false;

    QObject::connect(&dnsChecker, &DnsChecker::dnsCheckCompleted,
                     [&eventLoop, &result](bool available) {
                         result = available;
                         eventLoop.quit();
                     });
    dnsChecker.checkAvailability(dnsAddress, port, timeoutMs);
    eventLoop.exec();

    return result;
}

} // namespace NetworkUtils
