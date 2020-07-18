#include "tapinstall_win.h"
#include <QFile>
#include <QDir>
#include "Utils/logger.h"
#include <QCoreApplication>
#include "Engine/Helper/windscribeinstallhelper_win.h"
#define PSAPI_VERSION 1
#include <psapi.h>
#include <shlwapi.h>
#include <functional>
#include "Utils/utils.h"
#include "Utils/winutils.h"

bool TapInstall_win::installTap(const QString &subfolder)
{
    QString strTapPath = QCoreApplication::applicationDirPath();
    QString tapInstallUtil = strTapPath + "/" + subfolder + "/tapinstall.exe";
    if (!QFile::exists(tapInstallUtil))
    {
        qCDebug(LOG_BASIC) << "File not exists: " << tapInstallUtil;
        return false;
    }
    else
    {
        qCDebug(LOG_BASIC) << "Found tapinstall.exe: " << tapInstallUtil;
    }

    QDir dir(strTapPath + "/" + subfolder);
    QStringList infFiles = dir.entryList(QStringList() << "*.inf");
    if (infFiles.size() == 0)
    {
        qCDebug(LOG_BASIC) << "No inf files in: " << strTapPath + "/" + subfolder;
        return false;
    }
    else
    {
        qCDebug(LOG_BASIC) << "Found inf files: " << infFiles;
    }

    QString infPath = strTapPath + "/" + subfolder  + "/" + infFiles[0];
    return WindscribeInstallHelper_win::executeInstallTapCmd(tapInstallUtil, infPath);
}

bool TapInstall_win::reinstallTap(TAP_TYPE tap)
{
    /*TAP_TYPE curInstalledTap = detectInstalledTapDriver(false);


    if (tap != curInstalledTap)
    {
        QString uninstallSubfolder;
        if (curInstalledTap == TapInstall_win::TAP5)
        {
            uninstallSubfolder = "tap5";
        }
        else if (curInstalledTap == TapInstall_win::TAP6)
        {
            uninstallSubfolder = "tap";
        }

        if (tap == TapInstall_win::TAP5)
        {
            if (!TapInstall_win::reinstallTapWithFolders("tap5", uninstallSubfolder))
            {
                qCDebug(LOG_BASIC) << "Failed reinstall TAP5-adapter";
                return false;
            }
            else
            {
                qCDebug(LOG_BASIC) << "TAP5-adapter reinstalled successfully";
            }
        }

        else if (tap == TapInstall_win::TAP6)
        {
            if (!TapInstall_win::reinstallTapWithFolders("tap", uninstallSubfolder))
            {
                qCDebug(LOG_BASIC) << "Failed reinstall TAP6-adapter";
                return false;
            }
            else
            {
                qCDebug(LOG_BASIC) << "TAP6-adapter reinstalled successfully";
            }
        }
    }*/
    return true;
}

// returns the path to the windows directory
std::wstring GetWindowsDirectoryString()
{
    std::wstring winDir(GetSystemWindowsDirectory(NULL, 0), 0);
    GetSystemWindowsDirectory(&winDir[0], static_cast<UINT>(winDir.size()));
    // remove the terminating null
    winDir.resize(winDir.size() - 1);
    return winDir;
}


// converts a NT Path to a more familiar (and user mode friendly) path
// we can pass to createfile
std::wstring TapInstall_win::NTPathToDosPath(LPCWSTR ntPath)
{
    std::wstring dosPath;
    static const WCHAR backSlash = L'\\';
    dosPath.reserve(MAX_PATH);
    // if it's a relative path
    // add on the path to the drivers directory
    if(PathIsRelativeW(ntPath))
    {
        static const WCHAR driverString[] = L"drivers/";
        WCHAR buf[MAX_PATH] = {0};
        GetSystemDirectory(buf, MAX_PATH - (ARRAYSIZE(driverString) + 1));
        PathAppend(buf, driverString);
        dosPath = buf;
        dosPath += ntPath;
    }
    else
    {
        // otherwise check for various prefixes
        static const WCHAR* sysRoot = L"\\systemroot\\";
        const std::wstring& windir = GetWindowsDirectoryString();
        // find the first slash on the windir path
        const WCHAR* startOfWindirPath = PathSkipRoot(windir.c_str()) - 1;
        static const WCHAR* dosDevSymLink = L"\\??\\";
        // lowercase the input string
        std::transform(ntPath, ntPath + wcslen(ntPath) + 1, std::back_inserter(dosPath), std::ptr_fun(towlower));
        std::wstring::size_type pos = 0;
        // if the prefix is systemroot
        if((pos = dosPath.find(sysRoot)) != std::wstring::npos)
        {
            // replace the first and last slashes with percent signs
            dosPath[0] = dosPath[11] = L'%';
            // add in a backslash after the last percent
            dosPath.insert(12, 1, backSlash);
            // expand the systemroot environment string
            std::vector<WCHAR> temp(ExpandEnvironmentStrings(dosPath.c_str(), NULL, 0));
            ExpandEnvironmentStrings(dosPath.c_str(), &temp[0], temp.size());
            temp.pop_back(); // remove the terminating NULL
            dosPath.assign(temp.begin(), temp.end());
        }
        // if the prefix is the dos device symlink, just remove it
        else if((pos = dosPath.find(dosDevSymLink)) != std::wstring::npos)
        {
            dosPath.erase(0, 4);
        }
        // if the prefix is the start of the windows path
        // add on the drive
        else if((pos = dosPath.find(startOfWindirPath)) != std::wstring::npos)
        {
            // insert the drive and the colon
            dosPath.insert(0, windir, 0, 2);
        }
        // else leave it alone
        else __assume(0);
    }
    return dosPath;
}

