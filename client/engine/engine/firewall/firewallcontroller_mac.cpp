#include "firewallcontroller_mac.h"
#include <QStandardPaths>
#include "utils/ws_assert.h"
#include "utils/logger.h"
#include "utils/network_utils/network_utils_mac.h"
#include <ifaddrs.h>
#include <QDir>
#include <QCoreApplication>
#include <QSettings>

class Anchor
{
public:
    Anchor(const QString &name) : name_(name) {}
    void addRule(const QString &rule) {
        rules_ << rule;
    }

    void addRules(const QStringList &rules) {
        rules_ << rules;
    }

    QString getString() const {
        if (rules_.isEmpty()) {
            return "anchor " + name_ + " all";
        } else {
            QString str = "anchor " + name_ + " all {\n";
            for (const auto &r : rules_) {
                str += r + "\n";
            }
            str += "}";
            return str;
        }
    }

    QString generateConf() {
        QString str;
        for (const auto &r : rules_) {
            str += r + "\n";
        }
        return str;
    }

private:
    QString name_;
    QStringList rules_;
};

FirewallController_mac::FirewallController_mac(QObject *parent, IHelper *helper) :
    FirewallController(parent), isWindscribeFirewallEnabled_(false), isAllowLanTraffic_(false), isCustomConfig_(false), connectingIp_("")
{
    helper_ = dynamic_cast<Helper_mac *>(helper);

    awdl_p2p_interfaces_ = NetworkUtils_mac::getP2P_AWDL_NetworkInterfaces();

    FirewallState firewallState;
    getFirewallStateFromPfctl(firewallState);

    if (firewallState.isEnabled && !firewallState.isBasicWindscribeRulesCorrect) {
        setPfWasEnabledState(true);
        isWindscribeFirewallEnabled_ = false;
    } else if (firewallState.isEnabled && firewallState.isBasicWindscribeRulesCorrect) {
        windscribeIps_ = firewallState.windscribeIps;
        if (windscribeIps_.isEmpty())
        {
            qCDebug(LOG_FIREWALL_CONTROLLER) << "Warning: the firewall was enabled at the start, but windscribe_ips table not found or empty.";
            WS_ASSERT(false);
        }
        interfaceToSkip_ = firewallState.interfaceToSkip;
        isAllowLanTraffic_ = firewallState.isAllowLanTraffic;
        isCustomConfig_ = firewallState.isCustomConfig;
        isWindscribeFirewallEnabled_ = true;
    } else {
        setPfWasEnabledState(false);
        isWindscribeFirewallEnabled_ = false;
    }
}

bool FirewallController_mac::firewallOn(const QString &connectingIp, const QSet<QString> &ips, bool bAllowLanTraffic, bool bIsCustomConfig)
{
    QMutexLocker locker(&mutex_);
    FirewallState firewallState;

    if (!checkInternalVsPfctlState(&firewallState)) {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Fatal error: firewall internal state not equal firewall state from pfctl";
    }

    if (!isWindscribeFirewallEnabled_) {
        setPfWasEnabledState(firewallState.isEnabled);
        QString pfConfig = generatePfConf(connectingIp, ips, bAllowLanTraffic, bIsCustomConfig, interfaceToSkip_);
        if (!pfConfig.isEmpty()) {
            helper_->setFirewallRules(kIpv4, "", "", pfConfig);
            connectingIp_ = connectingIp;
            windscribeIps_ = ips;
            isAllowLanTraffic_ = bAllowLanTraffic;
            isCustomConfig_ = bIsCustomConfig;
            isWindscribeFirewallEnabled_ = true;
        } else {
            qCDebug(LOG_FIREWALL_CONTROLLER) << "Fatal error: can't set firewall rules";
        }
    } else {
        if (bIsCustomConfig != isCustomConfig_) {
            isCustomConfig_ = bIsCustomConfig;
            updateVpnAnchor();
        }

        if (connectingIp != connectingIp_) {
            connectingIp_ = connectingIp;
            updateVpnAnchor();
        }

        if (ips != windscribeIps_) {
            QString table = generateTable(ips);
            helper_->setFirewallRules(kIpv4, "windscribe_ips", "", table);
            windscribeIps_ = ips;
        }

        if (bAllowLanTraffic != isAllowLanTraffic_) {
            Anchor anchor("windscribe_lan_traffic");
            anchor.addRules(lanTrafficRules(bAllowLanTraffic));

            QString anchorConf = anchor.generateConf();
            helper_->setFirewallRules(kIpv4, "", "windscribe_lan_traffic", anchorConf);
            isAllowLanTraffic_ = bAllowLanTraffic;
        }
    }

    return true;
}

