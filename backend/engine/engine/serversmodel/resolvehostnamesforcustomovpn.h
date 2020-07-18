#ifndef RESOLVEHOSTNAMESFORCUSTOMOVPN_H
#define RESOLVEHOSTNAMESFORCUSTOMOVPN_H

#include <QObject>
#include "Engine/Types/serverlocation.h"
#include "Engine/DnsResolver/dnsresolver.h"

// helper class for resolve hostnames for custom ovpn location (used in ServersModel)
class ResolveHostnamesForCustomOvpn : public QObject
{
    Q_OBJECT
public:
    explicit ResolveHostnamesForCustomOvpn(QObject *parent);

    // return false, if no need resolution
    // return true and later emit signal resolved, if need at least one DNS-resolution
    bool resolve(QSharedPointer<ServerLocation> location);

signals:
    void resolved(QSharedPointer<ServerLocation> location);

private slots:
    void onDnsResolvedFinished(const QString &hostname, const QHostInfo &hostInfo, void *userPointer, quint64 userId);

private:
    ServerLocation location_;

    quint64 nextUniqueId_;
    QHash<QString, quint64> dnsResolversInProgress_;
};

#endif // RESOLVEHOSTNAMESFORCUSTOMOVPN_H
