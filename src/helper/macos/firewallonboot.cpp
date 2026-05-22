#include "firewallonboot.h"
#include <filesystem>
#include <sstream>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <spdlog/spdlog.h>

#include "../common/io_posix.h"
#include "types/ipaddress.h"
#include "utils.h"

FirewallOnBootManager::FirewallOnBootManager()
{
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

    // Pre-parse ipTable_ once and split entries by address family. The list comes from
    // Helper_posix::setFirewallOnBoot (Engine::firewallExceptions_), which can include v6
    // DNS / API / endpoint IPs in dual-stack builds. UniqueIpList does not filter by family,
    // so a v6 entry reaching the boot ruleset would otherwise wedge `pfctl -f`: the v4-typed
    // `inet from any to <windscribe_ips>` rule would never match v6 entries even if loaded,
    // and pf rejects an empty `from any to <table>` match, so each family is emitted
    // independently with its own non-empty guard below. Each token is round-tripped through
    // types::IpAddress so we both classify by real family and drop anything that fails
    // inet_pton — defence in depth against a future producer widening the set to include
    // malformed strings, which would otherwise wedge `pfctl -f` and break the boot-time
    // killswitch entirely. Helper_posix serialises the set with a trailing space
    // (`ipTableStr += " "`), so we guard against the empty trailing token too.
    std::string v4Ips;
    std::string v6Ips;
    {
        std::istringstream ips(ipTable_);
        std::string ip;
        while (std::getline(ips, ip, ' ')) {
            if (ip.empty()) {
                continue;
            }
            const types::IpAddress parsed(ip);
            if (!parsed.isValid()) {
                spdlog::warn("FirewallOnBootManager: dropping invalid IP \"{}\" from boot ruleset", ip);
                continue;
            }
            (parsed.isV6() ? v6Ips : v4Ips) += ip + " ";
        }
    }

    rules << "set block-policy return\n";
    rules << "set skip on { lo0 }\n";
    rules << "scrub in all fragment reassemble\n";
    rules << "block all\n";
    // pf's `from any to <table>` rule needs a typed selector (`inet` vs `inet6`) and a
    // non-empty table, so each family is only emitted when the engine actually pushed at
    // least one entry of that family. In practice the v4 list is always populated
    // (firewall exceptions, API endpoints, etc.), but the symmetric guard is defensive
    // against an all-v6 exception list and matches the v6 branch below.
    if (!v4Ips.empty()) {
        rules << "table <" WS_PRODUCT_NAME_LOWER "_ips> persist {" << v4Ips << "}\n";
        rules << "pass out quick inet from any to <" WS_PRODUCT_NAME_LOWER "_ips>\n";
    }
    if (!v6Ips.empty()) {
        rules << "table <" WS_PRODUCT_NAME_LOWER "_ips_v6> persist {" << v6Ips << "}\n";
        rules << "pass out quick inet6 from any to <" WS_PRODUCT_NAME_LOWER "_ips_v6>\n";
    }
    rules << "anchor " WS_PRODUCT_NAME_LOWER "_vpn_traffic all\n";
    rules << "pass out quick inet proto udp from any to any port = 67\n";
    rules << "pass in quick inet proto udp from any to any port = 68\n";

    // DHCPv6 (RFC 8415): client port 546, server port 547. Allowed unconditionally so the host
    // can obtain/renew a v6 lease on a stateful (M-flag) v6 LAN segment in the boot→engine
    // window — mirrors the v4 DHCP pair immediately above. Out: dest port 547 (client→server),
    // in: dest port 546 (server→client). SLAAC-only segments don't need this — RA is ICMPv6
    // type 134 with link-local source, covered by the fe80::/10 permit just below.
    rules << "pass out quick inet6 proto udp from any to any port = 547\n";
    rules << "pass in quick inet6 proto udp from any to any port = 546\n";

    // IPv6 link-local (fe80::/10): always allow. Required for neighbor discovery / SLAAC /
    // router advertisements on a v6 LAN segment before the engine takes over the rules.
    // Parity with the engine-side lanTrafficRules link-local permits in
    // firewallcontroller_mac.cpp, which are also outside the allowLanTraffic gate.
    // Loopback ::1 is covered by `set skip on { lo0 }` above — no separate v6 loopback rule
    // needed (and unlike Linux ip6tables, macOS pf doesn't filter v4/v6 ICMP NDP separately
    // from the link-local match, so the fe80::/10 permit is sufficient for ND).
    rules << "pass out quick inet6 from any to fe80::/10\n";
    rules << "pass in quick inet6 from fe80::/10 to any\n";

    if (allowLanTraffic) {
        rules << "anchor " WS_PRODUCT_NAME_LOWER "_lan_traffic all {\n";

        // Always allow localhost
        rules << "pass out quick inet from any to 127.0.0.0/8\n";
        rules << "pass in quick inet from 127.0.0.0/8 to any\n";

        // Local Network
        rules << "pass out quick inet from any to 192.168.0.0/16\n";
        rules << "pass in quick inet from 192.168.0.0/16 to any\n";
        rules << "pass out quick inet from any to 172.16.0.0/12\n";
        rules << "pass in quick inet from 172.16.0.0/12 to any\n";
        rules << "pass out quick inet from any to 169.254.0.0/16\n";
        rules << "pass in quick inet from 169.254.0.0/16 to any\n";
        rules << "block out quick inet from any to 10.255.255.0/24\n";
        rules << "block in quick inet from 10.255.255.0/24 to any\n";
        rules << "pass out quick inet from any to 10.0.0.0/8\n";
        rules << "pass in quick inet from 10.0.0.0/8 to any\n";

        // Multicast addresses (IPv4 + IPv6). The v6 ff00::/8 rule covers application-layer
        // multicast (mDNSv6, SSDPv6, MLD reports, etc.) per allowLanTraffic — mirrors the v4
        // 224.0.0.0/4 rule above and the engine-side lanTrafficRules pair in
        // firewallcontroller_mac.cpp. Not a true family-for-family mirror (v4 mDNS uses
        // 224.0.0.251, v6 mDNS uses ff02::fb), but ff00::/8 is a strict superset of every
        // v6 multicast scope and matches the engine convention.
        rules << "pass out quick inet from any to 224.0.0.0/4\n";
        rules << "pass in quick inet from 224.0.0.0/4 to any\n";
        rules << "pass out quick inet6 from any to ff00::/8\n";
        rules << "pass in quick inet6 from ff00::/8 to any\n";

        // UPnP
        rules << "pass out quick inet proto udp from any to any port = 1900\n";
        rules << "pass in quick proto udp from any to any port = 1900\n";
        rules << "pass out quick inet proto udp from any to any port = 1901\n";
        rules << "pass in quick proto udp from any to any port = 1901\n";

        rules << "pass out quick inet proto udp from any to any port = 5350\n";
        rules << "pass in quick proto udp from any to any port = 5350\n";
        rules << "pass out quick inet proto udp from any to any port = 5351\n";
        rules << "pass in quick proto udp from any to any port = 5351\n";
        rules << "pass out quick inet proto udp from any to any port = 5353\n";
        rules << "pass in quick proto udp from any to any port = 5353\n";

        rules << "}\n";
    }
    rules << "anchor " WS_PRODUCT_NAME_LOWER "_static_ports_traffic all\n";

    // write rules
    // open()'s mode is ignored on pre-existing files; unlink first to force the intended perms
    unlink(WS_POSIX_CONFIG_DIR "/boot_pf.conf");
    int fd = open(WS_POSIX_CONFIG_DIR "/boot_pf.conf", O_CREAT | O_WRONLY | O_TRUNC, 0600);
    if (fd < 0) {
        spdlog::error("Could not open boot firewall rules for writing");
        return false;
    }

    if (!IO::writeAll(fd, rules.str())) {
        spdlog::error("Failed to write boot_pf.conf: {}", IO::strerror(errno));
        close(fd);
        unlink(WS_POSIX_CONFIG_DIR "/boot_pf.conf");
        return false;
    }
    close(fd);

    Utils::executeCommand("launchctl", {"disable", "system/com.apple.pfctl"});

    return true;
}

bool FirewallOnBootManager::disable()
{
    Utils::executeCommand("launchctl", {"enable", "system/com.apple.pfctl"});
    std::error_code ec;
    std::filesystem::remove(WS_POSIX_CONFIG_DIR "/boot_pf.conf", ec);
    if (ec) {
        spdlog::warn("Failed to remove boot_pf.conf: {}", ec.message());
    }
    return true;
}
