#include "utils.h"
#include <random>
#include <time.h>
#include <thread>
#include <limits>
#include <QDir>
#include "ws_assert.h"
#include "logger.h"

#ifdef Q_OS_WIN
    #include <Windows.h>
    #include "winutils.h"
#elif defined Q_OS_MAC
    #include "macutils.h"
    #include "network_utils/network_utils_mac.h"
    #include <math.h>
    #include <unistd.h>
    #include <ApplicationServices/ApplicationServices.h>
#elif defined Q_OS_LINUX
    #include "linuxutils.h"
    #include <unistd.h>
#endif

using namespace Utils;

QString Utils::getPlatformName()
{
#ifdef Q_OS_WIN
    return "windows";
#elif defined Q_OS_MAC
    return "macos";
#elif defined Q_OS_LINUX
    return LinuxUtils::getLastInstallPlatform();
#endif
}

QString Utils::getOSVersion()
{
#ifdef Q_OS_WIN
    return WinUtils::getWinVersionString();
#elif defined Q_OS_MAC
    return ("OS X " + MacUtils::getOsVersion());
#elif defined Q_OS_LINUX    
    return LinuxUtils::getOsVersionString();
#endif
}

QString Utils::humanReadableByteCount(double bytes, bool isUseSpace, bool isDecimal)
{
    const int kUnit = 1024;
    if (bytes < kUnit)
        return QString::number(static_cast<quint64>(bytes)) + " B";

    const int exp = static_cast<int>(log(bytes) / log(kUnit));
    const QString pre = QString("KMGTPE").at(exp-1);
    double fvalue = bytes / pow(kUnit, exp);
    fvalue = isDecimal ? (floor(fvalue * 10.0) / 10.0) : floor(fvalue);
    return QString("%1%2%3B").arg(QString::number(fvalue, 'f', isDecimal ? 1 : 0))
                             .arg(isUseSpace ? " " : "").arg(pre);
}

void Utils::parseVersionString(const QString &version, int &major, int &minor, bool &bSuccess)
{
    QStringList strs = version.split(".");
    if (strs.count() == 2)
    {
        bool bOk1, bOk2;
        major = strs[0].toInt(&bOk1);
        minor = strs[1].toInt(&bOk2);
        bSuccess = bOk1 && bOk2;
    }
    else
    {
        bSuccess = false;
    }
}

void Utils::getOSVersionAndBuild(QString &osVersion, QString &build)
{
#ifdef Q_OS_WIN
    WinUtils::getOSVersionAndBuild(osVersion, build);
#elif defined Q_OS_MAC
    MacUtils::getOSVersionAndBuild(osVersion, build);
#elif defined Q_OS_LINUX
    osVersion = LinuxUtils::getLinuxKernelVersion();
    build = "";
#endif
}

#ifdef Q_OS_WIN
    __declspec(thread) char _generator_backing_double[sizeof(std::mt19937)];
    __declspec(thread) std::mt19937* _generator_double;
    __declspec(thread) char _generator_backing_int[sizeof(std::mt19937)];
    __declspec(thread) std::mt19937* _generator_int;
#endif

double Utils::generateDoubleRandom(const double &min, const double &max)
{
    double generatedValue;
    #ifdef Q_OS_WIN
        std::uniform_real_distribution<> distribution(min, std::nextafter(max, DBL_MAX));
    #else
        std::uniform_real_distribution<> distribution(min, std::nextafter( max, std::numeric_limits<double>::max() ));
    #endif

    #ifdef Q_OS_WIN
        static __declspec(thread) bool inited = false;
        if (!inited)
        {
            _generator_double = new(_generator_backing_double) std::mt19937(clock() + std::hash<std::thread::id>()(std::this_thread::get_id()));
            inited = true;
        }
        generatedValue = distribution(*_generator_double);
    #else
        static thread_local std::mt19937* generator = nullptr;
        if (!generator) generator = new std::mt19937(clock() + std::hash<std::thread::id>()(std::this_thread::get_id()));
        generatedValue = distribution(*generator);
    #endif
    if (generatedValue < min) generatedValue = min;
    if (generatedValue > max) generatedValue = max;
    return generatedValue;
}

// generate random intereg in [min, max]
int Utils::generateIntegerRandom(const int &min, const int &max)
{
    std::uniform_int_distribution<int> distribution(min, max);

    #ifdef Q_OS_WIN
        static __declspec(thread) bool inited = false;
        if (!inited)
        {
            _generator_int = new(_generator_backing_int) std::mt19937(clock() + std::hash<std::thread::id>()(std::this_thread::get_id()));
            inited = true;
        }
        return distribution(*_generator_int);
    #else
        static thread_local std::mt19937* generator = nullptr;
        if (!generator) generator = new std::mt19937(clock() + std::hash<std::thread::id>()(std::this_thread::get_id()));
        return distribution(*generator);
    #endif
}

