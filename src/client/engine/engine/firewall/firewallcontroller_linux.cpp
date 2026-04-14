#include "firewallcontroller_linux.h"
#include <QStandardPaths>
#include "utils/log/categories.h"
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <QDir>
#include <QRegularExpression>

FirewallController_linux::FirewallController_linux(QObject *parent, Helper *helper) :
    FirewallController(parent), helper_(helper), forceUpdateInterfaceToSkip_(false), comment_(WS_PRODUCT_NAME " client rule")
{
}

FirewallController_linux::~FirewallController_linux()
{
}

void FirewallController_linux::firewallOn(const QString &connectingIp, const QSet<QString> &ips, bool bAllowLanTraffic, bool bIsCustomConfig, bool isVpnConnected)
{
    QMutexLocker locker(&mutex_);
    FirewallController::firewallOn(connectingIp, ips, bAllowLanTraffic, bIsCustomConfig, isVpnConnected);
    if (isStateChanged()) {
        qCInfo(LOG_FIREWALL_CONTROLLER) << "firewall enabled with ips count:" << ips.count() + 1;
        firewallOnImpl(connectingIp, ips, bAllowLanTraffic, bIsCustomConfig, latestStaticIpPorts_);
    } else if (forceUpdateInterfaceToSkip_) {
        qCInfo(LOG_FIREWALL_CONTROLLER) << "firewall changed due to interface-to-skip update";
        firewallOnImpl(connectingIp, ips, bAllowLanTraffic, bIsCustomConfig, latestStaticIpPorts_);
    }
}

void FirewallController_linux::firewallOff()
{
    QMutexLocker locker(&mutex_);
    FirewallController::firewallOff();
    if (isStateChanged()) {
        qCInfo(LOG_FIREWALL_CONTROLLER) << "firewall off";
        QString cmd;

        // remove IPv4 rules
        removeAppRules(comment_, false);

        // remove IPv6 rules
        removeAppRules(comment_, true);

        bool ret = helper_->clearFirewallRules(false);
        if (!ret) {
            qCWarning(LOG_FIREWALL_CONTROLLER) << "Clear firewall rules unsuccessful:" << ret;
        }
    }
}

bool FirewallController_linux::firewallActualState()
{
    QMutexLocker locker(&mutex_);
    return helper_->checkFirewallState(comment_);
}

bool FirewallController_linux::whitelistPorts(const api_responses::StaticIpPortsVector &ports)
{
    Q_UNUSED(ports);
    return true;
}

bool FirewallController_linux::deleteWhitelistPorts()
{
    return true;
}

void FirewallController_linux::setInterfaceToSkip_posix(const QString &interfaceToSkip)
{
    QMutexLocker locker(&mutex_);
    qCInfo(LOG_BASIC) << "FirewallController_linux::setInterfaceToSkip_posix ->" << interfaceToSkip;
    if (interfaceToSkip_ != interfaceToSkip) {
        interfaceToSkip_ = interfaceToSkip;
        forceUpdateInterfaceToSkip_ = true;
    }
}

void FirewallController_linux::setFirewallOnBoot(bool bEnable, const QSet<QString>& ipTable, bool isAllowLanTraffic)
{
    Q_UNUSED(bEnable);
    helper_->setFirewallOnBoot(bEnable, ipTable, isAllowLanTraffic);
}

