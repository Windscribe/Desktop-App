#pragma once

#include <QThreadPool>
#include "areslibraryinit.h"

// Singleton for dns requests. Do not use it directly. Use DnsLookup instead
class DnsResolver : public QObject
{
    Q_OBJECT

public:
    static DnsResolver &instance()
    {
        static DnsResolver s;
        return s;
    }

    void lookup(const QString &hostname, QSharedPointer<QObject> object, const QStringList &dnsServers, int timeoutMs);
    QStringList lookupBlocked(const QString &hostname, const QStringList &dnsServers, int timeoutMs, int *outErrorCode);

private:
    explicit DnsResolver(QObject *parent = nullptr);
    virtual ~DnsResolver();

private:
    AresLibraryInit aresLibraryInit_;
    QThreadPool *threadPool_;
};

