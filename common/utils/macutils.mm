#import <Cocoa/Cocoa.h>
#include <AppKit/AppKit.h>

#include "logger.h"
#import "macutils.h"
#include "utils/utils.h"
#include <QFile>
#include <QProcess>
#include <QDir>
#include <google/protobuf/repeated_field.h>
#include <semaphore.h>

#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

void MacUtils::activateApp()
{
    [NSApp activateIgnoringOtherApps:YES];
}

void MacUtils::invalidateShadow(void *pNSView)
{
    NSView *view = (NSView *)pNSView;
    NSWindow *win = [view window];
    [win invalidateShadow];
}

void MacUtils::invalidateCursorRects(void *pNSView)
{
    NSView *view = (NSView *)pNSView;
    NSWindow *win = [view window];
    [win resetCursorRects];
}

void MacUtils::getOSVersionAndBuild(QString &osVersion, QString &build)
{
    osVersion = QString::fromStdString(execCmd("sw_vers -productVersion")).trimmed();
    build = QString::fromStdString(execCmd("sw_vers -buildVersion")).trimmed();
}

QString MacUtils::getOsVersion()
{
    NSProcessInfo *processInfo = [NSProcessInfo processInfo];
    NSString *osVer = processInfo.operatingSystemVersionString;
    return QString::fromCFString((__bridge CFStringRef)osVer);
}

