#include "firewallcontroller_linux.h"
#include <QStandardPaths>
#include "utils/logger.h"
#include <ifaddrs.h>
#include <QDir>
#include <QRegularExpression>
#include "engine/helper/ihelper.h"

FirewallController_linux::FirewallController_linux(QObject *parent, IHelper *helper) :
    FirewallController(parent), forceUpdateInterfaceToSkip_(false), comment_("Windscribe client rule")
{
    helper_ = dynamic_cast<Helper_linux *>(helper);
}

FirewallController_linux::~FirewallController_linux()
{
}

bool FirewallController_linux::firewallOn(const QString &connectingIp, const QSet<QString> &ips, bool bAllowLanTraffic, bool bIsCustomConfig)
{
    QMutexLocker locker(&mutex_);
    FirewallController::firewallOn(connectingIp, ips, bAllowLanTraffic, bIsCustomConfig);
    if (isStateChanged()) {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "firewall enabled with ips count:" << ips.count() + 1;
        return firewallOnImpl(connectingIp, ips, bAllowLanTraffic, bIsCustomConfig, latestStaticIpPorts_);
    } else if (forceUpdateInterfaceToSkip_) {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "firewall changed due to interface-to-skip update";
        return firewallOnImpl(connectingIp, ips, bAllowLanTraffic, bIsCustomConfig, latestStaticIpPorts_);
    }
    return true;
}

bool FirewallController_linux::firewallOff()
{
    QMutexLocker locker(&mutex_);
    FirewallController::firewallOff();
    if (isStateChanged()) {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "firewall off";
        QString cmd;

        // remove IPv4 rules
        removeWindscribeRules(comment_, false);

        // remove IPv6 rules
        removeWindscribeRules(comment_, true);

        bool ret = helper_->clearFirewallRules(false);
        if (!ret) {
            qCDebug(LOG_FIREWALL_CONTROLLER) << "Clear firewall rules unsuccessful:" << ret;
        }
        return true;
    }
    return true;
}

bool FirewallController_linux::firewallActualState()
{
    QMutexLocker locker(&mutex_);
    if (helper_->currentState() != IHelper::STATE_CONNECTED) {
        return false;
    }
    return helper_->checkFirewallState(comment_);
}

bool FirewallController_linux::whitelistPorts(const apiinfo::StaticIpPortsVector &ports)
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
    qCDebug(LOG_BASIC) << "FirewallController_linux::setInterfaceToSkip_posix ->" << interfaceToSkip;
    if (interfaceToSkip_ != interfaceToSkip) {
        interfaceToSkip_ = interfaceToSkip;
        forceUpdateInterfaceToSkip_ = true;
    }
}

void FirewallController_linux::enableFirewallOnBoot(bool bEnable, const QSet<QString>& ipTable)
{
    Q_UNUSED(bEnable);
    //nothing todo for Linux
}

