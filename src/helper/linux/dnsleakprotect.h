#pragma once

#include <string>
#include <vector>

// Linux DNS-leak protection. Snapshots the OS-default DNS resolvers (via resolvectl / nmcli) plus the
// physical default gateway, and installs a `dnsleaks` chain in the `inet windscribe` table that drops
// UDP/TCP port 53 to every snapshotted resolver except loopback, the tunnel interface, and the
// tunnel's own DNS servers. The privileged helper is the sole author of the rules; only the validated
// intent crosses the IPC boundary.
//
// The chain is a base chain at filter/output priority -20 (ahead of the firewall output chain) with
// policy accept, so it only ever DROPs leaking DNS and leaves every other verdict to the firewall.
namespace DnsLeakProtect {

// Snapshots OS DNS and installs/refreshes the dnsleaks chain. vpnInterfaceName (may be empty) and
// allowedDnsServers (the tunnel's resolvers) are excluded from the blocklist. Returns false if the
// nft apply fails.
//
// Resolvers are read per-link (resolvectl/nmcli), so the physical link's servers are captured
// correctly even after the tunnel resolver is in place. defaultGatewayV4 is the physical default
// gateway (commonly a LAN DNS forwarder); it must be supplied by the caller because by connect time
// the live default route points at the tunnel, so reading it here would miss the physical gateway.
// Empty when unknown.
bool enable(const std::string &vpnInterfaceName, const std::vector<std::string> &allowedDnsServers,
            const std::string &defaultGatewayV4);

// Removes the dnsleaks chain.
bool disable();

} // namespace DnsLeakProtect
