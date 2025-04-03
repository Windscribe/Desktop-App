#include "firewallonboot.h"
#include <sstream>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <spdlog/spdlog.h>
#include "utils.h"

FirewallOnBootManager::FirewallOnBootManager(): comment_("Windscribe client rule")
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
    int bytes;

    rules << "*filter\n";
    rules << ":windscribe_input - [0:0]\n";
    rules << ":windscribe_output - [0:0]\n";
    rules << ":windscribe_block - [0:0]\n";
    rules << "-I INPUT -j windscribe_input -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A INPUT -j windscribe_block -m comment --comment \"" + comment_ + "\"\n";
    rules << "-I OUTPUT -j windscribe_output -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A OUTPUT -j windscribe_block -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A windscribe_input -i lo -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A windscribe_output -o lo -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A windscribe_input -p udp --sport 67 --dport 68 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A windscribe_output -p udp --sport 68 --dport 67 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";

    std::istringstream ips(ipTable_);
    std::string ip;

    while (std::getline(ips, ip, ' ')) {
        rules << "-A windscribe_input -s " + ip + " -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
        rules << "-A windscribe_output -d " + ip + " -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
    }

    std::string action;
    if (allowLanTraffic) {
        action = "ACCEPT";
    } else {
        action = "DROP";
    }

    // Local Network
    rules << "-A windscribe_input -s 192.168.0.0/16 -j " + action + " -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A windscribe_output -d 192.168.0.0/16 -j " + action + " -m comment --comment \"" + comment_ + "\"\n";

    rules << "-A windscribe_input -s 172.16.0.0/12 -j " + action + " -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A windscribe_output -d 172.16.0.0/12 -j " + action + " -m comment --comment \"" + comment_ + "\"\n";

    rules << "-A windscribe_input -s 169.254.0.0/16 -j " + action + " -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A windscribe_output -d 169.254.0.0/16 -j " + action + " -m comment --comment \"" + comment_ + "\"\n";

    rules << "-A windscribe_input -s 10.255.255.0/24 -j DROP -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A windscribe_output -d 10.255.255.0/24 -j DROP -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A windscribe_input -s 10.0.0.0/8 -j " + action + " -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A windscribe_output -d 10.0.0.0/8 -j " + action + " -m comment --comment \"" + comment_ + "\"\n";

    // Multicast addresses
    rules << "-A windscribe_input -s 224.0.0.0/4 -j " + action + " -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A windscribe_output -d 224.0.0.0/4 -j " + action + " -m comment --comment \"" + comment_ + "\"\n";

    // Block all rule
    rules << "-A windscribe_block -j DROP -m comment --comment \"" + comment_ + "\"\n";
    rules << "COMMIT\n";

    // write rules
    int fd = open("/etc/windscribe/boot_rules.v4", O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRGRP | S_IROTH);
    if (fd < 0) {
        spdlog::error("Could not open boot firewall rules for writing");
        return false;
    }

    bytes = write(fd, rules.str().c_str(), rules.str().length());
    (void)bytes;
    close(fd);

    rules.str("");
    rules.clear();

    rules << "*filter\n";
    rules << ":windscribe_input - [0:0]\n";
    rules << ":windscribe_output - [0:0]\n";
    rules << "-A INPUT -j windscribe_input -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A OUTPUT -j windscribe_output -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A windscribe_input -s ::1/128 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A windscribe_output -d ::1/128 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A windscribe_input -j DROP -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A windscribe_output -j DROP -m comment --comment \"" + comment_ + "\"\n";
    rules << "COMMIT\n";

    // write rules
    fd = open("/etc/windscribe/boot_rules.v6", O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRGRP | S_IROTH);
    if (fd < 0) {
        spdlog::error("Could not open v6 boot firewall rules for writing");
        return false;
    }

    bytes = write(fd, rules.str().c_str(), rules.str().length());
    (void)bytes;
    close(fd);

    return true;
}

bool FirewallOnBootManager::disable()
{
    Utils::executeCommand("rm", {"-f", "/etc/windscribe/boot_rules.v4"});
    Utils::executeCommand("rm", {"-f", "/etc/windscribe/boot_rules.v6"});
    return true;
}