bool FirewallController_linux::firewallOnImpl(const QString &connectingIp, const QSet<QString> &ips, bool bAllowLanTraffic, bool bIsCustomConfig, const apiinfo::StaticIpPortsVector &ports)
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
        rules << ":windscribe_input - [0:0]\n";
        rules << ":windscribe_output - [0:0]\n";

        if (!bExists) {
            rules << "-A INPUT -j windscribe_input -m comment --comment \"" + comment_ + "\"\n";
            rules << "-A OUTPUT -j windscribe_output -m comment --comment \"" + comment_ + "\"\n";
        }

        rules << "-A windscribe_input -i lo -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
        rules << "-A windscribe_output -o lo -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";

        if (!interfaceToSkip_.isEmpty()) {
            if (!bIsCustomConfig) {
                // Allow local addresses
                QStringList localAddrs = getLocalAddresses(interfaceToSkip_);
                for (QString addr : localAddrs) {
                    rules << "-A windscribe_input -i " + interfaceToSkip_ + " -s " + addr + "/32 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
                    rules << "-A windscribe_output -o " + interfaceToSkip_ + " -d " + addr + "/32 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
                }

                // Disallow LAN addresses (except 10.255.255.0/24), link-local addresses, loopback,
                // and local multicast addresses from going into the tunnel
                rules << "-A windscribe_input -i " + interfaceToSkip_ + " -s 192.168.0.0/16 -j DROP -m comment --comment \"" + comment_ + "\"\n";
                rules << "-A windscribe_output -o " + interfaceToSkip_ + " -d 192.168.0.0/16 -j DROP -m comment --comment \"" + comment_ + "\"\n";
                rules << "-A windscribe_input -i " + interfaceToSkip_ + " -s 172.16.0.0/12 -j DROP -m comment --comment \"" + comment_ + "\"\n";
                rules << "-A windscribe_output -o " + interfaceToSkip_ + " -d 172.16.0.0/12 -j DROP -m comment --comment \"" + comment_ + "\"\n";
                rules << "-A windscribe_input -i " + interfaceToSkip_ + " -s 169.254.0.0/16 -j DROP -m comment --comment \"" + comment_ + "\"\n";
                rules << "-A windscribe_output -o " + interfaceToSkip_ + " -d 169.254.0.0/16 -j DROP -m comment --comment \"" + comment_ + "\"\n";
                rules << "-A windscribe_input -i " + interfaceToSkip_ + " -s 10.255.255.0/24 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
                rules << "-A windscribe_output -o " + interfaceToSkip_ + " -d 10.255.255.0/24 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
                rules << "-A windscribe_input -i " + interfaceToSkip_ + " -s 10.0.0.0/8 -j DROP -m comment --comment \"" + comment_ + "\"\n";
                rules << "-A windscribe_output -o " + interfaceToSkip_ + " -d 10.0.0.0/8 -j DROP -m comment --comment \"" + comment_ + "\"\n";
                rules << "-A windscribe_input -i " + interfaceToSkip_ + " -s 224.0.0.0/24 -j DROP -m comment --comment \"" + comment_ + "\"\n";
                rules << "-A windscribe_output -o " + interfaceToSkip_ + " -d 224.0.0.0/24 -j DROP -m comment --comment \"" + comment_ + "\"\n";
            }

            rules << "-A windscribe_input -i " + interfaceToSkip_ + " -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
            rules << "-A windscribe_output -o " + interfaceToSkip_ + " -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";

            // accept filter for the hotspot adapter in the connected state
            if (!hotspotAdapter.isEmpty()) {
                rules << "-A windscribe_input -i " + hotspotAdapter + " -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
                rules << "-A windscribe_output -o " + hotspotAdapter + " -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
            }
        }

        if (!connectingIp.isEmpty()) {
            rules << "-A windscribe_input -s " + connectingIp + "/32 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
            // Allow packets from gid 0
            rules << "-A windscribe_output -d " + connectingIp + "/32 -j ACCEPT -m owner --gid-owner 0 -m comment --comment \"" + comment_ + "\"\n";
            // Allow packets from windscribe group
            rules << "-A windscribe_output -d " + connectingIp + "/32 -j ACCEPT -m owner --gid-owner windscribe -m comment --comment \"" + comment_ + "\"\n";
            // Allow packets from kernel (no uid), for wireguard control traffic
            rules << "-A windscribe_output -d " + connectingIp + "/32 -j ACCEPT -m owner ! --uid-owner 0-4294967294 -m comment --comment \"" + comment_ + "\"\n";
            // Allow marked packets.  These packets are marked by the wireguard adapter code.
            // This is necessary because wg packets have uid/gid of the app that created the packet, not wg itself.
            rules << "-A windscribe_output -d " + connectingIp + "/32 -j ACCEPT -m mark --mark 51820 -m comment --comment \"" + comment_ + "\"\n";
        }

        for (const auto &i : ips) {
            rules << "-A windscribe_input -s " + i + "/32 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
            rules << "-A windscribe_output -d " + i + "/32 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
        }

        // drop filter for the hotspot adapter in the disconnected state
        if (!hotspotAdapter.isEmpty()) {
            rules << "-A windscribe_input -i " + hotspotAdapter + " -j DROP -m comment --comment \"" + comment_ + "\"\n";
            rules << "-A windscribe_output -o " + hotspotAdapter + " -j DROP -m comment --comment \"" + comment_ + "\"\n";
        }

        // Loopback addresses to the local host
        rules << "-A windscribe_input -s 127.0.0.0/8 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
        rules << "-A windscribe_output -d 127.0.0.0/8 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";

        if (bAllowLanTraffic) {
            // Local Network
            rules << "-A windscribe_input -s 192.168.0.0/16 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
            rules << "-A windscribe_output -d 192.168.0.0/16 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";

            rules << "-A windscribe_input -s 172.16.0.0/12 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
            rules << "-A windscribe_output -d 172.16.0.0/12 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";

            rules << "-A windscribe_input -s 169.254.0.0/16 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
            rules << "-A windscribe_output -d 169.254.0.0/16 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";

            rules << "-A windscribe_input -s 10.255.255.0/24 -j DROP -m comment --comment \"" + comment_ + "\"\n";
            rules << "-A windscribe_output -d 10.255.255.0/24 -j DROP -m comment --comment \"" + comment_ + "\"\n";
            rules << "-A windscribe_input -s 10.0.0.0/8 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
            rules << "-A windscribe_output -d 10.0.0.0/8 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";

            // Multicast addresses
            rules << "-A windscribe_input -s 224.0.0.0/4 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
            rules << "-A windscribe_output -d 224.0.0.0/4 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
        }

        rules << "-A windscribe_input -j DROP -m comment --comment \"" + comment_ + "\"\n";
        rules << "-A windscribe_output -j DROP -m comment --comment \"" + comment_ + "\"\n";
        rules << "COMMIT\n";

        bool ret = helper_->setFirewallRules(kIpv4, "", "", rules.join("\n"));
        if (!ret) {
            qCDebug(LOG_FIREWALL_CONTROLLER) << "Could not set v4 firewall rules:" << ret;
        }
    }

    // rules for IPv6 (disable IPv6)
    {
        QStringList rules;

        rules << "*filter\n";
        rules << ":windscribe_input - [0:0]\n";
        rules << ":windscribe_output - [0:0]\n";

        if (!bExists) {
            rules << "-A INPUT -j windscribe_input -m comment --comment \"" + comment_ + "\"\n";
            rules << "-A OUTPUT -j windscribe_output -m comment --comment \"" + comment_ + "\"\n";
        }

        // Loopback addresses to the local host
        rules << "-A windscribe_input -s ::1/128 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";
        rules << "-A windscribe_output -d ::1/128 -j ACCEPT -m comment --comment \"" + comment_ + "\"\n";

        rules << "-A windscribe_input -j DROP -m comment --comment \"" + comment_ + "\"\n";
        rules << "-A windscribe_output -j DROP -m comment --comment \"" + comment_ + "\"\n";
        rules << "COMMIT\n";

        bool ret = helper_->setFirewallRules(kIpv6, "", "", rules.join("\n"));
        if (!ret) {
            qCDebug(LOG_FIREWALL_CONTROLLER) << "Could not set v6 firewall rules:" << ret;
        }
    }

    return true;
}

