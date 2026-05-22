#include "proxydestinationfilter.h"

#include <QPointer>
#include <QTimer>
#include <memory>
#include <wsnet/WSNet.h>

namespace ProxyDestinationFilter {

namespace {

constexpr int kLookupTimeoutMs = 5000;

// Convert IPv4-mapped IPv6 (::ffff:a.b.c.d) to its underlying IPv4 form so that range checks against IPv4 loopback,
// RFC1918, etc. fire even when the destination is encoded as IPv6.
QHostAddress canonicalize(const QHostAddress &addr)
{
    if (addr.protocol() == QAbstractSocket::IPv6Protocol) {
        bool ok = false;
        quint32 v4 = addr.toIPv4Address(&ok);
        if (ok) return QHostAddress(v4);
    }
    return addr;
}

bool inSubnet(const QHostAddress &addr, const QHostAddress &subnet, int prefix)
{
    return addr.isInSubnet(subnet, prefix);
}

bool isLoopback(const QHostAddress &addr)
{
    if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
        return inSubnet(addr, QHostAddress("127.0.0.0"), 8);
    }
    if (addr.protocol() == QAbstractSocket::IPv6Protocol) {
        return addr == QHostAddress(QHostAddress::LocalHostIPv6);
    }
    return false;
}

bool isUnspecified(const QHostAddress &addr)
{
    return addr == QHostAddress(QHostAddress::AnyIPv4) ||
           addr == QHostAddress(QHostAddress::AnyIPv6) ||
           addr.isNull();
}

bool isMulticastOrReserved(const QHostAddress &addr)
{
    if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
        // 224.0.0.0/4 multicast, 240.0.0.0/4 reserved (incl. 255.255.255.255 broadcast)
        if (inSubnet(addr, QHostAddress("224.0.0.0"), 4)) return true;
        if (inSubnet(addr, QHostAddress("240.0.0.0"), 4)) return true;
    } else if (addr.protocol() == QAbstractSocket::IPv6Protocol) {
        // ff00::/8 multicast
        if (inSubnet(addr, QHostAddress("ff00::"), 8)) return true;
    }
    return false;
}

bool isLinkLocal(const QHostAddress &addr)
{
    if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
        return inSubnet(addr, QHostAddress("169.254.0.0"), 16);
    } else if (addr.protocol() == QAbstractSocket::IPv6Protocol) {
        return inSubnet(addr, QHostAddress("fe80::"), 10);
    }
    return false;
}

bool isPrivateRfc1918(const QHostAddress &addr)
{
    if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
        if (inSubnet(addr, QHostAddress("10.0.0.0"), 8)) return true;
        if (inSubnet(addr, QHostAddress("172.16.0.0"), 12)) return true;
        if (inSubnet(addr, QHostAddress("192.168.0.0"), 16)) return true;
    } else if (addr.protocol() == QAbstractSocket::IPv6Protocol) {
        if (inSubnet(addr, QHostAddress("fc00::"), 7)) return true;        // ULA
    }
    return false;
}

bool isPrivateOrLinkLocal(const QHostAddress &addr)
{
    return isLinkLocal(addr) || isPrivateRfc1918(addr);
}

}  // namespace

bool isAllowedDestination(const QHostAddress &addr)
{
    const QHostAddress canon = canonicalize(addr);
    if (isUnspecified(canon)) return false;
    if (isLoopback(canon)) return false;
    if (isMulticastOrReserved(canon)) return false;
    // Link-local (169.254/16, fe80::/10) is never a legitimate proxy destination on a real LAN — it's the OS's
    // "DHCP failed" fallback. The point is to block 169.254.169.254 (cloud-provider instance metadata): a classic
    // SSRF-to-account-takeover target if Windscribe is ever run on a cloud VM, and there's no functional cost on
    // a normal home/office network where 169.254 addresses don't appear.
    if (isLinkLocal(canon)) return false;
    // RFC1918 (10/8, 172.16/12, 192.168/16) and IPv6 ULA are deliberately allowed. The proxy gateway is designed to
    // be set as a system-wide proxy on same-LAN clients: if it refused private destinations, those clients would
    // lose access to their own LAN devices (printers, NAS, router admin, etc.) the moment they pointed at us — that
    // would defeat the use case. The tradeoff is that any peer reaching the proxy can also reach anything else on
    // the proxy host's LAN, which we accept because peer access is already gated by isAllowedPeer (peer must be on
    // a private range or in the bind subnet) and, when configured, by basic auth.
    return true;
}

