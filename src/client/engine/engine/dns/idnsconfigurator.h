#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

#include "engine/connectionmanager/adaptergatewayinfo.h"
#include "types/connecteddnsinfo.h"

// Owns everything connected-DNS: the ConnectedDnsInfo settings, the ctrld DNS proxy, the VPN
// adapter DNS override and the macOS system DNS restore. Engine constructs and owns the production
// implementation and feeds it settings; ConnectionManager sequences the attempt-scoped calls
// (prepare/stopDnsProxy/restoreSystemDns and the DNS queries) without any DNS knowledge of its own.
// The exception is applyWhileConnected, which Engine sequences from its connected handler.
class IDnsConfigurator : public QObject
{
    Q_OBJECT

public:
    explicit IDnsConfigurator(QObject *parent) : QObject(parent) {}
    virtual ~IDnsConfigurator() {}

    virtual void setConnectedDnsInfo(const types::ConnectedDnsInfo &info) = 0;

    // Per connection attempt, before the connector starts: disables OS DoH for auto-DNS, starts the
    // ctrld proxy when the upstream requires it, and excludes the custom DNS addresses from DNS leak
    // protection on Windows. Returns false when ctrld failed to start.
    virtual bool prepare() = 0;

    // Extra DNS IPs that must be permitted through DNS-leak protection (ctrld listen IP plus its
    // plain-DNS :53 upstreams). These are NOT system resolvers; they only bypass the :53 leak block
    // so ctrld can reach its upstreams. Consumed by the engine when pushing ConnectStatus.
    virtual const QStringList &dnsWhitelistIps() const = 0;

    // The single DNS server the connector should use, empty for auto-DNS (the tunnel's own DNS applies).
    virtual QString primaryDnsServer() const = 0;
    // The DNS servers the tunnel will use, for firewall exceptions; defaultDns is the tunnel's own
    // DNS, used for auto-DNS.
    virtual QStringList tunnelDnsServers(const QString &defaultDns) const = 0;

    // Rewrites the adapter's DNS servers when a custom/local DNS overrides the tunnel-supplied ones.
    virtual void overrideAdapterDns(AdapterGatewayInfo &adapterInfo) const = 0;

    // Windows: sets the custom DNS on the VPN adapter once connected (WireGuard configures its own).
    virtual void applyWhileConnected(const AdapterGatewayInfo &vpnAdapter, bool isWireGuard) = 0;

    // Stops the ctrld proxy if it is running.
    virtual void stopDnsProxy() = 0;

    // macOS: restores the system DNS the tunnel replaced.
    virtual void restoreSystemDns() = 0;
};
