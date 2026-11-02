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
- Only **one CLI command** can run at a time; to poll `status` while connecting, use `connect -n`

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

Quote a location that contains spaces (`connect "New York"`); an unquoted second word is rejected as an unknown protocol. Without `-n`, the CLI blocks until the connection completes or fails.

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

Use `protocol:port` syntax to specify a port (e.g. `stealth:143`). Default is the first port. A port that is not a number is rejected; a number the protocol doesn't offer falls back to the default. Protocol names are case-insensitive.

## Status

```bash
windscribe-cli status
```

Prints one `Label: value` line per item, values aligned:

```
Login:       Logged in
Connection:  Connected to Toronto
Protocol:    WireGuard:443
IP:          203.0.113.5 (VPN)
Firewall:    On
Data usage:  1.20 GB / Unlimited
Internet:    Available
Update:      2.26.1 available
```

- `Connection` is omitted when logged out; `Protocol` is shown only when connected; `IP` is hidden while connecting, disconnecting, or checking the connection, and for a few seconds after, until the new IP has been looked up, and while connected if the tunnel test failed; `Data usage` is shown only when logged in; `Update` only when one is available.
- `Connected to <City> (checking the connection)` = tunnel test pending
- `Connected to <City> (network interference detected)` = tunnel test detected issues
- `IP` is `(VPN)` when connected and `(public)` when disconnected

## Locations

```bash
windscribe-cli locations              # All locations
windscribe-cli locations fav          # Favourites (with pinned IPs)
windscribe-cli locations static       # Static IPs (Pro)
```

Format, one location per line:
- Cities: `Region - City - Nickname`
- Best location: `Region - Nickname`
- Static IPs: `City - IP`, after a first line `Device name: <name>` and a blank line
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

Prompts need a terminal (stdin a TTY). Without one, as when an agent runs the CLI, `login` never prompts: it reads a missing username, password, and 2FA code from stdin, one per line, and fails with exit 1 if stdin has none (it is empty or `/dev/null`), or if the account needs a CAPTCHA. Do not pipe the user's password yourself; ask the user to run `windscribe-cli login` in their own terminal in those cases.

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

`status` lines (split on the first `:`, then trim the value):

| Label | Values | Example |
|-------|--------|---------|
| `Login` | `Logged in`, `Logged out`, `Logging in`, `Error: <reason>` | `Login:       Logged in` |
| `Connection` | `Connected to <City>`, `Connecting to <City>`, `Disconnecting`, `Disconnected`, `Error: <reason>` | `Connection:  Connected to Toronto` |
| `Protocol` | `<Name>:<port>` | `Protocol:    WireGuard:443` |
| `IP` | `<ip> (VPN)` when connected, `<ip> (public)` when disconnected | `IP:          203.0.113.5 (VPN)` |
| `Firewall` | `On`, `Off`, `Always On` | `Firewall:    On` |
| `Data usage` | `<used> / <total>` | `Data usage:  1.20 GB / Unlimited` |
| `Internet` | `Available`, `Unavailable` | `Internet:    Available` |
| `Update` | `<version> available` | `Update:      2.26.1 available` |

A blocking `connect` prints progress lines (`Connecting to <City>`) and ends with `Connected to <City>` or an `Error: ...` line. `login` ends with `Logged in` or `Error: <reason>`.

Only one CLI command runs at a time; another one started meanwhile, `status` included, fails with `Windscribe CLI is already running`. To check the status while connecting, run `connect -n` and then poll `status`.

Exit codes: 0 = success, non-zero = failure. On failure the message goes to **stderr**; everything else goes to stdout. Invalid arguments or an unknown command print `Error: <what is wrong>` and the command's usage to stderr and exit 1. `ip rotate` and `ip fav` exit 2 while an IP rotation is still in progress.

Run `windscribe-cli <command> --help` (or `windscribe-cli help <command>`) for a command's arguments, and `windscribe-cli version` for the version.

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