bool MacUtils::isOsVersion10_11_or_greater()
{
    if (floor(NSAppKitVersionNumber) < NSAppKitVersionNumber10_11)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool MacUtils::isOsVersionIsSierra_or_greater()
{
    if (floor(NSAppKitVersionNumber) >= 1500)
    {
        return true;
    }
    else
    {
        return false;
    }
}


void MacUtils::hideDockIcon()
{
    ProcessSerialNumber psn = { 0, kCurrentProcess };
    TransformProcessType(&psn, kProcessTransformToUIElementApplication);
}

void MacUtils::showDockIcon()
{
    ProcessSerialNumber psn = { 0, kCurrentProcess };
    TransformProcessType(&psn, kProcessTransformToForegroundApplication);
}

void MacUtils::setHandCursor()
{
    [[NSCursor pointingHandCursor] set];
}

void MacUtils::setArrowCursor()
{
    [[NSCursor arrowCursor] set];
}

std::string MacUtils::execCmd(const char *cmd)
{
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "";
    while (!feof(pipe))
    {
        if (fgets(buffer, 128, pipe) != NULL)
        {
            result += buffer;
        }
    }
    pclose(pipe);
    return result;
}

bool MacUtils::reportGuiEngineInit()
{
    sem_t *sem = sem_open("WindscribeGuiStarted", 0);
    if (sem != SEM_FAILED)
    {
        sem_post(sem);
        sem_close(sem);
        return true;
    }

    return false;
}

bool MacUtils::isGuiAlreadyRunning()
{
    // Look for process containing "Windscribe" -- exclude grep and Engine
    QString cmd = "ps axco command | grep Windscribe | grep -v grep | grep -v WindscribeEngine | grep -v windscribe-cli";
    QString response = QString::fromStdString(execCmd(cmd.toStdString().c_str()));
    return response.trimmed() != "";
}

QString MacUtils::iconPathFromBinPath(const QString &binPath)
{
    QString iconPath = "";
    const QString iconFolderPath = binPath + "/Contents/Resources/";
    const QString listIcnsCmd = "ls '" + iconFolderPath + "' | grep .icns";

    QList<QString> icons = MacUtils::listedCmdResults(listIcnsCmd);

    if (icons.length() > 0)
    {
        iconPath = iconFolderPath + icons[0];
    }
    return iconPath;
}

QList<QString> MacUtils::enumerateInstalledPrograms()
{
    QString cmd = "ls -d1 /Applications/* | grep .app";
    return listedCmdResults(cmd);
}

QList<QString> MacUtils::listedCmdResults(const QString &cmd)
{
    QList<QString> result;
    QString response = QString::fromStdString(execCmd(cmd.toStdString().c_str()));

    QList<QString> lines = response.split("\n");
    foreach (const QString &line, lines)
    {
        const QString iName = line.trimmed();
        if (iName != "")
        {
            result.append(iName);
        }
    }

    return result;
}

bool MacUtils::isActiveEnInterface()
{
    struct ifaddrs * interfaces = NULL;
    struct ifaddrs * temp_addr = NULL;

    if( getifaddrs(&interfaces) == 0 )
    {
        //Loop through linked list of interfaces
        temp_addr = interfaces;
        while( temp_addr != NULL )
        {
            if( temp_addr->ifa_addr->sa_family == AF_INET )
            {
                QString tname = temp_addr->ifa_name;
                if( tname.startsWith("en") )
                {
                    return true;
                }
            }

            temp_addr = temp_addr->ifa_next;
        }

        freeifaddrs(interfaces);
    }
    return false;
}

QString MacUtils::getDefaultGatewayForPrimaryInterface()
{
    char strReply[64000];
    strReply[0] = '\0';
    FILE *file = popen("netstat -rn | grep 'default' | awk '{print $2}'", "r");
    if (file)
    {
        char szLine[4096];
        while(fgets(szLine, sizeof(szLine), file) != 0)
        {
            strcat(strReply, szLine);
        }
        pclose(file);
    }

    QString strGateways = QString::fromLocal8Bit(strReply).trimmed();
    QStringList gateways = strGateways.split("\n");


    strReply[0] = '\0';
    file = popen("netstat -rn | grep 'default' | awk '{print $6}'", "r");
    if (file)
    {
        char szLine[4096];
        while(fgets(szLine, sizeof(szLine), file) != 0)
        {
            strcat(strReply, szLine);
        }
        pclose(file);
    }

    QString strInterfaces = QString::fromLocal8Bit(strReply).trimmed();
    QStringList interfaces = strGateways.split("\n");


    strReply[0] = '\0';
    file = popen("echo 'show State:/Network/Global/IPv4' | scutil | grep PrimaryInterface | sed -e 's/.*PrimaryInterface : //'", "r");
    if (file)
    {
        char szLine[4096];
        while(fgets(szLine, sizeof(szLine), file) != 0)
        {
            strcat(strReply, szLine);
        }
        pclose(file);
    }

    QString primaryInterface = QString::fromLocal8Bit(strReply).trimmed();
    if (primaryInterface.isEmpty())
    {
        qCDebug(LOG_BASIC) << "no primary interface";
        return "";
    }
    if (interfaces.isEmpty() || gateways.isEmpty())
    {
        qCDebug(LOG_BASIC) << "error: interfaces.isEmpty() || gateways.isEmpty()";
        return "";
    }
    if (interfaces.count() != gateways.count())
    {
        qCDebug(LOG_BASIC) << "error: interfaces.count() != gateways.count()";
        return "";
    }

    for (int i = 0; i < interfaces.count(); i++)
    {
        if (interfaces[i] == gateways[i])
        {
            return gateways[i];
        }
    }

    return "";
}

QString MacUtils::lastConnectedNetworkInterfaceName()
{
    QString ifname("");

    struct ifaddrs * interfaces = NULL;
    struct ifaddrs * temp_addr = NULL;

    if( getifaddrs(&interfaces) == 0 )
    {
        //Loop through linked list of interfaces
        temp_addr = interfaces;
        while( temp_addr != NULL )
        {
            if( temp_addr->ifa_addr->sa_family == AF_INET )
            {
                QString tname = temp_addr->ifa_name;
                if( tname.startsWith("utun") )
                    ifname = tname;
                else if( tname.startsWith("ipsec") )
                    ifname = tname;
                else if( tname.startsWith("ppp") )
                    ifname = tname;
            }

            temp_addr = temp_addr->ifa_next;
        }

        freeifaddrs(interfaces);
    }
    return ifname;
}


QMap<QString, int> MacUtils::currentHardwareInterfaceIndexes()
{
    QMap<QString, int> result;
    QString cmd = "networksetup -listallhardwareports | grep Device | awk '{print $2}'";
    QString response = QString::fromStdString(execCmd(cmd.toStdString().c_str()));

    QList<QString> lines = response.split("\n");

    int index = 1;
    foreach (const QString &line, lines)
    {
        const QString iName = line.trimmed();
        if (iName != "")
        {
            if (isAdapterUp(iName))
            {
                result.insert(iName, index);
            }
        }
        index++;
    }

    return result;
}

QList<QString> MacUtils::currentNetworkHwInterfaces()
{
    QList<QString> result;
    QString cmd = "networksetup -listnetworkserviceorder | grep Device | sed -e 's/.*Device: //' -e 's/)//'";
    QString response = QString::fromStdString(execCmd(cmd.toStdString().c_str()));

    QList<QString> lines = response.split("\n");

    foreach (const QString &line, lines)
    {
        const QString iName = line.trimmed();
        if (iName != "")
        {
            if (isAdapterUp(iName))
            {
                result.append(iName);
            }
        }
    }

    return result;
}

ProtoTypes::NetworkInterfaces MacUtils::currentNetworkInterfaces(bool includeNoInterface)
{
    ProtoTypes::NetworkInterfaces networkInterfaces;

    // Add "No Interface" selection
    if (includeNoInterface)
    {
        *networkInterfaces.add_networks() = Utils::noNetworkInterface();
    }

    QList<QString> hwInterfaces = currentNetworkHwInterfaces();
    QMap<QString, int> interfaceIndexes = currentHardwareInterfaceIndexes();

    foreach (const QString & interfaceName, hwInterfaces)
    {
        ProtoTypes::NetworkInterface networkInterface;

        int index = 0;
        if (interfaceIndexes.contains(interfaceName)) index = interfaceIndexes[interfaceName];
        networkInterface.set_interface_index(index);
        networkInterface.set_interface_name(interfaceName.toStdString());

        bool wifi = MacUtils::isWifiAdapter(interfaceName);
        QString macAddress = macAddressFromInterfaceName(interfaceName);
        networkInterface.set_physical_address(macAddress.toStdString());

        if (wifi)
        {
            networkInterface.set_interface_type(ProtoTypes::NETWORK_INTERFACE_WIFI);
            QString ssid = ssidOfInterface(interfaceName);
            networkInterface.set_network_or_ssid(ssid.toStdString());
        }
        else // Eth
        {
            networkInterface.set_interface_type(ProtoTypes::NETWORK_INTERFACE_ETH);
            networkInterface.set_network_or_ssid(macAddress.toStdString());
        }

        networkInterface.set_active(isAdapterActive(interfaceName));
        *networkInterfaces.add_networks() = networkInterface;

        // TODO: The following fields should be removeable from ProtoTypes::NetworkInterface:
        // interface_guid
        // state
        // metric
        // dw_type
        // device_name
        // connector_present
        // end_point_interface
        // active ?
    }

    return networkInterfaces;
}

ProtoTypes::NetworkInterfaces MacUtils::currentlyActiveNetworkInterfaces(bool includeNoInterface)
{
    ProtoTypes::NetworkInterfaces interfaces = currentNetworkInterfaces(includeNoInterface);

    ProtoTypes::NetworkInterfaces activeInterfaces;
    if (includeNoInterface)
    {
        *activeInterfaces.add_networks() = Utils::noNetworkInterface();
    }

    for (const ProtoTypes::NetworkInterface &interface : interfaces.networks())
    {
        if (interface.active())
        {
            *activeInterfaces.add_networks() = interface;
        }
    }

    return activeInterfaces;
}

QString MacUtils::ssidOfInterface(QString networkInterface)
{
    QString strReply;

    QString command = "echo 'show State:/Network/Interface/" + networkInterface + "/AirPort' | scutil | grep SSID_STR | sed -e 's/.*SSID_STR : //'";
    FILE *file = popen(command.toStdString().c_str(), "r");
    if (file)
    {
        char szLine[4096];
        while(fgets(szLine, sizeof(szLine), file) != 0)
        {
            strReply += szLine;
        }
        pclose(file);
    }

    strReply = strReply.trimmed();
    networkInterface = strReply;

    return strReply;
}

QString MacUtils::macAddressFromIP(QString ipAddr, QString interfaceName)
{
    QString command  = QString("arp %1").arg(ipAddr);

    QString strReply;
#ifdef Q_OS_MAC

    FILE *file = popen(command.toStdString().c_str(), "r");
    if (file)
    {
        char szLine[4096];
        while(fgets(szLine, sizeof(szLine), file) != 0)
        {
            strReply += szLine;
        }
        pclose(file);
    }


    QList<QString> lines = strReply.split('\n');

    bool foundLineWithInterface = false;
    QString line = "";
    foreach (QString line, lines)
    {
        if (line.contains(interfaceName))
        {
            foundLineWithInterface = true;
            break;
        }
    }

    if (foundLineWithInterface)
    {
        if (strReply.contains("at"))
        {
            int atLength = 2;
            int atIndex = strReply.indexOf("at");
            int onIndex = strReply.indexOf("on", atIndex+atLength);
            int diff = onIndex-(atIndex+atLength);

            strReply = strReply.mid(atIndex+atLength, diff);
            strReply = strReply.trimmed();
        }
    }
#endif

    return strReply;
}

QString MacUtils::macAddressFromInterfaceName(QString interfaceName)
{
    char strReply[64000];
    strReply[0] = '\0';
    QString command = "ifconfig " + interfaceName + " | grep 'ether' | awk '{print $2}'";
    FILE *file = popen(command.toStdString().c_str(), "r");
    if (file)
    {
        char szLine[4096];
        while(fgets(szLine, sizeof(szLine), file) != 0)
        {
            strcat(strReply, szLine);
        }
        pclose(file);
    }
    QString strIP = QString::fromLocal8Bit(strReply).trimmed();
    return strIP;
}

QString MacUtils::trueMacAddress(const QString &interfaceName)
{
    QString cmdGetTrueMAC = "networksetup -getmacaddress " + interfaceName + " | awk '{print $3}'";
    return QString::fromStdString(execCmd(cmdGetTrueMAC.toStdString().c_str())).trimmed();
}

QString MacUtils::currentNetworkHwInterfaceName()
{
    QList<QString> hwInterfaces = currentNetworkHwInterfaces();
    foreach (const QString & interface, hwInterfaces)
    {
        if (isAdapterUp(interface))
        {
            return interface;
        }
    }

    return "";
}

ProtoTypes::NetworkInterfaces MacUtils::currentSpoofedInterfaces()
{
    ProtoTypes::NetworkInterfaces spoofed;
    ProtoTypes::NetworkInterfaces currentInterfaces = MacUtils::currentlyUpNetInterfaces();
    for (const ProtoTypes::NetworkInterface &ii : currentInterfaces.networks())
    {
        const QString &interfaceName = QString::fromStdString(ii.interface_name());

        if (interfaceSpoofed(interfaceName))
        {
            *spoofed.add_networks() = ii;
        }
    }

    return spoofed;
}

bool MacUtils::interfaceSpoofed(const QString &interfaceName)
{
    return trueMacAddress(interfaceName) != macAddressFromInterfaceName(interfaceName);
}

bool MacUtils::isWifiAdapter(const QString &networkInterface)
{
    QString command = "echo 'list' | scutil | grep State:/Network/Interface/" + networkInterface + "/AirPort";
    QString statusResult = QString::fromStdString(execCmd(command.toStdString().c_str())).trimmed();
    return statusResult != "";
}

bool MacUtils::isAdapterUp(const QString &networkInterfaceName)
{
    if (isWifiAdapter(networkInterfaceName))
    {
        QString command = "echo 'show State:/Network/Interface/" + networkInterfaceName + "/AirPort' | scutil | grep 'Power Status' | awk '{print $4}'";
        QString result = QString::fromStdString(execCmd(command.toStdString().c_str())).trimmed();
        return result == "TRUE";
    }
    else
    {
        QString command = "echo 'list' | scutil | grep State:/Network/Interface/" + networkInterfaceName + "/IPv4";
        QString result = QString::fromStdString(execCmd(command.toStdString().c_str())).trimmed();
        return result != "";
    }
}

bool MacUtils::isAdapterActive(const QString &networkInterface)
{
    QString cmdInterfaceStatus = "ifconfig " + networkInterface + " | grep status | awk '{print $2}'";
    QString statusResult = QString::fromStdString(execCmd(cmdInterfaceStatus.toStdString().c_str())).trimmed();
    return statusResult == "active";
}

ProtoTypes::NetworkInterfaces MacUtils::currentlyUpNetInterfaces()
{
    auto isUp = [](const ProtoTypes::NetworkInterface &ni)
    {
        return isAdapterUp(QString::fromStdString(ni.interface_name()));
    };

    ProtoTypes::NetworkInterfaces interfaces = currentNetworkInterfaces(false);
    ProtoTypes::NetworkInterfaces upInterfaces;
    std::copy_if(interfaces.networks().begin(), interfaces.networks().end(),
                 google::protobuf::RepeatedFieldBackInserter(upInterfaces.mutable_networks()), isUp);
    return upInterfaces;
}

ProtoTypes::NetworkInterfaces MacUtils::currentWifiInterfaces()
{
    auto isWifi = [](const ProtoTypes::NetworkInterface &ni)
    {
        return isWifiAdapter(QString::fromStdString(ni.interface_name()));
    };

    ProtoTypes::NetworkInterfaces interfaces = currentNetworkInterfaces(false);
    ProtoTypes::NetworkInterfaces upInterfaces;
    std::copy_if(interfaces.networks().begin(), interfaces.networks().end(),
                 google::protobuf::RepeatedFieldBackInserter(upInterfaces.mutable_networks()), isWifi);
    return upInterfaces;
}

ProtoTypes::NetworkInterfaces MacUtils::currentlyUpWifiInterfaces()
{
    auto isWifiUp = [](const ProtoTypes::NetworkInterface &ni)
    {
        return isAdapterUp(QString::fromStdString(ni.interface_name()));
    };

    ProtoTypes::NetworkInterfaces interfaces = currentWifiInterfaces();
    ProtoTypes::NetworkInterfaces upInterfaces;
    std::copy_if(interfaces.networks().begin(), interfaces.networks().end(),
                 google::protobuf::RepeatedFieldBackInserter(upInterfaces.mutable_networks()), isWifiUp);
    return upInterfaces;
}

NSRunningApplication *guiApplicationByBundleName()
{
    NSRunningApplication *currentApp = [NSRunningApplication currentApplication];
    NSWorkspace * ws = [NSWorkspace sharedWorkspace];
    NSArray * apps = [ws runningApplications];

    NSUInteger count = [apps count];
    for (NSUInteger i = 0; i < count; i++)
    {
        NSRunningApplication *app = [apps objectAtIndex: i];
        QString appBundleId = QString::fromNSString([app bundleIdentifier]);
        if (appBundleId == "com.windscribe.gui.macos") // Windscribe GUI // TODO: will this always be the case?
        {
            if ([app processIdentifier] != [currentApp processIdentifier])
            {
                return app;
            }
        }
    }

    return NULL;
}

bool MacUtils::giveFocusToGui()
{
    NSRunningApplication *guiApp = guiApplicationByBundleName();
    if (guiApp != NULL)
    {
        [guiApp activateWithOptions: NSApplicationActivateIgnoringOtherApps];
        return true;
    }
    return false;
}


bool MacUtils::showGui()
{
    NSRunningApplication *guiApp = guiApplicationByBundleName();
    if (guiApp != NULL)
    {
        [guiApp activateWithOptions: NSApplicationActivateAllWindows];
        [guiApp unhide];
        return true;
    }
    return false;
}

void MacUtils::openGuiLocations()
{
    [[NSDistributedNotificationCenter defaultCenter] postNotificationName:@"WindscribeGuiOpenLocations" object:nil];
}

bool MacUtils::isOsVersionIsCatalina_or_greater()
{
    return floor(NSAppKitVersionNumber) >= 1800;
}

const ProtoTypes::NetworkInterface MacUtils::currentNetworkInterface()
{
    QString ifname = currentNetworkHwInterfaceName();
    ProtoTypes::NetworkInterface networkInterface = Utils::interfaceByName(MacUtils::currentlyUpNetInterfaces(), ifname);
    return networkInterface;
}

bool MacUtils::pingWithMtu(int mtu)
{
    const QString cmd = QString("ping -c 1 -W 1000 -D -s %1 checkip.windscribe.com 2> /dev/null").arg(mtu);
    QString result = QString::fromStdString(MacUtils::execCmd(cmd.toStdString().c_str())).trimmed();

    if (result.contains("icmp_seq="))
    {
        return true;
    }
    return false;
}

QString MacUtils::getLocalIP()
{
    QString result;
    FILE *file = popen("ifconfig "
                       "| grep -E \"([0-9]{1,3}\\.){3}[0-9]{1,3}\" "
                       "| grep -vE \"127\\.([0-9]{1,3}\\.){2}[0-9]{1,3}\" "
                       "| awk '{ print $2 }' | cut -f2 -d: | head -n1", "r");
    if (file) {
        char line[4096] = {};
        while (fgets(line, sizeof(line), file) != 0)
            result.append(QString::fromLocal8Bit(line));
        pclose(file);
    }
    return result.trimmed();
}


// TODO: use system APIs for this rather than relying on flaky and changeable system tools (in terms of their output format)
void MacUtils::getDefaultRoute(QString &outGatewayIp, QString &outInterfaceName)
{
    QString command = "netstat -nr -f inet | sed '1,3 d' | awk 'NR==1 { for (i=1; i<=NF; i++) { f[$i] = i  } } NR>1 && $(f[\"Destination\"])==\"default\" { print $(f[\"Gateway\"]), $(f[\"Netif\"]) ; exit }'";

    QString strReply;
    FILE *file = popen(command.toStdString().c_str(), "r");
    if (file)
    {
        char szLine[4096];
        while(fgets(szLine, sizeof(szLine), file) != 0)
        {
            strReply += szLine;
        }
        pclose(file);
    }
    QStringList result = strReply.split(' ');
    if (result.count() == 2)
    {
        outGatewayIp = result.first().trimmed();
        outInterfaceName = result.last().trimmed();
    }
    else
    {
        Q_ASSERT(false);
    }
}

QString MacUtils::ipAddressByInterfaceName(const QString &interfaceName)
{
    struct ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1)
    {
        return "";
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
        {
            continue;
        }
        int family = ifa->ifa_addr->sa_family;
        QString iname = QString::fromStdString(ifa->ifa_name);

        if (family == AF_INET && iname == interfaceName)
        {
            int s = getnameinfo(ifa->ifa_addr,
                            (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                                  sizeof(struct sockaddr_in6),
                            host, NI_MAXHOST,
                            NULL, 0, NI_NUMERICHOST);
            if (s != 0)
            {
                continue;
            }

            return QString::fromStdString(host);
        }
    }

    freeifaddrs(ifaddr);
    return "";
}

// parse lines, like this:
// resolver #1
//      nameserver[0] : 192.168.1.1
//      if_index : 8 (en0)
QStringList MacUtils::getDnsServersForInterface(const QString &interfaceName)
{
    QString command = "scutil --dns";
    QStringList dnsList;

    FILE *file = popen(command.toStdString().c_str(), "r");
    if (file)
    {
        char szLine[4096];
        while (fgets(szLine, sizeof(szLine), file) != 0)
        {
            QString line = QString::fromStdString(szLine);
            // parse line: "nameserver[0] : 192.168.1.1"
            if (line.contains("nameserver", Qt::CaseInsensitive))
            {
                QStringList strs = line.split(':');
                if (strs.count() == 2)
                {
                    QString dnsIp = strs[1].trimmed();
                    // get next line and parse:
                    // example:
                    //      if_index : 8 (en0)
                    if (fgets(szLine, sizeof(szLine), file) != 0)
                    {
                        QString line2 = QString::fromStdString(szLine);
                        if (line2.contains("if_index", Qt::CaseInsensitive) && line2.contains(interfaceName))
                        {
                            dnsList << dnsIp;
                        }
                    }
                }
            }
        }
        pclose(file);
    }

    dnsList.removeDuplicates();
    return dnsList;
}
