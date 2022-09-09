#include "firewallcontroller_mac.h"
#include <QStandardPaths>
#include "utils/ws_assert.h"
#include "utils/logger.h"
#include <QDir>
#include <QCoreApplication>

namespace {

class Anchor
{
public:
    Anchor(const QString &name) : name_(name) {}
    void addRule(const QString &rule)
    {
        rules_ << rule;
    }

    void addRules(const QStringList &rules)
    {
        rules_ << rules;
    }

    QString getString() const
    {
        if (rules_.isEmpty())
        {
            return "anchor " + name_ + " all";
        }
        else
        {
            QString str = "anchor " + name_ + " all {\n";
            for (const auto &r : rules_)
            {
                str += r + "\n";
            }
            str += "}";
            return str;
        }
    }

    bool generateFile(QTemporaryFile &tempFile)
    {
        if (tempFile.open())
        {
            QString str;
            for (const auto &r : rules_)
            {
                str += r + "\n";
            }

            tempFile.resize(0);
            QTextStream ts(&tempFile);
            ts << str;
            tempFile.close();
            return true;
        }
        else
        {
            return false;
        }
    }

private:
    QString name_;
    QStringList rules_;
};

} // namespace

FirewallController_mac::FirewallController_mac(QObject *parent, IHelper *helper) :
    FirewallController(parent), isFirewallEnabled_(false), isAllowLanTraffic_(false)
{
    helper_ = dynamic_cast<Helper_mac *>(helper);

    FirewallState firewallState;
    getFirewallStateFromPfctl(firewallState);

    if (firewallState.isEnabled && !firewallState.isBasicWindscribeRulesCorrect)
    {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Warning: the firewall was enabled at the start, but not by the Windscribe program.";
        WS_ASSERT(false);
        firewallOffImpl();
        isFirewallEnabled_ = false;
    }
    else if (firewallState.isEnabled && firewallState.isBasicWindscribeRulesCorrect)
    {
        windscribeIps_ = firewallState.windscribeIps;
        if (windscribeIps_.isEmpty())
        {
            qCDebug(LOG_FIREWALL_CONTROLLER) << "Warning: the firewall was enabled at the start, but windscribe_ips table not found or empty.";
            WS_ASSERT(false);
        }
        interfaceToSkip_ = firewallState.interfaceToSkip;
        isAllowLanTraffic_ = firewallState.isAllowLanTraffic;
        isFirewallEnabled_ = true;
    }
    else
    {
        isFirewallEnabled_ = false;
    }
}

bool FirewallController_mac::firewallOn(const QSet<QString> &ips, bool bAllowLanTraffic)
{
    QMutexLocker locker(&mutex_);

    if (!checkInternalVsPfctlState())
    {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Fatal error: firewall internal state not equal firewall state from pfctl";
    }


    if (!isFirewallEnabled_)
    {
        QString pfConfigFilePath = generatePfConfFile(ips, bAllowLanTraffic, interfaceToSkip_);
        if (!pfConfigFilePath.isEmpty())
        {
            helper_->executeRootCommand("pfctl -v -F all -f \"" + pfConfigFilePath + "\"");
            helper_->executeRootCommand("pfctl -e");
            windscribeIps_ = ips;
            isAllowLanTraffic_ = bAllowLanTraffic;
            isFirewallEnabled_ = true;
        }
        else
        {
            qCDebug(LOG_FIREWALL_CONTROLLER) << "Fatal error: can't create file" << pfConfigFilePath;
        }
    }
    else
    {
        if (ips != windscribeIps_)
        {
            if (generateTableFile(tempFile_, ips))
            {
                helper_->executeRootCommand("pfctl -T load -f \"" + tempFile_.fileName() + "\"");
                windscribeIps_ = ips;
            }
            else
            {
                qCDebug(LOG_FIREWALL_CONTROLLER) << "Fatal error: can't create file" << tempFile_.fileName();
            }
        }

        if (bAllowLanTraffic != isAllowLanTraffic_)
        {
            Anchor anchor("windscribe_lan_traffic");
            if (bAllowLanTraffic)
            {
                anchor.addRules(lanTrafficRules());
            }

            if (anchor.generateFile(tempFile_))
            {
                helper_->executeRootCommand("pfctl -a windscribe_lan_traffic -f \"" + tempFile_.fileName() + "\"");
                isAllowLanTraffic_ = bAllowLanTraffic;
                // kill states of current connections
                helper_->executeRootCommand("pfctl -k 0.0.0.0/0");
            }
            else
            {
                qCDebug(LOG_FIREWALL_CONTROLLER) << "Fatal error: can't create file" << tempFile_.fileName();
            }
        }
    }

    return true;
}

