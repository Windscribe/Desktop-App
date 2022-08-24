#ifndef WINUTILS_H
#define WINUTILS_H

#include <QString>
#include <QMap>

#include "types/networkinterface.h"
#include "networktypes.h"

namespace WinUtils
{
    const std::wstring classNameIcon = L"Qt631QWindowIcon";
    const std::wstring classNameIconDebug = L"Qt631dQWindowIcon";
    const std::wstring wsGuiIcon = L"Windscribe";
    const std::wstring wmActivateGui = L"WindscribeAppActivate";

    bool reboot();
    bool isWindows10orGreater();
    bool isWindows7();
    bool isWindowsVISTAor7or8();
    QString getWinVersionString();
    void getOSVersionAndBuild(QString &osVersion, QString &build);
    QMap<QString,QString> enumerateInstalledProgramIconLocations();
    QList<QString> enumerateRunningProgramLocations();

    QString executeBlockingCmd(QString cmd, const QString &params, int timeoutMs = -1);

    bool isGuiAlreadyRunning();

    bool isServiceRunning(const QString &serviceName);

    bool is32BitAppRunningOn64Bit();
    QString iconPathFromBinPath(const QString &binPath);

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
    bool isWindows64Bit();
    unsigned long Win32GetErrorString(unsigned long errorCode, wchar_t *buffer, unsigned long bufferSize);
}


#endif // WINUTILS_H
