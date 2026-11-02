#pragma once

#include <string>

struct AmneziawgConfig;

namespace WireGuardConfigWriter {

// Authors the wireguard-windows .conf from validated parameters. The helper is the sole
// author: a caller of configureWireGuard supplies fields, never a finished config document.
// AllowedIPs catch-all /0 entries are expanded to /1 pairs so tunnel.dll does not install
// its own WFP firewall (https://git.zx2c4.com/wireguard-windows/about/docs/netquirk.md).
// Returns an empty string when a field is missing or would inject a directive.
std::wstring generateConfigFile(const std::wstring &clientPrivateKey,
                                const std::wstring &clientIpAddress,
                                const std::wstring &clientDnsAddress,
                                const std::wstring &clientListenPort,
                                const std::wstring &peerPublicKey,
                                const std::wstring &peerPresharedKey,
                                const std::wstring &peerEndpoint,
                                const std::wstring &allowedIps,
                                const std::wstring &persistentKeepalive,
                                const AmneziawgConfig &amneziawgConfig);

} // namespace WireGuardConfigWriter
