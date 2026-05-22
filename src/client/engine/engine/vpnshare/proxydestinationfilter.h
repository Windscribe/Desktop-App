#pragma once

#include <QHostAddress>
#include <QList>
#include <QObject>
#include <QString>
#include <functional>
#include <memory>
#include <wsnet/WSNet.h>

namespace ProxyDestinationFilter {

// Reject loopback, multicast/reserved, and link-local destinations. Link-local (169.254/16, fe80::/10) covers
// 169.254.169.254 (cloud metadata) — never a legitimate proxy destination on a real network.
bool isAllowedDestination(const QHostAddress &addr);

// A peer is allowed if its source address is in any of the standard private ranges (RFC1918,
// CGNAT, link-local, IPv6 ULA, IPv6 link-local) OR sits inside the bind interface's subnet.
// The bind-subnet branch is what lets a same-LAN neighbour reach the proxy when the LAN is not
// in one of the standard private ranges; in the typical RFC1918 case it adds nothing.
bool isAllowedPeer(const QHostAddress &peer, const QHostAddress &bindIp, int prefixLength);

// Callback receives `resolved` = true if the host was an IP literal or DNS lookup succeeded (regardless of whether
// any addresses survived filtering); false if the lookup itself failed or timed out. `allowed` is the survivor list.
// Returns the wsnet cancelable for the in-flight DNS lookup, or nullptr for the IP-literal fast path. Caller should
// hold the returned handle until destruction and call cancel() before destroying `context` — cancel() shares a
// recursive_mutex with the wsnet callback, so it doubles as a synchronization barrier guaranteeing the worker
// thread is no longer inside our outer lambda when destruction runs.
std::shared_ptr<wsnet::WSNetCancelableCallback>
resolve(const QString &host, QObject *context, std::function<void(bool resolved, const QList<QHostAddress> &)> cb);

} // namespace ProxyDestinationFilter
