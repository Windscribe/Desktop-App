#pragma once

// The interfaces the helper creates for WireGuard and OpenVPN. The names are fixed so that the NetworkManager
// drop-in installed with the app (usr/lib/NetworkManager/conf.d/90-<product>.conf) can leave them
// unmanaged. NetworkManager otherwise adopts each one as an external connection and runs its
// connectivity check on it, which stalls NetworkManager for seconds on every connect and disconnect,
// and with it every nmcli call the client and helper make. The tunnelinterfacenames test keeps the
// drop-in in sync with these names.
namespace TunnelInterfaceNames
{
inline constexpr const char *kWireGuard = "utun420";
inline constexpr const char *kOpenVpn = "tun420";
}
