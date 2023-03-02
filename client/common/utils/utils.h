#ifndef UTILS_H
#define UTILS_H

#include <random>
#include <QString>
#include <QStringList>
#include "types/networkinterface.h"
#include "types/splittunneling.h"

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

    template<typename T>
    T randomizeList(const T &list)
    {
        T result = list;
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::shuffle( result.begin(), result.end(), std::default_random_engine(seed));
        return result;
    }

    bool isSubdomainsEqual(const QString &hostname1, const QString &hostname2);
    std::wstring getDirPathFromFullPath(const std::wstring &fullPath);
    QString getDirPathFromFullPath(const QString &fullPath);
    QString fileNameFromFullPath(const QString &fullPath);
    QList<types::SplitTunnelingApp> insertionSort(QList<types::SplitTunnelingApp> apps);
    bool accessibilityPermissions();

    unsigned long getCurrentPid();
    bool isGuiAlreadyRunning();

    QString generateRandomMacAddress();
    QString formatMacAddress(QString macAddress);

    // Network
    types::NetworkInterface noNetworkInterface();
    bool sameNetworkInterface(const types::NetworkInterface &interface1, const types::NetworkInterface &interface2);
    types::NetworkInterface interfaceByName(const QVector<types::NetworkInterface> &interfaces, const QString &interfaceName);
    QVector<types::NetworkInterface> interfacesExceptOne(const QVector<types::NetworkInterface> &interfaces, const types::NetworkInterface &exceptInterface);

    bool pingWithMtu(const QString &url, int mtu);
    QString getLocalIP();

    const QString filenameQuotedSingle(const QString &filename);
    const QString filenameQuotedDouble(const QString &filename);
    const QString filenameEscapeSpaces(const QString &filename);

    bool copyDirectoryRecursive(QString fromDir, QString toDir);
    bool removeDirectory(const QString dir);

#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
    QString execCmd(const QString &cmd);
#endif

}
#endif // UTILS_H
