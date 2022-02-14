#include "firewallcontroller_linux.h"
#include <QStandardPaths>
#include "utils/logger.h"
#include "utils/utils.h"
#include <QDir>
#include "engine/helper/ihelper.h"

FirewallController_linux::FirewallController_linux(QObject *parent, IHelper *helper) :
    FirewallController(parent), forceUpdateInterfaceToSkip_(false), mutex_(QMutex::Recursive),
    comment_("\"Windscribe client rule\"")
{
    helper_ = dynamic_cast<Helper_linux *>(helper);

    pathToIp6SavedTable_ = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QDir dir(pathToIp6SavedTable_);
    dir.mkpath(pathToIp6SavedTable_);
    pathToTempTable_ = pathToIp6SavedTable_;

    pathToIp6SavedTable_ += "/ip6table_saved.txt";
    pathToTempTable_ += "/windscribe_table.txt";
}

FirewallController_linux::~FirewallController_linux()
{
}

bool FirewallController_linux::firewallOn(const QString &ip, bool bAllowLanTraffic)
{
    QMutexLocker locker(&mutex_);
    FirewallController::firewallOn(ip, bAllowLanTraffic);
    if (isStateChanged())
    {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "firewall enabled with ips count:" << countIps(ip);
        return firewallOnImpl(ip, bAllowLanTraffic, latestStaticIpPorts_);
    }
    else if (forceUpdateInterfaceToSkip_)
    {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "firewall changed due to interface-to-skip update";
        return firewallOnImpl(ip, bAllowLanTraffic, latestStaticIpPorts_);
    }
    else
    {
        return true;
    }
}

bool FirewallController_linux::firewallOff()
{
    QMutexLocker locker(&mutex_);
    FirewallController::firewallOff();
    if (isStateChanged())
    {
        QString cmd;
        int exitCode;
        QStringList rules = getWindscribeRules(comment_, true);

        // delete Windscribe rules, if found
        if (!rules.isEmpty())
        {
            if (rules.last().contains("COMMIT"))
            {
                rules.insert(rules.count() - 1, "-X windscribe_input");
                rules.insert(rules.count() - 1, "-X windscribe_output");
            }

            QFile file2(pathToTempTable_);
            if (file2.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                QTextStream out(&file2);
                for (const auto &l : rules)
                {
                    out << l << "\n";
                }
                file2.close();
            }


            cmd = "iptables-restore -n < " + pathToTempTable_;
            helper_->executeRootCommand(cmd, &exitCode);
            if (exitCode != 0)
            {
                qCDebug(LOG_FIREWALL_CONTROLLER) << "Unsuccessful exit code:" << exitCode << " for cmd:" << cmd;
            }
        }
        QFile::remove(pathToTempTable_);

        if (QFile::exists(pathToIp6SavedTable_))
        {
            cmd = "ip6tables-restore < " + pathToIp6SavedTable_;
            helper_->executeRootCommand(cmd, &exitCode);
            if (exitCode != 0)
            {
                qCDebug(LOG_FIREWALL_CONTROLLER) << "Unsuccessful exit code:" << exitCode << " for cmd:" << cmd;
            }

            QFile::remove(pathToIp6SavedTable_);
        }


        // remove rules from /etc/windscribe directory to avoid enabling them on OS reboot
        cmd = "rm -f /etc/windscribe/rules.v4";
        helper_->executeRootCommand(cmd, &exitCode);
        if (exitCode != 0)
        {
            qCDebug(LOG_FIREWALL_CONTROLLER) << "Unsuccessful exit code:" << exitCode << " for cmd:" << cmd;
        }
        cmd = "rm -f /etc/windscribe/rules.v6";
        helper_->executeRootCommand(cmd, &exitCode);
        if (exitCode != 0)
        {
            qCDebug(LOG_FIREWALL_CONTROLLER) << "Unsuccessful exit code:" << exitCode << " for cmd:" << cmd;
        }

        return true;
    }
    else
    {
        return true;
    }
}

