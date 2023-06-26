#pragma once

#include <QObject>
#include <QString>

// Interface for DnsResolver
class IDnsResolver
{

public:
    virtual ~IDnsResolver() {};

    virtual void lookup(const QString &hostname, QSharedPointer<QObject> object, const QStringList &dnsServers, int timeoutMs) = 0;
    virtual QStringList lookupBlocked(const QString &hostname, const QStringList &dnsServers, int timeoutMs, QString *outError) = 0;
};