bool FirewallController_mac::firewallOff()
{
    QMutexLocker locker(&mutex_);

    if (!checkInternalVsPfctlState()) {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Fatal error: firewall internal state not equal firewall state from pfctl";
    }
    firewallOffImpl();
    isWindscribeFirewallEnabled_ = false;
    windscribeIps_.clear();
    return true;
}

bool FirewallController_mac::firewallActualState()
{
    QMutexLocker locker(&mutex_);

    if (!checkInternalVsPfctlState()) {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Fatal error: firewall internal state not equal firewall state from pfctl";
    }

    return isWindscribeFirewallEnabled_;
}

bool FirewallController_mac::whitelistPorts(const apiinfo::StaticIpPortsVector &ports)
{
    QMutexLocker locker(&mutex_);
    if (!checkInternalVsPfctlState()) {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Fatal error: firewall internal state not equal firewall state from pfctl";
    }

    if (isWindscribeFirewallEnabled_) {
        if (staticIpPorts_ != ports) {
            Anchor portsAnchor("windscribe_static_ports_traffic");
            if (!ports.isEmpty()) {
                for (unsigned int port : ports) {
                    portsAnchor.addRule("pass in quick proto tcp from any to any port = " + QString::number(port));
                }
            }

            QString anchorConf = portsAnchor.generateConf();
            staticIpPorts_ = ports;
            helper_->setFirewallRules(kIpv4, "", "windscribe_static_ports_traffic", anchorConf);
        }
    } else {
        staticIpPorts_ = ports;
    }

    return true;
}

bool FirewallController_mac::deleteWhitelistPorts()
{
    return whitelistPorts(apiinfo::StaticIpPortsVector());
}

void FirewallController_mac::firewallOffImpl()
{
    helper_->clearFirewallRules(isPfWasEnabled());
    isWindscribeFirewallEnabled_ = false;
    qCDebug(LOG_FIREWALL_CONTROLLER) << "firewallOff disabled";
}


QStringList FirewallController_mac::lanTrafficRules(bool bAllowLanTraffic) const
{
    QStringList rules;

    // Always allow localhost
    rules << "pass out quick inet from any to 127.0.0.0/8";
    rules << "pass in quick inet from 127.0.0.0/8 to any";

    if (bAllowLanTraffic) {
        // Local Network
        rules << "pass out quick inet from any to 192.168.0.0/16";
        rules << "pass in quick inet from 192.168.0.0/16 to any";
        rules << "pass out quick inet from any to 172.16.0.0/12";
        rules << "pass in quick inet from 172.16.0.0/12 to any";
        rules << "pass out quick inet from any to 169.254.0.0/16";
        rules << "pass in quick inet from 169.254.0.0/16 to any";
        rules << "block out quick inet from any to 10.255.255.0/24";
        rules << "block in quick inet from 10.255.255.0/24 to any";
        rules << "pass out quick inet from any to 10.0.0.0/8";
        rules << "pass in quick inet from 10.0.0.0/8 to any";

        // Multicast addresses
        rules << "pass out quick inet from any to 224.0.0.0/4";
        rules << "pass in quick inet from 224.0.0.0/4 to any";

        // UPnP
        rules << "pass out quick inet proto udp from any to any port = 1900";
        rules << "pass in quick proto udp from any to any port = 1900";
        rules << "pass out quick inet proto udp from any to any port = 1901";
        rules << "pass in quick proto udp from any to any port = 1901";

        rules << "pass out quick inet proto udp from any to any port = 5350";
        rules << "pass in quick proto udp from any to any port = 5350";
        rules << "pass out quick inet proto udp from any to any port = 5351";
        rules << "pass in quick proto udp from any to any port = 5351";
        rules << "pass out quick inet proto udp from any to any port = 5353";
        rules << "pass in quick proto udp from any to any port = 5353";
    }
    return rules;
}

