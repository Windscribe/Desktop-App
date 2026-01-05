#include "firewallonboot.h"
#include <sstream>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <spdlog/spdlog.h>

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

    rules << "set block-policy return\n";
    rules << "set skip on { lo0 }\n";
    rules << "scrub in all fragment reassemble\n";
    rules << "block all\n";
    rules << "table <windscribe_ips> persist {";
    rules << ipTable_.c_str();
    rules << "}\n";
    rules << "pass out quick inet from any to <windscribe_ips>\n";
    rules << "anchor windscribe_vpn_traffic all\n";
    rules << "pass out quick inet proto udp from any to any port = 67\n";
    rules << "pass in quick inet proto udp from any to any port = 68\n";
    if (allowLanTraffic) {
        rules << "anchor windscribe_lan_traffic all {\n";

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

        // Multicast addresses
        rules << "pass out quick inet from any to 224.0.0.0/4\n";
        rules << "pass in quick inet from 224.0.0.0/4 to any\n";

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
    rules << "anchor windscribe_static_ports_traffic all\n";

    // write rules
    int fd = open("/etc/windscribe/boot_pf.conf", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        spdlog::error("Could not open boot firewall rules for writing");
        return false;
    }

    write(fd, rules.str().c_str(), rules.str().length());
    close(fd);

    Utils::executeCommand("launchctl", {"disable", "system/com.apple.pfctl"});

    return true;
}

bool FirewallOnBootManager::disable()
{
    Utils::executeCommand("launchctl", {"enable", "system/com.apple.pfctl"});
    Utils::executeCommand("rm", {"-f", "/etc/windscribe/boot_pf.conf"});
    return true;
}
