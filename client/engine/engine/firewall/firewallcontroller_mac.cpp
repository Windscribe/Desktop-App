#include "firewallcontroller_mac.h"

#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <QSettings>

#include "utils/log/categories.h"
#include "utils/macutils.h"
#include "utils/network_utils/network_utils_mac.h"
#include "utils/ws_assert.h"

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

FirewallController_mac::FirewallController_mac(QObject *parent, Helper *helper) :
    FirewallController(parent), helper_(helper), isWindscribeFirewallEnabled_(false), isAllowLanTraffic_(false), isCustomConfig_(false), connectingIp_("")
{
    awdl_p2p_interfaces_ = NetworkUtils_mac::getP2P_AWDL_NetworkInterfaces();

    FirewallState firewallState;
    getFirewallStateFromPfctl(firewallState);

    if (firewallState.isEnabled && !firewallState.isBasicWindscribeRulesCorrect) {
        setPfWasEnabledState(true);
        isWindscribeFirewallEnabled_ = false;
    } else if (firewallState.isEnabled && firewallState.isBasicWindscribeRulesCorrect) {
        windscribeIps_ = firewallState.windscribeIps;
        if (windscribeIps_.isEmpty()) {
            qCWarning(LOG_FIREWALL_CONTROLLER) << "Warning: the firewall was enabled at the start, but windscribe_ips table not found or empty.";
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

void FirewallController_mac::firewallOn(const QString &connectingIp, const QSet<QString> &ips, bool bAllowLanTraffic, bool bIsCustomConfig, bool isVpnConnected)
{
    QMutexLocker locker(&mutex_);
    FirewallState firewallState;

    checkInternalVsPfctlState(&firewallState);

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
            qCCritical(LOG_FIREWALL_CONTROLLER) << "Fatal error: can't set firewall rules";
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
}

void FirewallController_mac::firewallOff()
{
    QMutexLocker locker(&mutex_);

    checkInternalVsPfctlState();
    firewallOffImpl();
    isWindscribeFirewallEnabled_ = false;
    windscribeIps_.clear();
}

bool FirewallController_mac::firewallActualState()
{
    QMutexLocker locker(&mutex_);

    checkInternalVsPfctlState();
    return isWindscribeFirewallEnabled_;
}

bool FirewallController_mac::whitelistPorts(const api_responses::StaticIpPortsVector &ports)
{
    QMutexLocker locker(&mutex_);
    checkInternalVsPfctlState();

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
    return whitelistPorts(api_responses::StaticIpPortsVector());
}

void FirewallController_mac::firewallOffImpl()
{
    helper_->clearFirewallRules(isPfWasEnabled());
    isWindscribeFirewallEnabled_ = false;
    qCInfo(LOG_FIREWALL_CONTROLLER) << "firewallOff disabled";
}


QStringList FirewallController_mac::lanTrafficRules(bool bAllowLanTraffic) const
{
    QStringList rules;

    // Always allow localhost
    rules << "pass out quick inet from any to 127.0.0.0/8";
    rules << "pass in quick inet from 127.0.0.0/8 to any";

    QString action = bAllowLanTraffic ? "pass" : "block";

    // Allow or block local network traffic based on setting.
    rules << action + " out quick inet from any to 192.168.0.0/16";
    rules << action + " in quick inet from 192.168.0.0/16 to any";
    rules << action + " out quick inet from any to 172.16.0.0/12";
    rules << action + " in quick inet from 172.16.0.0/12 to any";
    rules << action + " out quick inet from any to 169.254.0.0/16";
    rules << action + " in quick inet from 169.254.0.0/16 to any";
    rules << "block out quick inet from any to 10.255.255.0/24";
    rules << "block in quick inet from 10.255.255.0/24 to any";
    rules << action + " out quick inet from any to 10.0.0.0/8";
    rules << action + " in quick inet from 10.0.0.0/8 to any";

    // Multicast addresses
    rules << action + " out quick inet from any to 224.0.0.0/4";
    rules << action + " in quick inet from 224.0.0.0/4 to any";

    // UPnP
    rules << action + " out quick inet proto udp from any to any port = 1900";
    rules << action + " in quick proto udp from any to any port = 1900";
    rules << action + " out quick inet proto udp from any to any port = 1901";
    rules << action + " in quick proto udp from any to any port = 1901";

    rules << action + " out quick inet proto udp from any to any port = 5350";
    rules << action + " in quick proto udp from any to any port = 5350";
    rules << action + " out quick inet proto udp from any to any port = 5351";
    rules << action + " in quick proto udp from any to any port = 5351";
    rules << action + " out quick inet proto udp from any to any port = 5353";
    rules << action + " in quick proto udp from any to any port = 5353";

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
}

bool FirewallController_mac::checkInternalVsPfctlState(FirewallState *outFirewallState /*= nullptr*/)
{
    FirewallState firewallState;
    getFirewallStateFromPfctl(firewallState);

    if (outFirewallState)
        *outFirewallState = firewallState;

    if (firewallState.isEnabled && firewallState.isBasicWindscribeRulesCorrect) {
        if (!isWindscribeFirewallEnabled_) {
            WS_ASSERT("Firewall state doesn't match");
            return false;
        }
        if (windscribeIps_ != firewallState.windscribeIps) {
            WS_ASSERT("Windscribe IPs don't match");
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
    pf += "pass quick proto igmp allow-opts\n";

    // always allow esp/gre
    pf += "pass quick proto {esp, gre} from any to any\n";

    // block everything
    pf += "block all\n";

    // add Windscribe rules
    pf += "table <windscribe_ips> persist {";
    for (auto &ip : ips) {
        pf += ip + " ";
    }
    pf += "}\n";

    pf += "pass out quick inet from any to <windscribe_ips> no state\n";
    pf += "pass in quick inet from <windscribe_ips> to any no state\n";

    // this table is filled in by the helper
    pf += "table <windscribe_dns> persist\n";
    // Allow VPN DNS, disllow other DNS
    pf += "pass out quick proto udp from any to <windscribe_dns> port 53\n";
    pf += "pass in quick proto udp from <windscribe_dns> port 53 to any\n";

    pf += "table <disallowed_dns> persist {";
    QSet<QString> disallowedDns = MacUtils::getOsDnsServers();
    for (auto &ip : disallowedDns) {
        pf += ip + " ";
    }
    pf += "}\n";
    pf += "block out quick proto {tcp, udp} from any to <disallowed_dns> port 53\n";
    pf += "block in quick proto {tcp, udp} from <disallowed_dns> port 53 to any\n";

    // LAN traffic rules have precedence over split tunnel rules.  This is because on an inclusive tunnel,
    // whether included apps should be able to access the LAN should by controlled by the "Allow LAN traffic" setting.
    Anchor lanTrafficAnchor("windscribe_lan_traffic");
    lanTrafficAnchor.addRules(lanTrafficRules(bAllowLanTraffic));
    pf += lanTrafficAnchor.getString() + "\n";

    Anchor vpnTrafficAnchor("windscribe_vpn_traffic");
    vpnTrafficAnchor.addRules(vpnTrafficRules(connectingIp, interfaceToSkip, bIsCustomConfig));
    pf += vpnTrafficAnchor.getString() + "\n";

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
    checkInternalVsPfctlState();

    if (isWindscribeFirewallEnabled_) {
        if (interfaceToSkip_ != interfaceToSkip) {
            interfaceToSkip_ = interfaceToSkip;
            updateVpnAnchor();
        }
    } else {
        interfaceToSkip_ = interfaceToSkip;
    }
}

void FirewallController_mac::setFirewallOnBoot(bool bEnable, const QSet<QString> &ipTable, bool isAllowLanTraffic)
{
    helper_->setFirewallOnBoot(bEnable, ipTable, isAllowLanTraffic);
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

        // allow windscribe group to send to any address, for split tunnel extension
        rules << "pass out quick inet from any to any group windscribe flags any";
    }

    if (!connectingIp.isEmpty()) {
        // only allow gid 0 and windscribe group to send plaintext to VPN server
        // NB: set 'flags any' here, otherwise TCP-based VPN connections will drop if the firewall is enabled after the connection
        rules << "pass out quick inet from any to " + connectingIp + " group 0 flags any";
        rules << "pass out quick inet from any to " + connectingIp + " group windscribe flags any";
        rules << "pass in quick inet from " + connectingIp + " to any flags any";
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
