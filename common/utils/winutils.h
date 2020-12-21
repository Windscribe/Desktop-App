#ifndef WINUTILS_H
#define WINUTILS_H

#include <QString>
#include <QMap>

#include "networktypes.h"

namespace WinUtils
{
    const std::wstring classNameIcon = L"Qt5QWindowIcon";
    const std::wstring wsGuiIcon = L"Windscribe";
    const std::wstring wmActivateGui = L"WindscribeAppActivate";
    const std::wstring wmOpenGuiLocations = L"WindscribeAppOpenLocations";

    bool reboot();
    bool isWindows10orGreater();
    QString getWinVersionString();
    void getOSVersionAndBuild(QString &osVersion, QString &build);
    QMap<QString,QString> enumerateInstalledProgramIconLocations();
    QList<QString> enumerateRunningProgramLocations();

    QString executeBlockingCmd(QString cmd, const QString &params, int timeoutMs = -1);

    bool isGuiAlreadyRunning();
    bool giveFocusToGui();
    void openGuiLocations();
    bool reportGuiEngineInit();

    bool isServiceRunning(const QString &serviceName);

    // Registry Adapters
    bool regHasLocalMachineSubkeyProperty(QString keyPath, QString propertyName);
    QString regGetLocalMachineRegistryValueSz(QString keyPath, QString propertyName);

    // Network
    const ProtoTypes::NetworkInterface currentNetworkInterface();
    ProtoTypes::NetworkInterfaces currentNetworkInterfaces(bool includeNoInterface);

    ProtoTypes::NetworkInterface interfaceByIndex(int index, bool &success);
    IfTableRow lowestMetricNonWindscribeIfTableRow();
    IfTableRow ifRowByIndex(int index);

    QString interfaceSubkeyPath(int interfaceIndex);
    QString interfaceSubkeyName(int interfaceIndex);
    bool interfaceSubkeyHasProperty(int interfaceIndex, QString propertyName);
    QList<QString> interfaceSubkeys(QString keyPath);

    QList<IpForwardRow> getIpForwardTable();
    QList<IfTableRow> getIfTable();
    QList<IfTable2Row> getIfTable2();
    QList<IpAdapter> getIpAdapterTable();
    QList<AdapterAddress> getAdapterAddressesTable();

    QString ssidFromInterfaceGUID(QString interfaceGUID);
    QString networkNameFromInterfaceGUID(QString adapterGUID);
    QList<QString> singleHexChars();

    bool pingWithMtu(const QString &url, int mtu);
    QString getLocalIP();
}


#endif // WINUTILS_H
