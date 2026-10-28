---
name: windscribe-vpn
description: "Controls Windscribe VPN via the windscribe-cli command-line tool. Use when installing Windscribe, connecting or disconnecting VPN, switching server locations or protocols, checking VPN status, managing the firewall, listing available locations, troubleshooting VPN issues, automating privacy and security workflows, or testing geo-dependent features across regions. All operations use shell commands via windscribe-cli. Not for Windscribe account management, billing, or browser extension control."
---

# Windscribe VPN — AI Integration Skill

Control Windscribe VPN entirely from the command line. Any AI agent with shell access can use it — no custom server or protocol needed.

> **📝 GUI vs CLI behavior:** `windscribe-cli` communicates with the Windscribe app via IPC. All commands print their output to the terminal, whether the **GUI desktop app** or the **CLI-only build** (Linux headless) is running.

> **📦 Not installed?** See [references/install.md](references/install.md) for platform-specific download and installation instructions.

## Prerequisites

- Windscribe must be **installed and running** (CLI auto-starts the app/service if needed)
- User must be **logged in** (`windscribe-cli login`)
- Only **one CLI instance** can run at a time

| Platform | Binary Path |
|----------|-------------|
| Linux | `windscribe-cli` (in PATH) |
| macOS | `/Applications/Windscribe.app/Contents/MacOS/windscribe-cli` |
| Windows | `"C:\Program Files\Windscribe\windscribe-cli.exe"` |

## Connection

```bash
windscribe-cli connect                       # Last location (or best)
windscribe-cli connect best                  # Auto-select best server
windscribe-cli connect "Toronto"             # By city, nickname, region, or ISO code
windscribe-cli connect best wireguard        # Location + protocol
windscribe-cli connect best udp:443          # Location + protocol:port
windscribe-cli connect wireguard             # Last location (or best) + protocol
windscribe-cli connect -n best               # Non-blocking (returns immediately)
windscribe-cli connect static "Dallas"       # Static IP location, by city (Pro)
windscribe-cli connect static "203.0.113.5"  # Static IP location, by IP (Pro)

windscribe-cli disconnect                    # Disconnect
windscribe-cli disconnect -n                 # Non-blocking disconnect
```

Location matching is case-insensitive. Use city name, nickname, region name, or ISO country code. Static IP locations are the exception: `connect static` needs the city or IP exactly as `windscribe-cli locations static` prints it, including case.

The `-n` flag must come before the location argument. Without `-n`, the CLI blocks until the connection completes or fails.

## Protocols & Ports

| Protocol | Notes |
|----------|-------|
| `wireguard` | Recommended; supports post-quantum encryption |
| `ikev2` | Windows/macOS only |
| `udp` | OpenVPN UDP |
| `tcp` | OpenVPN TCP |
| `stealth` | Anti-censorship (stunnel over OpenVPN) |
| `wstunnel` | WebSocket tunnel for restrictive networks |

```bash
# List available ports dynamically
windscribe-cli ports wireguard    # e.g. 443, 80, 53, 123, 1194, 65142
windscribe-cli ports stealth
```

Use `protocol:port` syntax to specify a port (e.g. `stealth:143`). Default is the first port. Invalid ports silently fall back to the default. Protocol names are case-insensitive.

## Status

```bash
windscribe-cli status
```

Reports: Internet connectivity, login state, firewall state, connection state + city, protocol:port, public or VPN IP, data usage, and an available update (only when there is one).

- `*` prefix on "Connected" = tunnel test pending
- `[Network interference]` suffix = tunnel test detected issues

## Locations

```bash
windscribe-cli locations              # All locations
windscribe-cli locations fav          # Favourites (with pinned IPs)
windscribe-cli locations static       # Static IPs (Pro)
```

Format, one location per line:
- Cities: `Region - City - Nickname`
- Best location: `Region - Nickname`
- Static IPs: `City - IP`, after a first line `(Device name: <name>)`
- Favourites: `City - Nickname`, followed by ` [<pinned IP>]` when an IP is pinned