bool FirewallController_linux::firewallOnImpl(const QString &connectingIp, const QSet<QString> &ips, bool bAllowLanTraffic, bool bIsCustomConfig, const api_responses::StaticIpPortsVector &ports)
{
    // TODO: this is need for Linux?
    Q_UNUSED(ports);

    QString hotspotAdapter = getHotspotAdapter();

    forceUpdateInterfaceToSkip_ = false;
    bool bExists = firewallActualState();

    // rules for IPv4
    {
        QStringList rules;
        rules << "*filter\n";
        rules << ":" WS_PRODUCT_NAME_LOWER "_input - [0:0]\n";
        rules << ":" WS_PRODUCT_NAME_LOWER "_output - [0:0]\n";
        rules << ":" WS_PRODUCT_NAME_LOWER "_block - [0:0]\n";

        if (!bExists) {
            rules << "-I INPUT -j " WS_PRODUCT_NAME_LOWER "_input -m comment --comment \"" + comment_ + "\"\n";
            rules << "-I OUTPUT -j " WS_PRODUCT_NAME_LOWER "_output -m comment --comment \"" + comment_ + "\"\n";
        }

        if (!hasBlockRule()) {
            rules << "-A INPUT -j " WS_PRODUCT_NAME_LOWER "_block -m comment --comment \"" + comment_ + "\"\n";
            rules << "-A OUTPUT -j " WS_PRODUCT_NAME_LOWER "_block -m comment --comment \"" + comment_ + "\"\n";
        }

        rules << "-A " WS_PRODUCT_NAME_LOWER "_input -i lo -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
        rules << "-A " WS_PRODUCT_NAME_LOWER "_output -o lo -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";

        rules << "-A " WS_PRODUCT_NAME_LOWER "_input -p udp --sport 67:68 --dport 67:68 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
        rules << "-A " WS_PRODUCT_NAME_LOWER "_output -p udp --sport 67:68 --dport 67:68 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";

        if (!interfaceToSkip_.isEmpty()) {
            if (!bIsCustomConfig) {
                // Allow local addresses
                QStringList localAddrs = getLocalAddresses(interfaceToSkip_);
                for (QString addr : localAddrs) {
                    rules << "-A " WS_PRODUCT_NAME_LOWER "_input -i " + interfaceToSkip_ + " -s " + addr + "/32 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
                    rules << "-A " WS_PRODUCT_NAME_LOWER "_output -o " + interfaceToSkip_ + " -d " + addr + "/32 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
                }

                // Disallow LAN addresses (except 10.255.255.0/24), link-local addresses, loopback,
                // and local multicast addresses from going into the tunnel
                rules << "-A " WS_PRODUCT_NAME_LOWER "_input -i " + interfaceToSkip_ + " -s 192.168.0.0/16 -j DROP -m comment --comment \"" + comment_ + "\"\n";
                rules << "-A " WS_PRODUCT_NAME_LOWER "_output -o " + interfaceToSkip_ + " -d 192.168.0.0/16 -j DROP -m comment --comment \"" + comment_ + "\"\n";
                rules << "-A " WS_PRODUCT_NAME_LOWER "_input -i " + interfaceToSkip_ + " -s 172.16.0.0/12 -j DROP -m comment --comment \"" + comment_ + "\"\n";
                rules << "-A " WS_PRODUCT_NAME_LOWER "_output -o " + interfaceToSkip_ + " -d 172.16.0.0/12 -j DROP -m comment --comment \"" + comment_ + "\"\n";
                rules << "-A " WS_PRODUCT_NAME_LOWER "_input -i " + interfaceToSkip_ + " -s 169.254.0.0/16 -j DROP -m comment --comment \"" + comment_ + "\"\n";
                rules << "-A " WS_PRODUCT_NAME_LOWER "_output -o " + interfaceToSkip_ + " -d 169.254.0.0/16 -j DROP -m comment --comment \"" + comment_ + "\"\n";
                rules << "-A " WS_PRODUCT_NAME_LOWER "_input -i " + interfaceToSkip_ + " -s 10.255.255.0/24 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
                rules << "-A " WS_PRODUCT_NAME_LOWER "_output -o " + interfaceToSkip_ + " -d 10.255.255.0/24 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
                rules << "-A " WS_PRODUCT_NAME_LOWER "_input -i " + interfaceToSkip_ + " -s 10.0.0.0/8 -j DROP -m comment --comment \"" + comment_ + "\"\n";
                rules << "-A " WS_PRODUCT_NAME_LOWER "_output -o " + interfaceToSkip_ + " -d 10.0.0.0/8 -j DROP -m comment --comment \"" + comment_ + "\"\n";
                rules << "-A " WS_PRODUCT_NAME_LOWER "_input -i " + interfaceToSkip_ + " -s 224.0.0.0/4 -j DROP -m comment --comment \"" + comment_ + "\"\n";
                rules << "-A " WS_PRODUCT_NAME_LOWER "_output -o " + interfaceToSkip_ + " -d 224.0.0.0/4 -j DROP -m comment --comment \"" + comment_ + "\"\n";
            }

            rules << "-A " WS_PRODUCT_NAME_LOWER "_input -i " + interfaceToSkip_ + " -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
            rules << "-A " WS_PRODUCT_NAME_LOWER "_output -o " + interfaceToSkip_ + " -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";

            // accept filter for the hotspot adapter in the connected state
            if (!hotspotAdapter.isEmpty()) {
                rules << "-A " WS_PRODUCT_NAME_LOWER "_input -i " + hotspotAdapter + " -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
                rules << "-A " WS_PRODUCT_NAME_LOWER "_output -o " + hotspotAdapter + " -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
            }
        }

        if (!connectingIp.isEmpty()) {
            rules << "-A " WS_PRODUCT_NAME_LOWER "_input -s " + connectingIp + "/32 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
            // Allow packets from gid 0
            rules << "-A " WS_PRODUCT_NAME_LOWER "_output -d " + connectingIp + "/32 -j ACCEPT -m owner --gid-owner 0 -m comment --comment \"" + comment_ + "\"\n";
            // Allow packets from app group
            rules << "-A " WS_PRODUCT_NAME_LOWER "_output -d " + connectingIp + "/32 -j ACCEPT -m owner --gid-owner " WS_PRODUCT_NAME_LOWER " -m comment --comment \"" + comment_ + "\"\n";
            // Allow packets from kernel (no uid), for wireguard control traffic
            rules << "-A " WS_PRODUCT_NAME_LOWER "_output -d " + connectingIp + "/32 -j ACCEPT -m owner ! --uid-owner 0-4294967294 -m comment --comment \"" + comment_ + "\"\n";
            // Allow marked packets.  These packets are marked by the wireguard adapter code.
            // This is necessary because wg packets have uid/gid of the app that created the packet, not wg itself.
            rules << "-A " WS_PRODUCT_NAME_LOWER "_output -d " + connectingIp + "/32 -j ACCEPT -m mark --mark 51820 -m comment --comment \"" + comment_ + "\"\n";
        }

        for (const auto &i : ips) {
            rules << "-A " WS_PRODUCT_NAME_LOWER "_input -s " + i + "/32 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
            rules << "-A " WS_PRODUCT_NAME_LOWER "_output -d " + i + "/32 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
        }

        // drop filter for the hotspot adapter in the disconnected state
        if (!hotspotAdapter.isEmpty()) {
            rules << "-A " WS_PRODUCT_NAME_LOWER "_input -i " + hotspotAdapter + " -j DROP -m comment --comment \"" + comment_ + "\"\n";
            rules << "-A " WS_PRODUCT_NAME_LOWER "_output -o " + hotspotAdapter + " -j DROP -m comment --comment \"" + comment_ + "\"\n";
        }

        // Loopback addresses to the local host
        rules << "-A " WS_PRODUCT_NAME_LOWER "_input -s 127.0.0.0/8 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
        rules << "-A " WS_PRODUCT_NAME_LOWER "_output -d 127.0.0.0/8 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";

        // Local Network
        // Custom configs need private ranges allowed so third-party VPN DNS/gateway/routes work
        QString action = (bAllowLanTraffic || bIsCustomConfig) ? "ACCEPT" : "DROP";
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

        rules << "-A " WS_PRODUCT_NAME_LOWER "_block -j DROP -m comment --comment \"" + comment_ + "\"\n";
        rules << "COMMIT\n";

        bool ret = helper_->setFirewallRules(kIpv4, "", "", rules.join("\n"));
        if (!ret) {
            qCCritical(LOG_FIREWALL_CONTROLLER) << "Could not set v4 firewall rules:" << ret;
        }
    }

    // rules for IPv6 (disable IPv6)
    {
        QStringList rules;

        rules << "*filter\n";
        rules << ":" WS_PRODUCT_NAME_LOWER "_input - [0:0]\n";
        rules << ":" WS_PRODUCT_NAME_LOWER "_output - [0:0]\n";

        if (!bExists) {
            rules << "-I INPUT -j " WS_PRODUCT_NAME_LOWER "_input -m comment --comment \"" + comment_ + "\"\n";
            rules << "-I OUTPUT -j " WS_PRODUCT_NAME_LOWER "_output -m comment --comment \"" + comment_ + "\"\n";
        }

        // Loopback addresses to the local host
        rules << "-A " WS_PRODUCT_NAME_LOWER "_input -s ::1 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
        rules << "-A " WS_PRODUCT_NAME_LOWER "_output -d ::1 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";

        // Always allow IPv6 link-local (required for neighbor discovery, etc.)
        rules << "-A " WS_PRODUCT_NAME_LOWER "_input -s fe80::/10 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
        rules << "-A " WS_PRODUCT_NAME_LOWER "_output -d fe80::/10 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";

        // IPv6 multicast (controlled by Allow LAN traffic setting)
        QString actionV6 = (bAllowLanTraffic || bIsCustomConfig) ? "ACCEPT" : "DROP";
        rules << "-A " WS_PRODUCT_NAME_LOWER "_input -s ff00::/8 -j " + actionV6 + " -m comment --comment \"" + comment_ + "\"\n";
        rules << "-A " WS_PRODUCT_NAME_LOWER "_output -d ff00::/8 -j " + actionV6 + " -m comment --comment \"" + comment_ + "\"\n";

        rules << "-A " WS_PRODUCT_NAME_LOWER "_input -j DROP -m comment --comment \"" + comment_ + "\"\n";
        rules << "-A " WS_PRODUCT_NAME_LOWER "_output -j DROP -m comment --comment \"" + comment_ + "\"\n";
        rules << "COMMIT\n";

        bool ret = helper_->setFirewallRules(kIpv6, "", "", rules.join("\n"));
        if (!ret) {
            qCCritical(LOG_FIREWALL_CONTROLLER) << "Could not set v6 firewall rules:" << ret;
        }
    }

    return true;
}

