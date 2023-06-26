#pragma once

#include "idnsresolver.h"
#include <QSet>
#include <QSharedPointer>
#include <QMutex>

struct ActiveDnsRequests;

// DnsResolver implementation based on the DnsQueryEx function (for Windows)
class DnsResolver_win : public IDnsResolver
{

public:
    static DnsResolver_win &instance()
    {
        static DnsResolver_win s;
        return s;
    }

    void lookup(const QString &hostname, QSharedPointer<QObject> object, const QStringList &dnsServers, int timeoutMs) override;
    QStringList lookupBlocked(const QString &hostname, const QStringList &dnsServers, int timeoutMs, QString *outError) override;

private:
    explicit DnsResolver_win();
    virtual ~DnsResolver_win();

private:
    ActiveDnsRequests *activeDnsRequests_;
    unsigned char *nextRequestId_ = 0;     // the identifier is declared as a pointer so that it can be passed in the callback function
};

