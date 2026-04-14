#include "launchonstartup_win.h"
#include <QSettings>
#include <QDir>
#include <QCoreApplication>
#include <windows.h>
#include "ws_branding.h"

void LaunchOnStartup_win::setLaunchOnStartup(bool enable)
{
    if (enable)
    {
        {
            QSettings settingsRun("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
            QString exePath = "\"" + QDir::toNativeSeparators(QCoreApplication::applicationFilePath()) + "\" --autostart";
            settingsRun.setValue(WS_PRODUCT_NAME, exePath);
        }
        clearDisableAutoStartFlag();
    }
    else
    {
        QSettings settingsRun("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
        settingsRun.remove(WS_PRODUCT_NAME);
    }
}

void LaunchOnStartup_win::clearDisableAutoStartFlag()
{
    const wchar_t wsGuiIcon[] = WS_APP_IDENTIFIER_W;

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