Any line can end with ` (Pro)` (needs Pro), ` (Disabled)` (temporarily unavailable), or ` (10 Gbps)`. Prints `No locations.` (exit 1) when the list is empty. Use nickname, city, region, or country code with `connect`.

## Firewall

```bash
windscribe-cli firewall on            # Block all non-VPN traffic
windscribe-cli firewall off           # Disable firewall
```

"Always On" mode (set in GUI) prevents CLI from disabling the firewall.

## IP Management

```bash
windscribe-cli ip rotate              # New IP on same server (requires connected + Pro/plan)
windscribe-cli ip fav                 # Pin current IP (requires connected + Pro/plan)
windscribe-cli ip unfav 76.76.2.2    # Unpin an IP (requires logged in)
```

## Authentication

```bash
windscribe-cli login                  # Interactive (prompts for credentials)
windscribe-cli login user pass        # Direct
windscribe-cli logout                 # Disconnect + logout, turns firewall off
windscribe-cli logout on              # Disconnect + logout, keeps firewall on
```

`logout` and `logout off` behave the same — both turn the firewall off. Only `logout on` preserves the firewall.

2FA: prompted after credentials if enabled. Linux CLI-only renders CAPTCHAs as ASCII art.

## Maintenance

```bash
windscribe-cli update                 # Install the available update (see `status`); requires logged in
windscribe-cli logs send              # Send debug logs to support
windscribe-cli amneziawg              # List AmneziaWG censorship presets
```

## CLI-Only Commands (Linux Headless)

These commands are only available in the Linux CLI-only build:

```bash
windscribe-cli keylimit keep|delete   # WireGuard key limit behavior
windscribe-cli ignoresslerrors on|off # Ignore SSL errors for this session (only offered after a failed login; a security risk)
```

Config file: `~/.config/Windscribe/windscribe_cli.conf`. Changes apply automatically as soon as the file is saved; `windscribe-cli preferences reload` is no longer needed and does nothing. See [references/config.md](references/config.md) for all options (split tunneling, LAN access, auto-connect, DNS, proxy sharing).

## Output Parsing

| Field | Pattern | Example |
|-------|---------|---------|
| Connection | `Connect state: Connected: <City>` | `Connect state: Connected: Toronto` |
| Firewall | `Firewall state: <On\|Off\|Always On>` | `Firewall state: On` |
| Protocol | `Protocol: <Name>:<port>` | `Protocol: WireGuard:443` |
| Login | `Login state: Logged in` | |
| IP | `Public IP: <ip>` or `VPN IP: <ip>` | `VPN IP: 203.0.113.5` |
| Data | `Data usage: <used> / <total>` | `Data usage: 1.20 GB / Unlimited` |

Exit codes: 0 = success, non-zero = error (error message on stdout, not stderr). Exceptions: a failed `login` exits 0, so check that its output says `Logged in`; `ip rotate` and `ip fav` exit 2 while an IP rotation is still in progress.

## Account Dashboard (Web Only)

Features not available via CLI — use https://windscribe.com/myaccount:
Unlock Streaming, ROBERT rules, port forwarding, static IP management, ScribeForce, manual config file downloads.

## Additional References

- **[references/install.md](references/install.md)** — Download links and installation steps per platform
- **[references/config.md](references/config.md)** — Linux CLI-only configuration file options
- **[references/workflows.md](references/workflows.md)** — Common AI workflows (geo-testing, privacy setup, region cycling, leak verification)
- **[references/troubleshooting.md](references/troubleshooting.md)** — Error messages, diagnosis, and fixes
- **[references/acknowledgements.md](references/acknowledgements.md)** — Community contributors

> **📋 Single-file use:** If your AI agent doesn't support multi-file Skills, concatenate the reference files into a single document.

## Help

- **Knowledge Base:** https://windscribe.com/knowledge-base
- **Garry (support bot):** Available on the Windscribe website
- **Community:** [Discord](https://discord.com/invite/vpn) · [Reddit](https://www.reddit.com/r/windscribe) · [X](https://x.com/windscribecom)
