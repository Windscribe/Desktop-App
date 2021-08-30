#include "tapinstall_win.h"
#include <QFile>
#include <QDir>
#include "Utils/logger.h"
#include <QCoreApplication>
#include "engine/helper/windscribeinstallhelper_win.h"
#define PSAPI_VERSION 1
#include <psapi.h>
#include <shlwapi.h>
#include <functional>
#include "utils/utils.h"
#include "utils/winutils.h"

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
            else if (baseName.compare("windtun420.sys", Qt::CaseInsensitive) == 0)
            {
                res << WIN_TUN;
                if (bWithLog)
                {
                    qCDebug(LOG_BASIC) << "Detected windtun420 adapter";
                }
            }
        }
    }

    return res;
}
