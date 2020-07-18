#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QStringList>
#include "ipc/generated_proto/types.pb.h"

#define SAFE_DELETE(x) if (x) { delete x; x = NULL; }
#define SAFE_DELETE_LATER(x) if (x) { x->deleteLater(); x = NULL; }

#define CONNECTION_MODE_DEFAULT     0
#define CONNECTION_MODE_BACKUP      1
#define CONNECTION_MODE_STUNNEL     2

namespace Utils {

    QString getOSVersion();
    QString humanReadableByteCount(double bytes, bool isUseSpace, bool isDecimal = false);
    void parseVersionString(const QString &version, int &major, int &minor, bool &bSuccess);
    void getOSVersionAndBuild(QString &osVersion, QString &build);
    //thread safe random number generator from min to max inclusive
    double generateDoubleRandom(const double &min, const double &max);
    int generateIntegerRandom(const int &min, const int &max);
    bool isSubdomainsEqual(const QString &hostname1, const QString &hostname2);
    std::wstring getDirPathFromFullPath(const std::wstring &fullPath);
    QString fileNameFromFullPath(const QString &fullPath);
    QList<ProtoTypes::SplitTunnelingApp> insertionSort(QList<ProtoTypes::SplitTunnelingApp> apps);

    bool isGuiAlreadyRunning();
    bool giveFocusToGui();
    void openGuiLocations();
    bool reportGuiEngineInit();

    QString generateRandomMacAddress();
    QString formatMacAddress(QString macAddress);

    // Network
    ProtoTypes::NetworkInterface noNetworkInterface();
    const ProtoTypes::NetworkInterface currentNetworkInterface();
    const ProtoTypes::NetworkInterfaces currentNetworkInterfaces(bool includeNoInterface);
    bool sameNetworkInterface(const ProtoTypes::NetworkInterface &interface1, const ProtoTypes::NetworkInterface &interface2);
    ProtoTypes::NetworkInterface interfaceByName(const ProtoTypes::NetworkInterfaces &interfaces, const QString &interfaceName);
    const ProtoTypes::NetworkInterfaces interfacesExceptOne(const ProtoTypes::NetworkInterfaces &interfaces, const ProtoTypes::NetworkInterface &exceptInterface);

    bool pingWithMtu(int mtu);
}
#endif // UTILS_H
