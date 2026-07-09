#pragma once

#include <mutex>
#include <string>

// The single nftables table the helper owns (e.g. "inet windscribe"). All firewall, DNS-leak, boot,
// and WireGuard chains live in this one table, so this name is the single source for every subsystem.
inline const std::string kNftTable = "inet " WS_PRODUCT_NAME_LOWER;

// Shared base-chain specs for the firewall input/output chains. Both the runtime FirewallController and
// the boot-time FirewallOnBootManager create these same chains and must agree on hook/priority/policy.
inline const std::string kInputChainSpec = "type filter hook input priority -10; policy drop;";
inline const std::string kOutputChainSpec = "type filter hook output priority -10; policy drop;";

// Builders for nft command-buffer text targeting kNftTable. Centralizing the table prologue, the
// idempotent base-chain idiom, and the teardown idiom keeps every subsystem (firewall, split tunnel,
// DNS-leak, boot, WireGuard) from re-spelling them, and keeps the EBUSY rationale in one place.
namespace nft {

// Idempotent prologue every transaction starts with.
inline std::string addTable() { return "add table " + kNftTable + "\n"; }

// Create-if-absent, then flush the chain's rules. Pass spec (e.g. kInputChainSpec) for a base chain or
// "" for a regular chain. NEVER delete+re-add a base chain to replace it: nftables defers the hook
// unregistration to commit, so re-adding the same hook in one transaction is rejected as EBUSY ("Device
// or resource busy"). `add chain {spec}` is idempotent (keeps the existing hook); `flush` clears rules.
inline std::string ensureChain(const std::string &name, const std::string &spec = "")
{
    std::string s = "add chain " + kNftTable + " " + name;
    if (!spec.empty()) {
        s += " { " + spec + " }";
    }
    s += "\nflush chain " + kNftTable + " " + name + "\n";
    return s;
}

// Idempotent teardown: the add can't fail on a missing table/chain, the delete then removes it.
inline std::string deleteChain(const std::string &name)
{
    return "add chain " + kNftTable + " " + name + "\ndelete chain " + kNftTable + " " + name + "\n";
}

// One `add rule <table> <chain> <expr>` line.
inline std::string rule(const std::string &chain, const std::string &expr)
{
    return "add rule " + kNftTable + " " + chain + " " + expr + "\n";
}

// The private/LAN + multicast (v4) carve-out shared verbatim by the runtime firewall
// (FirewallController::buildFirewallRules) and the boot ruleset (FirewallOnBootManager). `action` is
// the LAN policy ("accept" when LAN traffic is allowed or for custom configs that need the private
// ranges so third-party VPN DNS/gateway/routes work, else "drop"). 10.255.255.0/24 is ALWAYS dropped
// (reserved for the VPN's own use) regardless of action. Centralized so the two builders, which must
// agree for the kill switch to be consistent across the boot window and runtime, cannot drift.
inline std::string lanRulesV4(const std::string &action)
{
    return rule("input",  "ip saddr 192.168.0.0/16 " + action)
         + rule("output", "ip daddr 192.168.0.0/16 " + action)
         + rule("input",  "ip saddr 172.16.0.0/12 " + action)
         + rule("output", "ip daddr 172.16.0.0/12 " + action)
         + rule("input",  "ip saddr 169.254.0.0/16 " + action)
         + rule("output", "ip daddr 169.254.0.0/16 " + action)
         + rule("input",  "ip saddr 10.255.255.0/24 drop")
         + rule("output", "ip daddr 10.255.255.0/24 drop")
         + rule("input",  "ip saddr 10.0.0.0/8 " + action)
         + rule("output", "ip daddr 10.0.0.0/8 " + action)
         + rule("input",  "ip saddr 224.0.0.0/4 " + action)
         + rule("output", "ip daddr 224.0.0.0/4 " + action);
}

// The v6 analog: application-layer multicast (mDNS/SSDP/LLMNR, ff00::/8) and unique-local addresses
// (RFC 4193, fc00::/7) per the same LAN policy. Shared by both builders.
inline std::string lanRulesV6(const std::string &action)
{
    return rule("input",  "ip6 saddr ff00::/8 " + action)
         + rule("output", "ip6 daddr ff00::/8 " + action)
         + rule("input",  "ip6 saddr fc00::/7 " + action)
         + rule("output", "ip6 daddr fc00::/7 " + action);
}

} // namespace nft

// Thin wrapper over libnftables. The helper is the sole author of all nftables rule text; this class
// is the single point that hands a command buffer to the kernel via nft_run_cmd_from_buffer(). Each
// run() call is one nftables transaction (atomic, cross-family).
class NftablesController
{
public:
    static NftablesController &instance();

    // Runs an nft command buffer (same syntax as `nft -f -`) in a single transaction. When dryRun is
    // true the buffer is parsed and validated against the kernel WITHOUT committing it, used to
    // validate a generated ruleset before applying. Returns true on success. On failure the captured
    // nft error buffer is logged (unless logOnError is false) and, when errOut != nullptr, copied
    // there. Thread-safe: all access is serialized — FirewallController, FirewallOnBootManager,
    // WireGuardAdapter and the DNS-leak path all funnel through here.
    bool run(const std::string &cmds, bool dryRun = false, std::string *errOut = nullptr, bool logOnError = true);

private:
    NftablesController();
    ~NftablesController();
    NftablesController(const NftablesController &) = delete;
    NftablesController &operator=(const NftablesController &) = delete;

    std::mutex mutex_;
};
