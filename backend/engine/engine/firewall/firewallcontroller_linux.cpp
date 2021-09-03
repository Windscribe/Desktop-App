#include "firewallcontroller_linux.h"
#include <QStandardPaths>
#include "utils/logger.h"
#include "utils/utils.h"
#include <QDir>
#include "engine/helper/ihelper.h"

FirewallController_linux::FirewallController_linux(QObject *parent, IHelper *helper) :
    FirewallController(parent), forceUpdateInterfaceToSkip_(false), mutex_(QMutex::Recursive)
{
    helper_ = dynamic_cast<Helper_linux *>(helper);

    pathToIp4SavedTable_ = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QDir dir(pathToIp4SavedTable_);
    dir.mkpath(pathToIp4SavedTable_);
    pathToIp6SavedTable_ = pathToIp4SavedTable_;
    pathToOurTable_ = pathToIp4SavedTable_;

    pathToIp4SavedTable_ += "/ip4table_saved.txt";
    pathToIp6SavedTable_ += "/ip6table_saved.txt";
    pathToOurTable_ += "/windscribe_table.txt";
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
    else
    {
        return true;
    }
}

bool FirewallController_linux::firewallChange(const QString &ip, bool bAllowLanTraffic)
{
    QMutexLocker locker(&mutex_);
    FirewallController::firewallChange(ip, bAllowLanTraffic);
    if (isStateChanged())
    {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "firewall changed with ips count:" << countIps(ip);
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

        int exitCode;
        QString cmd = "iptables-restore < " + pathToIp4SavedTable_;
        helper_->executeRootCommand(cmd, &exitCode);
        if (exitCode != 0)
        {
            qCDebug(LOG_FIREWALL_CONTROLLER) << "Unsuccessful exit code:" << exitCode << " for cmd:" << cmd;
        }
        cmd = "ip6tables-save < " + pathToIp6SavedTable_;
        helper_->executeRootCommand(cmd, &exitCode);
        if (exitCode != 0)
        {
            qCDebug(LOG_FIREWALL_CONTROLLER) << "Unsuccessful exit code:" << exitCode << " for cmd:" << cmd;
        }

        cmd = "rm " + pathToIp4SavedTable_;
        helper_->executeRootCommand(cmd);
        cmd = "rm " + pathToIp6SavedTable_;
        helper_->executeRootCommand(cmd);

        QFile::remove(pathToIp4SavedTable_);
        QFile::remove(pathToIp6SavedTable_);
        QFile::remove(pathToOurTable_);

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
    if (!helper_->isHelperConnected())
    {
        return false;
    }

    int exitCode;
    helper_->executeRootCommand("iptables -L windscribe", &exitCode);
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

}

bool FirewallController_linux::firewallOnImpl(const QString &ip, bool bAllowLanTraffic, const apiinfo::StaticIpPortsVector &ports)
{
    // if the firewall is not installed by the program, then save iptables to file in order to restore when will we turn off the firewall
    if (!firewallActualState())
    {
        int exitCode;
        QString cmd = "iptables-save > " + pathToIp4SavedTable_;
        helper_->executeRootCommand(cmd, &exitCode);
        if (exitCode != 0)
        {
            qCDebug(LOG_FIREWALL_CONTROLLER) << "Unsuccessful exit code:" << exitCode << " for cmd:" << cmd;
        }
        cmd = "ip6tables-save > " + pathToIp6SavedTable_;
        helper_->executeRootCommand(cmd, &exitCode);
        if (exitCode != 0)
        {
            qCDebug(LOG_FIREWALL_CONTROLLER) << "Unsuccessful exit code:" << exitCode << " for cmd:" << cmd;
        }
    }

    forceUpdateInterfaceToSkip_ = false;

    QFile file(pathToOurTable_);
    if (file.open(QIODevice::WriteOnly))
    {
        QTextStream stream(&file);

        stream << "*filter\n";
        stream << ":INPUT DROP [0:0]\n";
        stream << ":FORWARD DROP [0:0]\n";
        stream << ":OUTPUT DROP [0:0]\n";
        stream << ":windscribe - [0:0]\n";

        stream << "-A INPUT -i lo -j ACCEPT\n";
        stream << "-A OUTPUT -o lo -j ACCEPT\n";
        stream << "-A FORWARD -i lo -j ACCEPT\n";
        stream << "-A FORWARD -o lo -j ACCEPT\n";

        if (!interfaceToSkip_.isEmpty())
        {
            stream << "-A INPUT -i " + interfaceToSkip_ + " -j ACCEPT\n";
            stream << "-A OUTPUT -o " + interfaceToSkip_ + " -j ACCEPT\n";
            stream << "-A FORWARD -i " + interfaceToSkip_ + " -j ACCEPT\n";
            stream << "-A FORWARD -o " + interfaceToSkip_ + " -j ACCEPT\n";
        }

        const QStringList ips = ip.split(';');
        for (auto &i : ips)
        {
            stream << "-A INPUT -s " + i + "/32 -j ACCEPT\n";
            stream << "-A OUTPUT -d " + i + "/32 -j ACCEPT\n";
        }

        if (bAllowLanTraffic)
        {
            // Local Network
            stream << "-A INPUT -s 192.168.0.0/16 -j ACCEPT\n";
            stream << "-A OUTPUT -d 192.168.0.0/16 -j ACCEPT\n";

            stream << "-A INPUT -s 172.16.0.0/12 -j ACCEPT\n";
            stream << "-A OUTPUT -d 172.16.0.0/12 -j ACCEPT\n";

            stream << "-A INPUT -s 10.0.0.0/8 -j ACCEPT\n";
            stream << "-A OUTPUT -d 10.0.0.0/8 -j ACCEPT\n";

            // Loopback addresses to the local host
            stream << "-A INPUT -s 127.0.0.0/8 -j ACCEPT\n";

            // Multicast addresses
            stream << "-A INPUT -s 224.0.0.0/4 -j ACCEPT\n";
        }

        stream << "-A windscribe -j DROP\n";
        stream << "COMMIT\n";

        file.close();


        int exitCode;
        QString cmd = "iptables-restore < " + pathToOurTable_;
        helper_->executeRootCommand(cmd, &exitCode);
        if (exitCode != 0)
        {
            qCDebug(LOG_FIREWALL_CONTROLLER) << "Unsuccessful exit code:" << exitCode << " for cmd:" << cmd;
        }
        return true;
    }
    else
    {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Can't create file:" << pathToOurTable_;
        return false;
    }

    // disable IPv6
    QStringList cmds;
    cmds << "ip6tables -P INPUT DROP";
    cmds << "ip6tables -P OUTPUT DROP";
    cmds << "ip6tables -P FORWARD DROP";

    for (auto &cmd : cmds)
    {
        int exitCode;
        helper_->executeRootCommand(cmd, &exitCode);
        if (exitCode != 0)
        {
            qCDebug(LOG_FIREWALL_CONTROLLER) << "Unsuccessful exit code:" << exitCode << " for cmd:" << cmd;
        }
    }

    /*int exitCode;
    QString cmd = "iptables-save > /home/aaa/Documents/rules.txt";
    helper_->executeRootCommand(cmd, &exitCode);
    if (exitCode != 0)
    {
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Unsuccessful exit code:" << exitCode << " for cmd:" << cmd;
    }*/

}