bool isAllowedPeer(const QHostAddress &peer, const QHostAddress &bindIp, int prefixLength)
{
    const QHostAddress canon = canonicalize(peer);
    if (canon.isNull()) return false;
    if (isPrivateOrLinkLocal(canon)) return true;
    if (!bindIp.isNull() && prefixLength > 0 && canon.isInSubnet(bindIp, prefixLength)) return true;
    return false;
}

std::shared_ptr<wsnet::WSNetCancelableCallback>
resolve(const QString &host, QObject *context, std::function<void(bool resolved, const QList<QHostAddress> &)> cb)
{
    QHostAddress literal(host);
    if (!literal.isNull()) {
        QList<QHostAddress> result;
        if (isAllowedDestination(literal)) {
            result << canonicalize(literal);
        }
        QPointer<QObject> guard(context);
        auto cbCopy = std::move(cb);
        QTimer::singleShot(0, context, [guard, cbCopy = std::move(cbCopy), result]() {
            if (guard) {
                cbCopy(true, result);
            }
        });
        return nullptr;
    }

    // Use wsnet's resolver (c-ares with explicit DNS servers) instead of QHostInfo. The OS-level resolver caches the
    // pre-VPN DNS servers and the firewall blocks lookups against them once VPN is up; wsnet's resolver respects the
    // VPN-pushed DNS configuration set by Engine::onConnected and friends.
    QPointer<QObject> guard(context);
    auto fired = std::make_shared<bool>(false);
    auto cbCopy = std::make_shared<std::function<void(bool, const QList<QHostAddress> &)>>(std::move(cb));

    auto onLookup = [guard, fired, cbCopy](std::uint64_t, const std::string &, std::shared_ptr<wsnet::WSNetDnsRequestResult> result) {
        // wsnet calls us on its own worker thread. Hop to the context's thread via invokeMethod, but guard against
        // the context having been destroyed in the meantime. The caller is expected to cancel() this lookup before
        // destroying `context`; cancel() shares a mutex with this callback, so once the caller has canceled we are
        // guaranteed not to be running here, which is what makes the QPointer dereference safe.
        QObject *target = guard.data();
        if (!target) {
            return;
        }
        QMetaObject::invokeMethod(target, [guard, fired, cbCopy, result]() {
            if (!guard || *fired) {
                return;
            }
            *fired = true;
            const bool resolved = !result->error() || result->error()->isSuccess();
            QList<QHostAddress> filtered;
            if (resolved) {
                for (const std::string &ip : result->ips()) {
                    QHostAddress addr(QString::fromStdString(ip));
                    if (!addr.isNull() && isAllowedDestination(addr)) {
                        filtered << canonicalize(addr);
                    }
                }
            }
            (*cbCopy)(resolved, filtered);
        });
    };

    auto cancelable = wsnet::WSNet::instance()->dnsResolver()->lookup(host.toStdString(), 0, wsnet::IpFamily::kIpv4, onLookup);

    QTimer::singleShot(kLookupTimeoutMs, context, [guard, fired, cbCopy, cancelable]() {
        if (!guard || *fired) {
            return;
        }
        *fired = true;
        if (cancelable) {
            cancelable->cancel();
        }
        (*cbCopy)(false, {});
    });

    return cancelable;
}

}  // namespace ProxyDestinationFilter