void FirewallController_mac::getFirewallStateFromPfctl(FirewallState &outState)
{
    outState.isEnabled = helper_->checkFirewallState(QString());

    QString output;
    bool ret;

    // checking a few key rules to make sure the firewall rules is settled by our program
    helper_->getFirewallRules(kIpv4, "", "", output);
    if (output.indexOf("anchor \"windscribe_vpn_traffic\" all") != -1 &&
        output.indexOf("anchor \"windscribe_lan_traffic\" all") != -1 &&
        output.indexOf("anchor \"windscribe_static_ports_traffic\" all") != -1)
    {
        outState.isBasicWindscribeRulesCorrect = true;
    } else {
        outState.isBasicWindscribeRulesCorrect = false;
    }

    // get windscribe_ips table IPs
    output.clear();
    outState.windscribeIps.clear();
    ret = helper_->getFirewallRules(kIpv4, "windscribe_ips", "", output);
    if (ret && !output.isEmpty()) {
        const QStringList list = output.split("\n");
        for (auto &ip : list) {
            if (!ip.trimmed().isEmpty()) {
                outState.windscribeIps << ip.trimmed();
            }
        }
    }

    // read anchor windscribe_vpn_traffic rules
    output.clear();
    outState.interfaceToSkip.clear();
    ret = helper_->getFirewallRules(kIpv4, "", "windscribe_vpn_traffic", output);
    output = output.trimmed();
    if (ret && !output.isEmpty()) {
        QStringList rules = output.split("\n");
        if (rules.size() > 0) {
            QStringList words = rules[0].split(" ");
            if (words.size() >= 10) {
                // pass out quick on [interface]
                outState.interfaceToSkip = words[4];
            } else {
                WS_ASSERT(false);
            }
        }
        outState.isCustomConfig = (rules.size() == 2 && !outState.interfaceToSkip.isEmpty());
    } else {
        outState.isCustomConfig = false;
    }
    // read anchor windscribe_lan_traffic rules
    outState.isAllowLanTraffic = false;
    output.clear();
    ret = helper_->getFirewallRules(kIpv4, "", "windscribe_lan_traffic", output);
    output = output.trimmed();
    if (ret && !output.isEmpty()) {
        QStringList rules = output.split("\n");
        outState.isAllowLanTraffic = (rules.size() > 2);
    }

    // read anchor windscribe_static_ports_traffic rules
    outState.isStaticIpPortsEmpty = true;
    output.clear();
    ret = helper_->getFirewallRules(kIpv4, "", "windscribe_static_ports_traffic", output);
    output = output.trimmed();
    if (ret && !output.isEmpty()) {
        outState.isStaticIpPortsEmpty = false;
    }
}

bool FirewallController_mac::checkInternalVsPfctlState(FirewallState *outFirewallState /*= nullptr*/)
{
    FirewallState firewallState;
    getFirewallStateFromPfctl(firewallState);

    if (outFirewallState)
        *outFirewallState = firewallState;

    if (firewallState.isEnabled && firewallState.isBasicWindscribeRulesCorrect) {
        if (!isWindscribeFirewallEnabled_ ||
            windscribeIps_ != firewallState.windscribeIps ||
            interfaceToSkip_ != firewallState.interfaceToSkip ||
            isAllowLanTraffic_ != firewallState.isAllowLanTraffic ||
            isCustomConfig_ != firewallState.isCustomConfig ||
            (staticIpPorts_.isEmpty() && !firewallState.isStaticIpPortsEmpty) ||
            (!staticIpPorts_.isEmpty() && firewallState.isStaticIpPortsEmpty))
        {
            return false;
        }
    }
    return true;
}

