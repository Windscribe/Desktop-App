# Configuration File (Linux CLI-only)

The Linux CLI-only build reads preferences from `~/.config/Windscribe/windscribe_cli.conf` (QSettings INI format). Changes apply automatically as soon as the file is saved.

**⚠️ This applies only to the Linux CLI-only build. Windows, macOS, and Linux GUI builds manage these settings through the GUI.**

## LAN Access

Allow traffic to local network devices while VPN is active:

```ini
[Connection]
AllowLANTraffic=true
```

## Auto-Connect on Startup

Automatically connect to VPN when the service starts:

```ini
[Connection]
Autoconnect=true
```

## Connected DNS

Configure DNS resolution while connected to VPN:

```ini
[Connection]
ConnectedDNSMode=Auto
```

Available modes:
- `Auto` — use Windscribe's DNS (ROBERT filtering, default)
- `Custom` — use a custom DNS server:

```ini
[Connection]
ConnectedDNSMode=Custom
ConnectedDNSUpstream1=1.1.1.1
SplitDNS=false
ConnectedDNSUpstream2=
SplitDNSHostnames=
```

With split DNS enabled, specific domains use a different upstream:

```ini
[Connection]
ConnectedDNSMode=Custom
ConnectedDNSUpstream1=1.1.1.1
SplitDNS=true
ConnectedDNSUpstream2=8.8.8.8
SplitDNSHostnames=example.com, internal.corp.com
```

Upstream values accept IPs or DNS-over-HTTPS/TLS URLs.

## Proxy Sharing

Share the VPN connection as an HTTP or SOCKS proxy on the local network:

```ini
[Connection]
ShareProxyGatewayEnabled=true
ShareProxyGatewayMode=HTTP
ShareProxyGatewayPort=8888
ShareProxyGatewayWhileConnected=true
```

- `ShareProxyGatewayMode` — `HTTP` or `SOCKS`
- `ShareProxyGatewayPort` — port number (1024–65535)
- `ShareProxyGatewayWhileConnected` — `true` = proxy only active while VPN is connected

## Split Tunneling

Route specific apps or network ranges inside or outside the VPN tunnel:

```ini
[Connection]
SplitTunnelingEnabled=true
SplitTunnelingMode=Exclude
SplitTunnelingApps=/usr/bin/firefox, /usr/bin/curl
SplitTunnelingRoutes=192.168.1.0/24, example.com
```

- `SplitTunnelingMode` — `Exclude` (listed apps/routes bypass VPN) or `Include` (only listed apps/routes use VPN)
- `SplitTunnelingApps` — comma-separated list of executable paths (non-existent paths are silently skipped)
- `SplitTunnelingRoutes` — comma-separated list of IP/CIDR ranges or domain names

## Anti-Censorship (AmneziaWG)

Obfuscate the VPN connection with an AmneziaWG preset so it is harder to detect and block. Two settings work together:

```ini
[Connection]
ProtocolTweaksMethod=1
AmneziawgPreset=E - Baseline No Junk
```

- `ProtocolTweaksMethod` — a number: `0` Auto (default; a preset is used only when Windscribe suggests one, otherwise plain WireGuard), `1` Enabled (always use `AmneziawgPreset`), `2` Disabled (never)
- `AmneziawgPreset` — only used when `ProtocolTweaksMethod=1`. The value is a preset name exactly as `windscribe-cli amneziawg` prints it (the full name, e.g. `E - Baseline No Junk`, not just `E`). Empty uses the first preset. A name that is not listed also falls back to the first preset, and the app log says so. Before the list has ever been fetched, both use a built-in preset
- The preset list comes from Windscribe. Until the app has fetched it (log in first), `windscribe-cli amneziawg` prints "No AmneziaWG configurations available." and exits with 1
- `CircumventCensorship` does **not** turn this on. It pads the app's own requests to Windscribe and affects the emergency connection only; the VPN connection is unchanged

On Windows, macOS and Linux GUI builds, open Preferences > Connection > Anti-Censorship Settings, set Protocol Tweaks to Enabled, and choose the preset under Amnezia Config.

## Applying Changes

Changes apply automatically as soon as the file is saved. `windscribe-cli preferences reload` is no longer needed and does nothing.

## ⚠️ Known Issue: AI Agent Hangs After Disabling AllowLANTraffic

When `AllowLANTraffic` is set to `false` and the file is saved, the firewall rules update while VPN is active. This can disrupt the AI agent's outbound connection — the TCP connection dies when the firewall state changes, causing the agent to hang on a broken socket.

**Symptoms:** First command after the change works normally. Second command may execute but return no feedback. Subsequent commands hang indefinitely.

**Workaround:** Set `AllowLANTraffic` before starting the AI agent session rather than toggling it mid-session. If the agent becomes unresponsive, restart the agent process.
