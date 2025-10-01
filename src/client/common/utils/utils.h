#pragma once

#include <QString>
#include <QStringList>

#include <random>

#include "types/splittunneling.h"

#define SAFE_DELETE(x) if (x) { delete x; x = nullptr; }
#define SAFE_DELETE_LATER(x) if (x) { x->deleteLater(); x = nullptr; }
#define SAFE_CANCEL_AND_DELETE_WSNET_REQUEST(x) if (x) { x->cancel(); x.reset(); }

#define CONNECTION_MODE_DEFAULT     0
#define CONNECTION_MODE_BACKUP      1
#define CONNECTION_MODE_STUNNEL     2

namespace Utils {
    QString getPlatformName();
    // return win, mac or linux
    QString getBasePlatformName();
    QString getPlatformNameSafe();
    QString getOSVersion();
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
    bool isAppAlreadyRunning();

    const QString filenameQuotedSingle(const QString &filename);
    const QString filenameQuotedDouble(const QString &filename);
    const QString filenameEscapeSpaces(const QString &filename);

    bool copyDirectoryRecursive(QString fromDir, QString toDir);
    bool removeDirectory(const QString dir);
    QString toBase64(const QString& str);
    QString fromBase64(const QString& str);

#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
    QString execCmd(const QString &cmd);
#endif

    bool isCLIRunning(int minCount = 0);
}
