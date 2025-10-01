#pragma once

#include <Windows.h>

#include <QMap>
#include <QString>
#include <QStringList>

namespace WinUtils
{
    const std::wstring wmActivateGui = L"WindscribeAppActivate";

    bool reboot();
    bool isWindows10orGreater();
    bool isOSCompatible();
    bool isDohSupported();
    bool isNotificationEnabled();
    void openNotificationSettings();
    QString getSystemDir();
    QString getWinVersionString();
    void getOSVersionAndBuild(QString &osVersion, QString &build);
    QMap<QString,QString> enumerateInstalledProgramIconLocations();
    QStringList enumerateRunningProgramLocations(bool allowDuplicate = false);
    QStringList enumerateSubkeyNames(HKEY rootKey, const QString &keyPath);
    QStringList enumerateProcesses(const QString &processName);

    QString executeBlockingCmd(QString cmd, const QString &params, int timeoutMs = -1);

    bool isAppAlreadyRunning();

    bool isServiceRunning(const QString &serviceName);

    // Registry Adapters
    bool regHasLocalMachineSubkeyProperty(QString keyPath, QString propertyName);
    QString regGetLocalMachineRegistryValueSz(QString keyPath, QString propertyName);
    bool regGetCurrentUserRegistryDword(QString keyPath, QString propertyName, int &dwordValue);

    unsigned long Win32GetErrorString(unsigned long errorCode, wchar_t *buffer, unsigned long bufferSize);

    // Retrieve the version information item specified by itemName (e.g. "FileVersion") from the executable.
    QString getVersionInfoItem(QString exeName, QString itemName);

    GUID stringToGuid(const char *str);

    HWND appMainWindowHandle();

    DWORD getOSBuildNumber();
}
