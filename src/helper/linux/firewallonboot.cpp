#include "firewallonboot.h"

#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <spdlog/spdlog.h>

#include "../common/io_posix.h"
#include "nftables/nftablescontroller.h"
#include "types/ipaddress.h"
#include "utils.h"

namespace {
const char *kBootRulesPath = WS_LINUX_TMP_DIR "/boot_rules.nft";
}

FirewallOnBootManager::FirewallOnBootManager()
{
    std::error_code ec;

    // Detect whether firewall-on-boot was enabled under the old iptables format: the pre-nft helper
    // only wrote boot_rules.v4/.v6 when it was on. Check only the root-owned locations (a planted file
    // in the world-writable /var/tmp could otherwise force a lockdown). This signal is used below to
    // carry boot protection across the upgrade reboot until the engine re-applies the exact settings.
    const bool hadLegacyBootRules =
        std::filesystem::exists(WS_LINUX_TMP_DIR "/boot_rules.v4", ec) ||
        std::filesystem::exists(WS_LINUX_TMP_DIR "/boot_rules.v6", ec) ||
        std::filesystem::exists(WS_POSIX_CONFIG_DIR "/boot_rules.v4", ec) ||
        std::filesystem::exists(WS_POSIX_CONFIG_DIR "/boot_rules.v6", ec);

    // Purge obsolete iptables-era boot rule files (replaced by boot_rules.nft) from the canonical and
    // legacy locations, plus the stale runtime rules.v4/.v6 and the legacy world-writable
    // /var/tmp/windscribe staging dir. remove_all on a symlink removes only the link, so this is safe
    // even if an attacker planted one.
    std::filesystem::remove(WS_LINUX_TMP_DIR "/boot_rules.v4", ec);
    std::filesystem::remove(WS_LINUX_TMP_DIR "/boot_rules.v6", ec);
    std::filesystem::remove(WS_POSIX_CONFIG_DIR "/boot_rules.v4", ec);
    std::filesystem::remove(WS_POSIX_CONFIG_DIR "/boot_rules.v6", ec);
    std::filesystem::remove(WS_POSIX_CONFIG_DIR "/rules.v4", ec);
    std::filesystem::remove(WS_POSIX_CONFIG_DIR "/rules.v6", ec);
    std::filesystem::remove_all("/var/tmp/windscribe", ec);

    // Upgrade continuity: the iptables-era helper migrated boot rules across upgrades so the kill
    // switch survived a reboot. The nft format can't be converted from the old files, so if boot was
    // enabled but the upgrade left no boot_rules.nft yet, write a fail-closed ruleset now (allowLan
    // off — the most restrictive, safe default) rather than silently leaving the next boot unprotected.
    // The engine overwrites this with the user's real allowLan setting + IP exceptions via
    // updateFirewallOnBoot() on its next launch. One-shot: the legacy files that trigger it were just
    // removed above, so subsequent helper starts won't re-enter this path.
    if (hadLegacyBootRules && !std::filesystem::exists(kBootRulesPath, ec)) {
        spdlog::info("Migrating firewall-on-boot to nftables: installing fail-closed boot ruleset");
        enable(false);
    }

    // If firewall on boot is enabled, restore the persisted nft ruleset into our table. Only trust a
    // regular file owned by root and not group/other-writable (we write it 0600 in the root-owned tmp
    // dir) so a planted or tampered ruleset can't be loaded as root at boot.
    struct stat st = {};
    if (::lstat(kBootRulesPath, &st) == 0 && S_ISREG(st.st_mode) && st.st_uid == 0
        && (st.st_mode & (S_IWGRP | S_IWOTH)) == 0) {
        std::ifstream f(kBootRulesPath);
        std::stringstream ss;
        ss << f.rdbuf();
        if (!NftablesController::instance().run(ss.str())) {
            spdlog::error("Failed to restore firewall-on-boot ruleset");
        }
    }
}

FirewallOnBootManager::~FirewallOnBootManager()
{
}

bool FirewallOnBootManager::setEnabled(bool enabled, bool allowLanTraffic)
{
    if (enabled) {
        return enable(allowLanTraffic);
    }
    return disable();
}