bool Utils::isSubdomainsEqual(const QString &hostname1, const QString &hostname2)
{
    int i1 = hostname1.indexOf('.');
    int i2 = hostname2.indexOf('.');
    if (i1 != -1 && i2 != -1)
    {
        QString sub1 = hostname1.mid(0, i1);
        QString sub2 = hostname2.mid(0, i2);
        return sub1 == sub2;
    }
    else
    {
        return false;
    }
}

std::wstring Utils::getDirPathFromFullPath(const std::wstring &fullPath)
{
    size_t found = fullPath.find_last_of(L"/\\");
    if (found != std::wstring::npos)
    {
        return fullPath.substr(0, found);
    }
    else
    {
        return fullPath;
    }
}

QString Utils::fileNameFromFullPath(const QString &fullPath)
{
    int index = fullPath.lastIndexOf("/"); // TODO: verify this works on MAC
    if (index != -1)
    {
        return fullPath.mid(index+1);
    }
    else
    {
        return fullPath;
    }
}

QList<types::SplitTunnelingApp> Utils::insertionSort(QList<types::SplitTunnelingApp> apps)
{
    QList<types::SplitTunnelingApp> sortedApps;

    for (int i = 0; i < apps.length(); i++)
    {
        int insertIndex = 0;
        for (insertIndex = 0; insertIndex < sortedApps.length(); insertIndex++)
        {
            QString loweredInsertItem = sortedApps[insertIndex].name.toLower();
            if (apps[i].name.toLower() < loweredInsertItem)
            {
                break;
            }
        }
        sortedApps.insert(insertIndex, apps[i]);
    }
    return sortedApps;
}

QString Utils::generateRandomMacAddress()
{
    QString s;

    for (int i = 0; i < 6; i++)
    {
        char buf[256];
        int tp = generateIntegerRandom(0, 255);

        // Lowest bit in first byte must not be 1 ( 0 - Unicast, 1 - multicast )
        // 2nd lowest bit in first byte must be 1 ( 0 - device, 1 - locally administered mac address )
        if ( i == 0)
        {
            tp |= 0x02;
            tp &= 0xFE;
        }

        sprintf(buf, "%s%X", tp < 16 ? "0" : "", tp);
        s += QString::fromStdString(buf);
    }
    return s;
}

QString Utils::formatMacAddress(QString macAddress)
{
    // WS_ASSERT(macAddress.length() == 12);

    QString formattedMac = QString("%1:%2:%3:%4:%5:%6").arg(macAddress.mid(0,2))
            .arg(macAddress.mid(2,2))
            .arg(macAddress.mid(4,2))
            .arg(macAddress.mid(6,2))
            .arg(macAddress.mid(8,2))
            .arg(macAddress.mid(10,2));

    return formattedMac;
}

bool Utils::isGuiAlreadyRunning()
{
#ifdef Q_OS_WIN
    return WinUtils::isGuiAlreadyRunning();
#elif defined Q_OS_MAC
    return MacUtils::isGuiAlreadyRunning();
#elif defined Q_OS_LINUX
    return LinuxUtils::isGuiAlreadyRunning();
#endif
}

bool Utils::sameNetworkInterface(const types::NetworkInterface &interface1, const types::NetworkInterface &interface2)
{
    if (interface1.interfaceIndex != interface2.interfaceIndex)      return false;
    else if (interface1.interfaceName != interface2.interfaceName)   return false;
    else if (interface1.interfaceGuid != interface2.interfaceGuid)   return false;
    else if (interface1.networkOrSsid != interface2.networkOrSsid)   return false;
    else if (interface1.friendlyName != interface2.friendlyName)     return false;
    return true;
}

types::NetworkInterface Utils::noNetworkInterface()
{
    return types::NetworkInterface::noNetworkInterface();
}

types::NetworkInterface Utils::interfaceByName(const QVector<types::NetworkInterface> &interfaces, const QString &interfaceName)
{
    auto sameInterfaceName = [&interfaceName](const types::NetworkInterface &ni)
    {
        return (ni.interfaceName == interfaceName);
    };

    auto it = std::find_if(interfaces.begin(), interfaces.end(), sameInterfaceName);
    if (it == interfaces.end())
    {
        return noNetworkInterface();
    }
    return *it;
}

