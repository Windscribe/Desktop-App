#include "firewallcontroller_mac.h"
#include <QStandardPaths>
#include "utils/logger.h"
#include <QDir>
#include "engine/helper/ihelper.h"

FirewallController_mac::FirewallController_mac(QObject *parent, IHelper *helper) :
    FirewallController(parent, helper), forceUpdateInterfaceToSkip_(false)
{

}

FirewallController_mac::~FirewallController_mac()
{

}

bool FirewallController_mac::firewallOn(const QString &ip, bool bAllowLanTraffic)
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

bool FirewallController_mac::firewallChange(const QString &ip, bool bAllowLanTraffic)
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

bool FirewallController_mac::firewallOff()
{
    QMutexLocker locker(&mutex_);
    FirewallController::firewallOff();
    if (isStateChanged())
    {
        //restore settings
        QString str = helper_->executeRootCommand("pfctl -v -f /etc/pf.conf");
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Output from pfctl -v -f /etc/pf.conf command: " << str;
        str = helper_->executeRootCommand("pfctl -d");
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Output from disable firewall command: " << str;

        str = helper_->executeRootCommand("pfctl -si");
        qCDebug(LOG_FIREWALL_CONTROLLER) << "Output from status firewall command: " << str;

        qCDebug(LOG_FIREWALL_CONTROLLER) << "firewallOff disabled";

        return true;
    }
    else
    {
        return true;
    }
}

bool FirewallController_mac::firewallActualState()
{
    QMutexLocker locker(&mutex_);
    if (!helper_->isHelperConnected())
    {
        return false;
    }

    QString report = helper_->executeRootCommand("pfctl -si");
    if( report.indexOf("Status: Enabled") != -1 )
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool FirewallController_mac::whitelistPorts(const apiinfo::StaticIpPortsVector &ports)
{
    QMutexLocker locker(&mutex_);
    FirewallController::whitelistPorts(ports);
    if (isStateChanged() && latestEnabledState_)
    {
        return firewallOnImpl(latestIp_, latestAllowLanTraffic_, ports);
    }
    else
    {
        return true;
    }
}

bool FirewallController_mac::deleteWhitelistPorts()
{
    return whitelistPorts(apiinfo::StaticIpPortsVector());
}

bool FirewallController_mac::firewallOnImpl(const QString &ip, bool bAllowLanTraffic, const apiinfo::StaticIpPortsVector &ports )
{
    QString pfConfigFilePath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QDir dir(pfConfigFilePath);
    dir.mkpath(pfConfigFilePath);
    pfConfigFilePath += "/pf.conf";
    qCDebug(LOG_BASIC) << pfConfigFilePath;

    forceUpdateInterfaceToSkip_ = false;

    QString pf = "";
    pf += "# Automatically generated by Windscribe. Any manual change will be overridden.\n";
    pf += "# Block policy, RST for quickly notice\n";
    pf += "set block-policy return\n";

    pf += "# Skip interfaces: lo0 and utun (only when connected)\n";
    if (!interfaceToSkip_.isEmpty())
    {
        pf += "set skip on { lo0 " + interfaceToSkip_ + " }\n";
    }
    else
    {
        pf += "set skip on { lo0}\n";
    }

    pf += "# Scrub\n";
    pf += "scrub in all\n"; // 2.9

    pf += "# Drop everything that doesn't match a rule\n";
    pf += "block in all\n";
    pf += "block out all\n";

    QString ips =ip;
    ips = ips.replace(";", " ");
    pf += "table <windscribe_ips> const { " + ips + " }\n";

    pf += "pass out quick inet proto udp from 0.0.0.0 to 255.255.255.255 port = 67\n";
    pf += "pass in quick proto udp from any to any port = 68\n";

    pf += "pass out quick inet from any to <windscribe_ips> flags S/SA keep state\n";
    pf += "pass in quick inet from any to <windscribe_ips> flags S/SA keep state\n";

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
        // Local Network;
        pf += "pass out quick inet from 192.168.0.0/16 to 192.168.0.0/16 flags S/SA keep state\n";
        pf += "pass in quick inet from 192.168.0.0/16 to 192.168.0.0/16 flags S/SA keep state\n";
        pf += "pass out quick inet from 172.16.0.0/12 to 172.16.0.0/12 flags S/SA keep state\n";
        pf += "pass in quick inet from 172.16.0.0/12 to 172.16.0.0/12 flags S/SA keep state\n";
        pf += "pass out quick inet from 10.0.0.0/8 to 10.0.0.0/8 flags S/SA keep state\n";
        pf += "pass in quick inet from 10.0.0.0/8 to 10.0.0.0/8 flags S/SA keep state\n";

        // Loopback addresses to the local host
        pf += "pass in quick inet from 127.0.0.0/8 to 127.0.0.0/8 flags S/SA keep state\n";

        // Multicast addresses
        pf += "pass in quick inet from 224.0.0.0/4 to 224.0.0.0/4 flags S/SA keep state\n";

        // Allow AirDrop
        pf += "pass in quick on awdl0 inet6 proto udp from any to any port = 5353 keep state\n";
        pf += "pass out quick on awdl0 proto tcp all flags any keep state\n";

        // UPnP
        //pf += "pass out quick inet proto udp from 239.255.255.250 to 239.255.255.250 port = 1900\n";
        //pf += "pass in quick inet proto udp from 239.255.255.250 to 239.255.255.250 port = 1900\n";

        //pf += "pass out quick inet proto udp from 239.255.255.250 to 239.255.255.250 port = 1901\n";
        //pf += "pass in quick inet proto udp from 239.255.255.250 to 239.255.255.250 port = 1901\n";

        pf += "pass out quick inet proto udp from any to any port = 1900\n";
        pf += "pass in quick proto udp from any to any port = 1900\n";
        pf += "pass out quick inet proto udp from any to any port = 1901\n";
        pf += "pass in quick proto udp from any to any port = 1901\n";

        pf += "pass out quick inet proto udp from any to any port = 5350\n";
        pf += "pass in quick proto udp from any to any port = 5350\n";
        pf += "pass out quick inet proto udp from any to any port = 5351\n";
        pf += "pass in quick proto udp from any to any port = 5351\n";
        pf += "pass out quick inet proto udp from any to any port = 5353\n";
        pf += "pass in quick proto udp from any to any port = 5353\n";
    }

    QFile f(pfConfigFilePath);
    if (f.open(QIODevice::WriteOnly))
    {
        QTextStream ts(&f);
        ts << pf;
        f.close();


        QString reply = helper_->executeRootCommand("pfctl -v -F all -f \"" + pfConfigFilePath + "\"");
        Q_UNUSED(reply);
        //qCDebug(LOG_BASIC) << reply;

        helper_->executeRootCommand("pfctl -e");

        return true;
    }
    else
    {
        return false;
    }
}

void FirewallController_mac::setInterfaceToSkip_mac(const QString &interfaceToSkip)
{
    QMutexLocker locker(&mutex_);
    qCDebug(LOG_BASIC) << "FirewallController_mac::setInterfaceToSkip_mac ->" << interfaceToSkip;
    if (interfaceToSkip_ != interfaceToSkip) {
        interfaceToSkip_ = interfaceToSkip;
        forceUpdateInterfaceToSkip_ = true;
    }
}