QString FirewallController_mac::generatePfConf(const QString &connectingIp, const QSet<QString> &ips, bool bAllowLanTraffic, bool bIsCustomConfig, const QString &interfaceToSkip)
{
    QString pf = "";
    pf += "# Automatically generated by Windscribe. Any manual change will be overridden.\n";

    // general options
    pf += "set block-policy drop\n";
    pf += "set fingerprints '/etc/pf.os'\n";
    pf += "set skip on lo0\n";

    // Apple anchor
    pf += "scrub-anchor \"com.apple/*\"\n";
    pf += "nat-anchor \"com.apple/*\"\n";
    pf += "rdr-anchor \"com.apple/*\"\n";
    pf += "dummynet-anchor \"com.apple/*\"\n";
    pf += "anchor \"com.apple/*\"\n";
    pf += "load anchor \"com.apple\" from \"/etc/pf.anchors/com.apple\"\n";

    // skip awdl and p2p interfaces (awdl Apple Wireless Direct Link and p2p related to AWDL features)
    for (const auto &interface : awdl_p2p_interfaces_)
        pf += "pass quick on " + interface + "\n";

    // block malformed packets
    pf += "block in quick from no-route to any\n";
    pf += "block in quick from urpf-failed\n";

    // block inbound icmp echo requests
    pf += "pass proto icmp\n";
    pf += "block in quick inet proto icmp all icmp-type echoreq\n";

    // always allow DHCP
    pf += "pass out quick proto {tcp, udp} from any port {68} to any port {67}\n";
    pf += "pass in quick proto {tcp, udp} from any port {67} to any port {68}\n";
    pf += "pass out quick inet6 proto udp from any to any port {546}\n";
    pf += "pass in quick inet6 proto udp from any to any port {547}\n";

    // always allow igmp
    pf += "pass proto igmp allow-opts\n";

    // always allow esp/gre
    pf += "pass proto {esp, gre} from any to any\n";

    // block everything
    pf += "block all\n";

    // add Windscribe rules
    pf += "table <windscribe_ips> persist {";
    for (auto &ip : ips)
    {
        pf += ip + " ";
    }
    pf += "}\n";

    pf += "pass out quick inet from any to <windscribe_ips>\n";
    pf += "pass in quick inet from <windscribe_ips> to any\n";

    Anchor vpnTrafficAnchor("windscribe_vpn_traffic");
    vpnTrafficAnchor.addRules(vpnTrafficRules(connectingIp, interfaceToSkip, bIsCustomConfig));
    pf += vpnTrafficAnchor.getString() + "\n";

    Anchor lanTrafficAnchor("windscribe_lan_traffic");
    lanTrafficAnchor.addRules(lanTrafficRules(bAllowLanTraffic));
    pf += lanTrafficAnchor.getString() + "\n";

    Anchor portsAnchor("windscribe_static_ports_traffic");
    if (!staticIpPorts_.isEmpty()) {
        for (unsigned int port : staticIpPorts_) {
            portsAnchor.addRule("pass in quick proto tcp from any to any port = " + QString::number(port));
        }
    }
    pf += portsAnchor.getString() + "\n";
    return pf;
}

QString FirewallController_mac::generateTable(const QSet<QString> &ips)
{
    QString pf = "table <windscribe_ips> persist {";
    for (auto &ip : ips) {
        pf += ip + " ";
    }
    pf += "}\n";
    return pf;
}

