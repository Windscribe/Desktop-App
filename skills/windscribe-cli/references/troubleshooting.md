# Troubleshooting

## Error Messages

| Symptom | Diagnosis | Fix |
|---------|-----------|-----|
| `windscribe-cli` not found | Not installed or not in PATH | Install from [windscribe.com/download](https://windscribe.com/download) or check binary path in SKILL.md |
| "Windscribe CLI is already running" | Another CLI instance is active | Wait for it to finish, or kill the other process |
| "Aborting: app did not start in time" | Desktop app/service didn't start within timeout | Start Windscribe app/service manually, then retry |
| "Aborting: IPC communication error" | IPC connection to the app was lost | Restart the Windscribe app and retry |
| "Not logged in" | Session expired or never logged in | `windscribe-cli login` |
| "Already disconnected" | No active VPN tunnel | Normal — connect first |
| "Already logged in/out" | Auth state already matches request | Normal — no action needed |
| "Location does not exist or is disabled" | Wrong location name or server temporarily down | Check `windscribe-cli locations` for valid names |
| "You are out of data" | Free plan data cap reached or account disabled | Upgrade to Pro |
| "WireGuard adapter setup failed" | Driver/adapter issue | Try different protocol: `windscribe-cli connect best udp` |
| "Could not retrieve WireGuard configuration" | Server-side config issue | Try a different location or protocol |
| "Unable to start custom DNS service" | ctrld (DNS service) failed to start | Check system logs; try reconnecting |
| "Firewall set to always on and can't be turned off" | Always-On firewall enabled in preferences | Change in GUI settings, or leave enabled |
| "Disconnected due to reaching WireGuard key limit" | Too many WireGuard keys | Use `keylimit delete` (CLI-only) to auto-remove oldest key, then reconnect |
| "Connection has been overridden by another command" | Another connect/disconnect was issued | Retry your command |
| "Internet connectivity is not available" | No network (CLI-only builds) | Check network connection and retry |
| "Username too short" / "Password too short" | Credentials don't meet minimum length | Username: ≥3 chars, Password: ≥8 chars |
| Unable to connect | Network or protocol blocked | Try multiple protocol + port combos: `wireguard:443`, `udp:443`, `tcp:443`, `stealth:143`, `wstunnel:443`. Stealth and WStunnel are designed for restrictive networks. Try non-standard ports like `stealth:587` or `tcp:8080`. |

## Getting Help

- **Knowledge Base:** https://windscribe.com/knowledge-base — searchable help articles and guides
- **Garry (support bot):** Available on the Windscribe website
- **Windscribe Blog:** https://windscribe.com/blog/
- **Community:**
  - Discord: https://discord.com/invite/vpn
  - Reddit: https://www.reddit.com/r/windscribe
  - X: https://x.com/windscribecom
