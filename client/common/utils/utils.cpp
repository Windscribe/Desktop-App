#include "utils.h"

#include <QDir>

#include <thread>
#include <time.h>

#include "log/categories.h"
#include "ws_assert.h"

#ifdef Q_OS_WIN
    #include <Windows.h>
    #include "winutils.h"
#elif defined Q_OS_MACOS
    #include "macutils.h"
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
    #if defined(_M_ARM64)
    return "windows_arm64";
    #else
    return "windows";
    #endif
#elif defined Q_OS_MACOS
    return "macos";
#elif defined Q_OS_LINUX
    return LinuxUtils::getLastInstallPlatform();
#endif
}

QString Utils::getOSVersion()
{
#ifdef Q_OS_WIN
    return WinUtils::getWinVersionString();
#elif defined Q_OS_MACOS
    return ("MacOS " + MacUtils::getOsVersion());
#elif defined Q_OS_LINUX
    return LinuxUtils::getOsVersionString();
#endif
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
#elif defined Q_OS_MACOS
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

bool Utils::isAppAlreadyRunning()
{
#ifdef Q_OS_WIN
    return WinUtils::isAppAlreadyRunning();
#elif defined Q_OS_MACOS
    return MacUtils::isAppAlreadyRunning();
#elif defined Q_OS_LINUX
    return LinuxUtils::isAppAlreadyRunning();
#endif
}

unsigned long Utils::getCurrentPid()
{
#ifdef Q_OS_WIN
    return GetCurrentProcessId();
#elif defined Q_OS_MACOS || defined(Q_OS_LINUX)
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
    const auto opts = std::filesystem::copy_options::recursive |
                      std::filesystem::copy_options::copy_symlinks;
    std::error_code ec;
    std::filesystem::copy(fromDir.toStdString(), toDir.toStdString(), opts, ec);

    return !ec;
}

bool Utils::removeDirectory(const QString dir)
{
    QDir d(dir);
    return d.removeRecursively();
}

bool Utils::accessibilityPermissions()
{
#ifdef Q_OS_MACOS
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
        qCCritical(LOG_BASIC) << "Failed to find index of delimiter";
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
    // Default to debian so most of our API calls don't fail if we cannot find the /etc/windscribe/platform
    // file (someone would have to manually delete)
    if (platform.isEmpty())
#ifdef __aarch64__
        return LinuxUtils::DEB_PLATFORM_NAME_ARM64;
#else
        return LinuxUtils::DEB_PLATFORM_NAME_X64;
#endif
#endif
    return platform;
}

#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
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

QString Utils::getBasePlatformName()
{
#ifdef Q_OS_WIN
    return "win";
#elif defined Q_OS_MACOS
    return "mac";
#elif defined Q_OS_LINUX
    return "linux";
#else
    error
#endif
}

QString Utils::toBase64(const QString &str)
{
    return QString(str.toUtf8().toBase64());
}

QString Utils::fromBase64(const QString& str)
{
    QByteArray arr = QByteArray::fromBase64(str.toUtf8(), QByteArray::AbortOnBase64DecodingErrors);
    QString out = QString::fromUtf8(arr);

    // Removes control characters
    return out.remove(QRegularExpression("\\p{Cc}"));
}

bool Utils::isCLIRunning(int minCount)
{
#ifdef Q_OS_WIN
    return WinUtils::enumerateProcesses("windscribe-cli.exe").size() > minCount;
#else
    return Utils::execCmd("ps axco command | grep windscribe-cli | grep -v grep | wc -l").trimmed().toInt() > minCount;
#endif
}
