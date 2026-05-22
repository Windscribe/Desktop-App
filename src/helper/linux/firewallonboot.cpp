#include "firewallonboot.h"
#include <filesystem>
#include <sstream>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <spdlog/spdlog.h>
#include "../common/io_posix.h"
#include "types/ipaddress.h"
#include "utils.h"

FirewallOnBootManager::FirewallOnBootManager(): comment_(WS_PRODUCT_NAME " client rule")
{
    // Migrate boot rules from legacy locations: /etc/windscribe (pre-#673) and /var/tmp/windscribe
    // (#673 through #1799). Only trust a source if its containing directory is root-owned and not
    // group/other writable — otherwise a non-root attacker could have planted an arbitrary ruleset
    // for iptables-restore to load on boot. /var/tmp's world-writable parent makes the check on
    // /var/tmp/windscribe itself load-bearing (a symlink there would fail S_ISDIR/uid/mode).
    std::error_code ec;

    auto migrateOrPurge = [&ec](const char *legacyDir, const char *fileName, const char *dst) {
        // Check dir trust BEFORE touching any path through it — if the dir is an attacker-planted
        // symlink, even an unlink resolves through it and operates on attacker-chosen targets.
        struct stat st = {};
        const bool dirTrusted = (::lstat(legacyDir, &st) == 0)
            && S_ISDIR(st.st_mode)
            && st.st_uid == 0
            && (st.st_mode & 022) == 0;
        if (!dirTrusted) {
            // Don't reach through the path; remove_all on the parent below handles symlink cleanup.
            return;
        }
        const std::string src = std::string(legacyDir) + "/" + fileName;
        if (!Utils::isFileExists(src.c_str())) {
            return;
        }
        std::filesystem::rename(src, dst, ec);
        if (ec) {
            spdlog::warn("Failed to migrate {}: {}", src, ec.message());
            return;
        }
        std::filesystem::permissions(dst,
            std::filesystem::perms::owner_read | std::filesystem::perms::owner_write, ec);
        if (ec) {
            spdlog::warn("Failed to chmod migrated {}: {}", dst, ec.message());
        }
    };

    migrateOrPurge(WS_POSIX_CONFIG_DIR, "boot_rules.v4", WS_LINUX_TMP_DIR "/boot_rules.v4");
    migrateOrPurge(WS_POSIX_CONFIG_DIR, "boot_rules.v6", WS_LINUX_TMP_DIR "/boot_rules.v6");
    migrateOrPurge("/var/tmp/windscribe", "boot_rules.v4", WS_LINUX_TMP_DIR "/boot_rules.v4");
    migrateOrPurge("/var/tmp/windscribe", "boot_rules.v6", WS_LINUX_TMP_DIR "/boot_rules.v6");
    std::filesystem::remove(WS_POSIX_CONFIG_DIR "/rules.v4", ec);
    std::filesystem::remove(WS_POSIX_CONFIG_DIR "/rules.v6", ec);
    // Nuke the legacy /var/tmp/windscribe entirely (wg_firewall.v4 is transient, windscribe_nameservers
    // is per-connection cache, both regenerated on next use). remove_all on a symlink removes only
    // the link, not the target, so this is safe even if an attacker planted one before any helper ran.
    std::filesystem::remove_all("/var/tmp/windscribe", ec);

    // If firewall on boot is enabled, restore boot rules
    if (Utils::isFileExists(WS_LINUX_TMP_DIR "/boot_rules.v4")) {
        Utils::executeCommand("iptables-restore", {"-n", WS_LINUX_TMP_DIR "/boot_rules.v4"});
    }
    if (Utils::isFileExists(WS_LINUX_TMP_DIR "/boot_rules.v6")) {
        Utils::executeCommand("ip6tables-restore", {"-n", WS_LINUX_TMP_DIR "/boot_rules.v6"});
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

bool FirewallOnBootManager::enable(bool allowLanTraffic) {
    std::stringstream rules;

    // Pre-parse ipTable_ once and split entries by address family. The list is
    // populated from Engine::firewallExceptions_, which can include v6 DNS
    // servers / host IPs in dual-stack builds. Family detection goes through the
    // shared types::IpAddress boundary so this site stays consistent with the rest
    // of the codebase (instead of an ad-hoc ':' heuristic). The engine-side
    // validation in firewallexceptions.cpp already guarantees non-empty, syntactically
    // valid IPs; we only guard against the trailing-space token produced by
    // Helper_posix::setFirewallOnBoot's `ipTableStr += " "`.
    std::stringstream v4IpTableRules;
    std::stringstream v6IpTableRules;
    {
        std::istringstream ips(ipTable_);
        std::string ip;
        while (std::getline(ips, ip, ' ')) {
            if (ip.empty()) {
                continue;
            }
            const bool isV6 = (types::IpAddress::detectFamily(ip) == types::IpAddress::IPv6);
            std::stringstream &out = isV6 ? v6IpTableRules : v4IpTableRules;
            out << "-A " WS_PRODUCT_NAME_LOWER "_input -s " + ip + " -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
            out << "-A " WS_PRODUCT_NAME_LOWER "_output -d " + ip + " -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
        }
    }

    rules << "*filter\n";
    rules << ":" WS_PRODUCT_NAME_LOWER "_input - [0:0]\n";
    rules << ":" WS_PRODUCT_NAME_LOWER "_output - [0:0]\n";
    rules << ":" WS_PRODUCT_NAME_LOWER "_block - [0:0]\n";
    // `iptables-restore -n` does not flush chains declared via `:CHAIN` — it only
    // creates them if absent. Without explicit -F, a helper restart that re-runs
    // this loader after the constructor already populated the chains would stack
    // a duplicate ruleset on top of the existing one. Flush each chain explicitly
    // so the blob is authoritative without disturbing third-party rules elsewhere
    // in the filter table.
    rules << "-F " WS_PRODUCT_NAME_LOWER "_input\n";
    rules << "-F " WS_PRODUCT_NAME_LOWER "_output\n";
    rules << "-F " WS_PRODUCT_NAME_LOWER "_block\n";
    rules << "-I INPUT -j " WS_PRODUCT_NAME_LOWER "_input -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A INPUT -j " WS_PRODUCT_NAME_LOWER "_block -m comment --comment \"" + comment_ + "\"\n";
    rules << "-I OUTPUT -j " WS_PRODUCT_NAME_LOWER "_output -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A OUTPUT -j " WS_PRODUCT_NAME_LOWER "_block -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A " WS_PRODUCT_NAME_LOWER "_input -i lo -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A " WS_PRODUCT_NAME_LOWER "_output -o lo -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A " WS_PRODUCT_NAME_LOWER "_input -p udp --sport 67 --dport 68 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A " WS_PRODUCT_NAME_LOWER "_output -p udp --sport 68 --dport 67 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";

    rules << v4IpTableRules.str();

    std::string action;
    if (allowLanTraffic) {
        action = "ACCEPT";
    } else {
        action = "DROP";
    }

    // Local Network
    rules << "-A " WS_PRODUCT_NAME_LOWER "_input -s 192.168.0.0/16 -j " + action + " -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A " WS_PRODUCT_NAME_LOWER "_output -d 192.168.0.0/16 -j " + action + " -m comment --comment \"" + comment_ + "\"\n";

    rules << "-A " WS_PRODUCT_NAME_LOWER "_input -s 172.16.0.0/12 -j " + action + " -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A " WS_PRODUCT_NAME_LOWER "_output -d 172.16.0.0/12 -j " + action + " -m comment --comment \"" + comment_ + "\"\n";

    rules << "-A " WS_PRODUCT_NAME_LOWER "_input -s 169.254.0.0/16 -j " + action + " -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A " WS_PRODUCT_NAME_LOWER "_output -d 169.254.0.0/16 -j " + action + " -m comment --comment \"" + comment_ + "\"\n";

    rules << "-A " WS_PRODUCT_NAME_LOWER "_input -s 10.255.255.0/24 -j DROP -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A " WS_PRODUCT_NAME_LOWER "_output -d 10.255.255.0/24 -j DROP -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A " WS_PRODUCT_NAME_LOWER "_input -s 10.0.0.0/8 -j " + action + " -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A " WS_PRODUCT_NAME_LOWER "_output -d 10.0.0.0/8 -j " + action + " -m comment --comment \"" + comment_ + "\"\n";

    // Multicast addresses
    rules << "-A " WS_PRODUCT_NAME_LOWER "_input -s 224.0.0.0/4 -j " + action + " -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A " WS_PRODUCT_NAME_LOWER "_output -d 224.0.0.0/4 -j " + action + " -m comment --comment \"" + comment_ + "\"\n";

    // Block all rule
    rules << "-A " WS_PRODUCT_NAME_LOWER "_block -j DROP -m comment --comment \"" + comment_ + "\"\n";
    rules << "COMMIT\n";

    // write rules
    // open()'s mode is ignored on pre-existing files; unlink first to force the intended perms
    unlink(WS_LINUX_TMP_DIR "/boot_rules.v4");
    int fd = open(WS_LINUX_TMP_DIR "/boot_rules.v4", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        spdlog::error("Could not open boot firewall rules for writing");
        return false;
    }

    if (!IO::writeAll(fd, rules.str())) {
        spdlog::error("Failed to write boot_rules.v4: {}", IO::strerror(errno));
        close(fd);
        unlink(WS_LINUX_TMP_DIR "/boot_rules.v4");
        return false;
    }
    close(fd);

    rules.str("");
    rules.clear();

    rules << "*filter\n";
    rules << ":" WS_PRODUCT_NAME_LOWER "_input - [0:0]\n";
    rules << ":" WS_PRODUCT_NAME_LOWER "_output - [0:0]\n";
    // See the v4 block above for the rationale — `ip6tables-restore -n` has the
    // same non-flushing semantics for `:CHAIN` headers.
    rules << "-F " WS_PRODUCT_NAME_LOWER "_input\n";
    rules << "-F " WS_PRODUCT_NAME_LOWER "_output\n";
    rules << "-I INPUT -j " WS_PRODUCT_NAME_LOWER "_input -m comment --comment \"" + comment_ + "\"\n";
    rules << "-I OUTPUT -j " WS_PRODUCT_NAME_LOWER "_output -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A " WS_PRODUCT_NAME_LOWER "_input -s ::1/128 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A " WS_PRODUCT_NAME_LOWER "_output -d ::1/128 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";

    // IPv6 link-local (fe80::/10): always allow. Required for neighbor discovery
    // and SLAAC autoconfiguration on a v6 LAN segment before the engine starts.
    // Parity with the engine-side v6 rule set (firewallcontroller_linux.cpp).
    rules << "-A " WS_PRODUCT_NAME_LOWER "_input -s fe80::/10 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A " WS_PRODUCT_NAME_LOWER "_output -d fe80::/10 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";

    // DHCPv6 (RFC 8415): client uses port 546, server uses port 547. Allowed
    // unconditionally so the host can obtain/renew a v6 lease on a stateful
    // (M-flag) v6 LAN segment before the engine starts — mirrors the
    // unconditional DHCPv4 permit at lines ~116-117 and the engine-side v6
    // permit in firewallcontroller_linux.cpp. SLAAC-only segments don't need
    // this (RA is ICMPv6 type 134, carved out below) but stateful DHCPv6
    // deployments would otherwise lose their lease in the boot→engine window.
    rules << "-A " WS_PRODUCT_NAME_LOWER "_input -p udp --sport 547 --dport 546 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A " WS_PRODUCT_NAME_LOWER "_output -p udp --sport 546 --dport 547 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";

    // ICMPv6 NDP/MLD (RFC 4890): NS/NA/RS/RA/Redirect and MLD reports use
    // multicast destinations (solicited-node ff02::1:ff…/104, all-routers
    // ff02::2) that fall under the ff00::/8 rule below. Without an explicit
    // carve-out, the DROP branch (allowLanTraffic=false) kills neighbor
    // resolution entirely — even an explicit unicast ACCEPT from v6IpTableRules
    // becomes unreachable because the host can't resolve the gateway's MAC.
    // IPv4 doesn't need an equivalent because ARP is L2 and bypasses iptables.
    for (int icmpv6Type : {130, 131, 132, 133, 134, 135, 136, 137, 143}) {
        const std::string t = std::to_string(icmpv6Type);
        rules << "-A " WS_PRODUCT_NAME_LOWER "_input -p ipv6-icmp --icmpv6-type " + t + " -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
        rules << "-A " WS_PRODUCT_NAME_LOWER "_output -p ipv6-icmp --icmpv6-type " + t + " -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
    }

    // IPv6 multicast (ff00::/8): governs application-layer multicast (mDNS,
    // SSDP, LLMNR, etc.) per allowLanTraffic. Not a true mirror of the v4
    // 224.0.0.0/4 rule — v6 needs the NDP/MLD carve-out above because NDP runs
    // at L3 (ICMPv6) and is subject to ip6tables, unlike v4 ARP.
    rules << "-A " WS_PRODUCT_NAME_LOWER "_input -s ff00::/8 -j " + action + " -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A " WS_PRODUCT_NAME_LOWER "_output -d ff00::/8 -j " + action + " -m comment --comment \"" + comment_ + "\"\n";

    // v6 entries from ipTable_ (e.g. v6 DNS server IPs). Defensive — currently
    // the engine sends mostly v4 IPs, but UniqueIpList does not filter by family
    // so a v6 DNS server reaching this path lands here instead of being dropped
    // into the v4 block (which would make iptables-restore reject the whole file).
    rules << v6IpTableRules.str();

    rules << "-A " WS_PRODUCT_NAME_LOWER "_input -j DROP -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A " WS_PRODUCT_NAME_LOWER "_output -j DROP -m comment --comment \"" + comment_ + "\"\n";
    rules << "COMMIT\n";

    // write rules
    // open()'s mode is ignored on pre-existing files; unlink first to force the intended perms
    unlink(WS_LINUX_TMP_DIR "/boot_rules.v6");
    fd = open(WS_LINUX_TMP_DIR "/boot_rules.v6", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        spdlog::error("Could not open v6 boot firewall rules for writing");
        return false;
    }

    if (!IO::writeAll(fd, rules.str())) {
        spdlog::error("Failed to write boot_rules.v6: {}", IO::strerror(errno));
        close(fd);
        unlink(WS_LINUX_TMP_DIR "/boot_rules.v6");
        return false;
    }
    close(fd);

    return true;
}

bool FirewallOnBootManager::disable()
{
    std::error_code ec;
    std::filesystem::remove(WS_LINUX_TMP_DIR "/boot_rules.v4", ec);
    if (ec) {
        spdlog::warn("Failed to remove boot_rules.v4: {}", ec.message());
    }
    std::filesystem::remove(WS_LINUX_TMP_DIR "/boot_rules.v6", ec);
    if (ec) {
        spdlog::warn("Failed to remove boot_rules.v6: {}", ec.message());
    }
    return true;
}