bool FirewallOnBootManager::enable(bool allowLanTraffic)
{
    const std::string action = allowLanTraffic ? "accept" : "drop";

    // Split ipTable_ entries by family. Each is round-tripped through types::IpAddress so a malformed
    // token can't be injected into the boot ruleset (arbitrary rule injection, root, persisted across
    // reboots). A valid IP/CIDR carries no whitespace/nft-grammar characters.
    std::string v4IpRules, v6IpRules;
    for (const std::string &ip : ipTable_) {
        if (ip.empty()) {
            continue;
        }
        const types::IpAddress parsed(ip);
        if (!parsed.isValid()) {
            spdlog::warn("FirewallOnBootManager: dropping invalid IP \"{}\" from boot ruleset", ip);
            continue;
        }
        if (parsed.isV6()) {
            v6IpRules += nft::rule("input", "ip6 saddr " + ip + " accept");
            v6IpRules += nft::rule("output", "ip6 daddr " + ip + " accept");
        } else {
            v4IpRules += nft::rule("input", "ip saddr " + ip + " accept");
            v4IpRules += nft::rule("output", "ip daddr " + ip + " accept");
        }
    }

    std::stringstream r;
    r << nft::addTable();
    // Populate the shared firewall chains with the static boot subset. The runtime FirewallController
    // overwrites these same chains on connect, so boot and runtime are never simultaneously active.
    // Create-if-absent + flush (see nft::ensureChain). kInputChainSpec/kOutputChainSpec are shared with
    // FirewallController so boot and runtime agree on the chains' hook/priority/policy.
    r << nft::ensureChain("input", kInputChainSpec);
    r << nft::ensureChain("output", kOutputChainSpec);

    // Loopback.
    r << nft::rule("input", "iifname \"lo\" accept");
    r << nft::rule("output", "oifname \"lo\" accept");

    // DHCPv4. Scoped to ipv4 — in an inet table an L4-only match would also accept IPv6 on this port
    // pair, widening the boot firewall beyond v4 DHCP (mirrors FirewallController and the DHCPv6 rule).
    r << nft::rule("input", "meta nfproto ipv4 udp sport 67 udp dport 68 accept");
    r << nft::rule("output", "meta nfproto ipv4 udp sport 68 udp dport 67 accept");

    // v4 host/API exceptions.
    r << v4IpRules;

    // Local network + multicast (v4). Shared with the runtime firewall via nft::lanRulesV4.
    r << nft::lanRulesV4(action);

    // IPv6 loopback + link-local (neighbor discovery / SLAAC before the engine starts).
    r << nft::rule("input", "ip6 saddr ::1 accept");
    r << nft::rule("output", "ip6 daddr ::1 accept");
    r << nft::rule("input", "ip6 saddr fe80::/10 accept");
    r << nft::rule("output", "ip6 daddr fe80::/10 accept");

    // DHCPv6. Scoped to ipv6 — in an inet table an L4-only match would also accept IPv4 on this port
    // pair, widening the boot firewall beyond v6 DHCP.
    r << nft::rule("input", "meta nfproto ipv6 udp sport 547 udp dport 546 accept");
    r << nft::rule("output", "meta nfproto ipv6 udp sport 546 udp dport 547 accept");

    // ICMPv6 NDP/MLD carve-out (NDP runs at L3 unlike v4 ARP, so the drop branch would otherwise kill
    // neighbor resolution). Connectivity-critical error types 1-4 are deliberately NOT accepted here
    // (no VPN interface exists in the boot window to scope them to; a blanket accept would reopen a
    // covert channel).
    r << nft::rule("input", "icmpv6 type { 130, 131, 132, 133, 134, 135, 136, 137, 143 } accept");
    r << nft::rule("output", "icmpv6 type { 130, 131, 132, 133, 134, 135, 136, 137, 143 } accept");

    // IPv6 multicast + ULA per allowLanTraffic. Shared with the runtime firewall via nft::lanRulesV6.
    r << nft::lanRulesV6(action);

    // v6 host/API exceptions.
    r << v6IpRules;

    // open()'s mode is ignored on pre-existing files; unlink first to force the intended perms.
    unlink(kBootRulesPath);
    int fd = open(kBootRulesPath, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        spdlog::error("Could not open boot firewall rules for writing");
        return false;
    }
    if (!IO::writeAll(fd, r.str())) {
        spdlog::error("Failed to write boot_rules.nft: {}", IO::strerror(errno));
        close(fd);
        unlink(kBootRulesPath);
        return false;
    }
    close(fd);
    return true;
}

bool FirewallOnBootManager::disable()
{
    std::error_code ec;
    std::filesystem::remove(kBootRulesPath, ec);
    if (ec) {
        spdlog::warn("Failed to remove boot_rules.nft: {}", ec.message());
    }
    return true;
}