// Extract rules from iptables with comment.If modifyForDelete == true, then replace commands for delete.
QStringList FirewallController_linux::getAppRules(const QString &comment, bool modifyForDelete, bool isIPv6)
{
    QString rules;
    QStringList outRules;
    bool ret = helper_->getFirewallRules(isIPv6 ? kIpv6 : kIpv4, "", "", rules);
    if (!ret) {
      qCWarning(LOG_FIREWALL_CONTROLLER) << "Could not get firewall rules:" << ret;
      return {};
    }

    QTextStream in(&rules);
    bool bFound = false;
    while (!in.atEnd()) {
        std::string line = in.readLine().toStdString();
        if ((line.rfind("*", 0) == 0) || // string starts with "*"
            (line.find("COMMIT") != std::string::npos) ||
            ((line.rfind("-A", 0) == 0) && (line.find("-m comment --comment \"" + comment.toStdString() + "\"") != std::string::npos)) )
        {
            if (line.rfind("-A", 0) == 0) {
                if (modifyForDelete) {
                    line[1] = 'D';
                }
                bFound = true;
            }
            outRules << QString::fromStdString(line);
        }
    }

    if (!bFound) {
        outRules.clear();
    }
    return outRules;
}

void FirewallController_linux::removeAppRules(const QString &comment, bool isIPv6)
{
    QStringList rules = getAppRules(comment, true, isIPv6);

    // delete app rules, if found
    if (rules.isEmpty()) {
        return;
    }

    QString curTable;
    for (int ind = 0; ind < rules.count(); ++ind) {
        if (rules[ind].startsWith("*")) {
            curTable = rules[ind];
        }

        if (rules[ind].contains("COMMIT") && curTable.contains("*filter")) {
            rules.insert(ind, "-X " WS_PRODUCT_NAME_LOWER "_input");
            rules.insert(ind + 1, "-X " WS_PRODUCT_NAME_LOWER "_output");
            if (!isIPv6) {
                rules.insert(ind + 2, "-X " WS_PRODUCT_NAME_LOWER "_block");
            }
            break;
        }
    }

    bool ret = helper_->setFirewallRules(isIPv6 ? kIpv6 : kIpv4, "", "", rules.join("\n") + "\n");
    if (!ret) {
        qCWarning(LOG_FIREWALL_CONTROLLER) << "Could not remove " WS_PRODUCT_NAME_LOWER " rules:" << ret;
    }
}

