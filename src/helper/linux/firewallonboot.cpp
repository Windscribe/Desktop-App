#include "firewallonboot.h"
#include <sstream>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <spdlog/spdlog.h>
#include "utils.h"

FirewallOnBootManager::FirewallOnBootManager(): comment_(WS_PRODUCT_NAME " client rule")
{
    // Migrate old boot rules if necessary
    if (Utils::isFileExists(WS_POSIX_CONFIG_DIR "/boot_rules.v4")) {
        Utils::executeCommand("mv", {WS_POSIX_CONFIG_DIR "/boot_rules.v4", WS_LINUX_TMP_DIR "/boot_rules.v4"});
        Utils::executeCommand("rm", {WS_POSIX_CONFIG_DIR "/rules.v4"});
    }
    if (Utils::isFileExists(WS_POSIX_CONFIG_DIR "/boot_rules.v6")) {
        Utils::executeCommand("mv", {WS_POSIX_CONFIG_DIR "/boot_rules.v6", WS_LINUX_TMP_DIR "/boot_rules.v6"});
        Utils::executeCommand("rm", {WS_POSIX_CONFIG_DIR "/rules.v6"});
    }

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
    int bytes;

    rules << "*filter\n";
    rules << ":" WS_PRODUCT_NAME_LOWER "_input - [0:0]\n";
    rules << ":" WS_PRODUCT_NAME_LOWER "_output - [0:0]\n";
    rules << ":" WS_PRODUCT_NAME_LOWER "_block - [0:0]\n";
    rules << "-I INPUT -j " WS_PRODUCT_NAME_LOWER "_input -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A INPUT -j " WS_PRODUCT_NAME_LOWER "_block -m comment --comment \"" + comment_ + "\"\n";
    rules << "-I OUTPUT -j " WS_PRODUCT_NAME_LOWER "_output -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A OUTPUT -j " WS_PRODUCT_NAME_LOWER "_block -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A " WS_PRODUCT_NAME_LOWER "_input -i lo -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A " WS_PRODUCT_NAME_LOWER "_output -o lo -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A " WS_PRODUCT_NAME_LOWER "_input -p udp --sport 67 --dport 68 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A " WS_PRODUCT_NAME_LOWER "_output -p udp --sport 68 --dport 67 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";

    std::istringstream ips(ipTable_);
    std::string ip;

    while (std::getline(ips, ip, ' ')) {
        rules << "-A " WS_PRODUCT_NAME_LOWER "_input -s " + ip + " -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
        rules << "-A " WS_PRODUCT_NAME_LOWER "_output -d " + ip + " -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
    }

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
    int fd = open(WS_LINUX_TMP_DIR "/boot_rules.v4", O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRGRP | S_IROTH);
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
    rules << ":" WS_PRODUCT_NAME_LOWER "_input - [0:0]\n";
    rules << ":" WS_PRODUCT_NAME_LOWER "_output - [0:0]\n";
    rules << "-I INPUT -j " WS_PRODUCT_NAME_LOWER "_input -m comment --comment \"" + comment_ + "\"\n";
    rules << "-I OUTPUT -j " WS_PRODUCT_NAME_LOWER "_output -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A " WS_PRODUCT_NAME_LOWER "_input -s ::1/128 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A " WS_PRODUCT_NAME_LOWER "_output -d ::1/128 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A " WS_PRODUCT_NAME_LOWER "_input -j DROP -m comment --comment \"" + comment_ + "\"\n";
    rules << "-A " WS_PRODUCT_NAME_LOWER "_output -j DROP -m comment --comment \"" + comment_ + "\"\n";
    rules << "COMMIT\n";

    // write rules
    fd = open(WS_LINUX_TMP_DIR "/boot_rules.v6", O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRGRP | S_IROTH);
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
    Utils::executeCommand("rm", {"-f", WS_LINUX_TMP_DIR "/boot_rules.v4"});
    Utils::executeCommand("rm", {"-f", WS_LINUX_TMP_DIR "/boot_rules.v6"});
    return true;
}
