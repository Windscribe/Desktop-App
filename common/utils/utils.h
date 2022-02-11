#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QStringList>
#include "protobuf_includes.h"

#define SAFE_DELETE(x) if (x) { delete x; x = nullptr; }
#define SAFE_DELETE_LATER(x) if (x) { x->deleteLater(); x = nullptr; }

#define CONNECTION_MODE_DEFAULT     0
#define CONNECTION_MODE_BACKUP      1
#define CONNECTION_MODE_STUNNEL     2

namespace Utils {
    QString getPlatformName();
    QString getPlatformNameSafe();
    QString getOSVersion();
    QString humanReadableByteCount(double bytes, bool isUseSpace, bool isDecimal = false);
    void parseVersionString(const QString &version, int &major, int &minor, bool &bSuccess);
    void getOSVersionAndBuild(QString &osVersion, QString &build);
    //thread safe random number generator from min to max inclusive
    double generateDoubleRandom(const double &min, const double &max);
    int generateIntegerRandom(const int &min, const int &max);
    bool isSubdomainsEqual(const QString &hostname1, const QString &hostname2);
    std::wstring getDirPathFromFullPath(const std::wstring &fullPath);
    QString getDirPathFromFullPath(const QString &fullPath);
    QString fileNameFromFullPath(const QString &fullPath);
    QList<ProtoTypes::SplitTunnelingApp> insertionSort(QList<ProtoTypes::SplitTunnelingApp> apps);
    bool accessibilityPermissions();

    unsigned long getCurrentPid();
    bool isGuiAlreadyRunning();

    QString generateRandomMacAddress();
    QString formatMacAddress(QString macAddress);

    // Network
    ProtoTypes::NetworkInterface noNetworkInterface();
    const ProtoTypes::NetworkInterface currentNetworkInterface();
    const ProtoTypes::NetworkInterfaces currentNetworkInterfaces(bool includeNoInterface);
    bool sameNetworkInterface(const ProtoTypes::NetworkInterface &interface1, const ProtoTypes::NetworkInterface &interface2);
    ProtoTypes::NetworkInterface interfaceByName(const ProtoTypes::NetworkInterfaces &interfaces, const QString &interfaceName);
    const ProtoTypes::NetworkInterfaces interfacesExceptOne(const ProtoTypes::NetworkInterfaces &interfaces, const ProtoTypes::NetworkInterface &exceptInterface);

    bool pingWithMtu(const QString &url, int mtu);
    QString getLocalIP();

    const QString filenameQuotedSingle(const QString &filename);
    const QString filenameQuotedDouble(const QString &filename);
    const QString filenameEscapeSpaces(const QString &filename);

    bool copyDirectoryRecursive(QString fromDir, QString toDir);
    bool removeDirectory(const QString dir);

    // TODO: remove this function when the gui and engine are merged.
    bool isParentProcessGui();
}
#endif // UTILS_H
