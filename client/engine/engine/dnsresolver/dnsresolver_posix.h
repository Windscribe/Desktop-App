#pragma once

#include <QThreadPool>
#include "idnsresolver.h"
#include "areslibraryinit.h"

// DnsResolver implementation based on the cares library (for Mac/Linux)
class DnsResolver_posix : public IDnsResolver
{

public:
    static DnsResolver_posix &instance()
    {
        static DnsResolver_posix s;
        return s;
    }

    void lookup(const QString &hostname, QSharedPointer<QObject> object, const QStringList &dnsServers, int timeoutMs) override;
    QStringList lookupBlocked(const QString &hostname, const QStringList &dnsServers, int timeoutMs, QString *outError) override;

private:
    explicit DnsResolver_posix();
    virtual ~DnsResolver_posix();

private:
    AresLibraryInit aresLibraryInit_;
    QThreadPool *threadPool_;
};