QVector<TapInstall_win::TAP_TYPE> TapInstall_win::detectInstalledTapDriver(bool bWithLog)
{
    QVector<TapInstall_win::TAP_TYPE> res;
    DWORD cbNeeded;
    if (!EnumDeviceDrivers(NULL, 0, &cbNeeded))
    {
        qCDebug(LOG_BASIC) << "detectInstalledTapDriver, EnumDeviceDrivers failed:" << GetLastError();
        return res;
    }
    QByteArray arr(cbNeeded, Qt::Uninitialized);
    if (!EnumDeviceDrivers((LPVOID *)arr.data(), arr.size(), &cbNeeded))
    {
        qCDebug(LOG_BASIC) << "detectInstalledTapDriver, EnumDeviceDrivers failed:" << GetLastError();
        return res;
    }

    int cDrivers = cbNeeded / sizeof(LPVOID);
    LPVOID *drivers = (LPVOID *)arr.data();
    for (int i = 0; i < cDrivers; i++)
    {
        TCHAR szDriver[MAX_PATH];
        if (GetDeviceDriverBaseName(drivers[i], szDriver, sizeof(szDriver) / sizeof(szDriver[0])))
        {
            QString baseName = QString::fromStdWString(szDriver);
            if (baseName.compare("tapwindscribe0901.sys", Qt::CaseInsensitive) == 0)
            {
                res << TAP_ADAPTER;
                if (bWithLog)
                {
                    qCDebug(LOG_BASIC) << "Detected TAP adapter";
                }
            }
            else if (baseName.compare("wintun.sys", Qt::CaseInsensitive) == 0)
            {
                res << WIN_TUN;
                if (bWithLog)
                {
                    qCDebug(LOG_BASIC) << "Detected Wintun adapter";
                }
            }
        }
    }

    return res;
}

bool TapInstall_win::reinstallTapWithFolders(const QString &installSubfolder, const QString &uninstallSubfolder)
{
    QString strTapPath = QCoreApplication::applicationDirPath();
    QString tapInstallUtil = strTapPath + "/" + installSubfolder + "/tapinstall.exe";
    if (!QFile::exists(tapInstallUtil))
    {
        qCDebug(LOG_BASIC) << "File not exists: " << tapInstallUtil;
        return false;
    }
    else
    {
        qCDebug(LOG_BASIC) << "Found tapinstall.exe: " << tapInstallUtil;
    }

    QString strUninstallTapPath = QCoreApplication::applicationDirPath();
    QString tapUninstallUtil = strUninstallTapPath + "/" + uninstallSubfolder + "/tapinstall.exe";
    if (!QFile::exists(tapUninstallUtil))
    {
        qCDebug(LOG_BASIC) << "No need uninstall previous TAP";
    }
    else
    {
        qCDebug(LOG_BASIC) << "Found tapinstall.exe: " << tapUninstallUtil;
    }

    QDir dir(strTapPath + "/" + installSubfolder);
    QStringList infFiles = dir.entryList(QStringList() << "*.inf");
    if (infFiles.size() == 0)
    {
        qCDebug(LOG_BASIC) << "No inf files in: " << strTapPath + "/" + installSubfolder;
        return false;
    }
    else
    {
        qCDebug(LOG_BASIC) << "Found inf files: " << infFiles;
    }

    QString infPath = strTapPath + "/" + installSubfolder  + "/" + infFiles[0];
    return WindscribeInstallHelper_win::executeRemoveAndInstallTapCmd(tapInstallUtil, tapUninstallUtil, infPath);
}