QVector<types::NetworkInterface> Utils::interfacesExceptOne(const QVector<types::NetworkInterface> &interfaces, const types::NetworkInterface &exceptInterface)
{
    auto differentInterfaceName = [&exceptInterface](const types::NetworkInterface &ni)
    {
        return ni.interfaceName != exceptInterface.interfaceName;
    };

    QVector<types::NetworkInterface> resultInterfaces;
    std::copy_if(interfaces.begin(), interfaces.end(),  std::back_inserter(resultInterfaces), differentInterfaceName);
    return resultInterfaces;
}

bool Utils::pingWithMtu(const QString &url, int mtu)
{
#ifdef Q_OS_WIN
    return WinUtils::pingWithMtu(url, mtu);
#elif defined Q_OS_MAC
    return NetworkUtils_mac::pingWithMtu(url, mtu);
#elif defined Q_OS_LINUX
    //todo linux
    WS_ASSERT(false);
    Q_UNUSED(url);
    Q_UNUSED(mtu);
    return true;
#endif
}

QString Utils::getLocalIP()
{
#ifdef Q_OS_WIN
    return WinUtils::getLocalIP();
#elif defined Q_OS_MAC
    return NetworkUtils_mac::getLocalIP();
#elif defined Q_OS_LINUX
    return LinuxUtils::getLocalIP();
#endif
}

unsigned long Utils::getCurrentPid()
{
#ifdef Q_OS_WIN
    return GetCurrentProcessId();
#elif defined Q_OS_MAC || defined(Q_OS_LINUX)
    return static_cast<unsigned long>(getpid());
#endif
}

const QString Utils::filenameEscapeSpaces(const QString &filename)
{
    QString result("");
    for (QChar c : filename)
    {
        if (c == ' ') result.append('\\');
        result.append(c);
    }
    return result;
}

const QString Utils::filenameQuotedSingle(const QString &filename)
{
    return "'" + filename + "'";
}

const QString Utils::filenameQuotedDouble(const QString &filename)
{
    return "\"" + filename + "\"";
}

bool Utils::copyDirectoryRecursive(QString fromDir, QString toDir)
{
    if (!QFileInfo::exists(fromDir))
    {
        return false;
    }

    QDir dir;
    dir.setPath(fromDir);

    fromDir += QDir::separator();
    toDir += QDir::separator();

    const auto copy_file_list = dir.entryList(QDir::Files);
    for (const QString &copy_file : copy_file_list)
    {
        QString from = fromDir + copy_file;
        QString to = toDir + copy_file;

        if (QFile::exists(to))
        {
            if (QFile::remove(to) == false)
            {
                return false;
            }
        }

        if (QFile::copy(from, to) == false)
        {
            return false;
        }
    }

    const auto copy_dir_list = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &copy_dir : copy_dir_list)
    {
        QString from = fromDir + copy_dir;
        QString to = toDir + copy_dir;

        if (dir.mkpath(to) == false)
        {
            return false;
        }
        if (Utils::copyDirectoryRecursive(from, to) == false)
        {
            return false;
        }
    }
    return true;


}

bool Utils::removeDirectory(const QString dir)
{
    QDir d(dir);
    return d.removeRecursively();
}

bool Utils::accessibilityPermissions()
{
#ifdef Q_OS_MAC
    return AXIsProcessTrusted();
#else
    return true;
#endif
}

QString Utils::getDirPathFromFullPath(const QString &fullPath)
{
    int index = fullPath.lastIndexOf(QDir::separator());

    if (index < 0)
    {
        qCDebug(LOG_BASIC) << "Failed to find index of delimiter";
        WS_ASSERT(false);
        return fullPath;
    }

    // exludes the "/" tail
    return fullPath.mid(0, index);
}

QString Utils::getPlatformNameSafe()
{
    QString platform = getPlatformName();
#ifdef Q_OS_LINUX
    if (platform == "") return LinuxUtils::DEB_PLATFORM_NAME; // default to debian so most of our API calls don't fail if we cannot find the /etc/windscribe/platform file (someone would have to manually delete)
#endif
    return platform;
}

#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
QString Utils::execCmd(const QString &cmd)
{
    char buffer[1024];
    QString result = "";
    FILE* pipe = popen(cmd.toStdString().c_str(), "r");
    if (!pipe) return "";
    while (!feof(pipe))
    {
        if (fgets(buffer, 1024, pipe) != NULL)
        {
            result += buffer;
        }
    }
    pclose(pipe);
    return result;
}
#endif
