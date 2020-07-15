#pragma once

#include <QString>
#include <QList>
#include "IPC/generated_proto/types.pb.h"

namespace MacUtils
{
    void activateApp();
    void invalidateShadow(void *pNSView);
    void getOSVersionAndBuild(QString &osVersion, QString &build);
    QString getOsVersion();

    bool isOsVersion10_11_or_greater();
    bool isOsVersionIsSierra_or_greater();
    bool isOsVersionIsCatalina_or_greater();

    void hideDockIcon();
    void showDockIcon();
    void setHandCursor();
    void setArrowCursor();

    std::string execCmd(const char* cmd);

    // CLI
    bool reportGuiEngineInit();
    bool isGuiAlreadyRunning();
    bool giveFocusToGui();
    bool showGui();
    void openGuiLocations();

    // Split Routing
    QString iconPathFromBinPath(const QString &binPath);
    QList<QString> enumerateInstalledPrograms();
    QList<QString> listedCmdResults(const QString &cmd);

    // Networking
    bool isActiveEnInterface(); // still used?
    QString ipAddressByInterfaceName(const QString &interfaceName);
    void getDefaultRoute(QString &outGatewayIp, QString &outInterfaceName);
    QString getDefaultGatewayForPrimaryInterface();
    QString lastConnectedNetworkInterfaceName();

    QString ssidOfInterface(QString networkInterface);
    QString macAddressFromIP(QString ipAddr, QString interfaceName);
    QString macAddressFromInterfaceName(QString interfaceName);
    QString trueMacAddress(const QString &interfaceName);
    QString currentNetworkHwInterfaceName();

    bool isWifiAdapter(const QString &networkInterface);
    bool isAdapterUp(const QString &networkInterfaceName);
    bool isAdapterActive(const QString &networkInterface);

    QList<QString> currentNetworkHwInterfaces();
    QMap<QString, int> currentHardwareInterfaceIndexes();
    const ProtoTypes::NetworkInterface currentNetworkInterface();
    ProtoTypes::NetworkInterfaces currentNetworkInterfaces(bool includeNoInterface);
    ProtoTypes::NetworkInterfaces currentlyActiveNetworkInterfaces(bool includeNoInterface);
    ProtoTypes::NetworkInterfaces currentlyUpNetInterfaces();
    ProtoTypes::NetworkInterfaces currentSpoofedInterfaces();
    ProtoTypes::NetworkInterfaces currentWifiInterfaces();
    ProtoTypes::NetworkInterfaces currentlyUpWifiInterfaces();

    bool interfaceSpoofed(const QString &interfaceName);

    bool pingWithMtu(int mtu);

    // read DNS-servers for device name (now used for ipsec adapters for ikev2)
    // implemented with "scutil --dns" command
    // TODO: do not implement via an external util, as the output format of util may change in OS
    QStringList getDnsServersForInterface(const QString &interfaceName);
}




