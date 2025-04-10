#include "interfaceutils_mac.h"

#include <QPermission>

#include "utils/macutils.h"
#include "utils/network_utils/network_utils_mac.h"
#include "utils/utils.h"

InterfaceUtils_mac::InterfaceUtils_mac()
{
}

InterfaceUtils_mac::~InterfaceUtils_mac()
{
}

QString ssidOfInterface(const QString &networkInterface)
{
    // In MacOS 15.0, Apple removed the SSID from networksetup -getairportnetwork as well.
    // As of 24A335, even wdutil redacts it, so we must have the locations permission, otherwise we just use "Wi-F"
    if (MacUtils::isOsVersionAtLeast(15, 0)) {
        if (NetworkUtils_mac::isLocationPermissionGranted()) {
            // Use CoreWLAN API to get the SSID
            return NetworkUtils_mac::getWifiSsid(networkInterface);
        } else {
            return "Wi-Fi";
        }
    // In MacOS 14.4, Apple removed the SSID from scutil output, use an alternative method
    } else if (MacUtils::isOsVersionAtLeast(14, 4)) {
        QString command = "networksetup -getairportnetwork " + networkInterface + " | head -n 1 | cut -s -d ':' -f 2";
        QString strReply = Utils::execCmd(command).trimmed();
        if (strReply.length() > 32) {
            // SSIDs are at most 32 octets, so if the string is longer than that, it's probably an error message.
            strReply = "";
        }
        return strReply;
    }
    // Older versions of macOS should use scutil
    QString command = "echo 'show State:/Network/Interface/" + networkInterface + "/AirPort' | scutil | grep SSID_STR | sed -e 's/.*SSID_STR : //'";
    return Utils::execCmd(command).trimmed();
}

bool isAdapterActive(const QString &networkInterface)
{
    QString cmdInterfaceStatus = "ifconfig " + networkInterface + " | grep status | awk '{print $2}'";
    QString statusResult = Utils::execCmd(cmdInterfaceStatus).trimmed();
    return statusResult == "active";
}

QList<QString> currentNetworkHwInterfaces()
{
    QList<QString> result;
    QString cmd = "networksetup -listnetworkserviceorder | grep Device | sed -e 's/.*Device: //' -e 's/)//'";
    QString response = Utils::execCmd(cmd);

    const QList<QString> lines = response.split("\n");

    for (const QString &line : lines) {
        const QString iName = line.trimmed();
        if (iName != "") {
            if (NetworkUtils_mac::isAdapterUp(iName)) {
                result.append(iName);
            }
        }
    }

    return result;
}

QMap<QString, int> currentHardwareInterfaceIndexes()
{
    QMap<QString, int> result;
    QString cmd = "networksetup -listallhardwareports | grep Device | awk '{print $2}'";
    QString response = Utils::execCmd(cmd);

    const QList<QString> lines = response.split("\n");

    int index = 1;
    for (const QString &line : lines)
    {
        const QString iName = line.trimmed();
        if (iName != "")
        {
            result.insert(iName, index);
        }
        index++;
    }

    return result;
}

QVector<types::NetworkInterface> currentlyUpNetInterfaces()
{
    auto isUp = [](const types::NetworkInterface &ni) {
        return NetworkUtils_mac::isAdapterUp(ni.interfaceName);
    };

    QVector<types::NetworkInterface> interfaces = InterfaceUtils_mac::instance().currentNetworkInterfaces(false);
    QVector<types::NetworkInterface> upInterfaces;
    std::copy_if(interfaces.begin(), interfaces.end(), std::back_inserter(upInterfaces), isUp);
    return upInterfaces;
}

const types::NetworkInterface InterfaceUtils_mac::currentNetworkInterface()
{
    QVector<types::NetworkInterface> ni = currentNetworkInterfaces(false);
    if (ni.size() > 0) {
        return ni[0];
    }

    return types::NetworkInterface::noNetworkInterface();
}


QVector<types::NetworkInterface> InterfaceUtils_mac::currentSpoofedInterfaces()
{
    QVector<types::NetworkInterface> spoofed;
    QVector<types::NetworkInterface> currentInterfaces = currentlyUpNetInterfaces();
    for (const types::NetworkInterface &ii : currentInterfaces) {
        const QString &interfaceName = ii.interfaceName;

        if (NetworkUtils_mac::isInterfaceSpoofed(interfaceName)) {
            spoofed << ii;
        }
    }

    return spoofed;
}

QVector<types::NetworkInterface> InterfaceUtils_mac::currentNetworkInterfaces(bool includeNoInterface)
{
    QVector<types::NetworkInterface> networkInterfaces;

    if (includeNoInterface) {
        networkInterfaces << types::NetworkInterface::noNetworkInterface();
    }

    const QList<QString> hwInterfaces = currentNetworkHwInterfaces();
    const QMap<QString, int> interfaceIndexes = currentHardwareInterfaceIndexes();

    for (const QString &interfaceName : hwInterfaces) {
        types::NetworkInterface networkInterface;

        int index = 0;
        if (interfaceIndexes.contains(interfaceName)) index = interfaceIndexes[interfaceName];
        networkInterface.interfaceIndex = index;
        networkInterface.interfaceName = interfaceName;

        bool wifi = NetworkUtils_mac::isWifiAdapter(interfaceName);
        QString macAddress = NetworkUtils_mac::macAddressFromInterfaceName(interfaceName);
        networkInterface.physicalAddress = macAddress;

        // TODO: **JDRM** see if we can get a useful adapter friendlyName (e.g. Belkin USB-C Ethernet)
        // from a macOS API.
        if (wifi) {
            networkInterface.interfaceType = NETWORK_INTERFACE_WIFI;
            QString ssid = ssidOfInterface(interfaceName);
            networkInterface.networkOrSsid = ssid;
            networkInterface.friendlyName = "Wi-Fi";
        }
        else {
            networkInterface.interfaceType = NETWORK_INTERFACE_ETH;
            networkInterface.networkOrSsid = macAddress;
            networkInterface.friendlyName = "Ethernet";
        }

        networkInterface.active = isAdapterActive(interfaceName);
        networkInterfaces << networkInterface;

        // TODO: The following fields should be removeable from types::NetworkInterface:
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