// Extract rules from iptables with comment.If modifyForDelete == true, then replace commands for delete.
QStringList FirewallController_linux::getWindscribeRules(const QString &comment, bool modifyForDelete, bool isIPv6)
{
    QString rules;
    QStringList outRules;
    bool ret = helper_->getFirewallRules(isIPv6 ? kIpv6 : kIpv4, "", "", rules);
    if (!ret) {
      qCDebug(LOG_FIREWALL_CONTROLLER) << "Could not get firewall rules:" << ret;
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

void FirewallController_linux::removeWindscribeRules(const QString &comment, bool isIPv6)
{
    QStringList rules = getWindscribeRules(comment, true, isIPv6);

    // delete Windscribe rules, if found
    if (rules.isEmpty()) {
        return;
    }

    QString curTable;
    for (int ind = 0; ind < rules.count(); ++ind) {
        if (rules[ind].startsWith("*")) {
            curTable = rules[ind];
        }

        if (rules[ind].contains("COMMIT") && curTable.contains("*filter")) {
            rules.insert(ind, "-X windscribe_input");
            rules.insert(ind + 1, "-X windscribe_output");
            break;
        }
    }

    bool ret = helper_->setFirewallRules(isIPv6 ? kIpv6 : kIpv4, "", "", rules.join("\n") + "\n");
    if (!ret) {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Could not remove windscribe rules:" << ret;
    }
}

QStringList FirewallController_linux::getLocalAddresses(const QString iface) const
{
    QStringList addrs;
    struct ifaddrs *ifap;
    struct ifaddrs *cur;

    if (getifaddrs(&ifap)) {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Error: could not get local interface addresses";
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
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Can't parse get hotspot adapter name function output:" << strReply;
        return QString();
    }

    if (list[2].compare("connected", Qt::CaseInsensitive) == 0) {
        return list[0];
    }

    return QString();
}
