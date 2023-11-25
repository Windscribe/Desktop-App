#pragma once

#include <Windows.h>

#include <QMap>
#include <QString>

#include "networktypes.h"
#include "types/networkinterface.h"

namespace WinUtils
{
    const std::wstring wmActivateGui = L"WindscribeAppActivate";

    bool reboot();
    bool isWindows10orGreater();
    bool isWindows7();
    bool isWindowsVISTAor7or8();
    QString getWinVersionString();
    void getOSVersionAndBuild(QString &osVersion, QString &build);
    QMap<QString,QString> enumerateInstalledProgramIconLocations();
    QStringList enumerateRunningProgramLocations();

    QString executeBlockingCmd(QString cmd, const QString &params, int timeoutMs = -1);

    bool isGuiAlreadyRunning();

    bool isServiceRunning(const QString &serviceName);

    // Registry Adapters
    bool regHasLocalMachineSubkeyProperty(QString keyPath, QString propertyName);
    QString regGetLocalMachineRegistryValueSz(QString keyPath, QString propertyName);
    bool regGetCurrentUserRegistryDword(QString keyPath, QString propertyName, int &dwordValue);

    // Network
    types::NetworkInterface currentNetworkInterface();
    QVector<types::NetworkInterface> currentNetworkInterfaces(bool includeNoInterface);

    types::NetworkInterface interfaceByIndex(int index, bool &success);
    IfTable2Row lowestMetricNonWindscribeIfTableRow();
    IfTableRow ifRowByIndex(int index);
    IfTable2Row ifTable2RowByIndex(int index);

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

    std::optional<bool> haveInternetConnectivity();

    bool authorizeWithUac();
    unsigned long Win32GetErrorString(unsigned long errorCode, wchar_t *buffer, unsigned long bufferSize);

    // Retrieve the version information item specified by itemName (e.g. "FileVersion") from the executable.
    QString getVersionInfoItem(QString exeName, QString itemName);

    GUID stringToGuid(const char *str);

    HWND appMainWindowHandle();
}
