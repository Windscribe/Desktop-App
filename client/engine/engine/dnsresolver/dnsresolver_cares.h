#pragma once

#include <QThreadPool>
#include "idnsresolver.h"
#include "areslibraryinit.h"

// DnsResolver implementation based on the cares library
class DnsResolver_cares : public IDnsResolver
{

public:
    static DnsResolver_cares &instance()
    {
        static DnsResolver_cares s;
        return s;
    }

    void lookup(const QString &hostname, QSharedPointer<QObject> object, const QStringList &dnsServers, int timeoutMs) override;
    QStringList lookupBlocked(const QString &hostname, const QStringList &dnsServers, int timeoutMs, QString *outError) override;

private:
    explicit DnsResolver_cares();
    virtual ~DnsResolver_cares();

private:
    AresLibraryInit aresLibraryInit_;
    QThreadPool *threadPool_;
};

