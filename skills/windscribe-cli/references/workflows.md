# Common AI Workflows

## Verify protection before sensitive operations

```bash
windscribe-cli status
# Look for: "Connect state: Connected" and "Firewall state: On"
# Also verify: "Internet connectivity: available"
```

## Verify connection with leak tests

After connecting, confirm there are no IP or DNS leaks:

```bash
# Check your visible IP address (CLI-friendly endpoint)
curl -s https://checkip.windscribe.com
```

- **IP Address Check (browser):** https://windscribe.com/what-is-my-ip
- **DNS Leak Test (browser):** https://windscribe.com/dns-leak-test

## Switch regions for geo-testing

```bash
windscribe-cli disconnect
windscribe-cli connect "London"
windscribe-cli status
# Wait for "Connect state: Connected: London"

# Run geo-dependent test here...

windscribe-cli disconnect
windscribe-cli connect "Tokyo"
windscribe-cli status
```

## Automated region cycling

```bash
windscribe-cli locations

for loc in "US" "London" "Frankfurt" "Tokyo"; do
  windscribe-cli connect "$loc"
  sleep 5  # wait for connection to establish
  windscribe-cli status
  # run tests...
  windscribe-cli disconnect
  sleep 2
done
```

## Enable maximum privacy

```bash
windscribe-cli connect best wireguard
windscribe-cli firewall on
windscribe-cli status
curl -s https://checkip.windscribe.com
```

## Connect with anti-censorship on a specific port

```bash
# Use Stealth protocol on port 143 (looks like IMAP traffic)
windscribe-cli connect best stealth:143

# Or WStunnel if Stealth is also blocked
windscribe-cli connect best wstunnel:443
```