bool FirewallController_linux::firewallActualState()
{
    QMutexLocker locker(&mutex_);
    if (helper_->currentState() != IHelper::STATE_CONNECTED)
    {
        return false;
    }

    int exitCode;
    helper_->executeRootCommand("iptables --check INPUT -j windscribe_input -m comment --comment " + comment_ + " 2>&-", &exitCode);
    return exitCode == 0;
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

void FirewallController_linux::enableFirewallOnBoot(bool bEnable)
{
    Q_UNUSED(bEnable);
    //nothing todo for Linux
}

bool FirewallController_linux::firewallOnImpl(const QString &ip, bool bAllowLanTraffic, const apiinfo::StaticIpPortsVector &ports)
{
    // TODO: this is need for Linux?
    Q_UNUSED(ports);

    // if the firewall is not installed by the program, then save iptables to file in order to restore when will we turn off the firewall
    if (!firewallActualState())
    {
        int exitCode;
        QString cmd = "ip6tables-save > " + pathToIp6SavedTable_;
        helper_->executeRootCommand(cmd, &exitCode);
        if (exitCode != 0)
        {
            qCDebug(LOG_FIREWALL_CONTROLLER) << "Unsuccessful exit code:" << exitCode << " for cmd:" << cmd;
        }
    }

    // get firewall rules, which could have been installed by a script update-resolv-conf/update-systemd-resolved to avoid DNS-leaks
    // if these rules exist, then we should leave(not delete) them.
    const QStringList dnsLeaksRules = getWindscribeRules("\"Windscribe client dns leak protection\"", false);

    forceUpdateInterfaceToSkip_ = false;

    QFile file(pathToTempTable_);
    if (file.open(QIODevice::WriteOnly))
    {
        QTextStream stream(&file);

        stream << "*filter\n";
        stream << ":windscribe_input - [0:0]\n";
        stream << ":windscribe_output - [0:0]\n";

        if (!dnsLeaksRules.isEmpty())
        {
            stream << ":windscribe_dnsleaks - [0:0]\n";
            for (auto &rule : dnsLeaksRules)
            {
                if (rule.startsWith("-A"))
                {
                    stream << rule + "\n";
                }
            }
        }

        stream << "-A INPUT -j windscribe_input -m comment --comment " + comment_ + "\n";
        stream << "-A OUTPUT -j windscribe_output -m comment --comment " + comment_ + "\n";

        stream << "-A windscribe_input -i lo -j ACCEPT -m comment --comment " + comment_ + "\n";
        stream << "-A windscribe_output -o lo -j ACCEPT -m comment --comment " + comment_ + "\n";

        if (!interfaceToSkip_.isEmpty())
        {
            stream << "-A windscribe_input -i " + interfaceToSkip_ + " -j ACCEPT -m comment --comment " + comment_ + "\n";
            stream << "-A windscribe_output -o " + interfaceToSkip_ + " -j ACCEPT -m comment --comment " + comment_ + "\n";
        }

        const QStringList ips = ip.split(';');
        for (auto &i : ips)
        {
            stream << "-A windscribe_input -s " + i + "/32 -j ACCEPT -m comment --comment " + comment_ + "\n";
            stream << "-A windscribe_output -d " + i + "/32 -j ACCEPT -m comment --comment " + comment_ + "\n";
        }

        if (bAllowLanTraffic)
        {
            // Local Network
            stream << "-A windscribe_input -s 192.168.0.0/16 -j ACCEPT -m comment --comment " + comment_ + "\n";
            stream << "-A windscribe_output -d 192.168.0.0/16 -j ACCEPT -m comment --comment " + comment_ + "\n";

            stream << "-A windscribe_input -s 172.16.0.0/12 -j ACCEPT -m comment --comment " + comment_ + "\n";
            stream << "-A windscribe_output -d 172.16.0.0/12 -j ACCEPT -m comment --comment " + comment_ + "\n";

            stream << "-A windscribe_input -s 10.0.0.0/8 -j ACCEPT -m comment --comment " + comment_ + "\n";
            stream << "-A windscribe_output -d 10.0.0.0/8 -j ACCEPT -m comment --comment " + comment_ + "\n";

            // Loopback addresses to the local host
            stream << "-A windscribe_input -s 127.0.0.0/8 -j ACCEPT -m comment --comment " + comment_ + "\n";

            // Multicast addresses
            stream << "-A windscribe_input -s 224.0.0.0/4 -j ACCEPT -m comment --comment " + comment_ + "\n";
        }

        stream << "-A windscribe_input -j DROP -m comment --comment " + comment_ + "\n";
        stream << "-A windscribe_output -j DROP -m comment --comment " + comment_ + "\n";
        stream << "COMMIT\n";

        file.close();


        int exitCode;
        QString cmd = "iptables-restore < " + pathToTempTable_;
        helper_->executeRootCommand(cmd, &exitCode);
        if (exitCode != 0)
        {
            qCDebug(LOG_FIREWALL_CONTROLLER) << "Unsuccessful exit code:" << exitCode << " for cmd:" << cmd;
        }

        file.remove();
    }
    else
    {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Can't create file:" << pathToTempTable_;
        return false;
    }

    // disable IPv6
    QStringList cmds;
    cmds << "ip6tables -P INPUT DROP";
    cmds << "ip6tables -P OUTPUT DROP";
    cmds << "ip6tables -P FORWARD DROP";
    cmds << "ip6tables -Z";
    cmds << "ip6tables -F";
    cmds << "ip6tables -X";

    for (auto &cmd : cmds)
    {
        int exitCode;
        helper_->executeRootCommand(cmd, &exitCode);
        if (exitCode != 0)
        {
            qCDebug(LOG_FIREWALL_CONTROLLER) << "Unsuccessful exit code:" << exitCode << " for cmd:" << cmd;
        }
    }

    // save current rules to /etc/windscribe directory to make it restorable on OS boot with windscribe-helper
    int exitCode;
    QString cmd = "iptables-save > /etc/windscribe/rules.v4";
    helper_->executeRootCommand(cmd, &exitCode);
    if (exitCode != 0)
    {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Unsuccessful exit code:" << exitCode << " for cmd:" << cmd;
    }

    cmd = "ip6tables-save > /etc/windscribe/rules.v6";
    helper_->executeRootCommand(cmd, &exitCode);
    if (exitCode != 0)
    {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Unsuccessful exit code:" << exitCode << " for cmd:" << cmd;
    }

    return true;
}

// Extract rules from iptables with comment.If modifyForDelete == true, then replace commands for delete.
QStringList FirewallController_linux::getWindscribeRules(const QString &comment, bool modifyForDelete)
{
    QStringList rules;
    int exitCode;
    QString cmd = "iptables-save > " + pathToTempTable_;
    helper_->executeRootCommand(cmd, &exitCode);
    if (exitCode != 0)
    {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Unsuccessful exit code:" << exitCode << " for cmd:" << cmd;
    }
    // Get Windscribe rules
    QFile file(pathToTempTable_);
    if (!file.open(QIODevice::ReadOnly))
    {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Can't open file:" << pathToTempTable_;
    }

    QTextStream in(&file);
    bool bFound = false;
    while (!in.atEnd())
    {
        std::string line = in.readLine().toStdString();
        if ((line.rfind("*", 0) == 0) || // string starts with "*"
            (line.find("COMMIT") != std::string::npos) ||
            ((line.rfind("-A", 0) == 0) && (line.find("-m comment --comment " + comment.toStdString()) != std::string::npos)) )
        {
            if (line.rfind("-A", 0) == 0)
            {
                if (modifyForDelete)
                {
                    line[1] = 'D';
                }
                bFound = true;
            }
            rules << QString::fromStdString(line);
        }
    }

    file.close();
    file.remove();
    if (!bFound)
    {
        rules.clear();
    }
    return rules;
}