QStringList FirewallController_linux::getLocalAddresses(const QString iface) const
{
    QStringList addrs;
    struct ifaddrs *ifap;
    struct ifaddrs *cur;

    if (getifaddrs(&ifap)) {
        qCCritical(LOG_FIREWALL_CONTROLLER) << "Error: could not get local interface addresses";
        return addrs;
    }

    cur = ifap;
    while (cur) {
        if (strncmp(iface.toStdString().c_str(), cur->ifa_name, iface.length()) ||
            cur->ifa_addr == nullptr ||
            cur->ifa_addr->sa_family != AF_INET)
        {
            cur = cur->ifa_next;
            continue;
        }
        char str[16];
        inet_ntop(AF_INET, &(((struct sockaddr_in *)cur->ifa_addr)->sin_addr), str, 16);
        addrs << QString(str);
        cur = cur->ifa_next;
    }

    freeifaddrs(ifap);
    return addrs;
}

// Return hotspot adapter name, or empty string if hotspot adapter not found on not connected
QString FirewallController_linux::getHotspotAdapter() const
{
#ifndef CLI_ONLY
    QString strReply;
    FILE *file = popen("nmcli d | grep Hotspot", "r");
    if (file)
    {
        char szLine[4096];
        while(fgets(szLine, sizeof(szLine), file) != 0)
        {
            strReply += szLine;
        }
        pclose(file);
    }

    if (strReply.isEmpty())
        return QString();

    QRegularExpression regExp("\\s+");
    QStringList list = strReply.split(regExp, Qt::SkipEmptyParts);
    // must be 4 values
    if (list.count() != 4) {
        qCCritical(LOG_FIREWALL_CONTROLLER) << "Can't parse get hotspot adapter name function output:" << strReply;
        return QString();
    }

    if (list[2].compare("connected", Qt::CaseInsensitive) == 0) {
        return list[0];
    }
#endif

    return QString();
}

bool FirewallController_linux::hasBlockRule()
{
    QString rules;
    bool ret = helper_->getFirewallRules(kIpv4, "", "", rules);
    if (!ret) {
        return false;
    }

    QTextStream in(&rules);
    while (!in.atEnd()) {
        std::string line = in.readLine().toStdString();
        if (line.find("-j " WS_PRODUCT_NAME_LOWER "_block") != std::string::npos) {
            return true;
        }
    }
    return false;
}