bool FirewallController_mac::firewallOff()
{
    QMutexLocker locker(&mutex_);

    if (!checkInternalVsPfctlState())
    {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Fatal error: firewall internal state not equal firewall state from pfctl";
    }
    firewallOffImpl();
    isFirewallEnabled_ = false;
    windscribeIps_.clear();
    return true;
}

bool FirewallController_mac::firewallActualState()
{
    QMutexLocker locker(&mutex_);

    if (!checkInternalVsPfctlState())
    {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Fatal error: firewall internal state not equal firewall state from pfctl";
    }

    return isFirewallEnabled_;
}

bool FirewallController_mac::whitelistPorts(const apiinfo::StaticIpPortsVector &ports)
{
    QMutexLocker locker(&mutex_);
    if (!checkInternalVsPfctlState())
    {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Fatal error: firewall internal state not equal firewall state from pfctl";
    }

    if (isFirewallEnabled_)
    {
        if (staticIpPorts_ != ports)
        {
            Anchor portsAnchor("windscribe_static_ports_traffic");
            if (!ports.isEmpty())
            {
                for (unsigned int port : ports)
                {
                    portsAnchor.addRule("pass in quick proto tcp from any to any port = " + QString::number(port));
                }
            }

            if (portsAnchor.generateFile(tempFile_))
            {
                helper_->executeRootCommand("pfctl -a windscribe_static_ports_traffic -f \"" + tempFile_.fileName() + "\"");
                staticIpPorts_ = ports;
            }
            else
            {
                qCDebug(LOG_FIREWALL_CONTROLLER) << "Fatal error: can't create file" << tempFile_.fileName();
            }
        }
    }
    else
    {
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
    QString str = helper_->executeRootCommand("pfctl -v -F all -f /etc/pf.conf");
    qCDebug(LOG_FIREWALL_CONTROLLER) << "Output from pfctl -v -F all -f /etc/pf.conf command: " << str;
    str = helper_->executeRootCommand("pfctl -d");
    qCDebug(LOG_FIREWALL_CONTROLLER) << "Output from disable firewall command: " << str;

    // force delete the table "windscribe_ips", seems no need, because above -F all parameter does this work
    //str = helper_->executeRootCommand("pfctl -t windscribe_ips -T kill");

    // flush anchors
    str = helper_->executeRootCommand("pfctl -a windscribe_vpn_traffic -F all");
    str = helper_->executeRootCommand("pfctl -a windscribe_lan_traffic -F all");
    str = helper_->executeRootCommand("pfctl -a windscribe_static_ports_traffic -F all");

    str = helper_->executeRootCommand("pfctl -si");
    qCDebug(LOG_FIREWALL_CONTROLLER) << "Output from status firewall command: " << str;

    qCDebug(LOG_FIREWALL_CONTROLLER) << "firewallOff disabled";
}


QStringList FirewallController_mac::lanTrafficRules() const
{
    QStringList rules;
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

    // Loopback addresses to the local host
    rules << "pass in quick inet from 127.0.0.0/8 to any";

    // Multicast addresses
    rules << "pass in quick inet from 224.0.0.0/4 to any";

    // Allow AirDrop
    rules << "pass in quick on awdl0 inet6 proto udp from any to any port = 5353";
    rules << "pass out quick on awdl0 proto tcp all flags any";

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
    return rules;
}

void FirewallController_mac::getFirewallStateFromPfctl(FirewallState &outState)
{
    // checking enabled/disabled firewall
    QString output = helper_->executeRootCommand("pfctl -si");
    outState.isEnabled = output.indexOf("Status: Enabled") != -1;

    // checking a few key rules to make sure the firewall rules is settled by our program
    output = helper_->executeRootCommand("pfctl -s rules");
    if (output.indexOf("anchor \"windscribe_vpn_traffic\" all") != -1 &&
        output.indexOf("anchor \"windscribe_lan_traffic\" all") != -1 &&
        output.indexOf("anchor \"windscribe_static_ports_traffic\" all") != -1)
    {
        outState.isBasicWindscribeRulesCorrect = true;
    }
    else
    {
        outState.isBasicWindscribeRulesCorrect = false;
    }

    // get windscribe_ips table IPs
    outState.windscribeIps.clear();
    output = helper_->executeRootCommand("pfctl -t windscribe_ips -T show");
    if (!output.isEmpty())
    {
        const QStringList list = output.split("\n");
        for (auto &ip : list)
        {
            if (!ip.trimmed().isEmpty())
            {
                outState.windscribeIps << ip.trimmed();
            }
        }
    }

    // read anchor windscribe_vpn_traffic rules
    outState.interfaceToSkip.clear();
    output = helper_->executeRootCommand("pfctl -a windscribe_vpn_traffic -s rules").trimmed();
    if (!output.isEmpty())
    {
        QStringList rules = output.split("\n");
        WS_ASSERT(rules.size() == 2);
        if (rules.size() > 0)
        {
            QStringList words = rules[0].split(" ");
            if (words.size() >= 10)
            {
                outState.interfaceToSkip = words[4];
            }
            else
            {
                WS_ASSERT(false);
            }
        }
    }
    // read anchor windscribe_lan_traffic rules
    outState.isAllowLanTraffic = false;
    output = helper_->executeRootCommand("pfctl -a windscribe_lan_traffic -s rules").trimmed();
    if (!output.isEmpty())
    {
        outState.isAllowLanTraffic = true;
    }

    // read anchor windscribe_static_ports_traffic rules
    outState.isStaticIpPortsEmpty = true;
    output = helper_->executeRootCommand("pfctl -a windscribe_static_ports_traffic -s rules").trimmed();
    if (!output.isEmpty())
    {
        outState.isStaticIpPortsEmpty = false;
    }
}

bool FirewallController_mac::checkInternalVsPfctlState()
{
    FirewallState firewallState;
    getFirewallStateFromPfctl(firewallState);


    if (firewallState.isEnabled)
    {
        if (!firewallState.isBasicWindscribeRulesCorrect)
        {
            return false;
        }

        if (!isFirewallEnabled_)
        {
            return false;
        }

        if (windscribeIps_ != firewallState.windscribeIps)
        {
            return false;
        }

        if (interfaceToSkip_ != firewallState.interfaceToSkip)
        {
            return false;
        }
        if (isAllowLanTraffic_ != firewallState.isAllowLanTraffic)
        {
            return false;
        }
        if (staticIpPorts_.isEmpty() && !firewallState.isStaticIpPortsEmpty)
        {
            return false;
        }
        if (!staticIpPorts_.isEmpty() && firewallState.isStaticIpPortsEmpty)
        {
            return false;
        }
    }
    else
    {
        if (isFirewallEnabled_)
        {
             return false;
        }
    }
    return true;
}

QString FirewallController_mac::generatePfConfFile(const QSet<QString> &ips, bool bAllowLanTraffic, const QString &interfaceToSkip)
{
    QString pfConfigFilePath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir dir(pfConfigFilePath);
    dir.mkpath(pfConfigFilePath);
    pfConfigFilePath += "/pf.conf";

    QString pf = "";
    pf += "# Automatically generated by Windscribe. Any manual change will be overridden.\n";
    pf += "set block-policy return\n";
    pf += "set skip on { lo0 }\n";
    pf += "scrub in all fragment reassemble\n";
    pf += "block all\n";
    pf += "table <windscribe_ips> persist {";
    for (auto &ip : ips)
    {
        pf += ip + " ";
    }
    pf += "}\n";

    pf += "pass out quick inet from any to <windscribe_ips> \n";

    // allow traffic on VPN interface
    Anchor vpnTrafficAnchor("windscribe_vpn_traffic");
    if (!interfaceToSkip.isEmpty())
    {
        vpnTrafficAnchor.addRule("pass out quick on " + interfaceToSkip + " inet from any to any");
        vpnTrafficAnchor.addRule("pass in quick on " + interfaceToSkip + " inet from any to any");
    }
    pf += vpnTrafficAnchor.getString() + "\n";

    // Allow Dynamic Host Configuration Protocol (DHCP)
    pf += "pass out quick inet proto udp from 0.0.0.0 to 255.255.255.255 port = 67\n";
    pf += "pass in quick proto udp from any to any port = 68\n";

    Anchor lanTrafficAnchor("windscribe_lan_traffic");
    if (bAllowLanTraffic)
    {
        lanTrafficAnchor.addRules(lanTrafficRules());
    }
    pf += lanTrafficAnchor.getString() + "\n";

    Anchor portsAnchor("windscribe_static_ports_traffic");
    if (!staticIpPorts_.isEmpty())
    {
        for (unsigned int port : staticIpPorts_)
        {
            portsAnchor.addRule("pass in quick proto tcp from any to any port = " + QString::number(port));
        }
    }
    pf += portsAnchor.getString() + "\n";

    QFile f(pfConfigFilePath);
    if (f.open(QIODevice::WriteOnly))
    {
        QTextStream ts(&f);
        ts << pf;
        f.close();

        return pfConfigFilePath;
    }
    else
    {
        return QString();
    }
}

bool FirewallController_mac::generateTableFile(QTemporaryFile &tempFile, const QSet<QString> &ips)
{
    QString pf = "table <windscribe_ips> persist {";
    for (auto &ip : ips)
    {
        pf += ip + " ";
    }
    pf += "}\n";

    if (tempFile.open())
    {
        tempFile.resize(0);
        QTextStream ts(&tempFile);
        ts << pf;
        tempFile.close();
        return true;
    }
    else
    {
        return false;
    }
}

void FirewallController_mac::setInterfaceToSkip_posix(const QString &interfaceToSkip)
{
    QMutexLocker locker(&mutex_);
    qCDebug(LOG_BASIC) << "FirewallController_mac::setInterfaceToSkip_posix ->" << interfaceToSkip;
    if (!checkInternalVsPfctlState())
    {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Fatal error: firewall internal state not equal firewall state from pfctl";
    }

    if (isFirewallEnabled_)
    {
        if (interfaceToSkip_ != interfaceToSkip)
        {
            Anchor vpnTrafficAnchor("windscribe_vpn_traffic");
            if (!interfaceToSkip.isEmpty())
            {
                vpnTrafficAnchor.addRule("pass out quick on " + interfaceToSkip + " inet from any to any");
                vpnTrafficAnchor.addRule("pass in quick on " + interfaceToSkip + " inet from any to any");
            }

            if (vpnTrafficAnchor.generateFile(tempFile_))
            {
                helper_->executeRootCommand("pfctl -a windscribe_vpn_traffic -f \"" + tempFile_.fileName() + "\"");
                interfaceToSkip_ = interfaceToSkip;
            }
            else
            {
                qCDebug(LOG_FIREWALL_CONTROLLER) << "Fatal error: can't create file" << tempFile_.fileName();
            }
        }
    }
    else
    {
        interfaceToSkip_ = interfaceToSkip;
    }
}

void FirewallController_mac::enableFirewallOnBoot(bool bEnable)
{
    generatePfConfFile(windscribeIps_, isAllowLanTraffic_, interfaceToSkip_);

    qCDebug(LOG_BASIC) << "Enable firewall on boot, bEnable =" << bEnable;
    QString strTempFilePath = QString::fromLocal8Bit(getenv("TMPDIR")) + "windscribetemp.plist";
    QString filePath = "/Library/LaunchDaemons/com.aaa.windscribe.firewall_on.plist";

    QString pfConfFilePath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QString pfBashScriptFile = pfConfFilePath + "/windscribe_pf.sh";
    pfConfFilePath = pfConfFilePath + "/pf.conf";

    if (bEnable)
    {
        //create bash script
        {
            QString exePath = QCoreApplication::applicationFilePath();
            QFile file(pfBashScriptFile);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                file.resize(0);
                QTextStream in(&file);
                in << "#!/bin/bash\n";
                in << "FILE=\"" << exePath << "\"\n";
                in << "if [ ! -f \"$FILE\" ]\n";
                in << "then\n";
                in << "echo \"File $FILE does not exists\"\n";
                in << "launchctl stop com.aaa.windscribe.firewall_on\n";
                in << "launchctl unload " << filePath << "\n";
                in << "launchctl remove com.aaa.windscribe.firewall_on\n";
                in << "srm \"$0\"\n";
                //in << "rm " << filePath << "\n";
                in << "else\n";
                in << "echo \"File $FILE exists\"\n";
                in << "ipconfig waitall\n";
                in << "/sbin/pfctl -e -f \"" << pfConfFilePath << "\"\n";
                in << "fi\n";
                file.close();

                // set executable flag
                helper_->executeRootCommand("chmod +x \"" + pfBashScriptFile + "\"");
            }
        }

        // create plist
        QFile file(strTempFilePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            file.resize(0);
            QTextStream in(&file);
            in << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
            in << "<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
            in << "<plist version=\"1.0\">\n";
            in << "<dict>\n";
            in << "<key>Label</key>\n";
            in << "<string>com.aaa.windscribe.firewall_on</string>\n";

            in << "<key>ProgramArguments</key>\n";
            in << "<array>\n";
            in << "<string>/bin/bash</string>\n";
            in << "<string>" << pfBashScriptFile << "</string>\n";
            in << "</array>\n";

            in << "<key>StandardErrorPath</key>\n";
            in << "<string>/var/log/windscribe_pf.log</string>\n";
            in << "<key>StandardOutPath</key>\n";
            in << "<string>/var/log/windscribe_pf.log</string>\n";

            in << "<key>RunAtLoad</key>\n";
            in << "<true/>\n";
            in << "</dict>\n";
            in << "</plist>\n";

            file.close();

            helper_->executeRootCommand("cp " + strTempFilePath + " " + filePath);
            helper_->executeRootCommand("launchctl load -w " + filePath);
        }
        else
        {
            qCDebug(LOG_BASIC) << "Can't create plist file for startup firewall: " << filePath;
        }
    }
    else
    {
        qCDebug(LOG_BASIC) << "Execute command: "
                           << "launchctl unload " + Utils::cleanSensitiveInfo(filePath);
        helper_->executeRootCommand("launchctl unload " + filePath);
        qCDebug(LOG_BASIC) << "Execute command: " << "rm " + Utils::cleanSensitiveInfo(filePath);
        helper_->executeRootCommand("rm " + filePath);
        qCDebug(LOG_BASIC) << "Execute command: "
                           << "rm " + Utils::cleanSensitiveInfo(pfBashScriptFile);
        helper_->executeRootCommand("rm \"" + pfBashScriptFile + "\"");
    }
}