void FirewallController_mac::setInterfaceToSkip_posix(const QString &interfaceToSkip)
{
    QMutexLocker locker(&mutex_);
    qCDebug(LOG_BASIC) << "FirewallController_mac::setInterfaceToSkip_posix ->" << interfaceToSkip;
    if (!checkInternalVsPfctlState()) {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Fatal error: firewall internal state not equal firewall state from pfctl";
    }

    if (isWindscribeFirewallEnabled_) {
        if (interfaceToSkip_ != interfaceToSkip) {
            interfaceToSkip_ = interfaceToSkip;
            updateVpnAnchor();
        }
    } else {
        interfaceToSkip_ = interfaceToSkip;
    }
}

void FirewallController_mac::enableFirewallOnBoot(bool bEnable, const QSet<QString> &ipTable)
{
    helper_->setFirewallOnBoot(bEnable, ipTable);
}

QStringList FirewallController_mac::vpnTrafficRules(const QString &connectingIp, const QString &interfaceToSkip, bool bIsCustomConfig) const
{
    QStringList rules;

    if (!interfaceToSkip.isEmpty()) {
        if (!bIsCustomConfig) {
            // Allow local addresses
            QList<QString> localAddrs = getLocalAddresses(interfaceToSkip);
            for (QString addr : localAddrs) {
                rules << "pass out quick on " + interfaceToSkip + " inet from any to " + addr + "/32";
                rules << "pass in quick on " + interfaceToSkip + " inet from " + addr + "/32 to any";
            }
            // Disallow RFC1918/link local/loopback traffic to go over tunnel
            rules << "block out quick on " + interfaceToSkip + " inet from any to 192.168.0.0/16";
            rules << "block in quick on " + interfaceToSkip + " inet from 192.168.0.0/16 to any";
            rules << "block out quick on " + interfaceToSkip + " inet from any to 172.16.0.0/12";
            rules << "block in quick on " + interfaceToSkip + " inet from 172.16.0.0/12 to any";
            rules << "block out quick on " + interfaceToSkip + " inet from any to 169.254.0.0/16";
            rules << "block in quick on " + interfaceToSkip + " inet from 169.254.0.0/16 to any";
            // Allow reserved subnet
            rules << "pass out quick on " + interfaceToSkip + " inet from any to 10.255.255.0/24";
            rules << "pass in quick on " + interfaceToSkip + " inet from 10.255.255.0/24 to any";
            // Disallow RFC1918/link local/loopback traffic to go over tunnel (cont'd)
            rules << "block out quick on " + interfaceToSkip + " inet from any to 10.0.0.0/8";
            rules << "block in quick on " + interfaceToSkip + " inet from 10.0.0.0/8 to any";
            rules << "block out quick on " + interfaceToSkip + " inet from any to 127.0.0.0/8";
            rules << "block in quick on " + interfaceToSkip + " inet from 224.0.0.0/24 to any";
        }

        // Allow other traffic on VPN interface
        rules << "pass out quick on " + interfaceToSkip + " inet from any to any";
        rules << "pass in quick on " + interfaceToSkip + " inet from any to any";
    }

    if (!connectingIp.isEmpty()) {
        // only allow gid 0 and windscribe group to send plaintext to VPN server
        rules << "pass out quick inet from any to " + connectingIp + " group 0";
        rules << "pass out quick inet from any to " + connectingIp + " group windscribe";
        rules << "pass in quick inet from " + connectingIp + " to any";
    }
    return rules;
}

void FirewallController_mac::updateVpnAnchor() {
    Anchor vpnTrafficAnchor("windscribe_vpn_traffic");
    vpnTrafficAnchor.addRules(vpnTrafficRules(connectingIp_, interfaceToSkip_, isCustomConfig_));

    QString anchorConf = vpnTrafficAnchor.generateConf();
    helper_->setFirewallRules(kIpv4, "", "windscribe_vpn_traffic", anchorConf);
}

QStringList FirewallController_mac::getLocalAddresses(const QString iface) const
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

void FirewallController_mac::setPfWasEnabledState(bool b)
{
    QSettings settings;
    settings.setValue("pfIsEnabled", b);
}

bool FirewallController_mac::isPfWasEnabled() const
{
    QSettings settings;
    return settings.value("pfIsEnabled").toBool();
}
