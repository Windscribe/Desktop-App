#include "firewallcontroller_mac.h"
#include <QStandardPaths>
#include "utils/logger.h"
#include <QDir>
#include <QCoreApplication>

FirewallController_mac::FirewallController_mac(QObject *parent, IHelper *helper) :
    FirewallController(parent), forceUpdateInterfaceToSkip_(false), isFirewallEnabled_(false), isAllowLanTraffic_(false)
{
    helper_ = dynamic_cast<Helper_mac *>(helper);

    FirewallState firewallState;
    getFirewallStateFromPfctl(firewallState);

    if (firewallState.isEnabled && !firewallState.isBasicWindscribeRulesCorrect)
    {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Warning: the firewall was enabled at the start, but not by the Windscribe program.";
        Q_ASSERT(false);
        firewallOffImpl();
        isFirewallEnabled_ = false;
    }
    else if (firewallState.isEnabled && firewallState.isBasicWindscribeRulesCorrect)
    {
        windscribeIps_ = firewallState.windscribeIps;
        if (windscribeIps_.isEmpty())
        {
            qCDebug(LOG_FIREWALL_CONTROLLER) << "Warning: the firewall was enabled at the start, but windscribe_ips table not found or empty.";
            Q_ASSERT(false);
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

FirewallController_mac::~FirewallController_mac()
{

}

bool FirewallController_mac::firewallOn(const QSet<QString> &ips, bool bAllowLanTraffic)
{
    QMutexLocker locker(&mutex_);

    if (!isFirewallEnabled_)
    {
        QString pfConfigFilePath = generatePfConfFile(ips, bAllowLanTraffic, interfaceToSkip_);
        if (!pfConfigFilePath.isEmpty())
        {
            helper_->executeRootCommand("pfctl -v -f \"" + pfConfigFilePath + "\"");
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
            QString pfTableFilePath =  generateTableFile(ips);
            if (!pfTableFilePath.isEmpty())
            {
                helper_->executeRootCommand("pfctl -T load -f \"" + pfTableFilePath + "\"");
                windscribeIps_ = ips;
            }
            else
            {
                qCDebug(LOG_FIREWALL_CONTROLLER) << "Fatal error: can't create file" << pfTableFilePath;
            }
        }

        if (bAllowLanTraffic != isAllowLanTraffic_)
        {
            QString filePath =  generateLanTrafficAnchorFile(bAllowLanTraffic);
            if (!filePath.isEmpty())
            {
                helper_->executeRootCommand("pfctl -a windscribe_lan_traffic -f \"" + filePath + "\"");
                isAllowLanTraffic_ = bAllowLanTraffic;
            }
            else
            {
                qCDebug(LOG_FIREWALL_CONTROLLER) << "Fatal error: can't create file" << filePath;
            }
        }
    }

    return true;
}

bool FirewallController_mac::firewallOff()
{
    QMutexLocker locker(&mutex_);

    checkInternalVsPfctlState();
    firewallOffImpl();
    isFirewallEnabled_ = false;
    windscribeIps_.clear();
    return true;
}

bool FirewallController_mac::firewallActualState()
{
    QMutexLocker locker(&mutex_);

    checkInternalVsPfctlState();

    return isFirewallEnabled_;


    /*if (helper_->currentState() != IHelper::STATE_CONNECTED)
    {
        Q_ASSERT(false);
        qCDebug(LOG_FIREWALL_CONTROLLER) << "FATAL error: helper state is not connected";
        return false;
    }

    // Additionally check the table "windscribe_ips", which will indicate that the firewall is enabled by our program.
    QString tableReport = helper_->executeRootCommand("pfctl -t windscribe_ips -T show");
    QString report = helper_->executeRootCommand("pfctl -si");
    if (report.indexOf("Status: Enabled") != -1)
    {
        if (tableReport.isEmpty())
        {
            qCDebug(LOG_FIREWALL_CONTROLLER) << "FATAL error: firewall enabled, but windscribe_ips table doesn't exist. Probably 3rd party software intervention.";
        }

        return true;
    }
    else
    {
        if (!tableReport.isEmpty())
        {
            qCDebug(LOG_FIREWALL_CONTROLLER) << "FATAL error: firewall disabled, but windscribe_ips table exists.";
        }
        return false;
    }*/
}

bool FirewallController_mac::whitelistPorts(const types::StaticIpPortsVector &ports)
{
    /*QMutexLocker locker(&mutex_);
    FirewallController::whitelistPorts(ports);
    if (isStateChanged() && latestEnabledState_)
    {
        return firewallOnImpl(latestIp_, latestAllowLanTraffic_, ports);
    }
    else*/
    {
        return true;
    }
}

bool FirewallController_mac::deleteWhitelistPorts()
{
    return whitelistPorts(types::StaticIpPortsVector());
}

bool FirewallController_mac::firewallOnImpl(const QString &ip, bool bAllowLanTraffic, const types::StaticIpPortsVector &ports )
{
    QString pfConfigFilePath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir dir(pfConfigFilePath);
    dir.mkpath(pfConfigFilePath);
    pfConfigFilePath += "/pf.conf";
    qCDebug(LOG_BASIC) << pfConfigFilePath;

    forceUpdateInterfaceToSkip_ = false;

    QString pf = "";



    /*pf += "set block-policy return\n";


    pf += "set skip on { lo0}\n";


    //if (!interfaceToSkip_.isEmpty())
    {
        //pf += "pass out quick on utun420 inet from any to any\n";
        //pf += "pass in quick on utun420 inet from any to any\n";
    }

    pf += "scrub in all\n"; // 2.9

    // Drop everything that doesn't match a rule
    pf += "block all\n";

    QString ips =ip;
    ips = ips.replace(";", " ");
    pf += "table <windscribe_ips> persist { " + ips + " }\n";

    // allow traffic on VPN interface
    if (!interfaceToSkip_.isEmpty())
    {
        pf += "pass out quick on " + interfaceToSkip_ + " inet from any to any\n";
        pf += "pass in quick on " + interfaceToSkip_ + " inet from any to any\n";
    }

    // Allow Dynamic Host Configuration Protocol (DHCP)
    pf += "pass out quick inet proto udp from 0.0.0.0 to 255.255.255.255 port = 67\n";
    pf += "pass in quick proto udp from any to any port = 68\n";

    pf += "pass out quick inet from any to <windscribe_ips>\n";
    pf += "pass in quick inet from any to <windscribe_ips>\n";

    if (!ports.isEmpty())
    {
        //pass in proto tcp from any to any port 1234
        for (unsigned int port : ports)
        {
            pf += "pass in quick proto tcp from any to any port = " + QString::number(port) + "\n";
        }
    }


    if (bAllowLanTraffic)
    {
        pf += "anchor lan_traffic all {\n";
        const QStringList rules = lanTrafficRules();
        for (auto &r : rules)
        {
            pf += r + "\n";
        }
        pf += "}\n";
    }
    else
    {
        pf += "anchor lan_traffic all\n";
    }*/


    QFile f(pfConfigFilePath);
    if (f.open(QIODevice::WriteOnly))
    {
        QTextStream ts(&f);
        ts << pf;
        f.close();

        // Note:
        // Be careful adding '-F all' to this command to fix an issue.  Adding it will prevent the
        // OpenVPN over TCP and Stealth protocols from completing their connection setup.

        QString reply = helper_->executeRootCommand("pfctl -v -f \"" + pfConfigFilePath + "\"");
        //qCDebug(LOG_FIREWALL_CONTROLLER) << "Firewall on pfctl result:" << reply;
        Q_UNUSED(reply);

        helper_->executeRootCommand("pfctl -e");

        return true;
    }
    else
    {
        return false;
    }
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
        output.indexOf("anchor \"windscribe_lan_traffic\" all") != -1)
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
        Q_ASSERT(rules.size() == 2);
        if (rules.size() > 0)
        {
            QStringList words = rules[0].split(" ");
            if (words.size() >= 10)
            {
                outState.interfaceToSkip = words[4];
            }
            else
            {
                Q_ASSERT(false);
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
}


void FirewallController_mac::checkInternalVsPfctlState()
{

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
    if (!interfaceToSkip.isEmpty())
    {
        pf += "anchor windscribe_vpn_traffic all {\n";
        pf += "pass out quick on " + interfaceToSkip + " inet from any to any\n";
        pf += "pass in quick on " + interfaceToSkip + " inet from any to any\n";
        pf += "}\n";
    }
    else
    {
        pf += "anchor windscribe_vpn_traffic all\n";
    }
    // Allow Dynamic Host Configuration Protocol (DHCP)
    pf += "pass out quick inet proto udp from 0.0.0.0 to 255.255.255.255 port = 67\n";
    pf += "pass in quick proto udp from any to any port = 68\n";

    if (bAllowLanTraffic)
    {
        pf += "anchor windscribe_lan_traffic all {\n";
        const QStringList rules = lanTrafficRules();
        for (auto &r : rules)
        {
            pf += r + "\n";
        }
        pf += "}\n";
    }
    else
    {
        pf += "anchor windscribe_lan_traffic all\n";
    }

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

QString FirewallController_mac::generateTableFile(const QSet<QString> &ips)
{
    QString pfConfigFilePath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir dir(pfConfigFilePath);
    dir.mkpath(pfConfigFilePath);
    pfConfigFilePath += "/windscribe_table.conf";

    QString pf = "";
    pf += "table <windscribe_ips> persist {";
    for (auto &ip : ips)
    {
        pf += ip + " ";
    }
    pf += "}\n";

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

QString FirewallController_mac::generateInterfaceToSkipAnchorFile(const QString &interfaceToSkip)
{
    QString pfConfigFilePath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir dir(pfConfigFilePath);
    dir.mkpath(pfConfigFilePath);
    pfConfigFilePath += "/vpn_skip.conf";

    QString pf = "";
    if (!interfaceToSkip.isEmpty())
    {
        pf += "pass out quick on " + interfaceToSkip + " inet from any to any\n";
        pf += "pass in quick on " + interfaceToSkip + " inet from any to any\n";
    }

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

QString FirewallController_mac::generateLanTrafficAnchorFile(bool bAllowLanTraffic)
{
    QString pfConfigFilePath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir dir(pfConfigFilePath);
    dir.mkpath(pfConfigFilePath);
    pfConfigFilePath += "/lan_anchor.conf";

    QString pf = "";
    if (bAllowLanTraffic)
    {
        const QStringList rules = lanTrafficRules();
        for (auto &r : rules)
        {
            pf += r + "\n";
        }
    }

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


void FirewallController_mac::setInterfaceToSkip_posix(const QString &interfaceToSkip)
{
    QMutexLocker locker(&mutex_);
    qCDebug(LOG_BASIC) << "FirewallController_mac::setInterfaceToSkip_posix ->" << interfaceToSkip;

    if (isFirewallEnabled_)
    {
        if (interfaceToSkip_ != interfaceToSkip)
        {
            QString filePath = generateInterfaceToSkipAnchorFile(interfaceToSkip);
            if (!filePath.isEmpty())
            {
                helper_->executeRootCommand("pfctl -a windscribe_vpn_traffic -f \"" + filePath + "\"");
                interfaceToSkip_ = interfaceToSkip;
            }
            else
            {
                qCDebug(LOG_FIREWALL_CONTROLLER) << "Fatal error: can't create file" << filePath;
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
