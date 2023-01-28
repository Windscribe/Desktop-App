#include "firewallcontroller_linux.h"
#include <QStandardPaths>
#include "utils/logger.h"
#include "utils/utils.h"
#include <QDir>
#include "engine/helper/ihelper.h"

FirewallController_linux::FirewallController_linux(QObject *parent, IHelper *helper) :
    FirewallController(parent), forceUpdateInterfaceToSkip_(false), mutex_(QRecursiveMutex()),
    comment_("\"Windscribe client rule\"")
{
    helper_ = dynamic_cast<Helper_linux *>(helper);

    pathToTempTable_ = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(pathToTempTable_);
    dir.mkpath(pathToTempTable_);
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


        // remove IPv4 rules
        removeWindscribeRules(comment_, false);

        // remove IPv6 rules
        removeWindscribeRules(comment_, true);

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

    forceUpdateInterfaceToSkip_ = false;

    // rules for IPv4
    {
        QFile file(pathToTempTable_);
        if (file.open(QIODeviceBase::WriteOnly))
        {
            QTextStream stream(&file);

            stream << "*filter\n";
            stream << ":windscribe_input - [0:0]\n";
            stream << ":windscribe_output - [0:0]\n";

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
            QString cmd = "iptables-restore -n < " + pathToTempTable_;
            helper_->executeRootCommand(cmd, &exitCode);
            if (exitCode != 0)
            {
                qCDebug(LOG_FIREWALL_CONTROLLER) << "Unsuccessful exit code:" << exitCode << " for cmd:" << cmd;
            }

            // save current rules to /etc/windscribe directory to make it restorable on OS boot with windscribe-helper
            cmd = "cp " + pathToTempTable_ + " /etc/windscribe/rules.v4";
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
    }

    // rules for IPv6 (disable IPv6)
    {
        QFile file(pathToTempTable_);
        if (file.open(QIODeviceBase::WriteOnly))
        {
            QTextStream stream(&file);

            stream << "*filter\n";
            stream << ":windscribe_input - [0:0]\n";
            stream << ":windscribe_output - [0:0]\n";

            stream << "-A INPUT -j windscribe_input -m comment --comment " + comment_ + "\n";
            stream << "-A OUTPUT -j windscribe_output -m comment --comment " + comment_ + "\n";

            stream << "-A windscribe_input -j DROP -m comment --comment " + comment_ + "\n";
            stream << "-A windscribe_output -j DROP -m comment --comment " + comment_ + "\n";
            stream << "COMMIT\n";

            file.close();


            int exitCode;
            QString cmd = "ip6tables-restore -n < " + pathToTempTable_;
            helper_->executeRootCommand(cmd, &exitCode);
            if (exitCode != 0)
            {
                qCDebug(LOG_FIREWALL_CONTROLLER) << "Unsuccessful exit code:" << exitCode << " for cmd:" << cmd;
            }

            // save current ipv6 rules to /etc/windscribe directory to make it restorable on OS boot with windscribe-helper
            cmd = "cp " + pathToTempTable_ + " /etc/windscribe/rules.v6";
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
    }

    return true;
}

// Extract rules from iptables with comment.If modifyForDelete == true, then replace commands for delete.
QStringList FirewallController_linux::getWindscribeRules(const QString &comment, bool modifyForDelete, bool isIPv6)
{
    QStringList rules;
    int exitCode;
    QString cmd = (isIPv6 ? "ip6tables-save > " : "iptables-save > ") + pathToTempTable_;
    helper_->executeRootCommand(cmd, &exitCode);
    if (exitCode != 0)
    {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Unsuccessful exit code:" << exitCode << " for cmd:" << cmd;
    }
    // Get Windscribe rules
    QFile file(pathToTempTable_);
    if (!file.open(QIODeviceBase::ReadOnly))
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

void FirewallController_linux::removeWindscribeRules(const QString &comment, bool isIPv6)
{
    QString cmd;
    int exitCode;

    QStringList rules = getWindscribeRules(comment, true, isIPv6);

    // delete Windscribe rules, if found
    if (!rules.isEmpty())
    {
        QString curTable;
        for (int ind = 0; ind < rules.count(); ++ind)
        {
            if (rules[ind].startsWith("*"))
            {
                curTable = rules[ind];
            }

            if (rules[ind].contains("COMMIT") && curTable.contains("*filter"))
            {
                rules.insert(ind, "-X windscribe_input");
                rules.insert(ind + 1, "-X windscribe_output");
                break;
            }
        }

        QFile file(pathToTempTable_);
        if (file.open(QIODeviceBase::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&file);
            for (const auto &l : rules)
            {
                out << l << "\n";
            }
            file.close();
        }


        cmd = (isIPv6 ? "ip6tables-restore -n < " : "iptables-restore -n < ") + pathToTempTable_;
        helper_->executeRootCommand(cmd, &exitCode);
        if (exitCode != 0)
        {
            qCDebug(LOG_FIREWALL_CONTROLLER) << "Unsuccessful exit code:" << exitCode << " for cmd:" << cmd;
        }

        QFile::remove(pathToTempTable_);
    }
}
