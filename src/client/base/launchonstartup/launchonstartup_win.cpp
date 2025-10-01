#include "launchonstartup_win.h"
#include <QSettings>
#include <QDir>
#include <QCoreApplication>
#include <windows.h>

void LaunchOnStartup_win::setLaunchOnStartup(bool enable)
{
    if (enable)
    {
        {
            QSettings settingsRun("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
            QString exePath = "\"" + QDir::toNativeSeparators(QCoreApplication::applicationFilePath()) + "\" --autostart";
            settingsRun.setValue("Windscribe", exePath);
        }
        clearDisableAutoStartFlag();
    }
    else
    {
        QSettings settingsRun("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
        settingsRun.remove("Windscribe");
    }
}

void LaunchOnStartup_win::clearDisableAutoStartFlag()
{
    const wchar_t wsGuiIcon[] = L"Windscribe";

    HKEY hKey;
    LPCTSTR sk = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\Run");
    LONG openRes = RegOpenKeyEx(HKEY_CURRENT_USER, sk, 0, KEY_ALL_ACCESS , &hKey);
    if (openRes == ERROR_SUCCESS)
    {
        unsigned char data[12];
        memset(data, 0, sizeof(data));
        data[0] = 2;
        RegSetValueEx(hKey, wsGuiIcon, 0, REG_BINARY, (BYTE *)data, sizeof(data));
        RegCloseKey(hKey);
    }
}
